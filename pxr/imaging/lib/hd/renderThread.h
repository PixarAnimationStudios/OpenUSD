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
#ifndef HD_RENDER_THREAD_H
#define HD_RENDER_THREAD_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include <atomic>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdRenderThread
///
/// HdRenderThread is a utility that specific render delegates can choose to
/// use depending on their needs.  It provides a system for rendering in a
/// background thread, and synchronizing between hydra (either in the main
/// thread, or the sync threadpool) and the rendering thread.
///
/// \section HdRenderThread_StateMachine State Machine
///
/// The render thread is implemented in terms of a state machine, and hydra
/// requests to the render thread are implemented in terms of transitions on
/// that state machine.
///
/// States:
/// - \em StateInitial - indicates the render thread hasn't been started.
/// - \em StateIdle - indicates the render thread is running, but not rendering.
/// - \em StateRendering - indicates the render thread is rendering.
/// - \em StateTerminated - indicates the render thread is shutting down.
///
/// Transitions:
/// - StartThread():  StateInitial => StateIdle
/// - StartRender():  StateIdle, StateRendering => StateRendering
/// - StopRender():   StateIdle, StateRendering => StateIdle
/// - StopThread():   StateIdle, StateRendering => StateTerminated
/// - StopThread():   StateTerminated => StateInitial
///
/// \section HdRenderThread_Usage Example Usage
///
/// \code{.cpp}
/// class ExampleRenderDelegate : HdRenderDelegate {
///   public:
///     ExampleRenderDelegate() {
///       _renderThread.SetRenderCallback(
///         std::bind(&ExampleRenderDelegate::_RenderCallback, this));
///       _renderThread.StartThread();
///     }
///     ~ExampleRenderDelegate() {
///       _renderThread.StopThread();
///     }
///   private:
///     void _RenderCallback() {
///       while(!_renderThread.IsStopRequested()) {
///         // generate N pixels.
///         auto lock = _renderThread.LockFramebuffer();
///         // resolve pixels to shared buffer.
///       }
///     }
///     HdRenderThread _renderThread;
/// };
///
/// class ExampleRenderParam : HdRenderParam {
///   public:
///     ExampleRenderParam(HdRenderThread* renderThread, SceneData *scene);
///     SceneData* AcquireSceneForEdit() {
///       _renderThread->StopRender();
///       return _scene;
///     }
/// };
///
/// class ExamplePrim : HdMesh {
///   public:
///     void Sync(...) {
///       SceneData *scene = renderParam->AcquireSceneForEdit();
///       ...
///     }
/// };
///
/// class ExampleRenderPass : HdRenderPass {
///   public:
///     ExampleRenderPass(HdRenderThread *renderThread);
///   protected:
///     void _Execute(...) {
///       _renderThread->StartRendering();
///       auto lock = _renderThread->LockFramebuffer();
///       // blit pixels from shared to application buffer.
///     }
/// };
/// \endcode
///
/// Having a locked and shared framebuffer is important if you want to avoid
/// tearing, or if the rendering API disallows multithreaded access to buffers
/// (for example, if your framebuffers are on a GPU). It might be unnecessary
/// for some renderers.
///
/// Stopping the render only when you're about to make a scene edit means that
/// long-running renders aren't interrupted if the scene is static. Hiding the
/// renderer's scene data handle behind AcquireSceneForEdit helps callers
/// use the synchronization mechanisms correctly.
///
/// The render is restarted at the last possible second, in the render pass,
/// after we know scene edits are done.
///
/// The render callback should use IsStopRequested() as a cancellation
/// mechanism.
///
class HdRenderThread {
public:

    HD_API
    HdRenderThread();

    HD_API
    ~HdRenderThread();

    /// \anchor Management
    /// \name API for thread management
    ///
    /// Methods to configure, start, and stop the render thread. These functions
    /// are not threadsafe.
    ///
    /// @{

    /// Set the rendering callback for the render thread to use.
    HD_API
    void SetRenderCallback(std::function<void()> renderCallback);

    /// Start the rendering background thread.
    /// Note: it's an error to call this function when the render thread is
    /// already running, but it's acceptable to stop the render thread and then
    /// start it again.
    HD_API
    void StartThread();

    /// Stop the rendering background thread. This function will ask the render
    /// thread to transition to StateTerminated, and then join on the thread,
    /// so it will block.  After this function returns, the rendering state
    /// machine will be back in its initial state, and the render thread can be
    /// started again.
    HD_API
    void StopThread();

    /// Check whether the background thread is running (i.e. StartThread was
    /// called successfully, but StopThread has not been).
    HD_API
    bool IsThreadRunning();

    /// @}

    /// \anchor HydraAPI
    /// \name API for hydra threads
    ///
    /// Methods for hydra to communicate with the render thread. These methods
    /// can be called from the application thread or hydra threadpool threads.
    ///
    /// @{

    /// Ask the render thread to start rendering.  This call is a no-op if the
    /// render thread is already rendering. Otherwise, it may block briefly.
    /// This is threadsafe against the render thread, but it shouldn't be
    /// called at the same time as StopRender(), and it shouldn't be called
    /// from multiple hydra threads at once.
    HD_API
    void StartRender();

    /// Ask the render thread to stop rendering, and block until the render
    /// thread is idle. This is fully threadsafe, and can be called from
    /// multiple hydra threads at once.
    HD_API
    void StopRender();

    /// Query whether the render thread is currently rendering. This is set by
    /// StartRender() and reset after the render callback exits, or reset by
    /// StopRender() if the render callback never runs. This does not
    /// block, and is fully threadsafe.
    HD_API
    bool IsRendering();

    /// @}

    /// \anchor RenderThreadAPI
    /// \name API for hydra threads
    ///
    /// Methods for the render thread to communicate with hydra. These should
    /// only be called from the render thread, from inside the render callback.
    ///
    /// @{

    /// Query whether hydra has asked to interrupt the current frame since
    /// the last time StartRender() was called. The render callback can check
    /// this to determine whether to cancel rendering.
    HD_API
    bool IsStopRequested();

    /// @}

    /// \anchor CommonAPI
    /// \name API for both hydra and render threads
    ///
    /// Methods for both hydra and the render threads to synchronize access to
    /// other data.
    ///
    /// @{

    /// Return a scoped lock on the render delegate's framebuffer. Hydra and the
    /// render thread can use this to synchronize blits between render-thread
    /// owned resources, and application-owned resources.
    HD_API
    std::unique_lock<std::mutex> LockFramebuffer();

    /// @}

private:
    // _RenderLoop implements the render thread's state machine; see
    // \ref HdRenderThread_StateMachine for details.  It runs in a background
    // thread and manages synchronization with hydra.  To implement rendering,
    // it calls out to the render callback provided via SetRenderCallback.
    void _RenderLoop();

    // _renderCallback is the render-delegate-provided function responsible for
    // actually rendering.  It's called from _RenderLoop.
    std::function<void()> _renderCallback;

    // A placeholder initial value for _renderCallback.
    static void _DefaultRenderCallback();

    // The state enumeration of the render thread state machine; see
    // \ref HdRenderThread_StateMachine for details.
    enum State {
        // Initial constructed state. Render thread is not running.
        StateInitial,
        // Render thread is running and ready for scene edits. No rendering
        // is taking place.
        StateIdle,
        // Render thread is running and rendering; no scene edits are allowed.
        StateRendering,
        // Render thread is shutting down.
        StateTerminated,
    };

    // _requestedState is set by hydra to direct the render thread's state
    // machine; see \ref HdRenderThread_StateMachine for details.
    // _requestedState is protected by a mutex/condition variable combination.
    // The render thread holds _requestedStateMutex while rendering; the
    // frequency with which it can give it up is the interruption frequency.
    //
    // StartRender() and StopRender() lock and write to _requestedState.
    // _RenderLoop() locks and reads _requestedState.
    State _requestedState;
    std::mutex _requestedStateMutex;
    std::condition_variable _requestedStateCV;

    // _enableRender provides an out-of-band way for hydra to cancel a
    // render while the render thread is still holding _requestedStateMutex.
    //
    // StartRender() and StopRender() will write true/false to _enableRender.
    // IsStopRequested() will read from _enableRender.
    std::atomic_flag _enableRender;
    // _stopRequested keeps track of whether _enableRender has gone low since
    // the last time the render callback was called (since _enableRender is
    // reset on read).
    bool _stopRequested;

    // _rendering records whether the render thread is currently inside the
    // render callback, or planning to be inside the render callback.
    // It is managed by StartRender(), StopRender(), and _RenderLoop().
    // IsRendering() will read from _rendering.
    std::atomic<bool> _rendering;

    // _renderThread is the background render thread; it runs _RenderLoop().
    std::thread _renderThread;

    // _frameBufferMutex protects access to the render delegate's framebuffer,
    // and provides an optional synchronization point for blits between the
    // render thread's resources and the application's resources.
    std::mutex _frameBufferMutex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_RENDER_THREAD_H
