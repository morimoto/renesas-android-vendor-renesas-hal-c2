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

#define LOG_TAG "C2VendorComponentInferface"
//#define R_LOG_VERBOSITY

#include "C2VendorComponentInferface.h"

#include "C2VendorDebug.h"

#ifdef R_LOG_VERBOSITY
struct C2_HIDE _C2ParamInspector {
    static uint32_t GetIndex(const C2ParamField& pf) { return pf._mIndex; }
};
#endif

namespace android::hardware::media::c2::V1_0::renesas {

#ifdef R_LOG_VERBOSITY
C2String asString(C2Param::type_index_t i);
#endif

template <typename T, typename... ARGS>
std::shared_ptr<T> AllocString(const ARGS&&... args, const std::string& str) {
    const std::shared_ptr<T> res =
        T::AllocShared(str.length() + 1, std::forward<ARGS>(args)...);

    CHECK(res);

    std::copy(str.cbegin(), str.cend(), res->m.value);

    return res;
}

template <typename T>
struct Setter {
    using Type = typename std::remove_reference<T>::type;

    static C2R NonStrictValue(bool mayBlock ATTRIBUTE_UNUSED,
                              C2InterfaceHelper::C2P<Type>& me) {
        return me.F(me.v.value).validatePossible(me.v.value);
    }

    static C2R NonStrictValues(bool mayBlock ATTRIBUTE_UNUSED,
                               C2InterfaceHelper::C2P<Type>& me) {
        auto res = C2R::Ok();

        for (size_t ix = 0u; ix < me.v.flexCount(); ++ix) {
            const auto& value = me.v.m.values[ix];
            res = res.plus(me.F(value).validatePossible(value));
        }

        return res;
    }
};

IntfImpl::IntfImpl(const std::shared_ptr<C2ReflectorHelper>& reflector,
                   const CodecInfo& info)
    : C2InterfaceHelper{reflector},
      mC2Name{info.c2Name},
      mOMXName{info.omxName},
      mOMXCodingType{info.omxCodingType} {
    setDerivedInstance(this);

    constexpr C2Component::domain_t domain = C2Component::DOMAIN_VIDEO;
    const bool isDecoder = info.isDecoder;
    const C2Component::kind_t kind =
        isDecoder ? C2Component::KIND_DECODER : C2Component::KIND_ENCODER;

    const char* const rawMediaType = MEDIA_MIMETYPE_VIDEO_RAW;
    const C2BufferData::type_t rawBufferType = C2BufferData::GRAPHIC;
    const C2BlockPool::local_id_t rawPoolId = C2BlockPool::BASIC_GRAPHIC;

    const std::string& codedMediaType = info.mediaType;
    constexpr C2BufferData::type_t codedBufferType = C2BufferData::LINEAR;
    constexpr C2BlockPool::local_id_t codedPoolId = C2BlockPool::BASIC_LINEAR;

    const std::string inputMediaType =
        isDecoder ? codedMediaType : rawMediaType;
    const std::string outputMediaType =
        isDecoder ? rawMediaType : codedMediaType;

    const C2BufferData::type_t inputBufferType =
        isDecoder ? codedBufferType : rawBufferType;
    const C2BufferData::type_t outputBufferType =
        isDecoder ? rawBufferType : codedBufferType;

    const C2BlockPool::local_id_t outputPoolId =
        isDecoder ? rawPoolId : codedPoolId;
    const C2BlockPool::local_id_t outputPoolIds[] = {outputPoolId};

    const uint32_t pixelFormat = info.pixelFormat;

    // TODO: get framerate|bitrate from info
    constexpr float defaultFps = 30.f;
    constexpr float minFps = 10.f;
    constexpr float maxFps = 60.f;

    constexpr uint32_t defaultBitrate = 20'000'000u;
    constexpr uint32_t minBitrate = 12'000'000u;
    constexpr uint32_t maxBitrate = 40'000'000u;

    addParameter(
        DefineParam(mKind, C2_PARAMKEY_COMPONENT_KIND)
            .withConstValue(std::make_shared<C2ComponentKindSetting>(kind))
            .build());

    addParameter(
        DefineParam(mDomain, C2_PARAMKEY_COMPONENT_DOMAIN)
            .withConstValue(std::make_shared<C2ComponentDomainSetting>(domain))
            .build());

    addParameter(DefineParam(mInputMediaType, C2_PARAMKEY_INPUT_MEDIA_TYPE)
                     .withConstValue(AllocString<C2PortMediaTypeSetting::input>(
                         inputMediaType))
                     .build());

    addParameter(
        DefineParam(mOutputMediaType, C2_PARAMKEY_OUTPUT_MEDIA_TYPE)
            .withConstValue(
                AllocString<C2PortMediaTypeSetting::output>(outputMediaType))
            .build());

    addParameter(
        DefineParam(mInputFormat, C2_PARAMKEY_INPUT_STREAM_BUFFER_TYPE)
            .withConstValue(
                std::make_shared<
                    C2StreamBufferTypeSetting::input>(0u, inputBufferType))
            .build());

    addParameter(
        DefineParam(mOutputFormat, C2_PARAMKEY_OUTPUT_STREAM_BUFFER_TYPE)
            .withConstValue(
                std::make_shared<
                    C2StreamBufferTypeSetting::output>(0u, outputBufferType))
            .build());

    // TODO: make dependent from size
    addParameter(
        DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
            .withDefault(
                std::make_shared<C2StreamProfileLevelInfo>(!isDecoder, 0u,
                                                           info.defaultProfile,
                                                           info.defaultLevel))
            .withFields({
                C2F(mProfileLevel, profile).oneOf(info.profiles),
                C2F(mProfileLevel, level).oneOf(info.levels),
            })
            .withSetter(ProfileLevelSetter)
            .build());

    addParameter(
        DefineParam(mCodedFrameRate, C2_PARAMKEY_FRAME_RATE)
            .withDefault(std::make_shared<C2StreamFrameRateInfo>(!isDecoder, 0u,
                                                                 defaultFps))
            .withFields({
                C2F(mCodedFrameRate, value).inRange(minFps, maxFps),
            })
            .withSetter(Setter<C2StreamFrameRateInfo>::NonStrictValue)
            .build());

    addParameter(
        DefineParam(mBitrate, C2_PARAMKEY_BITRATE)
            .withDefault(std::make_shared<C2StreamBitrateInfo>(!isDecoder, 0u,
                                                               defaultBitrate))
            .withFields({
                C2F(mBitrate, value).inRange(minBitrate, maxBitrate),
            })
            .withSetter(Setter<C2StreamBitrateInfo>::NonStrictValue)
            .build());

    addParameter(
        DefineParam(mSize, C2_PARAMKEY_PICTURE_SIZE)
            .withDefault(
                std::make_shared<C2StreamPictureSizeInfo>(isDecoder, 0u,
                                                          info.minSize.width,
                                                          info.minSize.height))
            .withFields({
                C2F(mSize, width)
                    .inRange(info.minSize.width, info.maxSize.width, 2u),
                C2F(mSize, height)
                    .inRange(info.minSize.height, info.maxSize.height, 2u),
            })
            .withSetter(SizeSetter)
            .build());

    addParameter(
        DefineParam(mPixelFormat, C2_PARAMKEY_PIXEL_FORMAT)
            .withDefault(std::make_shared<C2StreamPixelFormatInfo>(isDecoder,
                                                                   0u,
                                                                   pixelFormat))
            .withFields({C2F(mPixelFormat, value).oneOf({pixelFormat})})
            .withSetter(Setter<C2StreamPixelFormatInfo>::NonStrictValue)
            .build());

    addParameter(
        DefineParam(mOutputBlockPoolIds, C2_PARAMKEY_OUTPUT_BLOCK_POOLS)
            .withDefault(
                C2PortBlockPoolsTuning::output::AllocShared(outputPoolIds))
            .withFields({
                C2F(mOutputBlockPoolIds, m.values[0]).any(),
                C2F(mOutputBlockPoolIds, m.values).inRange(0, 1),
            })
            .withSetter(Setter<C2PortBlockPoolsTuning::output>::NonStrictValues)
            .build());
}

C2R IntfImpl::ProfileLevelSetter(bool mayBlock ATTRIBUTE_UNUSED,
                                 C2P<C2StreamProfileLevelInfo>& me) {
    R_LOG(DEBUG) << "Profile " << me.v.profile << ", level " << me.v.level;

    return C2R::Ok();
}

C2R IntfImpl::SizeSetter(bool mayBlock ATTRIBUTE_UNUSED,
                         const C2P<C2StreamPictureSizeInfo>& old,
                         C2P<C2StreamPictureSizeInfo>& me) {
    const auto& oldParam = old.v;
    auto& meParam = me.set();

    R_LOG(DEBUG) << oldParam.width << "x" << oldParam.height << " -> "
                 << meParam.width << "x" << meParam.height;

    C2R res = C2R::Ok();

    if (const auto widthField = me.F(meParam.width);
        !widthField.supportsAtAll(meParam.width)) {
        R_LOG(ERROR) << "Unsupported width " << meParam.width
                     << ", fallback to " << oldParam.width;

        meParam.width = oldParam.width;
        res = res.plus(C2SettingResultBuilder::BadValue(widthField));
    }

    if (const auto heightField = me.F(meParam.height);
        !heightField.supportsAtAll(meParam.height)) {
        R_LOG(ERROR) << "Unsupported height " << meParam.height
                     << ", fallback to " << oldParam.height;

        meParam.height = oldParam.height;
        res = res.plus(C2SettingResultBuilder::BadValue(heightField));
    }

    return res;
}

IntfImplDec::IntfImplDec(const std::shared_ptr<C2ReflectorHelper>& reflector,
                         const CodecInfo& info)
    : IntfImpl{reflector, info} {
    addParameter(
        DefineParam(mOutputMaxPictureSize, C2_PARAMKEY_MAX_PICTURE_SIZE)
            .withDefault(
                std::make_shared<C2StreamMaxPictureSizeTuning::output>(0u))
            .withFields({
                C2F(mOutputMaxPictureSize, width)
                    .inRange(info.minSize.width, info.maxSize.width, 2u),
                C2F(mOutputMaxPictureSize, height)
                    .inRange(info.minSize.height, info.maxSize.height, 2u),
            })
            .withSetter(MaxPictureSizeSetter, mSize)
            .build());

    constexpr uint32_t delay = 8u;

    // HACK: to overcome queue timeout in case of player's pause
    addParameter(
        DefineParam(mOutputDelay, C2_PARAMKEY_OUTPUT_DELAY)
            .withConstValue(
                std::make_shared<C2PortActualDelayTuning::output>(delay))
            .build());

    addParameter(
        DefineParam(mDefaultColorAspects, C2_PARAMKEY_DEFAULT_COLOR_ASPECTS)
            .withDefault(
                std::make_shared<C2StreamColorAspectsTuning::output>(0u))
            .withFields({
                C2F(mDefaultColorAspects, range)
                    .inRange(C2Color::RANGE_UNSPECIFIED, C2Color::RANGE_OTHER),
                C2F(mDefaultColorAspects, primaries)
                    .inRange(C2Color::PRIMARIES_UNSPECIFIED,
                             C2Color::PRIMARIES_OTHER),
                C2F(mDefaultColorAspects, transfer)
                    .inRange(C2Color::TRANSFER_UNSPECIFIED,
                             C2Color::TRANSFER_OTHER),
                C2F(mDefaultColorAspects, matrix)
                    .inRange(C2Color::MATRIX_UNSPECIFIED,
                             C2Color::MATRIX_OTHER),
            })
            .withSetter(DefaultColorAspectsSetter)
            .build());

    addParameter(
        DefineParam(mCodedColorAspects, C2_PARAMKEY_VUI_COLOR_ASPECTS)
            .withDefault(std::make_shared<C2StreamColorAspectsInfo::input>(0u))
            .withFields(
                {C2F(mCodedColorAspects, range)
                     .inRange(C2Color::RANGE_UNSPECIFIED, C2Color::RANGE_OTHER),
                 C2F(mCodedColorAspects, primaries)
                     .inRange(C2Color::PRIMARIES_UNSPECIFIED,
                              C2Color::PRIMARIES_OTHER),
                 C2F(mCodedColorAspects, transfer)
                     .inRange(C2Color::TRANSFER_UNSPECIFIED,
                              C2Color::TRANSFER_OTHER),
                 C2F(mCodedColorAspects, matrix)
                     .inRange(C2Color::MATRIX_UNSPECIFIED,
                              C2Color::MATRIX_OTHER)})
            .withSetter(CodedColorAspectsSetter)
            .build());

    addParameter(
        DefineParam(mColorAspects, C2_PARAMKEY_COLOR_ASPECTS)
            .withDefault(std::make_shared<C2StreamColorAspectsInfo::output>(0u))
            .withFields(
                {C2F(mColorAspects, range)
                     .inRange(C2Color::RANGE_UNSPECIFIED, C2Color::RANGE_OTHER),
                 C2F(mColorAspects, primaries)
                     .inRange(C2Color::PRIMARIES_UNSPECIFIED,
                              C2Color::PRIMARIES_OTHER),
                 C2F(mColorAspects, transfer)
                     .inRange(C2Color::TRANSFER_UNSPECIFIED,
                              C2Color::TRANSFER_OTHER),
                 C2F(mColorAspects, matrix)
                     .inRange(C2Color::MATRIX_UNSPECIFIED,
                              C2Color::MATRIX_OTHER)})
            .withSetter(ColorAspectsSetter, mDefaultColorAspects,
                        mCodedColorAspects)
            .build());
}

C2R IntfImplDec::MaxPictureSizeSetter(
    bool mayBlock ATTRIBUTE_UNUSED,
    const C2P<C2StreamMaxPictureSizeTuning::output>& old,
    C2P<C2StreamMaxPictureSizeTuning::output>& me,
    const C2P<C2StreamPictureSizeInfo>& size) {
    const auto& oldParam = old.v;
    auto& meParam = me.set();
    const auto& sizeParam = size.v;

    R_LOG(DEBUG) << oldParam.width << "x" << oldParam.height << " -> "
                 << meParam.width << "x" << meParam.height << ", size "
                 << sizeParam.width << "x" << sizeParam.height;

    C2R res = C2R::Ok();

    if (meParam.width != 0u) {
        if (const auto widthField = me.F(meParam.width);
            !widthField.supportsAtAll(meParam.width)) {
            R_LOG(ERROR) << "Unsupported width " << meParam.width
                         << ", fallback to " << oldParam.width;

            meParam.width = oldParam.width;
            res = res.plus(C2SettingResultBuilder::BadValue(widthField));
        } else {
            meParam.width = std::max(meParam.width, sizeParam.width);
        }
    }

    if (meParam.height != 0u) {
        if (const auto heightField = me.F(meParam.height);
            !heightField.supportsAtAll(meParam.height)) {
            R_LOG(ERROR) << "Unsupported height " << meParam.height
                         << ", fallback to " << oldParam.height;

            meParam.height = oldParam.height;
            res = res.plus(C2SettingResultBuilder::BadValue(heightField));
        } else {
            meParam.height = std::max(meParam.height, sizeParam.height);
        }
    }

    R_LOG(DEBUG) << "Final " << meParam.width << "x" << meParam.height;

    return res;
}

C2R IntfImplDec::DefaultColorAspectsSetter(
    bool mayBlock ATTRIBUTE_UNUSED,
    C2P<C2StreamColorAspectsTuning::output>& me) {
    auto& meParam = me.set();

    meParam.range = std::min(meParam.range, C2Color::RANGE_OTHER);
    meParam.primaries = std::min(meParam.primaries, C2Color::PRIMARIES_OTHER);
    meParam.transfer = std::min(meParam.transfer, C2Color::TRANSFER_OTHER);
    meParam.matrix = std::min(meParam.matrix, C2Color::MATRIX_OTHER);

    R_LOG(DEBUG) << "Default: " << meParam;

    return C2R::Ok();
}

C2R IntfImplDec::CodedColorAspectsSetter(
    bool mayBlock ATTRIBUTE_UNUSED, C2P<C2StreamColorAspectsInfo::input>& me) {
    auto& meParam = me.set();

    meParam.range = std::min(meParam.range, C2Color::RANGE_OTHER);
    meParam.primaries = std::min(meParam.primaries, C2Color::PRIMARIES_OTHER);
    meParam.transfer = std::min(meParam.transfer, C2Color::TRANSFER_OTHER);
    meParam.matrix = std::min(meParam.matrix, C2Color::MATRIX_OTHER);

    return C2R::Ok();
}

C2R IntfImplDec::ColorAspectsSetter(
    bool mayBlock ATTRIBUTE_UNUSED,
    C2P<C2StreamColorAspectsInfo::output>& me,
    const C2P<C2StreamColorAspectsTuning::output>& def,
    const C2P<C2StreamColorAspectsInfo::input>& coded) {
    const auto& defAspects = def.v;
    const auto& codedAspects = coded.v;
    auto& finalAspects = me.set();

    finalAspects.range = codedAspects.range == RANGE_UNSPECIFIED
        ? defAspects.range
        : codedAspects.range;
    finalAspects.primaries = codedAspects.primaries == PRIMARIES_UNSPECIFIED
        ? defAspects.primaries
        : codedAspects.primaries;
    finalAspects.transfer = codedAspects.transfer == TRANSFER_UNSPECIFIED
        ? defAspects.transfer
        : codedAspects.transfer;
    finalAspects.matrix = codedAspects.matrix == MATRIX_UNSPECIFIED
        ? defAspects.matrix
        : codedAspects.matrix;

    R_LOG(DEBUG) << "(coded: " << codedAspects << " | default: " << defAspects
                 << ") -> final: " << finalAspects;

    return C2R::Ok();
}

void IntfImplEnc::useRGBAPixelFormat() {
    addParameter(
        DefineParam(mUsage, C2_PARAMKEY_INPUT_STREAM_USAGE)
            .withConstValue(
                std::make_shared<
                    C2StreamUsageTuning::input>(0u, C2MemoryUsage::CPU_READ))
            .build());
}

c2_status_t C2VendorComponentInterface::query_vb(
    const std::vector<C2Param*>& stackParams,
    const std::vector<C2Param::Index>& heapParamIndices,
    c2_blocking_t mayBlock,
    std::vector<std::unique_ptr<C2Param>>* const heapParams) const {
#ifdef R_LOG_VERBOSITY
    std::vector<C2Param::type_index_t> typeIndexes;

    for (const C2Param* const p : stackParams) {
        if (p != nullptr) {
            typeIndexes.push_back(p->coreIndex().typeIndex());
        }
    }

    for (const C2Param::Index& index : heapParamIndices) {
        typeIndexes.push_back(index.typeIndex());
    }

    C2String indexStr;

    for (const C2Param::type_index_t ix : typeIndexes) {
        indexStr += asString(ix) + "|";
    }

    R_LOG(DEBUG) << stackParams.size() << '|' << heapParamIndices.size() << ", "
                 << indexStr;
#endif

    return mImpl->query(stackParams, heapParamIndices, mayBlock, heapParams);
}

c2_status_t C2VendorComponentInterface::config_vb(
    const std::vector<C2Param*>& params,
    c2_blocking_t mayBlock,
    std::vector<std::unique_ptr<C2SettingResult>>* const failures) {
#ifdef R_LOG_VERBOSITY
    std::vector<C2Param::type_index_t> typeIndexes;

    for (const C2Param* const p : params) {
        if (p != nullptr) {
            typeIndexes.push_back(p->coreIndex().typeIndex());
        }
    }

    C2String indexStr;

    for (const C2Param::type_index_t ix : typeIndexes) {
        indexStr += asString(ix) + "|";
    }

    R_LOG(DEBUG) << typeIndexes.size() << ", " << indexStr;

#endif

    return mImpl->config(params, mayBlock, failures);
}

c2_status_t C2VendorComponentInterface::querySupportedParams_nb(
    std::vector<std::shared_ptr<C2ParamDescriptor>>* const params) const {
    const c2_status_t res = mImpl->querySupportedParams(params);

#ifdef R_LOG_VERBOSITY
    C2String indexStr;

    for (const std::shared_ptr<C2ParamDescriptor>& descr : *params) {
        if (descr) {
            indexStr += asString(descr->index().typeIndex()) + "|";
        }
    }

    R_LOG(DEBUG) << asString(res) << ", " << params->size() << ", " << indexStr;
#endif

    return res;
}

c2_status_t C2VendorComponentInterface::querySupportedValues_vb(
    std::vector<C2FieldSupportedValuesQuery>& fields,
    c2_blocking_t mayBlock) const {
#ifdef R_LOG_VERBOSITY
    C2String indexStr;

    for (const C2FieldSupportedValuesQuery& f : fields) {
        indexStr +=
            asString(C2Param::CoreIndex{_C2ParamInspector::GetIndex(f.field())}
                         .typeIndex()) +
            "|";
    }

    R_LOG(DEBUG) << fields.size() << ", " << indexStr;
#endif

    return mImpl->querySupportedValues(fields, mayBlock);
}

#ifdef R_LOG_VERBOSITY

#define DECLARE_BASE_CASE(base, arg) \
    case base::arg:                  \
        return (#arg)

C2String asString(C2Param::type_index_t i) {
    const C2ParamIndexKind indexKind = static_cast<C2ParamIndexKind>(i);

    switch (indexKind) {
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexColorXy);
        DECLARE_BASE_CASE(C2ParamIndexKind,
                          kParamIndexMasteringDisplayColorVolume);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexChromaOffset);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexGopLayer);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexApiLevel);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexApiFeatures);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexName);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexAliases);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexKind);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDomain);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexAttributes);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTimeStretch);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexProfileLevel);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexInitData);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSupplementalData);
        DECLARE_BASE_CASE(C2ParamIndexKind,
                          kParamIndexSubscribedSupplementalData);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMediaType);
        DECLARE_BASE_CASE(C2ParamIndexKind, __kParamIndexRESERVED_0);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDelay);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMaxReferenceAge);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMaxReferenceCount);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexReorderBufferDepth);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexReorderKey);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexStreamCount);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSubscribedParamIndices);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSuggestedBufferCount);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBatchSize);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexCurrentWork);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexLastWorkQueued);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexAllocators);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBlockPools);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBufferType);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexUsage);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexOutOfMemory);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMaxBufferSize);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTripped);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexResourcesNeeded);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexResourcesReserved);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexOperatingRate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexRealTimePriority);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSecureMode);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDelayRequest);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBitrate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBitrateMode);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexQuality);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexComplexity);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPrependHeaderMode);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPictureSize);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexCropRect);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPixelFormat);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexRotation);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPixelAspectRatio);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexScaledPictureSize);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexScaledCropRect);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexScalingMethod);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexColorInfo);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexColorAspects);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexHdrStaticMetadata);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDefaultColorAspects);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBlockSize);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBlockCount);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexBlockRate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPictureTypeMask);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPictureType);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexHdr10PlusMetadata);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexFrameRate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMaxBitrate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMaxFrameRate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMaxPictureSize);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexGop);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSyncFrameInterval);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexRequestSyncFrame);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTemporalLayering);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexLayerIndex);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexLayerCount);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexIntraRefresh);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTileLayout);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTileHandling);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSampleRate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexChannelCount);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPcmEncoding);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexAacPackaging);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMaxChannelCount);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexAacSbrMode);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDrcEncodedTargetLevel);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDrcTargetReferenceLevel);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDrcCompression);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDrcBoostFactor);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDrcAttenuationFactor);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDrcEffectType);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPlatformLevel);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexPlatformFeatures);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexStoreIonUsage);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexAspectsToDataSpace);
        DECLARE_BASE_CASE(C2ParamIndexKind,
                          kParamIndexFlexiblePixelFormatDescriptor);
        DECLARE_BASE_CASE(C2ParamIndexKind,
                          kParamIndexFlexiblePixelFormatDescriptors);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDataSpaceToAspects);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexDataSpace);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSurfaceScaling);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexInputSurfaceEos);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTimedControl);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexStartAt);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSuspendAt);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexResumeAt);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexStopAt);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTimeOffset);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexMinFrameRate);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexTimestampGapAdjustment);
        DECLARE_BASE_CASE(C2ParamIndexKind, kParamIndexSurfaceAllocator);
    default:
        return std::to_string(indexKind);
    }
}
#endif

} // namespace android::hardware::media::c2::V1_0::renesas
