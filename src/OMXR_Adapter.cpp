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

#define LOG_TAG "OMXR_Adapter"
//#define R_LOG_VERBOSITY

#include "OMXR_Adapter.h"

#include "C2VendorBaseComponent.h"
#include "C2VendorDebug.h"
#include "OMXR_Core.h"

namespace android::hardware::media::c2::V1_0::renesas {

OMX_CALLBACKTYPE OMXR_Adapter::sCallbacks = {&EventHandler, &EmptyBufferDone,
                                             &FillBufferDone};

OMXR_Adapter::OMXR_Adapter(
    OMX_ERRORTYPE& omxError,
    const std::string& name,
    const std::shared_ptr<const OMXR_Core>& omxrCore,
    const std::weak_ptr<const C2VendorBaseComponent>& c2Component)
    : mOMXR_Core{omxrCore},
      mC2_Component{c2Component},
      mOMX_Component{nullptr} {
    omxError = mOMXR_Core->createComponent(reinterpret_cast<OMX_HANDLETYPE*>(
                                               &mOMX_Component),
                                           const_cast<OMX_STRING>(name.c_str()),
                                           this, &sCallbacks);
}

OMXR_Adapter::~OMXR_Adapter() {
    if (mOMX_Component != nullptr) {
        mOMXR_Core->destroyComponent(mOMX_Component);
    }
}

OMX_ERRORTYPE OMXR_Adapter::EventHandler(OMX_HANDLETYPE component
                                             ATTRIBUTE_UNUSED,
                                         OMX_PTR appData,
                                         OMX_EVENTTYPE event,
                                         OMX_U32 data1,
                                         OMX_U32 data2,
                                         OMX_PTR eventData ATTRIBUTE_UNUSED) {
    const std::shared_ptr<const C2VendorBaseComponent> c2Component =
        static_cast<OMXR_Adapter*>(appData)->mC2_Component.lock();

    if (c2Component) {
        c2Component->OnReceivedEvent(event, data1, data2);
    } else {
        R_LOG(WARNING) << "C2 component has expired";
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXR_Adapter::EmptyBufferDone(OMX_HANDLETYPE component
                                                ATTRIBUTE_UNUSED,
                                            OMX_PTR appData,
                                            OMX_BUFFERHEADERTYPE* header) {
    const std::shared_ptr<const C2VendorBaseComponent> c2Component =
        static_cast<OMXR_Adapter*>(appData)->mC2_Component.lock();

    if (c2Component) {
        c2Component->OnEmptyBufferDone(header);
    } else {
        R_LOG(WARNING) << "C2 component has expired";
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXR_Adapter::FillBufferDone(OMX_HANDLETYPE component
                                               ATTRIBUTE_UNUSED,
                                           OMX_PTR appData,
                                           OMX_BUFFERHEADERTYPE* header) {
    const std::shared_ptr<const C2VendorBaseComponent> c2Component =
        static_cast<OMXR_Adapter*>(appData)->mC2_Component.lock();

    if (c2Component) {
        c2Component->OnFillBufferDone(header);
    } else {
        R_LOG(WARNING) << "C2 component has expired";
    }

    return OMX_ErrorNone;
}

} // namespace android::hardware::media::c2::V1_0::renesas
