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

#define LOG_TAG "C2VendorComponentStore"
//#define R_LOG_VERBOSITY

#include "C2VendorComponentStore.h"

#include "C2VendorDebug.h"
#include "C2VendorDecComponent.h"
#include "C2VendorEncComponent.h"
#include "OMXR_Core.h"
#include "OMXR_Debug.h"
#include "OMXR_Extension_android.h"

namespace android::hardware::media::c2::V1_0::renesas {

constexpr android_pixel_format_t VENDOR_PIXEL_FORMAT = HAL_PIXEL_FORMAT_YV12;

// NOTE: should be consistent with "size" entry in media_codecs.xml
constexpr uint32_t MAX_PICTURE_WIDTH = 1920u;
constexpr uint32_t MAX_PICTURE_HEIGHT = 1088u;

// NOTE: should be consistent with "concurrent-instances" entry in
// media_codecs.xml
constexpr unsigned MAX_CONCURRENT_INSTANCES = 2u;

C2VendorComponentStore::C2VendorComponentStore()
    : mReflector{std::make_shared<C2ReflectorHelper>()},
      mOMXR_Core{std::make_shared<OMXR_Core>()},
      mInfos{
          {
              true,
              "c2.renesas.h263.decoder",
              "OMX.RENESAS.VIDEO.DECODER.H263",
              OMX_VIDEO_CodingH263,
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_H263,
              {128u, 96u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_H263_BASELINE,
              LEVEL_H263_40,
              {
                  PROFILE_H263_BASELINE,
              },
              {
                  LEVEL_H263_40,
              },
          },
          {
              true,
              "c2.renesas.avc.decoder",
              "OMX.RENESAS.VIDEO.DECODER.H264",
              OMX_VIDEO_CodingAVC,
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_AVC,
              {80u, 80u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_AVC_BASELINE,
              LEVEL_AVC_5_1,
              {
                  PROFILE_AVC_BASELINE,
                  PROFILE_AVC_CONSTRAINED_BASELINE,
                  PROFILE_AVC_MAIN,
                  PROFILE_AVC_HIGH,
                  PROFILE_AVC_CONSTRAINED_HIGH,
              },
              {
                  LEVEL_AVC_1,
                  LEVEL_AVC_1B,
                  LEVEL_AVC_1_1,
                  LEVEL_AVC_1_2,
                  LEVEL_AVC_1_3,
                  LEVEL_AVC_2,
                  LEVEL_AVC_2_1,
                  LEVEL_AVC_2_2,
                  LEVEL_AVC_3,
                  LEVEL_AVC_3_1,
                  LEVEL_AVC_3_2,
                  LEVEL_AVC_4,
                  LEVEL_AVC_4_1,
                  LEVEL_AVC_4_2,
                  LEVEL_AVC_5,
                  LEVEL_AVC_5_1,
              },
          },
          {
              true,
              "c2.renesas.hevc.decoder",
              "OMX.RENESAS.VIDEO.DECODER.H265",
              OMX_VIDEO_CodingHEVC,
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_HEVC,
              {80u, 80u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_HEVC_MAIN,
              LEVEL_HEVC_MAIN_5,
              {
                  PROFILE_HEVC_MAIN,
              },
              {
                  LEVEL_HEVC_MAIN_1,
                  LEVEL_HEVC_MAIN_2,
                  LEVEL_HEVC_MAIN_2_1,
                  LEVEL_HEVC_MAIN_3,
                  LEVEL_HEVC_MAIN_3_1,
                  LEVEL_HEVC_MAIN_4,
                  LEVEL_HEVC_MAIN_4_1,
                  LEVEL_HEVC_MAIN_5,
              },
          },
          {
              true,
              "c2.renesas.mpeg4.decoder",
              "OMX.RENESAS.VIDEO.DECODER.MPEG4",
              OMX_VIDEO_CodingMPEG4,
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_MPEG4,
              {80u, 80u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_MP4V_MAIN,
              LEVEL_MP4V_4,
              {
                  PROFILE_MP4V_MAIN,
              },
              {
                  LEVEL_MP4V_4,
              },
          },
#ifdef HAL_C2_VENDOR_ENABLE_VIDEO_VP8
          {
              true,
              "c2.renesas.vp8.decoder",
              "OMX.RENESAS.VIDEO.DECODER.VP8",
              OMX_VIDEO_CodingVP8,
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_VP8,
              {80u, 80u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_VP9_0,
              LEVEL_VP9_4_1,
              {
                  PROFILE_VP9_0,
              },
              {
                  LEVEL_VP9_4_1,
              },
          },
#endif
#ifdef HAL_C2_VENDOR_ENABLE_VIDEO_VP9
          {
              true,
              "c2.renesas.vp9.decoder",
              "OMX.RENESAS.VIDEO.DECODER.VP9",
              OMX_VIDEO_CodingVP9,
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_VP9,
              {192u, 208u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_VP9_0,
              LEVEL_VP9_4_1,
              {
                  PROFILE_VP9_0,
              },
              {
                  LEVEL_VP9_4_1,
              },
          },
#endif
          {
              false,
              "c2.renesas.avc.encoder",
              "OMX.RENESAS.VIDEO.ENCODER.H264",
              OMX_VIDEO_CodingAVC,
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_AVC,
              {80u, 80u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_AVC_BASELINE,
              LEVEL_AVC_5_1,
              {
                  PROFILE_AVC_BASELINE,
                  PROFILE_AVC_MAIN,
                  PROFILE_AVC_HIGH,
              },
              {
                  LEVEL_AVC_1,
                  LEVEL_AVC_1B,
                  LEVEL_AVC_1_1,
                  LEVEL_AVC_1_2,
                  LEVEL_AVC_1_3,
                  LEVEL_AVC_2,
                  LEVEL_AVC_2_1,
                  LEVEL_AVC_2_2,
                  LEVEL_AVC_3,
                  LEVEL_AVC_3_1,
                  LEVEL_AVC_3_2,
                  LEVEL_AVC_4,
                  LEVEL_AVC_4_1,
                  LEVEL_AVC_4_2,
                  LEVEL_AVC_5,
                  LEVEL_AVC_5_1,
              },
          },
#ifdef HAL_C2_VENDOR_ENABLE_VIDEO_VP8
          {
              false,
              "c2.renesas.vp8.encoder",
              "OMX.RENESAS.VIDEO.ENCODER.VP8",
              static_cast<OMX_VIDEO_CODINGTYPE>(OMXR_VIDEO_CodingVP8),
              VENDOR_PIXEL_FORMAT,
              MEDIA_MIMETYPE_VIDEO_VP8,
              {80u, 80u},
              {MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT},
              PROFILE_VP9_0,
              LEVEL_VP9_4_1,
              {
                  PROFILE_VP9_0,
              },
              {
                  LEVEL_VP9_4_1,
              },
          },
#endif
      } {
    mTraitsList.reserve(mInfos.size());

#ifdef R_LOG_VERBOSITY
    R_LOG(DEBUG) << "----------Supported components----------";
#endif

    for (const CodecInfo& info : mInfos) {
#ifdef R_LOG_VERBOSITY
        DumpInfo(info);

        R_LOG(DEBUG) << "----------------------------------------";
#endif

        constexpr C2Component::domain_t domain = C2Component::DOMAIN_VIDEO;
        constexpr C2Component::rank_t COMPONENT_VIDEO_RANK = 512u;
        const auto traits = std::make_shared<C2Component::Traits>();

        traits->name = info.c2Name;
        traits->domain = domain;
        traits->kind = info.isDecoder ? KIND_DECODER : KIND_ENCODER;
        traits->rank = COMPONENT_VIDEO_RANK;
        traits->mediaType = info.mediaType;

        mTraitsList.emplace_back(traits);
        mComponentCounters.emplace(info.c2Name, 0u);
    }
}

c2_status_t C2VendorComponentStore::createComponent(
    C2String name, std::shared_ptr<C2Component>* const component) {
    const auto infoIt = mInfos.find(name);

    if (infoIt == mInfos.cend()) {
        return C2_NOT_FOUND;
    }

    C2VendorBaseComponent* localComponent = nullptr;

    if (infoIt->isDecoder) {
        const auto impl = std::make_shared<IntfImplDec>(mReflector, *infoIt);

        localComponent = new C2VendorDecComponent{impl, mOMXR_Core};
    } else {
        const auto impl = std::make_shared<IntfImplEnc>(mReflector, *infoIt);

        localComponent = new C2VendorEncComponent{impl, mOMXR_Core};
    }

    const auto counterIt = mComponentCounters.find(name);

    CHECK(counterIt != mComponentCounters.end());

    std::atomic_uint& counter = counterIt->second;

    if (counter < MAX_CONCURRENT_INSTANCES) {
        counter++;

        component->reset(localComponent,
                         [&counter](C2VendorBaseComponent* component) {
                             --counter;
                             delete component;
                         });
    } else {
        R_LOG(ERROR) << name << " has reached instance limit " << counter;

        localComponent->forbidStart();

        component->reset(localComponent);
    }

    return C2_OK;
}

c2_status_t C2VendorComponentStore::createInterface(
    C2String name, std::shared_ptr<C2ComponentInterface>* const interface) {
    const auto infoIt = mInfos.find(name);

    if (infoIt == mInfos.cend()) {
        return C2_NOT_FOUND;
    }

    std::shared_ptr<IntfImpl> impl;

    if (infoIt->isDecoder) {
        impl = std::make_shared<IntfImplDec>(mReflector, *infoIt);
    } else {
        impl = std::make_shared<IntfImplEnc>(mReflector, *infoIt);
    }

    *interface = std::make_shared<C2VendorComponentInterface>(impl);

    return C2_OK;
}

void C2VendorComponentStore::DumpInfo(const CodecInfo& info) {
    constexpr const char* const indent = "    ";

    R_LOG(DEBUG) << indent << "isDecoder:      " << std::boolalpha
                 << info.isDecoder;
    R_LOG(DEBUG) << indent << "c2Name:         " << info.c2Name;
    R_LOG(DEBUG) << indent << "omxName:        " << info.omxName;
    R_LOG(DEBUG) << indent << "omxCodingType:  " << info.omxCodingType;
    R_LOG(DEBUG) << indent << "pixelFormat:    " << info.pixelFormat;
    R_LOG(DEBUG) << indent << "mediaType:      " << info.mediaType;
    R_LOG(DEBUG) << indent << "minSize:        " << info.minSize.width << "x"
                 << info.minSize.height;
    R_LOG(DEBUG) << indent << "maxSize:        " << info.maxSize.width << "x"
                 << info.maxSize.height;
    R_LOG(DEBUG) << indent << "defaultProfile: " << info.defaultProfile;
    R_LOG(DEBUG) << indent << "defaultLevel:   " << info.defaultLevel;

    std::string profilesStr;

    for (const uint32_t profile : info.profiles) {
        profilesStr += std::to_string(profile) + "|";
    }

    R_LOG(DEBUG) << indent << "profiles:       " << profilesStr;

    std::string levelsStr;

    for (const uint32_t level : info.levels) {
        levelsStr += std::to_string(level) + "|";
    }

    R_LOG(DEBUG) << indent << "levels:         " << levelsStr;
}

} // namespace android::hardware::media::c2::V1_0::renesas
