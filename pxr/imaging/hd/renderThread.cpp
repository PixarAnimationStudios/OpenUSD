//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRenderThread::HdRenderThread()
    : _renderCallback(_DefaultRenderCallback)
    , _shutdownCallback(_DefaultShutdownCallback)
    , _requestedState(StateInitial)
    , _stopRequested(false)
    , _pauseRender(false)
    , _pauseDirty(false)
    , _rendering(false)
{
}

HdRenderThread::~HdRenderThread()
{
    StopThread();
}

void
HdRenderThread::SetRenderCallback(std::function<void()> renderCallback)
{
    _renderCallback = renderCallback;
}

void
HdRenderThread::SetShutdownCallback(std::function<void()> shutdownCallback)
{
    _shutdownCallback = shutdownCallback;
}

void
HdRenderThread::StartThread()
{
    if (_renderThread.joinable()) {
        TF_CODING_ERROR("StartThread() called while render thread is "
                        "already running");
        return;
    }

    _requestedState = StateIdle;
    _renderThread = std::thread(&HdRenderThread::_RenderLoop, this);
}

void
HdRenderThread::StopThread()
{
    if (!_renderThread.joinable()) {
        return;
    }

    {
        _enableRender.clear();
        std::unique_lock<std::mutex> lock(_requestedStateMutex);
        _requestedState = StateTerminated;
        _rendering.store(false);
        _requestedStateCV.notify_one();
    }
    _renderThread.join();
}

bool
HdRenderThread::IsThreadRunning()
{
    return _renderThread.joinable();
}

void
HdRenderThread::StartRender()
{
    if (!IsRendering()) {
        std::unique_lock<std::mutex> lock(_requestedStateMutex);
        _enableRender.test_and_set();
        _requestedState = StateRendering;
        _rendering.store(true);
        _requestedStateCV.notify_one();
    }
}

void
HdRenderThread::StopRender()
{
    if (IsRendering()) {
        _enableRender.clear();
        std::unique_lock<std::mutex> lock(_requestedStateMutex);
        _requestedState = StateIdle;
        _rendering.store(false);
    }
}

bool
HdRenderThread::IsRendering()
{
    return _rendering.load();
}

void
HdRenderThread::PauseRender()
{
    _pauseDirty.store(true);
    _pauseRender.store(true);
}

void
HdRenderThread::ResumeRender()
{
    _pauseDirty.store(true);
    _pauseRender.store(false);
}

bool
HdRenderThread::IsStopRequested()
{
    if (!_enableRender.test_and_set()) {
        _stopRequested = true;
    }

    return _stopRequested;
}

bool
HdRenderThread::IsPauseRequested()
{
    return _pauseRender.load();
}

bool
HdRenderThread::IsPauseDirty() {
    return _pauseDirty.exchange(false);
}

std::unique_lock<std::mutex>
HdRenderThread::LockFramebuffer()
{
    return std::unique_lock<std::mutex>(_frameBufferMutex);
}

void
HdRenderThread::_RenderLoop()
{
    while (1) {
        std::unique_lock<std::mutex> lock(_requestedStateMutex);
        _requestedStateCV.wait(lock, [this]() {
            return _requestedState != StateIdle;
        });
        if (_requestedState == StateRendering) {
            _renderCallback();
            _stopRequested = false;
            _rendering.store(false);
            _requestedState = StateIdle;
        }
        else if (_requestedState == StateTerminated) {
            break;
        }
    }
    _shutdownCallback();
}

/*static*/
void
HdRenderThread::_DefaultRenderCallback()
{
    TF_CODING_ERROR("StartThread() called without a render callback set");
}

/*static*/
void
HdRenderThread::_DefaultShutdownCallback()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

