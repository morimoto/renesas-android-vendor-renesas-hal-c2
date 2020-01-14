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

#ifndef C2_VENDOR_COMPONENT_INTERFACE_H
#define C2_VENDOR_COMPONENT_INTERFACE_H

#include <C2Config.h>
#include <android-base/macros.h>
#include <util/C2InterfaceHelper.h>

#include "OMX_Video.h"

namespace android::hardware::media::c2::V1_0::renesas {

constexpr const char* const MEDIA_MIMETYPE_VIDEO_RAW = "video/raw";
constexpr const char* const MEDIA_MIMETYPE_VIDEO_H263 = "video/3gpp";
constexpr const char* const MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";
constexpr const char* const MEDIA_MIMETYPE_VIDEO_HEVC = "video/hevc";
constexpr const char* const MEDIA_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
constexpr const char* const MEDIA_MIMETYPE_VIDEO_VP8 = "video/x-vnd.on2.vp8";
constexpr const char* const MEDIA_MIMETYPE_VIDEO_VP9 = "video/x-vnd.on2.vp9";

struct CodecInfo {
    const bool isDecoder;
    const C2String c2Name;
    const C2String omxName;
    const OMX_VIDEO_CODINGTYPE omxCodingType;
    const uint32_t pixelFormat;
    const std::string mediaType;
    const C2PictureSizeStruct minSize;
    const C2PictureSizeStruct maxSize;
    const C2Config::profile_t defaultProfile;
    const C2Config::level_t defaultLevel;
    const std::vector<uint32_t> profiles;
    const std::vector<uint32_t> levels;
};

inline bool operator<(const CodecInfo& lhs, const CodecInfo& rhs) {
    return lhs.c2Name < rhs.c2Name;
}

inline bool operator<(const CodecInfo& info, const C2String& c2Name) {
    return info.c2Name < c2Name;
}

inline bool operator<(const C2String& c2Name, const CodecInfo& info) {
    return c2Name < info.c2Name;
}

class IntfImpl : public C2InterfaceHelper {
public:
    explicit IntfImpl(const std::shared_ptr<C2ReflectorHelper>& reflector,
                      const CodecInfo& info);

    DISALLOW_COPY_AND_ASSIGN(IntfImpl);

    const C2String& getC2Name() const { return mC2Name; }

    const C2String& getOMXName() const { return mOMXName; }

    OMX_VIDEO_CODINGTYPE getOMXCodingType() const { return mOMXCodingType; }

    uint32_t getCodedFrameRate() const {
        return static_cast<uint32_t>(mCodedFrameRate->value);
    }

    uint32_t getBitrate() const { return mBitrate->value; }

    const C2PictureSizeStruct& getDefaultPictureSize() const {
        return mMinSize;
    }

    const C2PictureSizeStruct& getPictureSize() const { return *mSize; }

    uint32_t getPixelFormat() const { return mPixelFormat->value; }

    C2BlockPool::local_id_t getOutputBlockPoolId() const {
        return mOutputBlockPoolIds->m.values[0u];
    }

private:
    static C2R ProfileLevelSetter(bool mayBlock,
                                  C2P<C2StreamProfileLevelInfo>& me);

    static C2R SizeSetter(bool mayBlock,
                          const C2P<C2StreamPictureSizeInfo>& old,
                          C2P<C2StreamPictureSizeInfo>& me);

    const C2String mC2Name;
    const C2String mOMXName;
    const OMX_VIDEO_CODINGTYPE mOMXCodingType;

    const C2PictureSizeStruct mMinSize;

    std::shared_ptr<C2ComponentKindSetting> mKind;
    std::shared_ptr<C2ComponentDomainSetting> mDomain;

    std::shared_ptr<C2PortMediaTypeSetting::input> mInputMediaType;
    std::shared_ptr<C2PortMediaTypeSetting::output> mOutputMediaType;

    std::shared_ptr<C2StreamBufferTypeSetting::input> mInputFormat;
    std::shared_ptr<C2StreamBufferTypeSetting::output> mOutputFormat;

    // CODED port parameters
    std::shared_ptr<C2StreamProfileLevelInfo> mProfileLevel;
    std::shared_ptr<C2StreamFrameRateInfo> mCodedFrameRate;
    std::shared_ptr<C2StreamBitrateInfo> mBitrate;

    // RAW port parameters
    std::shared_ptr<C2StreamPictureSizeInfo> mSize;
    std::shared_ptr<C2StreamPixelFormatInfo> mPixelFormat;

    std::shared_ptr<C2PortBlockPoolsTuning::output> mOutputBlockPoolIds;
};

static_assert(!std::is_copy_constructible<IntfImpl>::value);
static_assert(!std::is_copy_assignable<IntfImpl>::value);

class IntfImplDec final : public IntfImpl {
public:
    explicit IntfImplDec(const std::shared_ptr<C2ReflectorHelper>& reflector,
                         const CodecInfo& info);

    const C2PictureSizeStruct& getMaxPictureSize() const {
        return *mOutputMaxPictureSize;
    }

    const std::shared_ptr<C2StreamColorAspectsInfo::output>
    getColorAspectsParam() const {
        return mColorAspects;
    }

private:
    static C2R MaxPictureSizeSetter(
        bool mayBlock,
        const C2P<C2StreamMaxPictureSizeTuning::output>& old,
        C2P<C2StreamMaxPictureSizeTuning::output>& me);

    static C2R DefaultColorAspectsSetter(
        bool mayBlock, C2P<C2StreamColorAspectsTuning::output>& me);

    static C2R CodedColorAspectsSetter(
        bool mayBlock, C2P<C2StreamColorAspectsInfo::input>& me);

    static C2R ColorAspectsSetter(
        bool mayBlock,
        C2P<C2StreamColorAspectsInfo::output>& me,
        const C2P<C2StreamColorAspectsTuning::output>& def,
        const C2P<C2StreamColorAspectsInfo::input>& coded);

    std::shared_ptr<C2StreamMaxPictureSizeTuning::output> mOutputMaxPictureSize;

    std::shared_ptr<C2PortActualDelayTuning::output> mOutputDelay;

    std::shared_ptr<C2StreamColorAspectsTuning::output> mDefaultColorAspects;
    std::shared_ptr<C2StreamColorAspectsInfo::input> mCodedColorAspects;
    std::shared_ptr<C2StreamColorAspectsInfo::output> mColorAspects;
};

class IntfImplEnc final : public IntfImpl {
public:
    explicit IntfImplEnc(const std::shared_ptr<C2ReflectorHelper>& reflector,
                         const CodecInfo& info)
        : IntfImpl{reflector, info} {}

    // NOTE: forces gralloc to use RGBA pixel format instead of default BGRA
    void useRGBAPixelFormat();

private:
    std::shared_ptr<C2StreamUsageTuning::input> mUsage;
};

class C2VendorComponentInterface final : public C2ComponentInterface {
public:
    explicit C2VendorComponentInterface(const std::shared_ptr<IntfImpl>& impl)
        : mImpl{impl} {}

    DISALLOW_COPY_AND_ASSIGN(C2VendorComponentInterface);

    C2String getName() const final { return mImpl->getC2Name(); }

    c2_node_id_t getId() const final {
        // TODO: find a way to get the unique id
        return 0u;
    }

    c2_status_t query_vb(
        const std::vector<C2Param*>& stackParams,
        const std::vector<C2Param::Index>& heapParamIndices,
        c2_blocking_t mayBlock,
        std::vector<std::unique_ptr<C2Param>>* const heapParams) const final;

    c2_status_t config_vb(
        const std::vector<C2Param*>& params,
        c2_blocking_t mayBlock,
        std::vector<std::unique_ptr<C2SettingResult>>* const failures) final;

    c2_status_t querySupportedParams_nb(
        std::vector<std::shared_ptr<C2ParamDescriptor>>* const params)
        const final;

    c2_status_t querySupportedValues_vb(
        std::vector<C2FieldSupportedValuesQuery>& fields,
        c2_blocking_t mayBlock) const final;

    c2_status_t createTunnel_sm(
        c2_node_id_t targetComponent ATTRIBUTE_UNUSED) final {
        // TODO: implement
        return C2_OMITTED;
    }

    c2_status_t releaseTunnel_sm(
        c2_node_id_t targetComponent ATTRIBUTE_UNUSED) final {
        // TODO: implement
        return C2_OMITTED;
    }

private:
    const std::shared_ptr<IntfImpl> mImpl;
};

static_assert(!std::is_copy_constructible<C2VendorComponentInterface>::value);
static_assert(!std::is_copy_assignable<C2VendorComponentInterface>::value);

constexpr bool operator==(const C2ColorAspectsStruct& lhs,
                          const C2ColorAspectsStruct& rhs) {
    return lhs.range == rhs.range && lhs.primaries == rhs.primaries &&
        lhs.transfer == rhs.transfer && lhs.matrix == rhs.matrix;
}

constexpr bool operator!=(const C2ColorAspectsStruct& lhs,
                          const C2ColorAspectsStruct& rhs) {
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os,
                                const C2ColorAspectsStruct& aspects) {
    return os << aspects.range << ", " << aspects.primaries << ", "
              << aspects.transfer << ", " << aspects.matrix;
}

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // C2_VENDOR_COMPONENT_INTERFACE_H
