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

#ifndef OMXR_ADAPTER_H
#define OMXR_ADAPTER_H

#include <android-base/macros.h>

#include <memory>
#include <string>
#include <type_traits>

#include "OMX_Component.h"

namespace android::hardware::media::c2::V1_0::renesas {

class OMXR_Core;
class C2VendorBaseComponent;

class OMXR_Adapter {
public:
    explicit OMXR_Adapter(
        OMX_ERRORTYPE& omxError,
        const std::string& name,
        const std::shared_ptr<const OMXR_Core>& omxrCore,
        const std::weak_ptr<const C2VendorBaseComponent>& c2Component);

    DISALLOW_COPY_AND_ASSIGN(OMXR_Adapter);

    ~OMXR_Adapter();

    template <typename T>
    static constexpr void InitOMXParam(T& param) {
        param.nSize = sizeof(T);
        param.nVersion.nVersion = static_cast<OMX_U32>(OMX_VERSION);
    }

    OMX_ERRORTYPE sendCommand(OMX_COMMANDTYPE cmd, OMX_U32 param) const {
        return OMX_SendCommand(mOMX_Component, cmd, param, nullptr);
    }

    OMX_ERRORTYPE getExtensionIndex(const std::string& name,
                                    OMX_INDEXTYPE& index) const {
        return OMX_GetExtensionIndex(mOMX_Component,
                                     const_cast<OMX_STRING>(name.c_str()),
                                     &index);
    }

    template <typename T>
    OMX_ERRORTYPE getParam(OMX_INDEXTYPE index, T& param) const {
        return OMX_GetParameter(mOMX_Component, index, &param);
    }

    template <typename T>
    OMX_ERRORTYPE setParam(OMX_INDEXTYPE index, T& param) const {
        return OMX_SetParameter(mOMX_Component, index, &param);
    }

    template <typename T>
    OMX_ERRORTYPE getPortParam(OMX_U32 portIndex,
                               OMX_INDEXTYPE index,
                               T& param) const {
        InitOMXParam(param);
        param.nPortIndex = portIndex;

        return getParam(index, param);
    }

    template <typename T>
    OMX_ERRORTYPE setPortParam(OMX_U32 portIndex,
                               OMX_INDEXTYPE index,
                               T& param) const {
        InitOMXParam(param);
        param.nPortIndex = portIndex;

        return setParam(index, param);
    }

    template <typename T>
    OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, T& config) const {
        return OMX_GetConfig(mOMX_Component, index, &config);
    }

    template <typename T>
    OMX_ERRORTYPE setConfig(OMX_INDEXTYPE index, T& config) const {
        return OMX_SetConfig(mOMX_Component, index, &config);
    }

    OMX_ERRORTYPE useBuffer(OMX_BUFFERHEADERTYPE** header,
                            OMX_U32 portIndex,
                            OMX_PTR appPrivate,
                            OMX_U32 size,
                            OMX_U8* buf) const {
        return OMX_UseBuffer(mOMX_Component, header, portIndex, appPrivate,
                             size, buf);
    }

    OMX_ERRORTYPE allocateBuffer(OMX_BUFFERHEADERTYPE** header,
                                 OMX_U32 portIndex,
                                 OMX_PTR appPrivate,
                                 OMX_U32 size) const {
        return OMX_AllocateBuffer(mOMX_Component, header, portIndex, appPrivate,
                                  size);
    }

    OMX_ERRORTYPE freeBuffer(OMX_U32 portIndex,
                             OMX_BUFFERHEADERTYPE* header) const {
        return OMX_FreeBuffer(mOMX_Component, portIndex, header);
    }

    OMX_ERRORTYPE emptyBuffer(OMX_BUFFERHEADERTYPE* header) const {
        return OMX_EmptyThisBuffer(mOMX_Component, header);
    }

    OMX_ERRORTYPE fillBuffer(OMX_BUFFERHEADERTYPE* header) const {
        return OMX_FillThisBuffer(mOMX_Component, header);
    }

    OMX_ERRORTYPE getState(OMX_STATETYPE& state) const {
        return OMX_GetState(mOMX_Component, &state);
    }

private:
    static OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE component,
                                      OMX_PTR appData,
                                      OMX_EVENTTYPE event,
                                      OMX_U32 data1,
                                      OMX_U32 data2,
                                      OMX_PTR eventData);
    static OMX_ERRORTYPE EmptyBufferDone(OMX_HANDLETYPE component,
                                         OMX_PTR appData,
                                         OMX_BUFFERHEADERTYPE* header);
    static OMX_ERRORTYPE FillBufferDone(OMX_HANDLETYPE component,
                                        OMX_PTR appData,
                                        OMX_BUFFERHEADERTYPE* header);

    static OMX_CALLBACKTYPE sCallbacks;

    const std::shared_ptr<const OMXR_Core> mOMXR_Core;
    std::weak_ptr<const C2VendorBaseComponent> mC2_Component;
    OMX_COMPONENTTYPE* mOMX_Component;
};

static_assert(!std::is_copy_constructible<OMXR_Adapter>::value);
static_assert(!std::is_copy_assignable<OMXR_Adapter>::value);

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // OMXR_ADAPTER_H
