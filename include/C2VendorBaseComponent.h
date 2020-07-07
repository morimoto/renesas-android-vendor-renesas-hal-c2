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

#ifndef C2_VENDOR_BASE_COMPONENT_H
#define C2_VENDOR_BASE_COMPONENT_H

#include <C2Config.h>
#include <android-base/macros.h>
#include <img_gralloc1_public.h>

#include <atomic>
#include <condition_variable>
#include <map>
#include <thread>

#include "Mutexed.h"
#include "OMXR_Extension_video.h"

struct vspm_job_t;

namespace android::hardware::media::c2::V1_0::renesas {

class IntfImpl;
class OMXR_Core;
class OMXR_Adapter;

constexpr OMX_COLOR_FORMATTYPE OMXR_COLOR_FormatYV12 =
    static_cast<OMX_COLOR_FORMATTYPE>(OMX_COLOR_FormatYVU420Planar);

class C2VendorBaseComponent
    : public C2Component,
      public std::enable_shared_from_this<C2VendorBaseComponent> {
public:
    explicit C2VendorBaseComponent(
        const std::shared_ptr<IntfImpl>& intfImpl,
        const std::shared_ptr<const OMXR_Core>& omxrCore,
        bool useVspm);

    DISALLOW_COPY_AND_ASSIGN(C2VendorBaseComponent);

    ~C2VendorBaseComponent() override;

    c2_status_t setListener_vb(const std::shared_ptr<Listener>& listener,
                               c2_blocking_t mayBlock) final;

    c2_status_t queue_nb(std::list<std::unique_ptr<C2Work>>* const items) final;

    c2_status_t announce_nb(
        const std::vector<C2WorkOutline>& items ATTRIBUTE_UNUSED) final {
        // TODO: implement
        return C2_OMITTED;
    }

    c2_status_t flush_sm(
        flush_mode_t mode,
        std::list<std::unique_ptr<C2Work>>* const flushedWork) final;

    c2_status_t drain_nb(drain_mode_t mode ATTRIBUTE_UNUSED) final {
        // TODO: implement
        return C2_OMITTED;
    }

    c2_status_t start() final;

    c2_status_t stop() final;

    c2_status_t reset() final;

    c2_status_t release() final;

    std::shared_ptr<C2ComponentInterface> intf() final {
        return mComponentIntf;
    }

    void OnReceivedEvent(OMX_EVENTTYPE event,
                         OMX_U32 data1,
                         OMX_U32 data2) const;
    void OnEmptyBufferDone(OMX_BUFFERHEADERTYPE* header) const;
    void OnFillBufferDone(OMX_BUFFERHEADERTYPE* header) const;

    void forbidStart() { mStartForbidden = true; }

protected:
    enum PortIndex : OMX_U32 {
        InputPortIndex,
        OutputPortIndex,
        PortCount,
        AllPorts = OMX_ALL,
    };

    enum class ADAPTER_STATE : uint32_t {
        UNLOADED,
        LOADED,
        LOADED_TO_IDLE,
        IDLE,
        IDLE_TO_EXECUTING,
        EXECUTING,
        EXECUTING_TO_IDLE,
        IDLE_TO_LOADED,
        FLUSHING,
        INVALID,
    };

    struct ExtendedBufferData {
        OMX_BUFFERHEADERTYPE* const header;
        uint32_t width;
        uint32_t height;
        C2ColorAspectsStruct aspects;
        void* physAddr;

        ExtendedBufferData(OMX_BUFFERHEADERTYPE* const _header)
            : header{_header},
              width{0u},
              height{0u},
              aspects{},
              physAddr{nullptr} {}
    };

    struct BufferData {
        OMX_BUFFERHEADERTYPE* const header;
    };

    struct EventData {
        OMX_EVENTTYPE event;
        OMX_U32 data1;
        OMX_U32 data2;
    };

    class Message {
    public:
        enum : uint32_t {
            INIT,
            START,
            STOP,
            FLUSH,
            ASYNC_MSGS_START,
            DEQUEUE = ASYNC_MSGS_START,
            EVENT,
            INPUT_DONE,
            OUTPUT_DONE,
            EXTENSION_MSGS_START,
        };

        union {
            ExtendedBufferData extendedBufferData;
            BufferData bufferData;
            EventData eventData;
        };

        uint32_t mType;

        explicit Message(uint32_t type) : mType{type} {}
        explicit Message(ExtendedBufferData data)
            : extendedBufferData{std::move(data)}, mType{OUTPUT_DONE} {}
        explicit Message(BufferData data, uint32_t type)
            : bufferData{std::move(data)}, mType{type} {}
        explicit Message(EventData data)
            : eventData{std::move(data)}, mType{EVENT} {}

        static std::string AsString(uint32_t type) {
            switch (type) {
            case INIT:
                return "INIT";
            case START:
                return "START";
            case STOP:
                return "STOP";
            case FLUSH:
                return "FLUSH";
            case DEQUEUE:
                return "DEQUEUE";
            case EVENT:
                return "EVENT";
            case INPUT_DONE:
                return "INPUT_DONE";
            case OUTPUT_DONE:
                return "OUTPUT_DONE";
            default:
                return std::to_string(type);
            }
        }
    };

    static constexpr size_t GetBufferIndex(
        const OMX_BUFFERHEADERTYPE* const header) {
        return GetMeta(header)->mIndex;
    }

    // FrameIndex <-> timestamp conversions
    static constexpr OMX_TICKS FrameIndexToTs(uint64_t frameIndex) {
        return static_cast<OMX_TICKS>(frameIndex);
    }

    static constexpr uint64_t TsToFrameIndex(OMX_TICKS timestamp) {
        return static_cast<uint64_t>(timestamp);
    }

    // Methods used EXCLUSIVELY by client thread
    c2_status_t releaseComponent();

    // Methods used EXCLUSIVELY by component thread
    virtual void onExtensionMsg(const Message& msg);
    virtual bool onStateSet(OMX_STATETYPE omxState);
    virtual bool onConfigure(const OMXR_Adapter& omxrAdapter) = 0;
    virtual c2_status_t onProcessInput(const C2Work& work,
                                       OMX_BUFFERHEADERTYPE* const header,
                                       bool fromDequeue) = 0;

    virtual ExtendedBufferData onPreprocessOutput(
        const OMXR_Adapter& omxrAdapter ATTRIBUTE_UNUSED,
        OMX_BUFFERHEADERTYPE* const header) const {
        return {header};
    }

    virtual void onOutputDone(const ExtendedBufferData& data) = 0;

    template <
        uint32_t type,
        typename... ARGS,
        typename = std::enable_if_t<type >= Message::EXTENSION_MSGS_START>>
    constexpr void postExtensionMsg(ARGS&&... args) const {
        mComponentThread->post<type>(std::forward<ARGS>(args)...);
    }

    OMX_ERRORTYPE setPortFormat(PortIndex portIndex,
                                OMX_U32 frameRate,
                                OMX_VIDEO_CODINGTYPE coding,
                                OMX_COLOR_FORMATTYPE colorFormat) const;

    OMX_ERRORTYPE setPortDef(PortIndex portIndex,
                             OMX_U32 width,
                             OMX_U32 height,
                             OMX_U32 bitrate,
                             OMX_U32 frameRate,
                             OMX_VIDEO_CODINGTYPE coding,
                             OMX_COLOR_FORMATTYPE colorFormat) const;

    void querySupportedProfileLevels(PortIndex portIndex) const;

    OMX_ERRORTYPE emptyBuffer(OMX_BUFFERHEADERTYPE* const header) const;
    OMX_ERRORTYPE fillBuffer(OMX_BUFFERHEADERTYPE* const header);

    void reportWork(std::unique_ptr<C2Work> work, c2_status_t result);
    void reportError(c2_status_t result);
    void reportClonedWork(const C2Work& work);

    c2_status_t vspmQueueAndWaitJob(vspm_job_t* const ipParam);
    c2_status_t vspmCopy(const void* const omxPhysAddr,
                         const IMG_native_handle_t* const handle,
                         OMX_COLOR_FORMATTYPE colorFormat,
                         bool input);

    std::map<uint64_t, std::unique_ptr<C2Work>> mEmptiedWorks;

    std::unique_ptr<C2BlockPool> mOutputBlockPool;

private:
    enum class STATE : uint32_t {
        UNINITIALIZED,
        RUNNING,
        STOPPED,
    };

    class ComponentThread {
    public:
        explicit ComponentThread(C2VendorBaseComponent& parent);

        ~ComponentThread();

        template <
            uint32_t type,
            typename = std::enable_if_t<type == Message::DEQUEUE ||
                                        type >= Message::EXTENSION_MSGS_START>>
        void post() {
            createAndQueueMsg(type);
        }

        template <uint32_t type,
                  typename = std::enable_if_t<type == Message::EVENT>>
        void post(EventData data) {
            createAndQueueMsg(std::move(data));
        }

        template <
            uint32_t type,
            typename = std::enable_if_t<type == Message::INPUT_DONE ||
                                        type >= Message::EXTENSION_MSGS_START>>
        void post(BufferData data) {
            createAndQueueMsg(std::move(data), type);
        }

        template <uint32_t type,
                  typename = std::enable_if_t<type == Message::OUTPUT_DONE>>
        void post(ExtendedBufferData data) {
            createAndQueueMsg(std::move(data));
        }

        template <uint32_t type>
        c2_status_t postAndWait();

        void reply(c2_status_t result);

    private:
        template <typename... ARGS>
        void createAndQueueMsg(ARGS&&... args) {
            auto msg = std::make_unique<Message>(std::forward<ARGS>(args)...);

            std::lock_guard lock{mMsgMutex};
            mMsgQueue.push_back(std::move(msg));
            mMsgCV.notify_one();
        }

        void threadLoop();

        C2VendorBaseComponent& mParent;

        std::list<std::unique_ptr<Message>> mMsgQueue;
        bool mExitThread;
        std::mutex mMsgMutex;
        std::condition_variable mMsgCV;

        c2_status_t mReplyResult;
        std::mutex mReplyMutex;
        std::condition_variable mReplyCV;
        std::atomic_bool mTransitionInAction;

        std::thread mThread;
    };

    struct BufferMeta {
        const size_t mIndex;
    };

    static constexpr const char* AsStateString(ADAPTER_STATE state) {
        switch (state) {
        case ADAPTER_STATE::UNLOADED:
            return "UNLOADED";
        case ADAPTER_STATE::LOADED:
            return "LOADED";
        case ADAPTER_STATE::IDLE:
            return "IDLE";
        case ADAPTER_STATE::IDLE_TO_EXECUTING:
            return "IDLE_TO_EXECUTING";
        case ADAPTER_STATE::EXECUTING:
            return "EXECUTING";
        case ADAPTER_STATE::EXECUTING_TO_IDLE:
            return "EXECUTING_TO_IDLE";
        case ADAPTER_STATE::IDLE_TO_LOADED:
            return "IDLE_TO_LOADED";
        case ADAPTER_STATE::FLUSHING:
            return "FLUSHING";
        case ADAPTER_STATE::INVALID:
            return "INVALID";
        default:
            return "?";
        }
    }

    static constexpr const char* AsPortString(OMX_U32 portIndex) {
        switch (portIndex) {
        case InputPortIndex:
            return "input";
        case OutputPortIndex:
            return "output";
        case AllPorts:
            return "all";
        default:
            return "?";
        }
    }

    template <bool printOnError = true>
    bool checkState(ADAPTER_STATE expected) const;

    static constexpr BufferMeta* GetMeta(
        const OMX_BUFFERHEADERTYPE* const header) {
        return static_cast<BufferMeta*>(header->pAppPrivate);
    }

    static auto MakeList(std::unique_ptr<C2Work> work) {
        std::list<std::unique_ptr<C2Work>> res;
        res.push_back(std::move(work));

        return res;
    }

    static void VspmCompleteCallback(unsigned long jobId,
                                     long result,
                                     void* userData);

    void handleMsg(const Message& msg);
    c2_status_t handleInit();
    c2_status_t handleStart();
    c2_status_t handleStop();
    c2_status_t handleFlush();
    bool handleDequeue();
    void handleEvent(const EventData& data);
    void handleInputDone(const BufferData& data);
    void handleOutputDone(const ExtendedBufferData& data);

    void onOutputSettingsChanged();
    void onOutputPortDisabled();
    void onOutputPortEnabled();

    bool sendOMXCommand(OMX_COMMANDTYPE cmd, OMX_U32 param) const;

    bool allocatePortBuffers(PortIndex portIndex);
    void freePortBuffers(PortIndex portIndex);

    void postFillBuffers();

    c2_status_t createOutputPoolIfNeeded();

    void processConfigUpdate(
        std::vector<std::unique_ptr<C2Param>>& configUpdate);

    void pushPendingWork(uint64_t frameIndex, std::unique_ptr<C2Work> work);
    std::unique_ptr<C2Work> popPendingWork(uint64_t frameIndex);

    void initVspm();
    void deinitVspm();

    friend inline std::ostream& operator<<(std::ostream& os,
                                           ADAPTER_STATE state);

    bool mStartForbidden;

    const std::shared_ptr<C2ComponentInterface> mComponentIntf;

    std::atomic<STATE> mState;
    Mutexed<std::shared_ptr<Listener>> mListener;
    const std::unique_ptr<ComponentThread> mComponentThread;

    const std::string mOMXName;
    const std::shared_ptr<const OMXR_Core> mOMXR_Core;
    ADAPTER_STATE mAdapterState;
    std::unique_ptr<const OMXR_Adapter> mOMXR_Adapter;

    std::vector<OMX_BUFFERHEADERTYPE*> mHeaders[PortCount];
    std::list<size_t> mAvailableInputIndexes;
    std::vector<bool> mOwnedOutputIndexes;

    Mutexed<std::list<std::unique_ptr<C2Work>>> mInputWorkQueue;
    std::map<uint64_t, std::unique_ptr<C2Work>> mPendingWorks;

    bool mFlushInProgress[PortCount];
    bool mSettingsChanged;
    bool mSeenOutputEOS;

    void* mVspmHandle;
    std::mutex mVspmMutex;
    std::condition_variable mVspmCV;
    c2_status_t mVspmResult;
};

static_assert(!std::is_copy_constructible<C2VendorBaseComponent>::value);
static_assert(!std::is_copy_assignable<C2VendorBaseComponent>::value);

// Ceil value / 64 * 64. Only unsigned integral types
template <typename T,
          typename = std::enable_if_t<std::is_integral<T>::value &&
                                      !std::is_signed<T>::value>>
constexpr inline T Align64(const T& value) {
    return (value + 63u) & ~(63u);
}

void CopyBuffer(uint8_t* const dst, const uint8_t* const src, size_t size);

template <bool isEncoder>
void DumpBuffer(const char* const data,
                ptrdiff_t size,
                const std::string& filename);

template <bool isEncoder>
void DumpHeader(const OMX_BUFFERHEADERTYPE* const header,
                const std::string& filename);

template <bool isEncoder>
void DumpView(const C2GraphicView& view, const std::string& filename);

} // namespace android::hardware::media::c2::V1_0::renesas

#endif // C2_VENDOR_BASE_COMPONENT_H
