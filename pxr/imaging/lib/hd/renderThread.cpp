//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRenderThread::HdRenderThread()
    : _renderCallback(_DefaultRenderCallback)
    , _requestedState(StateInitial)
    , _stopRequested(false)
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

bool
HdRenderThread::IsStopRequested()
{
    if (!_enableRender.test_and_set()) {
        _stopRequested = true;
    }

    return _stopRequested;
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
}

/*static*/
void
HdRenderThread::_DefaultRenderCallback()
{
    TF_CODING_ERROR("StartThread() called without a render callback set");
}

PXR_NAMESPACE_CLOSE_SCOPE

