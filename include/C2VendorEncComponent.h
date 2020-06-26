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

#ifndef C2_VENDOR_ENC_COMPONENT_H
#define C2_VENDOR_ENC_COMPONENT_H

#include <img_gralloc1_public.h>

#include "C2VendorBaseComponent.h"

namespace android::hardware::media::c2::V1_0::renesas {

class IntfImplEnc;

class C2VendorEncComponent final : public C2VendorBaseComponent {
public:
    explicit C2VendorEncComponent(const std::shared_ptr<IntfImplEnc>& intfImpl,
                                  const std::shared_ptr<OMXR_Core>& omxrCore);

    ~C2VendorEncComponent() final;

protected:
    bool onStateSet(OMX_STATETYPE omxState) final;
    bool onConfigure(const OMXR_Adapter& omxrAdapter) final;
    c2_status_t onProcessInput(const C2Work& work,
                               OMX_BUFFERHEADERTYPE* const header,
                               bool fromDequeue) final;
    void onOutputDone(const ExtendedBufferData& data) final;

private:
    static bool MapAVCProfileLevel(C2Config::profile_t c2Profile,
                                   C2Config::level_t c2Level,
                                   OMX_VIDEO_AVCPROFILETYPE& omxProfile,
                                   OMX_VIDEO_AVCLEVELTYPE& omxLevel);

    static void SWConvertRGBAToYUVA(const C2GraphicView& view,
                                    int pixelFormat,
                                    OMX_BUFFERHEADERTYPE* const header);

    static void VspmCompleteCallback(unsigned long jobId,
                                     long result,
                                     void* userData);
    void initVspm();
    void deinitVspm();
    c2_status_t vspmCopy(const void* const omxPhysAddr,
                         const IMG_native_handle_t* const handle,
                         OMX_COLOR_FORMATTYPE colorFormat,
                         bool input);
    c2_status_t vspmConvertRGBAToYUVAWithEmpty(
        const void* const omxPhysAddr, const IMG_native_handle_t* const handle);

    void initGralloc();
    void deinitGralloc();

    void setAVCEncoderProfileLevel(const OMXR_Adapter& omxrAdapter,
                                   C2Config::profile_t c2Profile,
                                   C2Config::level_t c2Level) const;

    const std::shared_ptr<IntfImplEnc> mIntfImpl;

    void* mVspmHandle;
    std::mutex mVspmMutex;
    std::condition_variable mVspmCV;
    c2_status_t mVspmResult;

    gralloc1_device_t* mGrallocDevice;
    GRALLOC1_PFN_LOCK mGrallocLockFunc;
    GRALLOC1_PFN_UNLOCK mGrallocUnlockFunc;
};

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // C2_VENDOR_ENC_COMPONENT_H
