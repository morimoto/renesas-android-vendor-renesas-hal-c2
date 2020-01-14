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

#ifndef OMXR_CORE_H
#define OMXR_CORE_H

#include <android-base/macros.h>

#include <string>
#include <type_traits>
#include <vector>

#include "OMXR_Extension.h"

namespace android::hardware::media::c2::V1_0::renesas {

class OMXR_Core {
public:
    OMXR_Core();

    DISALLOW_COPY_AND_ASSIGN(OMXR_Core);

    ~OMXR_Core();

    OMX_ERRORTYPE createComponent(OMX_HANDLETYPE* handle,
                                  OMX_STRING name,
                                  OMX_PTR appData,
                                  OMX_CALLBACKTYPE* callbacks) const;

    void destroyComponent(OMX_HANDLETYPE handle) const;

private:
    using OMX_InitFunc = decltype(&OMX_Init);
    using OMX_DeinitFunc = decltype(&OMX_Deinit);
    using OMX_GetHandleFunc = decltype(&OMX_GetHandle);
    using OMX_FreeHandleFunc = decltype(&OMX_FreeHandle);
    using OMX_ComponentNameEnumFunc = decltype(&OMX_ComponentNameEnum);
    using OMXR_SetLogModeFunc = decltype(&OMXR_SetLogMode);

    OMX_ERRORTYPE init();

    // NOTE: should be called before OMX_Init to make an effect
    void updateMCLogMode() const;

    void* mLibHandle;
    OMX_InitFunc mOMX_Init;
    OMX_DeinitFunc mOMX_Deinit;
    OMX_GetHandleFunc mOMX_GetHandle;
    OMX_FreeHandleFunc mOMX_FreeHandle;
    OMX_ComponentNameEnumFunc mOMX_ComponentNameEnum;
    OMXR_SetLogModeFunc mOMXR_SetLogMode;

    std::vector<std::string> mEnumeratedComponents;
};

static_assert(!std::is_copy_constructible<OMXR_Core>::value);
static_assert(!std::is_copy_assignable<OMXR_Core>::value);

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // OMXR_CORE_H
