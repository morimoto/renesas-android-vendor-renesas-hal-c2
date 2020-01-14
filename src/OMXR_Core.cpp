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

#define LOG_TAG "OMXR_Core"
//#define R_LOG_VERBOSITY

#include "OMXR_Core.h"

#include <android-base/properties.h>
#include <dlfcn.h>

#include <algorithm>

#include "C2VendorDebug.h"

namespace android::hardware::media::c2::V1_0::renesas {

constexpr const char* const OMXR_CORE_LIBNAME = "libomxr_core.so";

constexpr const char* const OMXR_MC_DEBUG_PROPERTY_NAME = "debug.omxr.mc";
constexpr uint32_t OMXR_MC_DEBUG_PROPERTY_DEFAULT = 0x22222200u;

template <typename T>
constexpr void LoadSymbol(void* const handle, const char* const name, T& res) {
    res = reinterpret_cast<T>(dlsym(handle, name));

    if (res == nullptr) {
        R_LOG(ERROR) << "Failed to load symbol name \"" << name << "\", "
                     << dlerror();
    }
}

OMXR_Core::OMXR_Core()
    : mLibHandle{nullptr},
      mOMX_Init{nullptr},
      mOMX_Deinit{nullptr},
      mOMX_GetHandle{nullptr},
      mOMX_FreeHandle{nullptr},
      mOMX_ComponentNameEnum{nullptr},
      mOMXR_SetLogMode{nullptr} {
    CHECK_EQ(init(), OMX_ErrorNone);
}

OMXR_Core::~OMXR_Core() {
    if (mOMX_Deinit != nullptr) {
        CHECK_EQ(mOMX_Deinit(), OMX_ErrorNone);
    }

    if (mLibHandle != nullptr) {
        CHECK_EQ(dlclose(mLibHandle), 0);
    }
}

OMX_ERRORTYPE OMXR_Core::createComponent(OMX_HANDLETYPE* handle,
                                         OMX_STRING name,
                                         OMX_PTR appData,
                                         OMX_CALLBACKTYPE* callbacks) const {
    if (!std::binary_search(mEnumeratedComponents.cbegin(),
                            mEnumeratedComponents.cend(), name)) {
        R_LOG(ERROR) << "Unsupported OMX component " << name;
        return OMX_ErrorInvalidComponentName;
    }

    const OMX_ERRORTYPE omxError =
        mOMX_GetHandle(handle, name, appData, callbacks);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "OMX_GetHandle for " << name << " failed, " << omxError;
    }

    return omxError;
}

void OMXR_Core::destroyComponent(OMX_HANDLETYPE handle) const {
    const OMX_ERRORTYPE omxError = mOMX_FreeHandle(handle);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "OMX_FreeHandle failed, " << omxError;
    }
}

OMX_ERRORTYPE OMXR_Core::init() {
    mLibHandle = dlopen(OMXR_CORE_LIBNAME, RTLD_NOW);

    if (mLibHandle == nullptr) {
        R_LOG(ERROR) << "Failed to open " << OMXR_CORE_LIBNAME << ", "
                     << dlerror();
        return OMX_ErrorUndefined;
    }

    LoadSymbol(mLibHandle, "OMX_Init", mOMX_Init);
    LoadSymbol(mLibHandle, "OMX_Deinit", mOMX_Deinit);
    LoadSymbol(mLibHandle, "OMX_GetHandle", mOMX_GetHandle);
    LoadSymbol(mLibHandle, "OMX_FreeHandle", mOMX_FreeHandle);
    LoadSymbol(mLibHandle, "OMX_ComponentNameEnum", mOMX_ComponentNameEnum);
    LoadSymbol(mLibHandle, "OMXR_SetLogMode", mOMXR_SetLogMode);

    if (mOMX_Init == nullptr || mOMX_Deinit == nullptr ||
        mOMX_GetHandle == nullptr || mOMX_FreeHandle == nullptr ||
        mOMX_ComponentNameEnum == nullptr || mOMXR_SetLogMode == nullptr) {
        return OMX_ErrorUndefined;
    }

    updateMCLogMode();

    const OMX_ERRORTYPE omxError = mOMX_Init();

    if (omxError != OMX_ErrorNone) {
        mOMX_Deinit = nullptr;

        R_LOG(ERROR) << "OMX_Init failed, " << omxError;
        return omxError;
    }

    constexpr size_t PredictedComponentNum = 10u;
    mEnumeratedComponents.reserve(PredictedComponentNum);

    char name[OMX_MAX_STRINGNAME_SIZE];

    R_LOG(DEBUG) << OMXR_CORE_LIBNAME << " supports:";

    for (OMX_U32 ix = 0u;
         mOMX_ComponentNameEnum(name, sizeof(name), ix) == OMX_ErrorNone;
         ix++) {
        R_LOG(DEBUG) << "    " << name;

        mEnumeratedComponents.emplace_back(name);
    }

    std::sort(mEnumeratedComponents.begin(), mEnumeratedComponents.end());

    return OMX_ErrorNone;
}

void OMXR_Core::updateMCLogMode() const {
    const uint32_t mcLogMode =
        android::base::GetUintProperty(OMXR_MC_DEBUG_PROPERTY_NAME,
                                       OMXR_MC_DEBUG_PROPERTY_DEFAULT);

    R_LOG(DEBUG) << "OMXR MC log mode " << std::hex << mcLogMode << std::dec;

    const OMX_ERRORTYPE omxError = mOMXR_SetLogMode(mcLogMode);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to update MC log mode to " << mcLogMode << ", "
                     << omxError;
    }
}

} // namespace android::hardware::media::c2::V1_0::renesas
