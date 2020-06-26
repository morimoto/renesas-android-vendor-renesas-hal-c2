/*
 * Copyright (C) 2019 GlobalLogic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "C2VendorDecComponent"
//#define R_LOG_VERBOSITY

#include "C2VendorDecComponent.h"

#include "C2VendorComponentInferface.h"
#include "C2VendorDebug.h"
#include "OMXR_Adapter.h"
#include "OMXR_Debug.h"
#include "OMXR_Extension_h264d.h"
#include "OMXR_Extension_h265d.h"
#include "OMXR_Extension_m4vd.h"

extern "C" {
#include <mmngr_user_public.h>
#include <vspm_public.h>
}

namespace android::hardware::media::c2::V1_0::renesas {

constexpr OMX_COLOR_FORMATTYPE OMXDecColorFormat = OMXR_COLOR_FormatYV12;

static_assert(OMXDecColorFormat == OMX_COLOR_FormatYUV420SemiPlanar ||
              OMXDecColorFormat == OMX_COLOR_FormatYUV420Planar ||
              OMXDecColorFormat == OMXR_COLOR_FormatYV12);

constexpr uint32_t FallbackAdaptiveWidth = 1280u;
constexpr uint32_t FallbackAdaptiveHeight = 720u;

constexpr bool USE_HW_COPYING = true;

template <typename T, size_t size>
constexpr inline T GetValueByIndex(OMX_U32 index, const T (&arr)[size]) {
    return index < size ? arr[index] : T{};
}

C2VendorDecComponent::C2VendorDecComponent(
    const std::shared_ptr<IntfImplDec>& intfImpl,
    const std::shared_ptr<OMXR_Core>& omxrCore)
    : C2VendorBaseComponent{intfImpl, omxrCore},
      mIntfImpl{intfImpl},
      mCropWidth{0u},
      mCropHeight{0u},
      mBlockWidth{0u},
      mBlockHeight{0u},
      mSyntaxIndex{0u},
      mGetVUIColorAspectsFunc{nullptr},
      mVspmHandle{nullptr},
      mVspmResult{C2_NO_INIT} {
    R_LOG(DEBUG);

    constexpr std::tuple<OMX_VIDEO_CODINGTYPE, OMX_U32,
                         decltype(mGetVUIColorAspectsFunc)>
        codingToSyntax[] = {
            std::make_tuple(OMX_VIDEO_CodingAVC,
                            OMXR_MC_IndexConfigVideoInfoAvcSyntax,
                            &GetAVCVUIColorAspects),
            std::make_tuple(OMX_VIDEO_CodingHEVC,
                            OMXR_MC_IndexConfigVideoInfoHevSyntax,
                            &GetHEVCVUIColorAspects),
            std::make_tuple(OMX_VIDEO_CodingMPEG4,
                            OMXR_MC_IndexConfigVideoInfoM4vSyntax,
                            &GetMPEG4VUIColorAspects),
        };
    const OMX_VIDEO_CODINGTYPE omxCodingType = mIntfImpl->getOMXCodingType();

    const auto syntaxIt =
        std::find_if(std::cbegin(codingToSyntax), std::cend(codingToSyntax),
                     [omxCodingType](const auto& t) {
                         return std::get<0>(t) == omxCodingType;
                     });

    if (syntaxIt == std::cend(codingToSyntax)) {
        R_LOG(DEBUG) << mIntfImpl->getC2Name()
                     << " doesn't support color aspects";

        constexpr auto stub = [](auto&&...) {};

        mRetrieveColorAspectsFunc = stub;
        mVerifyColorAspectsFunc = stub;
    } else {
        R_LOG(DEBUG) << mIntfImpl->getC2Name() << " supports color aspects";

        mSyntaxIndex = std::get<1>(*syntaxIt);
        mGetVUIColorAspectsFunc = std::get<2>(*syntaxIt);
        mRetrieveColorAspectsFunc = [this](auto&&... args) {
            retrieveColorAspects(args...);
        };
        mVerifyColorAspectsFunc = [this](auto&&... args) {
            verifyColorAspects(args...);
        };
    }

    if constexpr (USE_HW_COPYING) {
        initVspm();
    }
}

C2VendorDecComponent::~C2VendorDecComponent() {
    R_LOG(DEBUG);

    releaseComponent();

    if constexpr (USE_HW_COPYING) {
        deinitVspm();
    }
}

bool C2VendorDecComponent::onStateSet(OMX_STATETYPE omxState) {
    if (omxState == OMX_StateLoaded) {
        mCropWidth = mCropHeight = 0u;
        mBlockWidth = mBlockHeight = 0u;
        mBitstreamAspects = {};
    }

    return C2VendorBaseComponent::onStateSet(omxState);
}

bool C2VendorDecComponent::onConfigure(const OMXR_Adapter& omxrAdapter) {
    const uint32_t bitrate = mIntfImpl->getBitrate();
    const uint32_t frameRate = mIntfImpl->getCodedFrameRate();
    const OMX_VIDEO_CODINGTYPE omxCodingType = mIntfImpl->getOMXCodingType();
    const uint32_t pixelFormat = mIntfImpl->getPixelFormat();
    const C2PictureSizeStruct& pictureSize = mIntfImpl->getPictureSize();
    const C2PictureSizeStruct& maxPictureSize = mIntfImpl->getMaxPictureSize();

    R_LOG(DEBUG) << "Got params from intfImpl: bitrate " << bitrate
                 << ", frameRate " << frameRate << ", omxCodingType "
                 << omxCodingType << ", pixelFormat " << pixelFormat
                 << ", pictureSize " << pictureSize.width << "x"
                 << pictureSize.height << ", maxPictureSize "
                 << maxPictureSize.width << "x" << maxPictureSize.height;

    const OMX_VIDEO_CODINGTYPE inputCoding = omxCodingType;
    constexpr OMX_COLOR_FORMATTYPE inputColFormat = OMX_COLOR_FormatUnused;
    constexpr OMX_VIDEO_CODINGTYPE outputCoding = OMX_VIDEO_CodingUnused;
    constexpr OMX_COLOR_FORMATTYPE outputColFormat = OMXDecColorFormat;

    R_LOG(DEBUG) << "OMX color format " << outputColFormat;

    if (setPortFormat(InputPortIndex, frameRate, inputCoding, inputColFormat) !=
        OMX_ErrorNone) {
        return false;
    }

    if (setPortFormat(OutputPortIndex, frameRate, outputCoding,
                      outputColFormat) != OMX_ErrorNone) {
        return false;
    }

    mCropWidth = pictureSize.width;
    mCropHeight = pictureSize.height;

    if (setPortDef(InputPortIndex, mCropWidth, mCropHeight, bitrate, frameRate,
                   inputCoding, inputColFormat) != OMX_ErrorNone) {
        return false;
    }

    if (!forceMaxDecodeCapIfNeeded(omxrAdapter, omxCodingType,
                                   maxPictureSize.width,
                                   maxPictureSize.height)) {
        mBlockWidth = mCropWidth;
        mBlockHeight = mCropHeight;
    }

    R_LOG(DEBUG) << "Using crop size " << mCropWidth << "x" << mCropHeight
                 << ", block size " << mBlockWidth << "x" << mBlockHeight;

    if (setPortDef(OutputPortIndex, mBlockWidth, mBlockHeight, bitrate,
                   frameRate, outputCoding, outputColFormat) != OMX_ErrorNone) {
        return false;
    }

    return true;
}

c2_status_t C2VendorDecComponent::onProcessInput(
    const C2Work& work, OMX_BUFFERHEADERTYPE* const header, bool fromDequeue) {
    const size_t index = GetBufferIndex(header);
    const uint64_t frameIndex = work.input.ordinal.frameIndex.peeku();
    const bool eos = (work.input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0u;
    const bool csd = (work.input.flags & C2FrameData::FLAG_CODEC_CONFIG) != 0u;
    uint32_t size = 0u;

    if (!work.input.buffers.empty()) {
        const C2ReadView view = work.input.buffers.front()
                                    ->data()
                                    .linearBlocks()
                                    .front()
                                    .map()
                                    .get();
        c2_status_t status = view.error();

        if (status != C2_OK) {
            R_LOG(ERROR) << "Mapping input failed, " << status;
            return status;
        }

        size = view.capacity();

        if (header->nAllocLen < size) {
            R_LOG(ERROR) << "Small OMX input buffer, " << header->nAllocLen
                         << " < " << size;
            return C2_CORRUPTED;
        }

        CopyBuffer(header->pBuffer, view.data(), static_cast<size_t>(size));
    }

    OMX_U32 omxFlags = OMX_BUFFERFLAG_ENDOFFRAME;

    if (eos) {
        omxFlags |= OMX_BUFFERFLAG_EOS;
    }

    if (csd) {
        omxFlags |= OMX_BUFFERFLAG_CODECCONFIG;
    }

    R_LOG(DEBUG) << (fromDequeue ? "Dequeue" : "handleInputDone") << " input:"
                 << " size " << size << ", flags " << std::hex
                 << work.input.flags << std::dec << ", ts "
                 << work.input.ordinal.timestamp.peeku() << ", index " << index
                 << ", frameIndex " << frameIndex;

    header->nOffset = 0u;
    header->nFilledLen = size;
    header->nTimeStamp = FrameIndexToTs(frameIndex);
    header->nFlags = omxFlags;

    return emptyBuffer(header) == OMX_ErrorNone ? C2_OK : C2_CORRUPTED;
}

C2VendorBaseComponent::ExtendedBufferData
C2VendorDecComponent::onPreprocessOutput(
    const OMXR_Adapter& omxrAdapter, OMX_BUFFERHEADERTYPE* const header) const {
    if (header->nFilledLen == 0u) {
        return C2VendorBaseComponent::onPreprocessOutput(omxrAdapter, header);
    }

    const OMXR_MC_VIDEO_DECODERESULTTYPE* const decodeResult =
        static_cast<OMXR_MC_VIDEO_DECODERESULTTYPE*>(
            header->pOutputPortPrivate);

    ExtendedBufferData data{header};
    data.width = decodeResult->u32PictWidth;
    data.height = decodeResult->u32PictHeight;
    data.physAddr = decodeResult->pvPhysImageAddressY;

    mRetrieveColorAspectsFunc(omxrAdapter, data.aspects);

    return data;
}

void C2VendorDecComponent::onOutputDone(const ExtendedBufferData& data) {
    OMX_BUFFERHEADERTYPE* const header = data.header;
    const size_t index = GetBufferIndex(header);
    uint64_t frameIndex = TsToFrameIndex(header->nTimeStamp);
    const bool eos = (header->nFlags & OMX_BUFFERFLAG_EOS) != 0u;
    const size_t size = static_cast<size_t>(header->nFilledLen);

    R_LOG(DEBUG) << "Size " << size << ", OMXFlags " << std::hex
                 << header->nFlags << std::dec << ", frameIndex " << frameIndex
                 << ", index " << index;

    // HACK: framework doesn't expect output csd buffer from decoders
    if ((header->nFlags & OMX_BUFFERFLAG_CODECCONFIG) != 0u) {
        fillBuffer(header);

        R_LOG(DEBUG) << "Csd with size " << size << " will be skipped";
        return;
    }

    CHECK(!mEmptiedWorks.empty());

    auto emptiedIt = mEmptiedWorks.end();

    if (eos) {
        emptiedIt =
            std::find_if(mEmptiedWorks.begin(), mEmptiedWorks.end(),
                         [&frameIndex](const auto& p) {
                             if ((p.second->input.flags &
                                  C2FrameData::FLAG_END_OF_STREAM) != 0u) {
                                 frameIndex = p.first;

                                 return true;
                             }

                             return false;
                         });
    } else {
        emptiedIt = mEmptiedWorks.find(frameIndex);
    }

    CHECK(emptiedIt != mEmptiedWorks.end());

    std::unique_ptr<C2Work> work = std::move(emptiedIt->second);
    mEmptiedWorks.erase(emptiedIt);

    C2FrameData& output = work->worklets.front()->output;

    if (size > 0u) {
        c2_status_t status = C2_OK;

        // debug
        // DumpHeader<false>(header, std::to_string(frameIndex) +
        // "_output_header");

        const uint32_t pictureWidth = data.width;
        const uint32_t pictureHeight = data.height;

        if (pictureWidth != mCropWidth || pictureHeight != mCropHeight) {
            R_LOG(DEBUG) << "Resolution changed: " << mCropWidth << "x"
                         << mCropHeight << " -> " << pictureWidth << "x"
                         << pictureHeight;

            mCropWidth = pictureWidth;
            mCropHeight = pictureHeight;

            if (mCropWidth > mBlockWidth || mCropHeight > mBlockHeight) {
                R_LOG(DEBUG) << "Block size is too small, increasing to crop";

                mBlockWidth = mCropWidth;
                mBlockHeight = mCropHeight;
            }

            std::vector<std::unique_ptr<C2SettingResult>> failures;
            auto pictureSizeParam =
                std::make_unique<C2StreamPictureSizeInfo::output>(0u,
                                                                  mCropWidth,
                                                                  mCropHeight);

            status = mIntfImpl->config({pictureSizeParam.get()}, C2_MAY_BLOCK,
                                       &failures);

            if (status != C2_OK) {
                R_LOG(ERROR)
                    << "Failed to config new picture size: " << mCropWidth
                    << "x" << mCropHeight << ", " << status;
            }

            output.configUpdate.push_back(std::move(pictureSizeParam));
        }

        constexpr android_pixel_format_t actualFormat = HAL_PIXEL_FORMAT_YV12;
        const C2MemoryUsage usage{0, 0}; //workaround for disabling ion cache when allocating buffers in DDK
        std::shared_ptr<C2GraphicBlock> graphicBlock;

        status = mOutputBlockPool->fetchGraphicBlock(mBlockWidth, mBlockHeight,
                                                     actualFormat, usage,
                                                     &graphicBlock);

        if (status != C2_OK) {
            reportWork(std::move(work), status);

            R_LOG(ERROR) << "Failed to create out block, " << status;
            return;
        }

        C2GraphicView graphicView = graphicBlock->map().get();
        status = graphicView.error();

        if (status != C2_OK) {
            reportWork(std::move(work), status);

            R_LOG(ERROR) << "Mapping output failed, " << status;
            return;
        }

        if constexpr (USE_HW_COPYING) {
            status = vspmCopy(data.physAddr,
                              reinterpret_cast<const IMG_native_handle_t*>(
                                  graphicBlock->handle()));

            if (status != C2_OK) {
                reportWork(std::move(work), status);

                R_LOG(ERROR) << "Failed to copy frameIndex " << frameIndex
                             << ", " << status;
                return;
            }
        } else {
            CopyBuffer(graphicView.data()[C2PlanarLayout::PLANE_Y],
                       header->pBuffer + header->nOffset, size);
        }

        const std::shared_ptr<C2Buffer> graphicBuffer =
            C2Buffer::CreateGraphicBuffer(
                graphicBlock->share({mCropWidth, mCropHeight}, C2Fence{}));

        output.buffers.push_back(graphicBuffer);

        mVerifyColorAspectsFunc(data.aspects, *graphicBuffer);
    }

    if (!eos && (work->input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0u) {
        R_LOG(DEBUG) << "Postponing frameIndex " << frameIndex << " till eos";

        reportClonedWork(*work);

        CHECK(mEmptiedWorks.emplace(frameIndex, std::move(work)).second);
    } else {
        reportWork(std::move(work), C2_OK);
    }

    if (!eos) {
        header->nFilledLen = 0u;
        header->nTimeStamp = 0;
        header->nFlags = 0u;

        fillBuffer(header);
    }
}

void C2VendorDecComponent::VspmCompleteCallback(
    unsigned long jobId ATTRIBUTE_UNUSED, long result, void* userData) {
    R_LOG(DEBUG) << "Result " << result;

    C2VendorDecComponent* const component =
        static_cast<C2VendorDecComponent*>(userData);

    std::lock_guard lock{component->mVspmMutex};
    component->mVspmResult =
        (component->mVspmResult == C2_NO_INIT && result == R_VSPM_OK)
        ? C2_OK
        : C2_CORRUPTED;
    component->mVspmCV.notify_one();
}

void C2VendorDecComponent::initVspm() {
    vspm_init_t vspmInitParam;
    vspmInitParam.use_ch = VSPM_EMPTY_CH;
    vspmInitParam.mode = VSPM_MODE_MUTUAL;
    vspmInitParam.type = VSPM_TYPE_VSP_AUTO;
    vspmInitParam.par.vsp = nullptr;

    CHECK_EQ(vspm_init_driver(&mVspmHandle, &vspmInitParam), R_VSPM_OK);
}

void C2VendorDecComponent::deinitVspm() {
    if (mVspmHandle != nullptr) {
        CHECK_EQ(vspm_quit_driver(mVspmHandle), R_VSPM_OK);
    }
}

c2_status_t C2VendorDecComponent::vspmCopy(
    const void* const omxPhysAddr, const IMG_native_handle_t* const handle) {
    uint64_t physAddr = 0u;

    CHECK_EQ(mmngr_get_buffer_phys_addr(handle->fd[0], &physAddr), R_MM_OK);

    const unsigned short width = static_cast<unsigned short>(handle->iWidth);
    const unsigned short height = static_cast<unsigned short>(handle->iHeight);
    const unsigned short stride = Align64(width);
    const unsigned short vStride = height;

    const uint8_t* const srcY = static_cast<const uint8_t*>(omxPhysAddr);
    const uint8_t* srcU = srcY + stride * vStride;
    const uint8_t* srcV = srcU + (stride >> 1) * (vStride >> 1);

    const uint8_t* const dstY = reinterpret_cast<const uint8_t*>(physAddr);
    const uint8_t* dstU = dstY + stride * vStride;
    const uint8_t* dstV = dstU + (stride >> 1) * (vStride >> 1);

    unsigned short vspFormat = 0u;
    unsigned short vspStrideC = 0u;

    if constexpr (OMXDecColorFormat == OMX_COLOR_FormatYUV420Planar) {
        vspFormat = VSP_OUT_YUV420_PLANAR;
        vspStrideC = stride >> 1;
    } else if constexpr (OMXDecColorFormat == OMXR_COLOR_FormatYV12) {
        vspFormat = VSP_OUT_YUV420_PLANAR;
        vspStrideC = stride >> 1;

        std::swap(srcU, srcV);
        std::swap(dstU, dstV);
    } else {
        vspFormat = VSP_OUT_YUV420_SEMI_PLANAR;
        vspStrideC = stride;
    }

    // IN params
    constexpr char jobPriority = VSPM_PRI_MAX;

    vsp_alpha_unit_t vspAlphaParam{};
    vspAlphaParam.swap = VSP_SWAP_NO;
    vspAlphaParam.asel = VSP_ALPHA_NUM5;
    vspAlphaParam.aext = VSP_AEXT_COPY;
    vspAlphaParam.afix = std::numeric_limits<unsigned char>::max();

    vsp_src_t vspSrcParam{};
    vspSrcParam.addr = *reinterpret_cast<const unsigned int*>(&srcY);
    vspSrcParam.addr_c0 = *reinterpret_cast<const unsigned int*>(&srcU);
    vspSrcParam.addr_c1 = *reinterpret_cast<const unsigned int*>(&srcV);
    vspSrcParam.stride = stride;
    vspSrcParam.stride_c = vspStrideC;
    vspSrcParam.width = width;
    vspSrcParam.height = height;
    vspSrcParam.format = vspFormat;
    vspSrcParam.swap = VSP_SWAP_B | VSP_SWAP_W | VSP_SWAP_L | VSP_SWAP_LL;
    vspSrcParam.pwd = VSP_LAYER_PARENT;
    vspSrcParam.cipm = VSP_CIPM_0_HOLD;
    vspSrcParam.cext = VSP_CEXT_EXPAN;
    vspSrcParam.csc = VSP_CSC_OFF;
    vspSrcParam.iturbt = VSP_ITURBT_709;
    vspSrcParam.clrcng = VSP_FULL_COLOR;
    vspSrcParam.vir = VSP_NO_VIR;
    vspSrcParam.alpha = &vspAlphaParam;

    vsp_dst_t vspDstParam{};
    vspDstParam.addr = *reinterpret_cast<const unsigned int*>(&dstY);
    vspDstParam.addr_c0 = *reinterpret_cast<const unsigned int*>(&dstU);
    vspDstParam.addr_c1 = *reinterpret_cast<const unsigned int*>(&dstV);
    vspDstParam.stride = stride;
    vspDstParam.stride_c = vspStrideC;
    vspDstParam.width = width;
    vspDstParam.height = height;
    vspDstParam.format = vspFormat;
    vspDstParam.swap = VSP_SWAP_B | VSP_SWAP_W | VSP_SWAP_L | VSP_SWAP_LL;
    vspDstParam.pxa = VSP_PAD_P;
    vspDstParam.csc = VSP_CSC_OFF;
    vspDstParam.iturbt = VSP_ITURBT_709;
    vspDstParam.clrcng = VSP_FULL_COLOR;
    vspDstParam.cbrm = VSP_CSC_ROUND_DOWN;
    vspDstParam.abrm = VSP_CONVERSION_ROUNDDOWN;
    vspDstParam.clmd = VSP_CLMD_NO;
    vspDstParam.dith = VSP_DITH_OFF;
    vspDstParam.rotation = VSP_ROT_OFF;

    vsp_ctrl_t vspCtrlParam{};

    vsp_start_t vspStartParam{};
    vspStartParam.rpf_num = 1u;
    vspStartParam.src_par[0u] = &vspSrcParam;
    vspStartParam.dst_par = &vspDstParam;
    vspStartParam.ctrl_par = &vspCtrlParam;

    vspm_job_t ipParam;
    ipParam.type = VSPM_TYPE_VSP_AUTO;
    ipParam.par.vsp = &vspStartParam;

    // OUT params
    unsigned long jobId = 0u;

    const long res = vspm_entry_job(mVspmHandle, &jobId, jobPriority, &ipParam,
                                    this, &VspmCompleteCallback);

    if (res != R_VSPM_OK) {
        R_LOG(ERROR) << "Failed to request vspm job " << jobId << ", " << res;
        return C2_CORRUPTED;
    }

    constexpr std::chrono::duration waitTimeout =
        std::chrono::milliseconds{100};
    c2_status_t status = C2_NO_INIT;

    {
        std::unique_lock lock{mVspmMutex};

        if (!mVspmCV.wait_for(lock, waitTimeout,
                              [this] { return mVspmResult != C2_NO_INIT; })) {
            R_LOG(ERROR) << "Timeout waiting for vpsm job completion";
            mVspmResult = C2_CORRUPTED;
        }

        std::swap(mVspmResult, status);
    }

    return status;
}

bool C2VendorDecComponent::forceMaxDecodeCapIfNeeded(
    const OMXR_Adapter& omxrAdapter,
    OMX_VIDEO_CODINGTYPE omxCodingType ATTRIBUTE_UNUSED,
    uint32_t maxPictureWidth,
    uint32_t maxPictureHeight) {
    C2StreamPictureSizeInfo::output pictureSize;

    CHECK_EQ(mIntfImpl->query({&pictureSize}, {}, C2_MAY_BLOCK, {}), C2_OK);

    const uint32_t curWidth = pictureSize.width;
    const uint32_t curHeight = pictureSize.height;

    R_LOG(DEBUG) << "Current picture size: " << curWidth << "x" << curHeight;

    std::vector<C2FieldSupportedValuesQuery> fields = {
        C2FieldSupportedValuesQuery::Possible(
            C2ParamField::Make(pictureSize, pictureSize.width)),
        C2FieldSupportedValuesQuery::Possible(
            C2ParamField::Make(pictureSize, pictureSize.height)),
    };

    CHECK_EQ(mIntfImpl->querySupportedValues(fields, C2_MAY_BLOCK), C2_OK);

    C2FieldSupportedValuesQuery& widthQuery = fields[0];
    C2FieldSupportedValuesQuery& heightQuery = fields[1];

    CHECK_EQ(widthQuery.status, C2_OK);
    CHECK_EQ(heightQuery.status, C2_OK);

    C2FieldSupportedValues& widthFsv = widthQuery.values;
    C2FieldSupportedValues& heightFsv = heightQuery.values;

    CHECK_EQ(widthFsv.type, C2FieldSupportedValues::RANGE);
    CHECK_EQ(heightFsv.type, C2FieldSupportedValues::RANGE);

    const uint32_t minWidth = widthFsv.range.min.ref<uint32_t>();
    const uint32_t minHeight = heightFsv.range.min.ref<uint32_t>();

    R_LOG(DEBUG) << "Min supported picture size: " << minWidth << "x"
                 << minHeight;

    // HACK: force enlarged max decode capability in the following cases:
    // - VP9 and HAL_C2_VENDOR_ENABLE_SUPPORTING_VP9_REFERENCE_SCALING is
    // defined;
    // - max picture size was presented;
    // - current resolution is minimal for appropriate codecs.
#ifdef HAL_C2_VENDOR_ENABLE_SUPPORTING_VP9_REFERENCE_SCALING
    if (omxCodingType == OMX_VIDEO_CodingVP9) {
        R_LOG(DEBUG) << "VP9 reference scaling feature is enabled";

        mBlockWidth = widthFsv.range.max.ref<uint32_t>();
        mBlockHeight = heightFsv.range.max.ref<uint32_t>();
    } else
#endif
        if (maxPictureWidth != 0u || maxPictureHeight != 0u ||
            (curWidth == minWidth && curHeight == minHeight)) {
        if (maxPictureWidth > FallbackAdaptiveWidth ||
            maxPictureHeight > FallbackAdaptiveHeight) {
            mBlockWidth = maxPictureWidth;
            mBlockHeight = maxPictureHeight;
        } else {
            mBlockWidth = FallbackAdaptiveWidth;
            mBlockHeight = FallbackAdaptiveHeight;
        }
    } else {
        return false;
    }

    OMXR_MC_VIDEO_PARAM_MAXIMUM_DECODE_CAPABILITYTYPE maxDecCap;
    constexpr OMX_INDEXTYPE maxDecCapIndex = static_cast<OMX_INDEXTYPE>(
        OMXR_MC_IndexParamVideoMaximumDecodeCapability);

    OMX_ERRORTYPE omxError =
        omxrAdapter.getPortParam(OutputPortIndex, maxDecCapIndex, maxDecCap);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to get max decode cap, " << omxError;
        return false;
    }

    R_LOG(DEBUG) << "Previous decode cap " << maxDecCap.nMaxDecodedWidth << "x"
                 << maxDecCap.nMaxDecodedHeight << ", enabled "
                 << maxDecCap.bForceEnable << ", requested " << mBlockWidth
                 << "x" << mBlockHeight;

    maxDecCap.bForceEnable = OMX_TRUE;
    maxDecCap.nMaxDecodedWidth = mBlockWidth;
    maxDecCap.nMaxDecodedHeight = mBlockHeight;

    omxError = omxrAdapter.setParam(maxDecCapIndex, maxDecCap);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to set max decode cap, " << omxError;
        return false;
    }

    return true;
}

constexpr void C2VendorDecComponent::GetAVCVUIColorAspects(
    const OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE& syntaxInfoConfig,
    VUIColorAspects& aspects) {
    const OMXR_MC_VIDEO_INFO_AVC_VUI_SYNTAXTYPE& avcVui =
        static_cast<OMXR_MC_VIDEO_INFO_AVC_SPS_SYNTAXTYPE*>(
            syntaxInfoConfig.pvSyntaxInfo)
            ->vui;

    aspects.range = avcVui.video_full_range_flag;
    aspects.primaries = avcVui.colour_primaries;
    aspects.transfer = avcVui.transfer_characteristics;
    aspects.matrixCoeffs = avcVui.matrix_coefficients;
}

constexpr void C2VendorDecComponent::GetHEVCVUIColorAspects(
    const OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE& syntaxInfoConfig,
    VUIColorAspects& aspects) {
    const OMXR_MC_VIDEO_INFO_HEV_VUI_SYNTAXTYPE& hevcVui =
        static_cast<OMXR_MC_VIDEO_INFO_HEV_SYNTAXTYPE*>(
            syntaxInfoConfig.pvSyntaxInfo)
            ->vui;

    aspects.range = static_cast<OMX_U32>(hevcVui.video_full_range_flag);
    aspects.primaries = hevcVui.colour_primaries;
    aspects.transfer = hevcVui.transfer_characteristics;
    aspects.matrixCoeffs = hevcVui.matrix_coeffs;
}

constexpr void C2VendorDecComponent::GetMPEG4VUIColorAspects(
    const OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE& syntaxInfoConfig,
    VUIColorAspects& aspects) {
    const OMXR_MC_VIDEO_INFO_M4V_VO_SYNTAXTYPE& m4vVoVui =
        static_cast<OMXR_MC_VIDEO_INFO_M4V_SYNTAXTYPE*>(
            syntaxInfoConfig.pvSyntaxInfo)
            ->vo;

    aspects.range = m4vVoVui.video_range;
    aspects.primaries = m4vVoVui.colour_primaries;
    aspects.transfer = m4vVoVui.transfer_characteristics;
    aspects.matrixCoeffs = m4vVoVui.matrix_coefficients;
}

constexpr void C2VendorDecComponent::ConvertVUIToC2ColorAspects(
    const VUIColorAspects& vuiAspects, C2ColorAspectsStruct& aspects) {
    constexpr C2Color::range_t rangeMap[] = {
        RANGE_LIMITED,
        RANGE_FULL,
    };

    constexpr C2Color::primaries_t primariesMap[] = {
        PRIMARIES_UNSPECIFIED, PRIMARIES_BT709,     PRIMARIES_UNSPECIFIED,
        PRIMARIES_UNSPECIFIED, PRIMARIES_BT470_M,   PRIMARIES_BT601_625,
        PRIMARIES_BT601_625,   PRIMARIES_BT601_625, PRIMARIES_GENERIC_FILM,
        PRIMARIES_BT2020,      PRIMARIES_OTHER,
    };

    constexpr C2Color::transfer_t transferMap[] = {
        TRANSFER_UNSPECIFIED, TRANSFER_170M,    TRANSFER_UNSPECIFIED,
        TRANSFER_UNSPECIFIED, TRANSFER_GAMMA22, TRANSFER_GAMMA28,
        TRANSFER_170M,        TRANSFER_240M,    TRANSFER_LINEAR,
        TRANSFER_OTHER,       TRANSFER_OTHER,   TRANSFER_XVYCC,
        TRANSFER_BT1361,      TRANSFER_SRGB,    TRANSFER_170M,
        TRANSFER_170M,        TRANSFER_ST2084,  TRANSFER_ST428,
        TRANSFER_HLG,
    };

    constexpr C2Color::matrix_t matrixMap[] = {
        MATRIX_UNSPECIFIED,     MATRIX_BT709,
        MATRIX_UNSPECIFIED,     MATRIX_UNSPECIFIED,
        MATRIX_FCC47_73_682,    MATRIX_BT601,
        MATRIX_BT601,           MATRIX_240M,
        MATRIX_OTHER,           MATRIX_BT2020,
        MATRIX_BT2020_CONSTANT,
    };

    aspects.range = GetValueByIndex(vuiAspects.range, rangeMap);
    aspects.primaries = GetValueByIndex(vuiAspects.primaries, primariesMap);
    aspects.transfer = GetValueByIndex(vuiAspects.transfer, transferMap);
    aspects.matrix = GetValueByIndex(vuiAspects.matrixCoeffs, matrixMap);
}

void C2VendorDecComponent::retrieveColorAspects(
    const OMXR_Adapter& omxrAdapter, C2ColorAspectsStruct& aspects) const {
    OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE infoConfig;
    OMXR_Adapter::InitOMXParam(infoConfig);
    infoConfig.nPortIndex = OutputPortIndex;
    infoConfig.u32SyntaxIndex = mSyntaxIndex;

    const OMX_ERRORTYPE omxError =
        omxrAdapter.getConfig(static_cast<OMX_INDEXTYPE>(
                                  OMXR_MC_IndexConfigVideoSyntaxInfo),
                              infoConfig);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to get syntax info config, " << omxError;
        return;
    } else if (infoConfig.pvSyntaxInfo == nullptr) {
        R_LOG(ERROR) << "Bad syntax info";
        return;
    }

    VUIColorAspects vuiAspects;

    mGetVUIColorAspectsFunc(infoConfig, vuiAspects);

    R_LOG(DEBUG) << "Got VUI aspects: " << vuiAspects.range << ", "
                 << vuiAspects.primaries << ", " << vuiAspects.transfer << ", "
                 << vuiAspects.matrixCoeffs;

    ConvertVUIToC2ColorAspects(vuiAspects, aspects);
}

void C2VendorDecComponent::verifyColorAspects(
    const C2ColorAspectsStruct& aspects, C2Buffer& buffer) {
    if (mBitstreamAspects == aspects) {
        return;
    }

    R_LOG(DEBUG) << "Bitstream color aspects changed " << mBitstreamAspects
                 << " -> " << aspects;

    mBitstreamAspects = aspects;
    std::vector<std::unique_ptr<C2SettingResult>> failures;
    C2StreamColorAspectsInfo::input codedAspectsParam = {0u, aspects.range,
                                                         aspects.primaries,
                                                         aspects.transfer,
                                                         aspects.matrix};

    const c2_status_t status =
        mIntfImpl->config({&codedAspectsParam}, C2_MAY_BLOCK, &failures);

    if (status != C2_OK) {
        R_LOG(ERROR) << "Failed to config coded color aspects: "
                     << codedAspectsParam << ", " << status;
    }

    buffer.setInfo(mIntfImpl->getColorAspectsParam());
}

} // namespace android::hardware::media::c2::V1_0::renesas
