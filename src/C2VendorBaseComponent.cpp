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

#define LOG_TAG "C2VendorBaseComponent"
//#define R_LOG_VERBOSITY
//#define TRACK_MSG_TIMINGS

#include "C2VendorBaseComponent.h"

#include <C2PlatformSupport.h>

#include <fstream>

#include "C2VendorComponentInferface.h"
#include "C2VendorDebug.h"
#include "OMXR_Adapter.h"
#include "OMXR_Debug.h"

namespace android::hardware::media::c2::V1_0::renesas {

#ifdef TRACK_MSG_TIMINGS
class Timer {
public:
    explicit Timer(const std::string& msg) : mStartPoint{Now()}, mMsg{msg} {}

    ~Timer() {
        R_LOG(DEBUG) << mMsg << " duration "
                     << std::chrono::duration_cast<std::chrono::milliseconds>(
                            Now() - mStartPoint)
                            .count();
    }

private:
    using Clock = std::chrono::high_resolution_clock;

    const std::chrono::time_point<Clock> mStartPoint;
    const std::string mMsg;

    static Clock::time_point Now() { return Clock::now(); }
};
#endif

class BlockingBlockPool final : public C2BlockPool {
public:
    explicit BlockingBlockPool(const std::shared_ptr<C2BlockPool>& base)
        : mBase{base} {}

    local_id_t getLocalId() const final { return mBase->getLocalId(); }

    C2Allocator::id_t getAllocatorId() const final {
        return mBase->getAllocatorId();
    }

    c2_status_t fetchLinearBlock(uint32_t capacity,
                                 C2MemoryUsage usage,
                                 std::shared_ptr<C2LinearBlock>* block) final {
        c2_status_t status;

        do {
            status = mBase->fetchLinearBlock(capacity, usage, block);
        } while (status == C2_BLOCKING);

        return status;
    }

    c2_status_t fetchCircularBlock(
        uint32_t capacity,
        C2MemoryUsage usage,
        std::shared_ptr<C2CircularBlock>* block) final {
        c2_status_t status;

        do {
            status = mBase->fetchCircularBlock(capacity, usage, block);
        } while (status == C2_BLOCKING);

        return status;
    }

    c2_status_t fetchGraphicBlock(
        uint32_t width,
        uint32_t height,
        uint32_t format,
        C2MemoryUsage usage,
        std::shared_ptr<C2GraphicBlock>* block) final {
        c2_status_t status;

        do {
            status =
                mBase->fetchGraphicBlock(width, height, format, usage, block);
        } while (status == C2_BLOCKING);

        return status;
    }

private:
    const std::shared_ptr<C2BlockPool> mBase;
};

C2VendorBaseComponent::ComponentThread::ComponentThread(
    C2VendorBaseComponent& parent)
    : mParent{parent},
      mExitThread{false},
      mReplyResult{C2_NO_INIT},
      mTransitionInAction{false},
      mThread{&ComponentThread::threadLoop, this} {
    R_LOG(DEBUG);
}

C2VendorBaseComponent::ComponentThread::~ComponentThread() {
    R_LOG(DEBUG);

    {
        std::lock_guard lock{mMsgMutex};
        mExitThread = true;
        mMsgCV.notify_one();
    }

    mThread.join();
}

template <uint32_t type>
c2_status_t C2VendorBaseComponent::ComponentThread::postAndWait() {
    static_assert(type < Message::ASYNC_MSGS_START);

    R_LOG(DEBUG) << Message::AsString(type);

    bool expected = false;

    if (!mTransitionInAction.compare_exchange_strong(expected, true)) {
        R_LOG(ERROR) << "Can't wait for msg " << type
                     << " while waiting for another one";
        return C2_DUPLICATE;
    }

#ifdef TRACK_MSG_TIMINGS
    const Timer timer{Message::AsString(type)};
#endif

    createAndQueueMsg(type);

    constexpr std::chrono::duration waitTimeout =
        std::chrono::milliseconds{1500};
    c2_status_t res = C2_NO_INIT;

    {
        std::unique_lock lock{mReplyMutex};

        if (!mReplyCV.wait_for(lock, waitTimeout,
                               [this] { return mReplyResult != C2_NO_INIT; })) {
            R_LOG(ERROR) << "Timeout waiting for " << Message::AsString(type)
                         << " msg completion";

            mReplyResult = C2_CORRUPTED;
        }

        std::swap(mReplyResult, res);
    }

    R_LOG(DEBUG) << Message::AsString(type) << " msg got result " << res;

    mTransitionInAction = false;

    return res;
}

void C2VendorBaseComponent::ComponentThread::reply(c2_status_t result) {
    R_LOG(DEBUG) << "Msg completed with result " << result;

    std::lock_guard lock{mReplyMutex};
    mReplyResult = result;
    mReplyCV.notify_one();
}

void C2VendorBaseComponent::ComponentThread::threadLoop() {
    while (true) {
        decltype(mMsgQueue) dispatchList;

        {
            std::unique_lock lock{mMsgMutex};
            mMsgCV.wait(lock,
                        [this] { return mExitThread || !mMsgQueue.empty(); });

            if (mExitThread) {
                R_LOG(DEBUG) << "Exit looping";
                return;
            }

            mMsgQueue.swap(dispatchList);
        }

        for (const std::unique_ptr<Message>& msg : dispatchList) {
            mParent.handleMsg(*msg);
        }
    }
}

C2VendorBaseComponent::C2VendorBaseComponent(
    const std::shared_ptr<IntfImpl>& intfImpl,
    const std::shared_ptr<const OMXR_Core>& omxrCore)
    : mStartForbidden{false},
      mComponentIntf{std::make_shared<C2VendorComponentInterface>(intfImpl)},
      mState{STATE::UNINITIALIZED},
      mComponentThread{std::make_unique<ComponentThread>(*this)},
      mOMXName{intfImpl->getOMXName()},
      mOMXR_Core{omxrCore},
      mAdapterState{ADAPTER_STATE::UNLOADED},
      mFlushInProgress{false, false},
      mSettingsChanged{false},
      mSeenOutputEOS{false} {
    R_LOG(DEBUG);
}

C2VendorBaseComponent::~C2VendorBaseComponent() {
    R_LOG(DEBUG);
}

c2_status_t C2VendorBaseComponent::setListener_vb(
    const std::shared_ptr<Listener>& listener,
    c2_blocking_t mayBlock ATTRIBUTE_UNUSED) {
    R_LOG(DEBUG);

    *mListener.lock() = listener;

    return C2_OK;
}

c2_status_t C2VendorBaseComponent::queue_nb(
    std::list<std::unique_ptr<C2Work>>* const items) {
    if (mState != STATE::RUNNING) {
        R_LOG(ERROR) << "Not running";
        return C2_BAD_STATE;
    }

    bool queueWasEmpty = false;

    {
        auto inputWorkQueue = mInputWorkQueue.lock();

        queueWasEmpty = inputWorkQueue->empty();

        for (std::unique_ptr<C2Work>& work : *items) {
            std::vector<std::shared_ptr<C2Buffer>>& buffers =
                work->input.buffers;

            if (!buffers.empty() && !buffers.front()) {
                R_LOG(DEBUG) << "Removing nullptr input buffer from frameIndex "
                             << work->input.ordinal.frameIndex.peeku();

                buffers.clear();
            }

            inputWorkQueue->push_back(std::move(work));
        }
    }

    items->clear();

    if (queueWasEmpty) {
        mComponentThread->post<Message::DEQUEUE>();
    }

    return C2_OK;
}

c2_status_t C2VendorBaseComponent::flush_sm(
    flush_mode_t mode, std::list<std::unique_ptr<C2Work>>* const flushedWork) {
    if (mode != FLUSH_COMPONENT) {
        R_LOG(ERROR) << "Unsupported flush mode " << mode;
        return C2_OMITTED;
    }

    if (mState != STATE::RUNNING) {
        R_LOG(ERROR) << "Not running";
        return C2_BAD_STATE;
    }

    {
        auto inputWorkQueue = mInputWorkQueue.lock();

        R_LOG(DEBUG) << "Abandoning " << inputWorkQueue->size()
                     << " inputWorks";

        for (std::unique_ptr<C2Work>& work : *inputWorkQueue) {
            flushedWork->push_back(std::move(work));
        }

        inputWorkQueue->clear();
    }

    {
        auto worksToProcess = mWorksToProcess.lock();

        R_LOG(DEBUG) << "Abandoning " << worksToProcess->size()
                     << " worksToProgress";

        for (std::unique_ptr<C2Work>& work : *worksToProcess) {
            flushedWork->push_back(std::move(work));
        }

        worksToProcess->clear();
    }

    for (const std::unique_ptr<C2Work>& work : *flushedWork) {
        work->input.buffers.clear();
    }

    return mComponentThread->postAndWait<Message::FLUSH>();
}

c2_status_t C2VendorBaseComponent::start() {
    R_LOG(DEBUG);

    if (mStartForbidden) {
        R_LOG(ERROR) << intf()->getName() << " wasn't allowed to start";
        return C2_NO_MEMORY;
    }

    if (mState == STATE::RUNNING) {
        R_LOG(ERROR) << "Already running";
        return C2_BAD_STATE;
    }

    const c2_status_t status = mState == STATE::UNINITIALIZED
        ? mComponentThread->postAndWait<Message::INIT>()
        : mComponentThread->postAndWait<Message::START>();

    if (status != C2_OK) {
        R_LOG(ERROR) << "Failed to start, " << status;

        releaseComponent();
    } else {
        mState = STATE::RUNNING;
    }

    return status;
}

c2_status_t C2VendorBaseComponent::stop() {
    R_LOG(DEBUG);

    if (mState != STATE::RUNNING) {
        R_LOG(DEBUG) << "Not running";
        return C2_BAD_STATE;
    }

    const c2_status_t status = mComponentThread->postAndWait<Message::STOP>();

    if (status != C2_OK) {
        R_LOG(ERROR) << "Failed to stop, " << status;
        return status;
    }

    mState = STATE::STOPPED;
    // NOTE: technically thread-unsafe
    mOutputBlockPool.reset();

    return C2_OK;
}

c2_status_t C2VendorBaseComponent::reset() {
    R_LOG(DEBUG);

    c2_status_t status = stop();

    if (status == C2_BAD_STATE) {
        status = C2_OK;
    }

    return status;
}

c2_status_t C2VendorBaseComponent::release() {
    R_LOG(DEBUG);

    return releaseComponent();
}

void C2VendorBaseComponent::OnReceivedEvent(OMX_EVENTTYPE event,
                                            OMX_U32 data1,
                                            OMX_U32 data2) const {
#ifdef R_LOG_VERBOSITY
    const char* arg1 = "";
    const char* arg2 = "";

    switch (event) {
    case OMX_EventCmdComplete:
        arg1 = AsString(static_cast<OMX_COMMANDTYPE>(data1));

        switch (static_cast<OMX_COMMANDTYPE>(data1)) {
        case OMX_CommandStateSet:
            arg2 = AsString(static_cast<OMX_STATETYPE>(data2));
            break;
        default:
            arg2 = AsPortString(data2);
            break;
        }
        break;
    case OMX_EventError:
        arg1 = AsString(static_cast<OMX_ERRORTYPE>(data1));
        break;
    case OMX_EventPortSettingsChanged:
        arg2 = AsString(static_cast<OMX_INDEXTYPE>(data2));
        FALLTHROUGH_INTENDED;
    default:
        arg1 = AsPortString(data1);
        break;
    }

    R_LOG(DEBUG) << event << ", " << arg1 << ", " << arg2;
#endif

    mComponentThread->post<Message::EVENT>(EventData{event, data1, data2});
}

void C2VendorBaseComponent::OnEmptyBufferDone(
    OMX_BUFFERHEADERTYPE* header) const {
    R_LOG(DEBUG) << "FrameIndex " << TsToFrameIndex(header->nTimeStamp);

    mComponentThread->post<Message::INPUT_DONE>(BufferData{header});
}

void C2VendorBaseComponent::OnFillBufferDone(
    OMX_BUFFERHEADERTYPE* header) const {
    R_LOG(DEBUG) << "FrameIndex " << TsToFrameIndex(header->nTimeStamp);

    mComponentThread->post<Message::OUTPUT_DONE>(
        onPreprocessOutput(*mOMXR_Adapter, header));
}

c2_status_t C2VendorBaseComponent::releaseComponent() {
    R_LOG(DEBUG);

    if (mState == STATE::UNINITIALIZED) {
        R_LOG(DEBUG) << "Already uninitialized";
        return C2_BAD_STATE;
    }

    c2_status_t status = stop();

    if (status != C2_OK && status != C2_BAD_STATE) {
        return status;
    }

    mState = STATE::UNINITIALIZED;
    // NOTE: technically thread-unsafe
    mOMXR_Adapter.reset();
    mAdapterState = ADAPTER_STATE::UNLOADED;

    return C2_OK;
}

void C2VendorBaseComponent::onExtensionMsg(const Message& msg) {
    R_LOG(DEBUG) << "Unknown msg type " << Message::AsString(msg.mType);
}

bool C2VendorBaseComponent::onStateSet(OMX_STATETYPE omxState) {
    R_LOG(DEBUG) << omxState;

    switch (omxState) {
    case OMX_StateInvalid: {
        mAdapterState = ADAPTER_STATE::INVALID;

        return false;
    }
    case OMX_StateIdle: {
        if (mAdapterState == ADAPTER_STATE::LOADED_TO_IDLE) {
            mAdapterState = ADAPTER_STATE::IDLE;

            if (!sendOMXCommand(OMX_CommandStateSet, OMX_StateExecuting)) {
                return false;
            }

            mAdapterState = ADAPTER_STATE::IDLE_TO_EXECUTING;
        } else {
            mAdapterState = ADAPTER_STATE::IDLE;

            if (!sendOMXCommand(OMX_CommandStateSet, OMX_StateLoaded)) {
                return false;
            }

            mAdapterState = ADAPTER_STATE::IDLE_TO_LOADED;

            freePortBuffers(InputPortIndex);
            freePortBuffers(OutputPortIndex);

            mAvailableInputIndexes.clear();
            mOwnedOutputIndexes.clear();
        }
        break;
    }
    case OMX_StateExecuting: {
        mAdapterState = ADAPTER_STATE::EXECUTING;

        postFillBuffers();

        break;
    }
    case OMX_StateLoaded: {
        mAdapterState = ADAPTER_STATE::LOADED;
        mFlushInProgress[InputPortIndex] = mFlushInProgress[OutputPortIndex] =
            false;
        mSettingsChanged = false;
        mSeenOutputEOS = false;
        break;
    }
    default: {
        R_LOG(WARNING) << "Unknown OMX state " << omxState;
        break;
    }
    }

    return true;
}

template <bool printOnError>
bool C2VendorBaseComponent::checkState(ADAPTER_STATE expected) const {
    if (mAdapterState != expected) {
        if constexpr (printOnError) {
            R_LOG(ERROR) << "Bad adapter state: actual " << mAdapterState
                         << ", expected " << expected;
        }

        return false;
    }

    return true;
}

template bool C2VendorBaseComponent::checkState<true>(
    ADAPTER_STATE expected) const;

template bool C2VendorBaseComponent::checkState<false>(
    ADAPTER_STATE expected) const;

OMX_ERRORTYPE C2VendorBaseComponent::setPortFormat(
    PortIndex portIndex,
    OMX_U32 frameRate,
    OMX_VIDEO_CODINGTYPE coding,
    OMX_COLOR_FORMATTYPE colorFormat) const {
    CHECK_NE(coding == OMX_VIDEO_CodingUnused,
             colorFormat == OMX_COLOR_FormatUnused);

    OMX_VIDEO_PARAM_PORTFORMATTYPE formatParam;
    formatParam.eCompressionFormat = coding;
    formatParam.eColorFormat = colorFormat;
    formatParam.xFramerate = (frameRate << 16u);

    const OMX_ERRORTYPE omxError =
        mOMXR_Adapter->setPortParam(portIndex, OMX_IndexParamVideoPortFormat,
                                    formatParam);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to set " << AsPortString(portIndex)
                     << " port format, " << omxError;
    }

    return omxError;
}

OMX_ERRORTYPE C2VendorBaseComponent::setPortDef(
    PortIndex portIndex,
    OMX_U32 width,
    OMX_U32 height,
    OMX_U32 bitrate,
    OMX_U32 frameRate,
    OMX_VIDEO_CODINGTYPE coding,
    OMX_COLOR_FORMATTYPE colorFormat) const {
    CHECK_NE(coding == OMX_VIDEO_CodingUnused,
             colorFormat == OMX_COLOR_FormatUnused);

    OMX_PARAM_PORTDEFINITIONTYPE portDef;
    OMX_VIDEO_PORTDEFINITIONTYPE& videoDef = portDef.format.video;

    OMX_ERRORTYPE omxError =
        mOMXR_Adapter->getPortParam(portIndex, OMX_IndexParamPortDefinition,
                                    portDef);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to get " << AsPortString(portIndex)
                     << " port def, " << omxError;
        return omxError;
    }

    videoDef.nFrameWidth = width;
    videoDef.nFrameHeight = height;
    videoDef.nSliceHeight = height;
    videoDef.nBitrate = bitrate;
    videoDef.xFramerate = (frameRate << 16u);
    videoDef.eCompressionFormat = coding;
    videoDef.eColorFormat = colorFormat;
    portDef.nBufferCountActual = portDef.nBufferCountMin;

    if (coding != OMX_VIDEO_CodingUnused) {
        videoDef.nStride = static_cast<OMX_S32>(width);
    } else {
        videoDef.nStride = static_cast<OMX_S32>(Align64(width));
        portDef.nBufferSize = static_cast<OMX_U32>(videoDef.nStride) *
            videoDef.nSliceHeight * 3u / 2u;
    }

    R_LOG(DEBUG) << "Setting port definition: " << videoDef.nFrameWidth << "x"
                 << videoDef.nFrameHeight << ", " << videoDef.nStride << "x"
                 << videoDef.nSliceHeight << ", bitrate " << videoDef.nBitrate
                 << ", frameRate " << videoDef.xFramerate << ", coding "
                 << videoDef.eCompressionFormat << ", colorFormat "
                 << videoDef.eColorFormat << ", bufs "
                 << portDef.nBufferCountMin << "|" << portDef.nBufferCountActual
                 << ", sz " << portDef.nBufferSize;

    omxError = mOMXR_Adapter->setParam(OMX_IndexParamPortDefinition, portDef);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to set " << AsPortString(portIndex)
                     << " port def, " << omxError;
    }

    return omxError;
}

void C2VendorBaseComponent::querySupportedProfileLevels(
    PortIndex portIndex) const {
    OMX_VIDEO_PARAM_PROFILELEVELTYPE profileLevel;
    OMXR_Adapter::InitOMXParam(profileLevel);
    profileLevel.nPortIndex = portIndex;

    for (profileLevel.nProfileIndex = 0u;
         mOMXR_Adapter->getParam(OMX_IndexParamVideoProfileLevelQuerySupported,
                                 profileLevel) == OMX_ErrorNone;
         profileLevel.nProfileIndex++) {
        R_LOG(INFO) << "#" << profileLevel.nProfileIndex << ": "
                     << profileLevel.eProfile << ", " << profileLevel.eLevel;
    }
}

void C2VendorBaseComponent::processConfigUpdate(
    std::vector<std::unique_ptr<C2Param>>& configUpdate) {
    if (configUpdate.empty()) {
        return;
    }

    std::vector<C2Param*> rawConfigUpdate;
    rawConfigUpdate.reserve(configUpdate.size());

    for (const std::unique_ptr<C2Param>& config : configUpdate) {
        if (config) {
            rawConfigUpdate.push_back(config.get());
        }
    }

    if (!rawConfigUpdate.empty()) {
        std::vector<std::unique_ptr<C2SettingResult>> failures;

        const c2_status_t status =
            intf()->config_vb(rawConfigUpdate, C2_MAY_BLOCK, &failures);

        R_LOG(DEBUG) << "Config " << rawConfigUpdate.size() << " updates, "
                     << status;
    }

    configUpdate.clear();
}

OMX_ERRORTYPE C2VendorBaseComponent::emptyBuffer(
    OMX_BUFFERHEADERTYPE* const header) const {
    const OMX_ERRORTYPE omxError = mOMXR_Adapter->emptyBuffer(header);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to empty frameIndex "
                     << TsToFrameIndex(header->nTimeStamp) << ", index "
                     << GetBufferIndex(header) << ", " << omxError;

        header->nFilledLen = 0u;
        header->nTimeStamp = 0;
        header->nFlags = 0u;
    }

    return omxError;
}

OMX_ERRORTYPE C2VendorBaseComponent::fillBuffer(
    OMX_BUFFERHEADERTYPE* const header) {
    const size_t index = GetBufferIndex(header);

    CHECK(mOwnedOutputIndexes[index]);

    const OMX_ERRORTYPE omxError = mOMXR_Adapter->fillBuffer(header);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to fill buffer by index " << index << ", "
                     << omxError;

        reportError(C2_CORRUPTED);
    } else {
        mOwnedOutputIndexes[index] = false;
    }

    return omxError;
}

void C2VendorBaseComponent::pushPendingWork(uint64_t frameIndex,
                                            std::unique_ptr<C2Work> work) {
    R_LOG(DEBUG) << "Pushing to pending frameIndex " << frameIndex;

    const auto pendingIt = mPendingWorks.find(frameIndex);

    if (pendingIt != mPendingWorks.cend()) {
        R_LOG(ERROR) << "Unexpected pending frameIndex " << frameIndex;

        std::unique_ptr<C2Work> unexpectedWork = std::move(pendingIt->second);
        mPendingWorks.erase(frameIndex);

        reportWork(std::move(unexpectedWork), C2_CORRUPTED);
    }

    mPendingWorks.emplace(frameIndex, std::move(work));
}

std::unique_ptr<C2Work> C2VendorBaseComponent::popPendingWork(
    uint64_t frameIndex) {
    const auto pendingIt = mPendingWorks.find(frameIndex);

    CHECK(pendingIt != mPendingWorks.cend());

    std::unique_ptr<C2Work> work = std::move(pendingIt->second);
    mPendingWorks.erase(pendingIt);

    R_LOG(DEBUG) << "Popped pending frameIndex " << frameIndex;
    return work;
}

void C2VendorBaseComponent::reportWork(std::unique_ptr<C2Work> work,
                                       c2_status_t result) {
    C2FrameData& output = work->worklets.front()->output;

    if ((work->input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0u) {
        output.flags = C2FrameData::FLAG_END_OF_STREAM;
    } else {
        output.flags = {};
    }

    output.ordinal = work->input.ordinal;
    work->workletsProcessed = 1u;
    work->result = result;

    R_LOG(DEBUG) << "Done frameIndex " << output.ordinal.frameIndex.peeku()
                 << ", ts " << output.ordinal.timestamp.peeku() << ", flags "
                 << std::hex << output.flags << std::dec << ", "
                 << work->result;

    (*mListener.lock())
        ->onWorkDone_nb(shared_from_this(), MakeList(std::move(work)));
}

void C2VendorBaseComponent::reportError(c2_status_t result) {
    R_LOG(DEBUG) << result;

    (*mListener.lock())
        ->onError_nb(shared_from_this(), static_cast<uint32_t>(result));
}

void C2VendorBaseComponent::reportClonedWork(const C2Work& work) {
    R_LOG(DEBUG) << work.input.ordinal.frameIndex.peeku();

    std::unique_ptr<C2Work> clonedWork = std::make_unique<C2Work>();
    clonedWork->worklets.push_back(std::make_unique<C2Worklet>());
    C2FrameData& clonedOutput = clonedWork->worklets.front()->output;

    clonedOutput.flags = C2FrameData::FLAG_INCOMPLETE;
    clonedOutput.ordinal = work.input.ordinal;

    std::swap(work.worklets.front()->output.buffers, clonedOutput.buffers);

    clonedWork->workletsProcessed = 1u;
    clonedWork->result = C2_OK;

    (*mListener.lock())
        ->onWorkDone_nb(shared_from_this(), MakeList(std::move(clonedWork)));
}

void C2VendorBaseComponent::handleMsg(const Message& msg) {
    c2_status_t res = C2_NO_INIT;

    switch (msg.mType) {
    case Message::INIT:
        res = handleInit();
        break;
    case Message::START:
        res = handleStart();
        break;
    case Message::STOP:
        res = handleStop();
        break;
    case Message::FLUSH:
        res = handleFlush();
        break;
    case Message::DEQUEUE:
        if (handleDequeue()) {
            mComponentThread->post<Message::DEQUEUE>();
        }
        return;
    case Message::EVENT:
        handleEvent(msg.eventData);
        return;
    case Message::INPUT_DONE:
        handleInputDone(msg.bufferData);
        return;
    case Message::OUTPUT_DONE:
        handleOutputDone(msg.extendedBufferData);
        return;
    default:
        onExtensionMsg(msg);
        return;
    }

    if (res != C2_OK) {
        mComponentThread->reply(res);
    }
}

c2_status_t C2VendorBaseComponent::handleInit() {
    R_LOG(DEBUG);

    if (!checkState(ADAPTER_STATE::UNLOADED)) {
        return C2_CORRUPTED;
    }

    OMX_ERRORTYPE omxError = OMX_ErrorNone;
    auto localAdapter =
        std::make_unique<OMXR_Adapter>(omxError, mOMXName, mOMXR_Core,
                                       shared_from_this());

    if (omxError != OMX_ErrorNone) {
        return C2_CORRUPTED;
    }

    mOMXR_Adapter = std::move(localAdapter);
    mAdapterState = ADAPTER_STATE::LOADED;

    return handleStart();
}

c2_status_t C2VendorBaseComponent::handleStart() {
    R_LOG(DEBUG);

    if (!checkState(ADAPTER_STATE::LOADED) || !onConfigure(*mOMXR_Adapter) ||
        !sendOMXCommand(OMX_CommandStateSet, OMX_StateIdle)) {
        return C2_CORRUPTED;
    }

    mAdapterState = ADAPTER_STATE::LOADED_TO_IDLE;

    if (!allocatePortBuffers(InputPortIndex) ||
        !allocatePortBuffers(OutputPortIndex)) {
        return C2_CORRUPTED;
    }

    for (size_t i = 0u, inputHeaderNum = mHeaders[InputPortIndex].size();
         i < inputHeaderNum; i++) {
        mAvailableInputIndexes.push_back(i);
    }

    mOwnedOutputIndexes.resize(mHeaders[OutputPortIndex].size(), true);

    return C2_OK;
}

c2_status_t C2VendorBaseComponent::handleStop() {
    R_LOG(DEBUG);

    if (!checkState(ADAPTER_STATE::EXECUTING) ||
        !sendOMXCommand(OMX_CommandStateSet, OMX_StateIdle)) {
        return C2_CORRUPTED;
    }

    mAdapterState = ADAPTER_STATE::EXECUTING_TO_IDLE;

    return C2_OK;
}

c2_status_t C2VendorBaseComponent::handleFlush() {
    if (!checkState(ADAPTER_STATE::EXECUTING)) {
        return C2_CORRUPTED;
    }

    R_LOG(DEBUG) << "About to flush both ports";

    if (!sendOMXCommand(OMX_CommandFlush, AllPorts)) {
        return C2_CORRUPTED;
    }

    mAdapterState = ADAPTER_STATE::FLUSHING;
    mFlushInProgress[InputPortIndex] = mFlushInProgress[OutputPortIndex] = true;

    return C2_OK;
}

bool C2VendorBaseComponent::handleDequeue() {
    if (mAdapterState == ADAPTER_STATE::FLUSHING) {
        R_LOG(DEBUG) << "Can't dequeue, flushing in progress";
        return false;
    }

    std::unique_ptr<C2Work> work;
    bool hasWork = false;

    {
        auto inputWorkQueue = mInputWorkQueue.lock();

        if (inputWorkQueue->empty()) {
            R_LOG(DEBUG) << "No more work to dequeue";
            return false;
        }

        work = std::move(inputWorkQueue->front());
        inputWorkQueue->pop_front();

        hasWork = !inputWorkQueue->empty();
    }

    const C2FrameData& input = work->input;
    const uint64_t frameIndex = input.ordinal.frameIndex.peeku();

    if (input.buffers.empty() &&
        ((input.flags & C2FrameData::FLAG_END_OF_STREAM) == 0u ||
         (!mOutputBlockPool && frameIndex == 0u))) {
        R_LOG(DEBUG) << "Empty frameIndex " << frameIndex
                     << ", no actions required";

        reportWork(std::move(work), C2_OK);

        return hasWork;
    }

    createOutputPoolIfNeeded();

    {
        auto worksToProgress = mWorksToProcess.lock();

        if (mAvailableInputIndexes.empty() || !worksToProgress->empty()) {
            R_LOG(DEBUG) << "FrameIndex " << frameIndex
                         << " will be processed lately";

            worksToProgress->push_back(std::move(work));

            return hasWork;
        }
    }

    const size_t index = mAvailableInputIndexes.front();

    if (onProcessInput(std::move(work), mHeaders[InputPortIndex][index],
                       true) == C2_OK) {
        mAvailableInputIndexes.pop_front();
    }

    return hasWork;
}

void C2VendorBaseComponent::handleEvent(const EventData& data) {
    const OMX_EVENTTYPE event = data.event;
    const OMX_U32 data1 = data.data1;
    const OMX_U32 data2 = data.data2;

    switch (event) {
    case OMX_EventCmdComplete: {
        const OMX_COMMANDTYPE omxCmd = static_cast<OMX_COMMANDTYPE>(data1);

        R_LOG(DEBUG) << omxCmd;

        switch (omxCmd) {
        case OMX_CommandStateSet: {
            const OMX_STATETYPE omxState = static_cast<OMX_STATETYPE>(data2);

            if (!onStateSet(omxState)) {
                mComponentThread->reply(C2_CORRUPTED);
            } else if (omxState == OMX_StateExecuting ||
                       omxState == OMX_StateLoaded) {
                mComponentThread->reply(C2_OK);
            }
            break;
        }
        case OMX_CommandFlush: {
            R_LOG(DEBUG) << AsPortString(data2);

            CHECK(data2 == InputPortIndex || data2 == OutputPortIndex);
            CHECK(mFlushInProgress[data2]);

            mFlushInProgress[data2] = false;

            if (!mFlushInProgress[InputPortIndex] &&
                !mFlushInProgress[OutputPortIndex]) {
                mSeenOutputEOS = false;

                R_LOG(DEBUG) << "Abandoning works: " << mPendingWorks.size()
                             << " from pending, " << mEmptiedWorks.size()
                             << " from emptied";

                for (auto& p : mPendingWorks) {
                    reportWork(std::move(p.second), C2_NOT_FOUND);
                }

                mPendingWorks.clear();

                for (auto& p : mEmptiedWorks) {
                    reportWork(std::move(p.second), C2_NOT_FOUND);
                }

                mEmptiedWorks.clear();

                if (!checkState(ADAPTER_STATE::FLUSHING)) {
                    mComponentThread->reply(C2_CORRUPTED);
                } else {
                    mAdapterState = ADAPTER_STATE::EXECUTING;

                    postFillBuffers();

                    R_LOG(DEBUG) << "Posting dequeue after flushing";

                    mComponentThread->post<Message::DEQUEUE>();
                    mComponentThread->reply(C2_OK);
                }
            }
            break;
        }
        case OMX_CommandPortDisable: {
            CHECK_EQ(data2, OutputPortIndex);

            onOutputPortDisabled();
            break;
        }
        case OMX_CommandPortEnable: {
            CHECK_EQ(data2, OutputPortIndex);

            onOutputPortEnabled();
            break;
        }
        default: {
            R_LOG(DEBUG) << "Not implemented " << omxCmd << ", " << data2;
            break;
        }
        }
        break;
    }
    case OMX_EventError: {
        R_LOG(ERROR) << "Error event " << static_cast<OMX_ERRORTYPE>(data1);

        reportError(C2_CORRUPTED);
        break;
    }
    case OMX_EventBufferFlag: {
        R_LOG(DEBUG) << AsPortString(data1) << " EOS event";
        break;
    }
    case OMX_EventPortSettingsChanged: {
        R_LOG(DEBUG) << "Settings changed on " << AsPortString(data1)
                     << ", index " << static_cast<OMX_INDEXTYPE>(data2);

        CHECK_EQ(data1, OutputPortIndex);

        onOutputSettingsChanged();
        break;
    }
    default: {
        R_LOG(DEBUG) << "Not implemented " << event << ", " << data1 << ", "
                     << data2;
        break;
    }
    }
}

void C2VendorBaseComponent::handleInputDone(const BufferData& data) {
    OMX_BUFFERHEADERTYPE* const header = data.header;
    const size_t index = GetBufferIndex(header);
    const uint64_t frameIndex = TsToFrameIndex(header->nTimeStamp);
    const bool csd = (header->nFlags & OMX_BUFFERFLAG_CODECCONFIG) != 0u;

    R_LOG(DEBUG) << "OMXFlags " << std::hex << header->nFlags << std::dec
                 << ", frameIndex " << frameIndex << ", index " << index;

    header->nFilledLen = 0u;
    header->nFlags = 0u;
    header->nTimeStamp = 0;

    std::unique_ptr<C2Work> emptiedWork = popPendingWork(frameIndex);
    emptiedWork->input.buffers.clear();

    if (csd) {
        R_LOG(DEBUG) << "Emptied csd frameIndex " << frameIndex;

        reportWork(std::move(emptiedWork), C2_OK);
    } else {
        CHECK(mEmptiedWorks.emplace(frameIndex, std::move(emptiedWork)).second);
    }

    if (!checkState<true>(ADAPTER_STATE::EXECUTING)) {
        R_LOG(DEBUG) << "Can't handle workToProgress, illegal state";

        mAvailableInputIndexes.push_back(index);

        return;
    }

    c2_status_t status = onInputDone(data);

    if (status != C2_OK && status != C2_OMITTED) {
        mAvailableInputIndexes.push_back(index);

        return;
    }

    status = C2_NO_INIT;

    while (status == C2_NO_INIT) {
        std::unique_ptr<C2Work> work = tryPopWorkToProcess();

        if (!work) {
            R_LOG(DEBUG) << "No more works to process";
            break;
        }

        if (work->input.buffers.empty() &&
            ((work->input.flags & C2FrameData::FLAG_END_OF_STREAM) == 0u)) {
            R_LOG(DEBUG) << "Empty frameIndex "
                         << TsToFrameIndex(header->nTimeStamp)
                         << ", no actions required";

            reportWork(std::move(work), C2_OK);

            continue;
        }

        status = onProcessInput(std::move(work), header, false);
    }

    if (status != C2_OK) {
        mAvailableInputIndexes.push_back(index);

        if (status == C2_CORRUPTED) {
            R_LOG(WARNING) << "Available indexes:";
            for (const size_t i : mAvailableInputIndexes) {
                R_LOG(WARNING) << "    " << i;
            }
        }
    }
}

void C2VendorBaseComponent::handleOutputDone(const ExtendedBufferData& data) {
    OMX_BUFFERHEADERTYPE* const header = data.header;
    const size_t index = GetMeta(header)->mIndex;
    uint64_t frameIndex = TsToFrameIndex(header->nTimeStamp);
    const bool eos = (header->nFlags & OMX_BUFFERFLAG_EOS) != 0u;

    CHECK(!mOwnedOutputIndexes[index]);

    mOwnedOutputIndexes[index] = true;

    if (mSeenOutputEOS) {
        R_LOG(DEBUG) << "Can't handle output buf, output EOS is seen";
        return;
    }

    if (mSettingsChanged) {
        R_LOG(DEBUG) << "Can't handle output buf, settings changed";

        delete GetMeta(header);

        const OMX_ERRORTYPE omxError =
            mOMXR_Adapter->freeBuffer(OutputPortIndex, header);

        if (omxError != OMX_ErrorNone) {
            R_LOG(ERROR) << "Failed to free " << AsPortString(OutputPortIndex)
                         << " buf, " << omxError;
        }

        return;
    }

    if (mAdapterState == ADAPTER_STATE::FLUSHING) {
        R_LOG(DEBUG) << "Can't handle output buf, flushing in progress";
        return;
    }

    if (mAdapterState == ADAPTER_STATE::EXECUTING_TO_IDLE) {
        R_LOG(DEBUG) << "Can't handle output buf, stopping";
        return;
    }

    onOutputDone(data);

    if (eos) {
        R_LOG(DEBUG) << "Seen output EOS, frameIndex " << frameIndex;

        mSeenOutputEOS = true;

        R_LOG(DEBUG) << "Queues after EOS " << mInputWorkQueue.lock()->size()
                     << ", " << mWorksToProcess.lock()->size() << ", "
                     << mPendingWorks.size() << ", " << mEmptiedWorks.size();

        // TODO: finish all works in all queues
    }
}

void C2VendorBaseComponent::onOutputSettingsChanged() {
    R_LOG(DEBUG);

    mSettingsChanged = true;

    constexpr PortIndex portIndex = OutputPortIndex;
    OMX_PARAM_PORTDEFINITIONTYPE portDef;
    OMX_VIDEO_PORTDEFINITIONTYPE& videoDef = portDef.format.video;

    OMX_ERRORTYPE omxError =
        mOMXR_Adapter->getPortParam(OutputPortIndex,
                                    OMX_IndexParamPortDefinition, portDef);

    if (omxError != OMX_ErrorNone) {
        reportError(C2_CORRUPTED);

        R_LOG(ERROR) << "Failed to get " << AsPortString(portIndex)
                     << " port def, " << omxError;
        return;
    }

    R_LOG(DEBUG) << "Got new port definition: " << videoDef.nFrameWidth << "x"
                 << videoDef.nFrameHeight << ", " << videoDef.nStride << "x"
                 << videoDef.nSliceHeight << ", bitrate " << videoDef.nBitrate
                 << ", frameRate " << videoDef.xFramerate << ", coding "
                 << videoDef.eCompressionFormat << ", colorFormat "
                 << videoDef.eColorFormat << ", bufs "
                 << portDef.nBufferCountMin << "|" << portDef.nBufferCountActual
                 << ", sz " << portDef.nBufferSize << ", enabled "
                 << portDef.bEnabled << ", populated " << portDef.bPopulated;

    std::vector<std::unique_ptr<C2SettingResult>> failures;
    auto pictureSizeParam =
        std::make_unique<C2StreamPictureSizeInfo::output>(0u,
                                                          videoDef.nFrameWidth,
                                                          videoDef
                                                              .nFrameHeight);

    const c2_status_t status =
        intf()->config_vb({pictureSizeParam.get()}, C2_MAY_BLOCK, &failures);

    if (status != C2_OK) {
        R_LOG(ERROR) << "Failed to config new picture size: "
                     << pictureSizeParam->width << "x"
                     << pictureSizeParam->height << ", " << status;
        return;
    }

    if (!sendOMXCommand(OMX_CommandPortDisable, portIndex)) {
        return;
    }

    std::vector<OMX_BUFFERHEADERTYPE*>& headers = mHeaders[portIndex];

    for (OMX_BUFFERHEADERTYPE* const header : headers) {
        const BufferMeta* meta = GetMeta(header);
        const size_t index = meta->mIndex;

        if (!mOwnedOutputIndexes[index]) {
            R_LOG(DEBUG) << "Index " << index
                         << " not owned by us, skipping freeing";
            continue;
        }

        delete meta;

        omxError = mOMXR_Adapter->freeBuffer(portIndex, header);

        if (omxError != OMX_ErrorNone) {
            R_LOG(ERROR) << "Failed to free " << AsPortString(portIndex)
                         << " buf, " << omxError;
        }
    }
}

void C2VendorBaseComponent::onOutputPortDisabled() {
    R_LOG(DEBUG);

    mHeaders[OutputPortIndex].clear();

    if (!sendOMXCommand(OMX_CommandPortEnable, OutputPortIndex) ||
        !allocatePortBuffers(OutputPortIndex)) {
        reportError(C2_CORRUPTED);
    }
}

void C2VendorBaseComponent::onOutputPortEnabled() {
    R_LOG(DEBUG);

    mSettingsChanged = false;

    postFillBuffers();

    R_LOG(DEBUG) << "Posting dequeue after settings changed";

    mComponentThread->post<Message::DEQUEUE>();
}

bool C2VendorBaseComponent::sendOMXCommand(OMX_COMMANDTYPE cmd,
                                           OMX_U32 param) const {
    const std::string paramStr =
        (cmd == OMX_CommandStateSet
             ? AsString(static_cast<OMX_STATETYPE>(param))
             : AsPortString(param));

    R_LOG(DEBUG) << cmd << ", " << paramStr;

    const OMX_ERRORTYPE omxError = mOMXR_Adapter->sendCommand(cmd, param);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to send " << cmd << ", " << paramStr << ", "
                     << omxError;
        return false;
    }

    return true;
}

bool C2VendorBaseComponent::allocatePortBuffers(PortIndex portIndex) {
    CHECK(portIndex == InputPortIndex || portIndex == OutputPortIndex);

    OMX_PARAM_PORTDEFINITIONTYPE portDef;

    OMX_ERRORTYPE omxError =
        mOMXR_Adapter->getPortParam(portIndex, OMX_IndexParamPortDefinition,
                                    portDef);

    if (omxError != OMX_ErrorNone) {
        R_LOG(ERROR) << "Failed to get " << AsPortString(portIndex)
                     << " port def, " << omxError;
        return false;
    }

    const size_t count = static_cast<size_t>(portDef.nBufferCountActual);
    const OMX_U32 size = portDef.nBufferSize;

    R_LOG(DEBUG) << "Allocating " << count << " bufs with size " << size
                 << " on " << AsPortString(portIndex);

    std::vector<OMX_BUFFERHEADERTYPE*>& headers = mHeaders[portIndex];
    headers.reserve(count);

    CHECK(headers.empty());

    BufferMeta* meta = nullptr;
    OMX_BUFFERHEADERTYPE* header = nullptr;

    for (size_t i = 0u; i < count; i++, meta = nullptr, header = nullptr) {
        meta = new BufferMeta{i};

        omxError =
            mOMXR_Adapter->allocateBuffer(&header, portIndex,
                                          static_cast<OMX_PTR>(meta), size);

        if (omxError != OMX_ErrorNone) {
            delete meta;

            R_LOG(ERROR) << "Failed to allocate buf #" << i << ", " << omxError;
            return false;
        }

        headers.push_back(header);
    }

    return true;
}

void C2VendorBaseComponent::freePortBuffers(PortIndex portIndex) {
    R_LOG(DEBUG) << AsPortString(portIndex);

    bool errorOccured = false;
    OMX_ERRORTYPE omxError = OMX_ErrorNone;
    std::vector<OMX_BUFFERHEADERTYPE*>& headers = mHeaders[portIndex];

    for (OMX_BUFFERHEADERTYPE* const header : headers) {
        delete GetMeta(header);

        omxError = mOMXR_Adapter->freeBuffer(portIndex, header);

        if (omxError != OMX_ErrorNone) {
            errorOccured = true;

            R_LOG(ERROR) << "Failed to free " << AsPortString(portIndex)
                         << " buf, " << omxError;
        }
    }

    headers.clear();

    if (errorOccured) {
        reportError(C2_CORRUPTED);
    }
}

void C2VendorBaseComponent::postFillBuffers() {
    R_LOG(DEBUG);

    for (OMX_BUFFERHEADERTYPE* const header : mHeaders[OutputPortIndex]) {
        if (fillBuffer(header) != OMX_ErrorNone) {
            break;
        }
    }
}

c2_status_t C2VendorBaseComponent::createOutputPoolIfNeeded() {
    if (mOutputBlockPool) {
        return C2_OK;
    }

    std::vector<std::unique_ptr<C2Param>> heapParams;
    constexpr uint32_t outputBlockPoolIndex =
        C2PortBlockPoolsTuning::output::PARAM_TYPE;

    c2_status_t status = intf()->query_vb({}, {outputBlockPoolIndex},
                                          C2_DONT_BLOCK, &heapParams);

    if (status != C2_OK) {
        R_LOG(ERROR) << "Failed to query output blockPool, " << status;
        return status;
    }

    if (heapParams.size() != 1u) {
        R_LOG(ERROR) << "Unexpected heap params num " << heapParams.size();
        return C2_CORRUPTED;
    }

    const C2PortBlockPoolsTuning::output* const outputBlockPoolParam =
        C2PortBlockPoolsTuning::output::From(heapParams.front().get());

    if (outputBlockPoolParam == nullptr) {
        R_LOG(ERROR) << "Unsuitable output block pool param";
        return C2_CORRUPTED;
    }

    const C2BlockPool::local_id_t poolId = outputBlockPoolParam->m.values[0u];
    std::shared_ptr<C2BlockPool> blockPool;

    status =
        android::GetCodec2BlockPool(poolId, shared_from_this(), &blockPool);

    if (status != C2_OK) {
        R_LOG(ERROR) << "Failed to create block pool " << poolId << ", "
                     << status;
        return status;
    }

    mOutputBlockPool = std::make_unique<BlockingBlockPool>(blockPool);

    R_LOG(DEBUG) << "AllocatorId " << mOutputBlockPool->getAllocatorId()
                 << ", poolId " << mOutputBlockPool->getLocalId();
    return C2_OK;
}

std::unique_ptr<C2Work> C2VendorBaseComponent::tryPopWorkToProcess() {
    auto worksToProcess = mWorksToProcess.lock();

    if (worksToProcess->empty()) {
        return nullptr;
    }

    std::unique_ptr<C2Work> work = std::move(worksToProcess->front());
    worksToProcess->pop_front();

    return work;
}

std::ostream& operator<<(std::ostream& os,
                         C2VendorBaseComponent::ADAPTER_STATE state) {
    return os << C2VendorBaseComponent::AsStateString(state);
}

void CopyBuffer(uint8_t* const dst, const uint8_t* const src, size_t size) {
    std::copy(src, src + size, dst);
}

#ifdef R_LOG_VERBOSITY
template <bool isEncoder>
void DumpBuffer(const char* const data,
                ptrdiff_t size,
                const std::string& filename) {
    constexpr const char* const DirPath =
        isEncoder ? "/data/encDir/" : "/data/decDir/";
    constexpr const char* const FileExt = ".data";
    const std::string filePath = DirPath + filename + FileExt;

    std::ofstream os{filePath};

    if (!os.is_open()) {
        R_LOG(ERROR) << "Can't open file " << filePath;
        return;
    }

    if (!os.write(data, size)) {
        R_LOG(ERROR) << "Can't write to " << filePath;
        return;
    }

    R_LOG(DEBUG) << "Written " << size << " bytes to " << filePath;
}

template <bool isEncoder>
void DumpHeader(const OMX_BUFFERHEADERTYPE* const header,
                const std::string& filename) {
    const char* const buf =
        reinterpret_cast<char*>(header->pBuffer) + header->nOffset;
    const std::streamsize size =
        static_cast<std::streamsize>(header->nFilledLen);

    DumpBuffer<isEncoder>(buf, size, filename);
}

template void DumpHeader<true>(const OMX_BUFFERHEADERTYPE* const header,
                               const std::string& filename);

template void DumpHeader<false>(const OMX_BUFFERHEADERTYPE* const header,
                                const std::string& filename);

template <bool isEncoder>
void DumpView(const C2GraphicView& view, const std::string& filename) {
    const size_t stride = static_cast<size_t>(Align64(view.width()));
    const size_t vStride = static_cast<size_t>(view.height());
    const char* const src = reinterpret_cast<const char*>(view.data()[0u]);
    const C2PlanarLayout layout = view.layout();
    const std::streamsize size =
        static_cast<std::streamsize>((layout.type == C2PlanarLayout::TYPE_RGB ||
                                      layout.type == C2PlanarLayout::TYPE_RGBA)
                                         ? stride * vStride * 4u
                                         : stride * vStride * 3u / 2u);

    DumpBuffer<isEncoder>(src, size, filename);
}

template void DumpView<true>(const C2GraphicView& view,
                             const std::string& filename);

template void DumpView<false>(const C2GraphicView& view,
                              const std::string& filename);
#endif

} // namespace android::hardware::media::c2::V1_0::renesas
