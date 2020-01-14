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

#ifndef MUTEXED_H
#define MUTEXED_H

#include <mutex>

namespace android::hardware::media::c2::V1_0::renesas {

template <typename T>
class Mutexed {
private:
    class Locked {
    public:
        Locked(T& data, std::mutex& mutex) : mData{data}, mLock{mutex} {}

        T& operator*() const { return mData; }

        T* operator->() const { return &mData; }

    private:
        T& mData;
        const std::lock_guard<std::mutex> mLock;
    };

    static_assert(!std::is_copy_constructible<Locked>::value);
    static_assert(!std::is_copy_assignable<Locked>::value);

    std::mutex mMutex;
    T mData;

public:
    template <typename... ARGS>
    explicit Mutexed(ARGS&&... args) : mData{std::forward<ARGS>(args)...} {}

    Locked lock() { return {mData, mMutex}; }
};

static_assert(!std::is_copy_constructible<Mutexed<int>>::value);
static_assert(!std::is_copy_assignable<Mutexed<int>>::value);

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // MUTEXED_H
