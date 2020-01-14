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

#ifndef C2_VENDOR_DEC_COMPONENT_H
#define C2_VENDOR_DEC_COMPONENT_H

#include "C2VendorBaseComponent.h"
#include "OMXR_Extension_vdcmn.h"

namespace android::hardware::media::c2::V1_0::renesas {

class IntfImplDec;

class C2VendorDecComponent final : public C2VendorBaseComponent {
public:
    explicit C2VendorDecComponent(const std::shared_ptr<IntfImplDec>& intfImpl,
                                  const std::shared_ptr<OMXR_Core>& omxrCore);

    ~C2VendorDecComponent() final;

protected:
    bool onStateSet(OMX_STATETYPE omxState) final;
    bool onConfigure(OMXR_Adapter& omxrAdapter) final;
    c2_status_t onProcessInput(std::unique_ptr<C2Work> work,
                               OMX_BUFFERHEADERTYPE* const header,
                               bool fromDequeue) final;
    ExtendedBufferData onPreprocessOutput(
        OMXR_Adapter& omxrAdapter,
        OMX_BUFFERHEADERTYPE* const header) const final;
    void onOutputDone(const ExtendedBufferData& data) final;

private:
    struct VUIColorAspects {
        OMX_U32 range;
        OMX_U32 primaries;
        OMX_U32 transfer;
        OMX_U32 matrixCoeffs;
    };

    static OMX_ERRORTYPE ForceMaxDecodeCap(OMXR_Adapter& omxrAdapter,
                                           OMX_U32 maxWidth,
                                           OMX_U32 maxHeight);

    static constexpr void GetAVCVUIColorAspects(
        const OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE& syntaxInfoConfig,
        VUIColorAspects& aspects);
    static constexpr void GetHEVCVUIColorAspects(
        const OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE& syntaxInfoConfig,
        VUIColorAspects& aspects);
    static constexpr void GetMPEG4VUIColorAspects(
        const OMXR_MC_VIDEO_CONFIG_SYNTAX_INFOTYPE& syntaxInfoConfig,
        VUIColorAspects& aspects);
    static constexpr void ConvertVUIToC2ColorAspects(
        const VUIColorAspects& vuiAspects, C2ColorAspectsStruct& aspects);

    bool shouldEnforceMaxDecodeCap(OMX_VIDEO_CODINGTYPE omxCodingType) const;
    void retrieveColorAspects(OMXR_Adapter& omxrAdapter,
                              C2ColorAspectsStruct& aspects) const;
    void verifyColorAspects(const C2ColorAspectsStruct& aspects,
                            C2Buffer& buffer);

    const std::shared_ptr<IntfImplDec> mIntfImpl;

    uint32_t mCropWidth;
    uint32_t mCropHeight;

    uint32_t mBlockWidth;
    uint32_t mBlockHeight;

    OMX_U32 mSyntaxIndex;
    decltype(&GetAVCVUIColorAspects) mGetVUIColorAspectsFunc;
    std::function<void(OMXR_Adapter&, C2ColorAspectsStruct&)>
        mRetrieveColorAspectsFunc;
    std::function<void(const C2ColorAspectsStruct&, C2Buffer&)>
        mVerifyColorAspectsFunc;
    C2ColorAspectsStruct mBitstreamAspects;
};

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // C2_VENDOR_DEC_COMPONENT_H
