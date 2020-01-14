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
    void onExtensionMsg(const Message& msg) final;
    bool onStateSet(OMX_STATETYPE omxState) final;
    bool onConfigure(OMXR_Adapter& omxrAdapter) final;
    c2_status_t onProcessInput(std::unique_ptr<C2Work> work,
                               OMX_BUFFERHEADERTYPE* const header,
                               bool fromDequeuex);
    c2_status_t onInputDone(const BufferData& data) final;
    void onOutputDone(const ExtendedBufferData& data) final;

private:
    struct VspmCallbackData {
        C2VendorEncComponent* const component;
        OMX_BUFFERHEADERTYPE* const header;
    };

    static void SWConvertRGBAToYUVA(const C2GraphicView& view,
                                    int pixelFormat,
                                    OMX_BUFFERHEADERTYPE* const header);

    static void VspmCompleteCallback(unsigned long jobId,
                                     long result,
                                     void* userData);
    void initVspm();
    void deinitVspm();
    c2_status_t vspmConvertRGBAToYUVAWithEmpty(
        const IMG_native_handle_t* const handle,
        OMX_BUFFERHEADERTYPE* const header);
    void onVspmConversionCompleted(bool success, const BufferData& data);

    void initGralloc();
    void deinitGralloc();

    c2_status_t submitInput(const C2GraphicView& view,
                            const IMG_native_handle_t* const handle,
                            OMX_BUFFERHEADERTYPE* const header);
    c2_status_t submitEmptyEOSInput(OMX_BUFFERHEADERTYPE* const header,
                                    uint64_t frameIndex) const;

    static constexpr uint32_t VSPM_CONVERTION_FAILED =
        Message::EXTENSION_MSGS_START;
    static constexpr uint32_t VSPM_CONVERTION_SUCCESS =
        VSPM_CONVERTION_FAILED + 1u;

    static constexpr uint64_t FRAMEINDEX_NONE =
        std::numeric_limits<uint64_t>::max();

    const std::shared_ptr<IntfImplEnc> mIntfImpl;

    void* mVspmHandle;
    size_t mVspmJobCounter;
    uint64_t mPendingEmptyEOSFrameIndex;

    gralloc1_device_t* mGrallocDevice;
    GRALLOC1_PFN_LOCK mGrallocLockFunc;
    GRALLOC1_PFN_UNLOCK mGrallocUnlockFunc;
};

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // C2_VENDOR_ENC_COMPONENT_H
