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

#define LOG_TAG "C2VendorService"
//#define R_LOG_VERBOSITY

#include <codec2/hidl/1.0/ComponentStore.h>
#include <hidl/HidlTransportSupport.h>

#include "C2VendorComponentStore.h"
#include "C2VendorDebug.h"

int main(int argc ATTRIBUTE_UNUSED, char** argv ATTRIBUTE_UNUSED) {
    using android::hardware::media::c2::V1_0::IComponentStore;
    using android::hardware::media::c2::V1_0::renesas::C2VendorComponentStore;
    using android::hardware::media::c2::V1_0::utils::ComponentStore;

    // NOTE: should be consistent with vintf_fragments HAL's instance
    constexpr const char* const INSTANCE_NAME = "vendor";
    constexpr size_t MAX_THREADS = 8u;
    constexpr bool CALLER_WILL_JOIN = true;

    SetMinimumLogSeverity(android::base::DEBUG);

    R_LOG(DEBUG) << LOG_TAG " is starting";

    android::hardware::configureRpcThreadpool(MAX_THREADS, CALLER_WILL_JOIN);

    const android::sp<IComponentStore> store =
        new ComponentStore{std::make_shared<C2VendorComponentStore>()};

    CHECK_EQ(store->registerAsService(INSTANCE_NAME), android::OK)
        << "Cannot register " LOG_TAG;

    R_LOG(DEBUG) << "Registered " LOG_TAG;

    android::hardware::joinRpcThreadpool();

    return EXIT_FAILURE;
}
