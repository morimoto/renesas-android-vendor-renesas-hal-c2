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

#ifndef C2_VENDOR_COMPONENT_STORE_H
#define C2_VENDOR_COMPONENT_STORE_H

#include <android-base/macros.h>

#include <atomic>
#include <set>

#include "C2VendorComponentInferface.h"

class C2ReflectorHelper;

namespace android::hardware::media::c2::V1_0::renesas {

class OMXR_Core;

class C2VendorComponentStore final : public C2ComponentStore {
public:
    C2VendorComponentStore();

    DISALLOW_COPY_AND_ASSIGN(C2VendorComponentStore);

    C2String getName() const final { return "vendor.componentStore.renesas"; }

    c2_status_t createComponent(
        C2String name, std::shared_ptr<C2Component>* const component) final;

    c2_status_t createInterface(
        C2String name,
        std::shared_ptr<C2ComponentInterface>* const interface) final;

    std::vector<std::shared_ptr<const C2Component::Traits>> listComponents()
        final {
        return mTraitsList;
    }

    c2_status_t copyBuffer(
        std::shared_ptr<C2GraphicBuffer> src ATTRIBUTE_UNUSED,
        std::shared_ptr<C2GraphicBuffer> dst ATTRIBUTE_UNUSED) final {
        // TODO: implement
        return C2_OMITTED;
    }

    c2_status_t query_sm(const std::vector<C2Param*>& stackParams,
                         const std::vector<C2Param::Index>& heapParamIndices,
                         std::vector<std::unique_ptr<C2Param>>* const heapParams
                             ATTRIBUTE_UNUSED) const final {
        return stackParams.empty() && heapParamIndices.empty() ? C2_OK
                                                               : C2_BAD_INDEX;
    }

    c2_status_t config_sm(const std::vector<C2Param*>& params,
                          std::vector<std::unique_ptr<C2SettingResult>>* const
                              failures ATTRIBUTE_UNUSED) final {
        return params.empty() ? C2_OK : C2_BAD_INDEX;
    }

    std::shared_ptr<C2ParamReflector> getParamReflector() const final {
        return mReflector;
    }

    c2_status_t querySupportedParams_nb(
        std::vector<std::shared_ptr<C2ParamDescriptor>>* const params
            ATTRIBUTE_UNUSED) const final {
        return C2_OK;
    }

    c2_status_t querySupportedValues_sm(
        std::vector<C2FieldSupportedValuesQuery>& fields) const final {
        return fields.empty() ? C2_OK : C2_BAD_INDEX;
    }

private:
    static void DumpInfo(const CodecInfo& info);

    const std::shared_ptr<C2ReflectorHelper> mReflector;
    const std::shared_ptr<OMXR_Core> mOMXR_Core;
    const std::set<CodecInfo, std::less<>> mInfos;
    std::map<C2String, std::atomic_uint> mComponentCounters;
    std::vector<std::shared_ptr<const C2Component::Traits>> mTraitsList;
};

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // C2_VENDOR_COMPONENT_STORE_H
