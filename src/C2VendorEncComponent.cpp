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

#define LOG_TAG "C2VendorEncComponent"
//#define R_LOG_VERBOSITY

#include "C2VendorEncComponent.h"

#include "C2VendorComponentInferface.h"
#include "C2VendorDebug.h"
#include "OMXR_Adapter.h"
#include "OMXR_Debug.h"

extern "C" {
#include <mmngr_user_public.h>
#include <vspm_public.h>
}

namespace android::hardware::media::c2::V1_0::renesas {

constexpr android_pixel_format_t C2EncInputFormat = HAL_PIXEL_FORMAT_BGRA_8888;

static_assert(C2EncInputFormat == HAL_PIXEL_FORMAT_RGBA_8888 ||
              C2EncInputFormat == HAL_PIXEL_FORMAT_BGRA_8888);

constexpr OMX_COLOR_FORMATTYPE OMXEncColorFormat = OMXR_COLOR_FormatYV12;

static_assert(OMXEncColorFormat == OMX_COLOR_FormatYUV420SemiPlanar ||
              OMXEncColorFormat == OMX_COLOR_FormatYUV420Planar ||
              OMXEncColorFormat == OMXR_COLOR_FormatYV12);

constexpr bool USE_HW_CONVERSION = true;

template <bool fromBGRA, bool toPlanar, bool toYVU>
void ConvertRGBAToYUVA(uint8_t* dstY,
                       size_t dstStride,
                       size_t dstVStride,
                       const C2GraphicView& src) {
    uint8_t* dstU = nullptr;
    uint8_t* dstV = nullptr;
    uint8_t* dstUV = nullptr;

    if constexpr (toPlanar) {
        dstU = dstY + dstStride * dstVStride;
        dstV = dstU + (dstStride >> 1) * (dstVStride >> 1);

        if constexpr (toYVU) {
            std::swap(dstU, dstV);
        }
    } else {
        dstUV = dstY + dstStride * dstVStride;
    }

    uint32_t rPlaneIndex = C2PlanarLayout::PLANE_R;
    constexpr uint32_t gPlaneIndex = C2PlanarLayout::PLANE_G;
    uint32_t bPlaneIndex = C2PlanarLayout::PLANE_B;

    if constexpr (fromBGRA) {
        std::swap(rPlaneIndex, bPlaneIndex);
    }

    const C2PlanarLayout& layout = src.layout();
    const uint8_t* pRed = src.data()[rPlaneIndex];
    const uint8_t* pGreen = src.data()[gPlaneIndex];
    const uint8_t* pBlue = src.data()[bPlaneIndex];

    const C2PlaneInfo& rInfo = layout.planes[rPlaneIndex];
    const C2PlaneInfo& gInfo = layout.planes[gPlaneIndex];
    const C2PlaneInfo& bInfo = layout.planes[bPlaneIndex];

    for (uint32_t y = 0u, height = src.height(); y < height; ++y) {
        for (uint32_t x = 0u, width = src.width(); x < width; ++x) {
            const uint8_t red = *pRed;
            const uint8_t green = *pGreen;
            const uint8_t blue = *pBlue;

            // Using ITU-R BT.601 conversion matrix
            dstY[x] = ((red * 66 + green * 129 + blue * 25) >> 8) + 16;

            if ((x & 1) == 0 && (y & 1) == 0) {
                const uint8_t U =
                    ((-red * 38 - green * 74 + blue * 112) >> 8) + 128;
                const uint8_t V =
                    ((red * 112 - green * 94 - blue * 18) >> 8) + 128;

                if constexpr (toPlanar) {
                    dstU[x >> 1] = U;
                    dstV[x >> 1] = V;
                } else {
                    dstUV[x] = U;
                    dstUV[x + 1] = V;
                }
            }

            pRed += rInfo.colInc;
            pGreen += gInfo.colInc;
            pBlue += bInfo.colInc;
        }

        if ((y & 1) == 0) {
            if constexpr (toPlanar) {
                dstU += dstStride >> 1;
                dstV += dstStride >> 1;
            } else {
                dstUV += dstStride;
            }
        }

        pRed += -static_cast<uint32_t>(rInfo.colInc) * src.width() +
            static_cast<uint32_t>(rInfo.rowInc);
        pGreen += -static_cast<uint32_t>(gInfo.colInc) * src.width() +
            static_cast<uint32_t>(gInfo.rowInc);
        pBlue += -static_cast<uint32_t>(bInfo.colInc) * src.width() +
            static_cast<uint32_t>(bInfo.rowInc);

        dstY += dstStride;
    }
}

C2VendorEncComponent::C2VendorEncComponent(
    const std::shared_ptr<IntfImplEnc>& intfImpl,
    const std::shared_ptr<OMXR_Core>& omxrCore)
    : C2VendorBaseComponent{intfImpl, omxrCore},
      mIntfImpl{intfImpl},
      mVspmHandle{nullptr},
      mVspmResult{C2_NO_INIT},
      mGrallocDevice{nullptr},
      mGrallocLockFunc{nullptr},
      mGrallocUnlockFunc{nullptr} {
    R_LOG(DEBUG);

    if constexpr (C2EncInputFormat == HAL_PIXEL_FORMAT_RGBA_8888) {
        intfImpl->useRGBAPixelFormat();
    }

    initGralloc();

    if constexpr (USE_HW_CONVERSION) {
        initVspm();
    }
}

C2VendorEncComponent::~C2VendorEncComponent() {
    R_LOG(DEBUG);

    releaseComponent();

    if constexpr (USE_HW_CONVERSION) {
        deinitVspm();
    }

    deinitGralloc();
}

bool C2VendorEncComponent::onStateSet(OMX_STATETYPE omxState) {
    if (omxState == OMX_StateLoaded) {
        mVspmResult = C2_NO_INIT;
    }

    return C2VendorBaseComponent::onStateSet(omxState);
}

bool C2VendorEncComponent::onConfigure(const OMXR_Adapter& omxrAdapter) {
    const C2ProfileLevelStruct& profileLevel = mIntfImpl->getProfileLevel();
    const uint32_t bitrate = mIntfImpl->getBitrate();
    const uint32_t frameRate = mIntfImpl->getCodedFrameRate();
    const OMX_VIDEO_CODINGTYPE omxCodingType = mIntfImpl->getOMXCodingType();
    const uint32_t pixelFormat = mIntfImpl->getPixelFormat();
    const C2PictureSizeStruct& pictureSize = mIntfImpl->getPictureSize();

    R_LOG(DEBUG) << "Got params from intfImpl: "
                 << "profile " << profileLevel.profile << ", level "
                 << profileLevel.level << ", bitrate " << bitrate
                 << ", frameRate " << frameRate << ", omxCodingType "
                 << omxCodingType << ", pixelFormat " << pixelFormat
                 << ", pictureSize " << pictureSize.width << "x"
                 << pictureSize.height;

    constexpr OMX_VIDEO_CODINGTYPE inputCoding = OMX_VIDEO_CodingUnused;
    constexpr OMX_COLOR_FORMATTYPE inputColFormat = OMXEncColorFormat;
    const OMX_VIDEO_CODINGTYPE outputCoding = omxCodingType;
    constexpr OMX_COLOR_FORMATTYPE outputColFormat = OMX_COLOR_FormatUnused;
    const uint32_t width = pictureSize.width;
    const uint32_t height = pictureSize.height;

    R_LOG(DEBUG) << "OMX color format " << inputColFormat;

#ifdef R_LOG_VERBOSITY
    querySupportedProfileLevels(OutputPortIndex);
#endif

    const bool res = setPortFormat(InputPortIndex, frameRate, inputCoding,
                                   inputColFormat) == OMX_ErrorNone &&
        setPortFormat(OutputPortIndex, frameRate, outputCoding,
                      outputColFormat) == OMX_ErrorNone &&
        setPortDef(InputPortIndex, width, height, bitrate, frameRate,
                   inputCoding, inputColFormat) == OMX_ErrorNone &&
        setPortDef(OutputPortIndex, width, height, bitrate, frameRate,
                   outputCoding, outputColFormat) == OMX_ErrorNone;

    if (res) {
        switch (outputCoding) {
        case OMX_VIDEO_CodingAVC:
            setAVCEncoderProfileLevel(omxrAdapter, profileLevel.profile,
                                      profileLevel.level);
            break;
        default:
            break;
        }
    }

    return res;
}

c2_status_t C2VendorEncComponent::onProcessInput(
    const C2Work& work, OMX_BUFFERHEADERTYPE* const header, bool fromDequeue) {
    const size_t index = GetBufferIndex(header);
    const uint64_t frameIndex = work.input.ordinal.frameIndex.peeku();
    const bool eos = (work.input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0u;
    c2_status_t status = C2_OK;
    uint32_t size = 0u;

    if (!work.input.buffers.empty()) {
        const C2ConstGraphicBlock& graphicBlock =
            work.input.buffers.front()->data().graphicBlocks().front();
        const C2GraphicView view = graphicBlock.map().get();
        status = view.error();

        if (status != C2_OK) {
            R_LOG(ERROR) << "Mapping input failed, " << status;
            return status;
        }

        size = Align64(view.width()) * view.height() * 3u / 2u;

        if (header->nAllocLen < size) {
            R_LOG(ERROR) << "Small OMX input buffer, " << header->nAllocLen
                         << " < " << size;
            return C2_CORRUPTED;
        }

        const IMG_native_handle_t* const handle =
            reinterpret_cast<const IMG_native_handle_t*>(graphicBlock.handle());
        const int pixelFormat = handle->iFormat;

        // debug
        // DumpView<true>(view, std::to_string(frameIndex) + "_view");

        // NOTE: hardware address is stored in pInputPortPrivate
        if (pixelFormat == HAL_PIXEL_FORMAT_YV12) {
            if constexpr (USE_HW_CONVERSION) {
                status = vspmCopy(header->pInputPortPrivate, handle,
                                  OMXR_COLOR_FormatYV12, true);

                if (status != C2_OK) {
                    return status;
                }
            } else {
                CopyBuffer(header->pBuffer,
                           view.data()[C2PlanarLayout::PLANE_Y],
                           static_cast<size_t>(size));
            }

            R_LOG(DEBUG) << "Copied frameIndex " << frameIndex;
        } else {
            if constexpr (USE_HW_CONVERSION) {
                status =
                    vspmConvertRGBAToYUVAWithEmpty(header->pInputPortPrivate,
                                                   handle);

                if (status != C2_OK) {
                    return status;
                }
            } else {
                SWConvertRGBAToYUVA(view, pixelFormat, header);
            }

            R_LOG(DEBUG) << "Converted frameIndex " << frameIndex;
        }
    }

    OMX_U32 omxFlags = OMX_BUFFERFLAG_ENDOFFRAME;

    if (eos) {
        omxFlags |= OMX_BUFFERFLAG_EOS;
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

void C2VendorEncComponent::onOutputDone(const ExtendedBufferData& data) {
    OMX_BUFFERHEADERTYPE* const header = data.header;
    const size_t index = GetBufferIndex(header);
    uint64_t frameIndex = TsToFrameIndex(header->nTimeStamp);
    const OMX_U32 size = header->nFilledLen;

    R_LOG(DEBUG) << "Size " << size << ", OMXFlags " << std::hex
                 << header->nFlags << std::dec << ", frameIndex " << frameIndex
                 << ", index " << index;

    CHECK(!mEmptiedWorks.empty());

    if ((header->nFlags & OMX_BUFFERFLAG_CODECCONFIG) != 0u) {
        if (size > 0u) {
            std::unique_ptr<C2StreamInitDataInfo::output> csdParam =
                C2StreamInitDataInfo::output::AllocUnique(size, 0u);

            CopyBuffer(csdParam->m.value, header->pBuffer + header->nOffset,
                       size);

            const auto firstIt = mEmptiedWorks.begin();
            C2Work& firstWork = *firstIt->second;
            frameIndex = firstIt->first;

            firstWork.worklets.front()->output.configUpdate.push_back(
                std::move(csdParam));

            R_LOG(DEBUG) << "Appended csd to frameIndex " << frameIndex
                         << " with size " << size;
        } else {
            R_LOG(DEBUG) << "Skipping zero-length csd";
        }

        header->nFilledLen = 0u;
        header->nTimeStamp = 0;
        header->nFlags = 0u;

        fillBuffer(header);

        return;
    }

    // HACK: ignore EOS data as it isn't expected by the framework and doesn't
    // have a correct timestamp (frameIndex)
    if ((header->nFlags & OMX_BUFFERFLAG_EOS) != 0u) {
        const auto emptiedEOSIt =
            std::find_if(mEmptiedWorks.begin(), mEmptiedWorks.end(),
                         [](const auto& p) {
                             return (p.second->input.flags &
                                     C2FrameData::FLAG_END_OF_STREAM) != 0u;
                         });

        CHECK(emptiedEOSIt != mEmptiedWorks.end());

        frameIndex = emptiedEOSIt->first;
        std::unique_ptr<C2Work> work = std::move(emptiedEOSIt->second);
        mEmptiedWorks.erase(emptiedEOSIt);

        reportWork(std::move(work), C2_OK);

        return;
    }

    const auto emptiedIt = mEmptiedWorks.find(frameIndex);

    CHECK(emptiedIt != mEmptiedWorks.end());

    std::unique_ptr<C2Work> work = std::move(emptiedIt->second);
    mEmptiedWorks.erase(emptiedIt);

    C2FrameData& output = work->worklets.front()->output;

    CHECK_GT(size, 0u);

    const C2MemoryUsage usage{0, 0}; //workaround for disabling ion cache when allocating buffers in DDK
    std::shared_ptr<C2LinearBlock> linearBlock;

    if (const c2_status_t status =
            mOutputBlockPool->fetchLinearBlock(size, usage, &linearBlock);
        status != C2_OK) {
        reportWork(std::move(work), status);

        R_LOG(ERROR) << "Failed to create out block, " << status;
        return;
    }

    C2WriteView linearView = linearBlock->map().get();

    if (const c2_status_t status = linearView.error(); status != C2_OK) {
        reportWork(std::move(work), status);

        R_LOG(ERROR) << "Mapping output failed, " << status;
        return;
    }

    CopyBuffer(linearView.data(), header->pBuffer + header->nOffset, size);

    const std::shared_ptr<C2Buffer> linearBuffer =
        C2Buffer::CreateLinearBuffer(linearBlock->share(0u, size, C2Fence{}));

    output.buffers.push_back(linearBuffer);

    if ((header->nFlags & OMX_BUFFERFLAG_SYNCFRAME) != 0u) {
        R_LOG(DEBUG) << "Appended sync frame to frameIndex " << frameIndex;

        linearBuffer->setInfo(
            std::make_shared<
                C2StreamPictureTypeMaskInfo::output>(0u, C2Config::SYNC_FRAME));
    }

    header->nFilledLen = 0u;
    header->nTimeStamp = 0;
    header->nFlags = 0u;

    fillBuffer(header);

    if ((work->input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0u) {
        R_LOG(DEBUG) << "Postponing frameIndex " << frameIndex << " till eos";

        CHECK(mEmptiedWorks.emplace(frameIndex, std::move(work)).second);
    } else {
        reportWork(std::move(work), C2_OK);
    }
}

bool C2VendorEncComponent::MapAVCProfileLevel(
    C2Config::profile_t c2Profile,
    C2Config::level_t c2Level,
    OMX_VIDEO_AVCPROFILETYPE& omxProfile,
    OMX_VIDEO_AVCLEVELTYPE& omxLevel) {
    constexpr std::pair<C2Config::profile_t, OMX_VIDEO_AVCPROFILETYPE>
        avcProfileMap[] = {
            {PROFILE_AVC_BASELINE, OMX_VIDEO_AVCProfileBaseline},
            {PROFILE_AVC_MAIN, OMX_VIDEO_AVCProfileMain},
            {PROFILE_AVC_HIGH, OMX_VIDEO_AVCProfileHigh},
        };

    constexpr std::pair<C2Config::level_t, OMX_VIDEO_AVCLEVELTYPE>
        avcLevelMap[] = {
            {LEVEL_AVC_1, OMX_VIDEO_AVCLevel1},
            {LEVEL_AVC_1B, OMX_VIDEO_AVCLevel1b},
            {LEVEL_AVC_1_1, OMX_VIDEO_AVCLevel11},
            {LEVEL_AVC_1_2, OMX_VIDEO_AVCLevel12},
            {LEVEL_AVC_1_3, OMX_VIDEO_AVCLevel13},
            {LEVEL_AVC_2, OMX_VIDEO_AVCLevel2},
            {LEVEL_AVC_2_1, OMX_VIDEO_AVCLevel21},
            {LEVEL_AVC_2_2, OMX_VIDEO_AVCLevel22},
            {LEVEL_AVC_3, OMX_VIDEO_AVCLevel3},
            {LEVEL_AVC_3_1, OMX_VIDEO_AVCLevel31},
            {LEVEL_AVC_3_2, OMX_VIDEO_AVCLevel32},
            {LEVEL_AVC_4, OMX_VIDEO_AVCLevel4},
            {LEVEL_AVC_4_1, OMX_VIDEO_AVCLevel41},
            {LEVEL_AVC_4_2, OMX_VIDEO_AVCLevel42},
            {LEVEL_AVC_5, OMX_VIDEO_AVCLevel5},
            {LEVEL_AVC_5_1, OMX_VIDEO_AVCLevel51},
        };

    const auto avcProfileIt =
        std::find_if(std::begin(avcProfileMap), std::end(avcProfileMap),
                     [c2Profile](const auto& p) {
                         return p.first == c2Profile;
                     });

    if (avcProfileIt == std::end(avcProfileMap)) {
        R_LOG(ERROR) << "C2 profile " << c2Profile << " is not supported";
        return false;
    }

    const auto avcLevelIt =
        std::find_if(std::begin(avcLevelMap), std::end(avcLevelMap),
                     [c2Level](const auto& p) { return p.first == c2Level; });

    if (avcLevelIt == std::end(avcLevelMap)) {
        R_LOG(ERROR) << "C2 level " << c2Level << " is not supported";
        return false;
    }

    omxProfile = avcProfileIt->second;
    omxLevel = avcLevelIt->second;

    R_LOG(DEBUG) << "OMX profile " << omxProfile << ", level " << omxLevel;

    return true;
}

void C2VendorEncComponent::SWConvertRGBAToYUVA(
    const C2GraphicView& view,
    int pixelFormat,
    OMX_BUFFERHEADERTYPE* const header) {
    CHECK(pixelFormat == HAL_PIXEL_FORMAT_RGBA_8888 ||
          pixelFormat == HAL_PIXEL_FORMAT_BGRA_8888);

    constexpr bool fromBGRA = C2EncInputFormat == HAL_PIXEL_FORMAT_BGRA_8888;
    const size_t dstStride = static_cast<size_t>(Align64(view.width()));
    const size_t dstVStride = static_cast<size_t>(view.height());

#ifdef R_LOG_VERBOSITY
    constexpr const char* const inputFmtStr = fromBGRA ? "BGRA" : "RGBA";
    constexpr auto fmtStringGetter = [](OMX_COLOR_FORMATTYPE colorFormat) {
        switch (colorFormat) {
        case OMXR_COLOR_FormatYV12:
            return "YV12";
        case OMX_COLOR_FormatYUV420Planar:
            return "Planar";
        case OMX_COLOR_FormatYUV420SemiPlanar:
            return "SemiPlanar";
        default:
            return "?";
        }
    };
    constexpr const char* const outputFmtStr =
        fmtStringGetter(OMXEncColorFormat);

    R_LOG(DEBUG) << inputFmtStr << " -> " << outputFmtStr << ", size "
                 << view.width() << "x" << view.height() << ", " << dstStride
                 << "x" << dstVStride;
#endif

    constexpr bool toYVU = OMXEncColorFormat == OMXR_COLOR_FormatYV12;
    constexpr bool toPlanar =
        toYVU || OMXEncColorFormat == OMX_COLOR_FormatYUV420Planar;

    ConvertRGBAToYUVA<fromBGRA, toPlanar, toYVU>(header->pBuffer, dstStride,
                                                 dstVStride, view);

    // debug
    // DumpHeader<true>(header,
    // std::to_string(TsToFrameIndex(header->nTimeStamp)) + "_sw_header");
}

void C2VendorEncComponent::VspmCompleteCallback(
    unsigned long jobId ATTRIBUTE_UNUSED, long result, void* userData) {
    R_LOG(DEBUG) << "Result " << result;

    C2VendorEncComponent* const component =
        static_cast<C2VendorEncComponent*>(userData);

    std::lock_guard lock{component->mVspmMutex};
    component->mVspmResult =
        (component->mVspmResult == C2_NO_INIT && result == R_VSPM_OK)
        ? C2_OK
        : C2_CORRUPTED;
    component->mVspmCV.notify_one();
}

void C2VendorEncComponent::initVspm() {
    vspm_init_t vspmInitParam;
    vspmInitParam.use_ch = VSPM_EMPTY_CH;
    vspmInitParam.mode = VSPM_MODE_MUTUAL;
    vspmInitParam.type = VSPM_TYPE_VSP_AUTO;
    vspmInitParam.par.vsp = nullptr;

    CHECK_EQ(vspm_init_driver(&mVspmHandle, &vspmInitParam), R_VSPM_OK);
}

void C2VendorEncComponent::deinitVspm() {
    if (mVspmHandle != nullptr) {
        CHECK_EQ(vspm_quit_driver(mVspmHandle), R_VSPM_OK);
    }
}

c2_status_t C2VendorEncComponent::vspmCopy(
    const void* const omxPhysAddr,
    const IMG_native_handle_t* const handle,
    OMX_COLOR_FORMATTYPE colorFormat,
    bool input) {
    uint64_t physAddr = 0u;

    CHECK_EQ(mmngr_get_buffer_phys_addr(handle->fd[0], &physAddr), R_MM_OK);

    const unsigned short width = static_cast<unsigned short>(handle->iWidth);
    const unsigned short height = static_cast<unsigned short>(handle->iHeight);
    const unsigned short stride = Align64(width);
    const unsigned short vStride = height;

    const uint8_t* srcY = static_cast<const uint8_t*>(omxPhysAddr);
    const uint8_t* srcU = srcY + stride * vStride;
    const uint8_t* srcV = srcU + (stride >> 1) * (vStride >> 1);

    const uint8_t* dstY = reinterpret_cast<const uint8_t*>(physAddr);
    const uint8_t* dstU = dstY + stride * vStride;
    const uint8_t* dstV = dstU + (stride >> 1) * (vStride >> 1);

    if (input) {
        std::swap(srcY, dstY);
        std::swap(srcU, dstU);
        std::swap(srcV, dstV);
    }

    unsigned short vspFormat = 0u;
    unsigned short vspStrideC = 0u;

    if (colorFormat == OMX_COLOR_FormatYUV420Planar) {
        vspFormat = VSP_OUT_YUV420_PLANAR;
        vspStrideC = stride >> 1;
    } else if (colorFormat == OMXR_COLOR_FormatYV12) {
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

c2_status_t C2VendorEncComponent::vspmConvertRGBAToYUVAWithEmpty(
    const void* const omxPhysAddr, const IMG_native_handle_t* const handle) {
    uint64_t physAddr = 0u;

    CHECK_EQ(mmngr_get_buffer_phys_addr(handle->fd[0u], &physAddr), R_MM_OK);

    unsigned short vspInputFormat = 0u;
    unsigned char vspInputSwap = 0u;

    if (handle->iFormat == HAL_PIXEL_FORMAT_RGBA_8888) {
        vspInputFormat = VSP_IN_RGBA8888;
        vspInputSwap = VSP_SWAP_B | VSP_SWAP_W | VSP_SWAP_L | VSP_SWAP_LL;
    } else if (handle->iFormat == HAL_PIXEL_FORMAT_BGRA_8888) {
        vspInputFormat = VSP_IN_ARGB8888;
        vspInputSwap = VSP_SWAP_L | VSP_SWAP_LL;
    } else {
        R_LOG(ERROR) << "Bad IMG handle format " << handle->iFormat
                     << ", expected " << C2EncInputFormat;
        return C2_BAD_VALUE;
    }

    const unsigned short width = static_cast<unsigned short>(handle->iWidth);
    const unsigned short height = static_cast<unsigned short>(handle->iHeight);
    const unsigned short stride = Align64(width);
    const unsigned short vStride = height;

    // IN params
    constexpr char jobPriority = VSPM_PRI_MAX;

    vsp_alpha_unit_t vspAlphaParam{};
    vspAlphaParam.swap = VSP_SWAP_NO;
    vspAlphaParam.asel = VSP_ALPHA_NUM5;
    vspAlphaParam.aext = VSP_AEXT_COPY;
    vspAlphaParam.afix = std::numeric_limits<unsigned char>::max();

    vsp_src_t vspSrcParam{};
    vspSrcParam.addr = static_cast<unsigned int>(physAddr);
    vspSrcParam.stride = stride * 4;
    vspSrcParam.width = width;
    vspSrcParam.height = height;
    vspSrcParam.format = vspInputFormat;
    vspSrcParam.swap = vspInputSwap;
    vspSrcParam.pwd = VSP_LAYER_PARENT;
    vspSrcParam.cipm = VSP_CIPM_0_HOLD;
    vspSrcParam.cext = VSP_CEXT_EXPAN;
    vspSrcParam.csc = VSP_CSC_ON;
    vspSrcParam.iturbt = VSP_ITURBT_601;
    vspSrcParam.clrcng = VSP_ITU_COLOR;
    vspSrcParam.vir = VSP_NO_VIR;
    vspSrcParam.alpha = &vspAlphaParam;

    const uint8_t* const dstY = static_cast<const uint8_t*>(omxPhysAddr);
    const uint8_t* dstU = dstY + stride * vStride;
    const uint8_t* dstV = dstU + (stride >> 1) * (vStride >> 1);

    unsigned short vspOutputFormat = 0u;
    unsigned short vspOutputStrideC = 0u;

    if constexpr (OMXEncColorFormat == OMX_COLOR_FormatYUV420Planar) {
        vspOutputFormat = VSP_OUT_YUV420_PLANAR;
        vspOutputStrideC = stride >> 1;
    } else if constexpr (OMXEncColorFormat == OMXR_COLOR_FormatYV12) {
        vspOutputFormat = VSP_OUT_YUV420_PLANAR;
        vspOutputStrideC = stride >> 1;

        std::swap(dstU, dstV);
    } else {
        vspOutputFormat = VSP_OUT_YUV420_SEMI_PLANAR;
        vspOutputStrideC = stride;
    }

    vsp_dst_t vspDstParam{};
    vspDstParam.addr = *reinterpret_cast<const unsigned int*>(&dstY);
    vspDstParam.addr_c0 = *reinterpret_cast<const unsigned int*>(&dstU);
    vspDstParam.addr_c1 = *reinterpret_cast<const unsigned int*>(&dstV);
    vspDstParam.stride = stride;
    vspDstParam.stride_c = vspOutputStrideC;
    vspDstParam.width = width;
    vspDstParam.height = height;
    vspDstParam.format = vspOutputFormat;
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

#ifdef R_LOG_VERBOSITY
    const char* const inputFmtStr =
        handle->iFormat == HAL_PIXEL_FORMAT_RGBA_8888 ? "RGBA" : "BGRA";
    constexpr auto fmtStringGetter = [](OMX_COLOR_FORMATTYPE colorFormat) {
        switch (colorFormat) {
        case OMXR_COLOR_FormatYV12:
            return "YV12";
        case OMX_COLOR_FormatYUV420Planar:
            return "Planar";
        case OMX_COLOR_FormatYUV420SemiPlanar:
            return "SemiPlanar";
        default:
            return "?";
        }
    };
    constexpr const char* const outputFmtStr =
        fmtStringGetter(OMXEncColorFormat);
    R_LOG(DEBUG) << inputFmtStr << " -> " << outputFmtStr << ", " << width
                 << "x" << height << ", " << stride << "x" << vStride
                 << ", strideC " << vspOutputStrideC;
#endif

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

void C2VendorEncComponent::initGralloc() {
    const hw_module_t* grallocModule = nullptr;

    CHECK_EQ(hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &grallocModule), 0);
    CHECK_EQ(gralloc1_open(grallocModule, &mGrallocDevice),
             GRALLOC1_ERROR_NONE);

    mGrallocLockFunc = reinterpret_cast<decltype(mGrallocLockFunc)>(
        mGrallocDevice->getFunction(mGrallocDevice, GRALLOC1_FUNCTION_LOCK));

    CHECK(mGrallocLockFunc != nullptr);

    mGrallocUnlockFunc = reinterpret_cast<decltype(mGrallocUnlockFunc)>(
        mGrallocDevice->getFunction(mGrallocDevice, GRALLOC1_FUNCTION_UNLOCK));

    CHECK(mGrallocUnlockFunc != nullptr);
}

void C2VendorEncComponent::deinitGralloc() {
    if (mGrallocDevice != nullptr) {
        CHECK_EQ(gralloc1_close(mGrallocDevice), GRALLOC1_ERROR_NONE);
    }
}

void C2VendorEncComponent::setAVCEncoderProfileLevel(
    const OMXR_Adapter& omxrAdapter,
    C2Config::profile_t c2Profile,
    C2Config::level_t c2Level) const {
    OMX_VIDEO_PARAM_AVCTYPE avcType;

    OMX_ERRORTYPE omxError =
        omxrAdapter.getPortParam(OutputPortIndex, OMX_IndexParamVideoAvc,
                                 avcType);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to get avc type, " << omxError;
        return;
    }

    R_LOG(DEBUG) << "Got avc type: "
                 << "sliceHeaderSpacing " << avcType.nSliceHeaderSpacing
                 << ", frames: B " << avcType.nBFrames << ", Ref "
                 << avcType.nRefFrames << ", P " << avcType.nPFrames
                 << ", profile " << avcType.eProfile << ", level "
                 << avcType.eLevel << ", loopFilterMode "
                 << avcType.eLoopFilterMode;

    if (!MapAVCProfileLevel(c2Profile, c2Level, avcType.eProfile,
                            avcType.eLevel)) {
        return;
    }

    if (avcType.eProfile != OMX_VIDEO_AVCProfileBaseline) {
        // Recomended setting for FHD resolution with B-frame
        avcType.nBFrames = 1;
        avcType.nRefFrames = 2;
        avcType.nPFrames = 10;
    }

    omxError = omxrAdapter.setParam(OMX_IndexParamVideoAvc, avcType);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to set avc type, " << omxError;
    }
}

} // namespace android::hardware::media::c2::V1_0::renesas
