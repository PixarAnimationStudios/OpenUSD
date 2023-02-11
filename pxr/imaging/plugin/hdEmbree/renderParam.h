//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_PARAM_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"

#include <embree4/rtcore.h>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdEmbreeRenderParam
///
/// The render delegate can create an object of type HdRenderParam, to pass
/// to each prim during Sync(). HdEmbree uses this class to pass top-level
/// embree state around.
/// 
class HdEmbreeRenderParam final : public HdRenderParam
{
public:
    HdEmbreeRenderParam(RTCDevice device, RTCScene scene,
                        HdRenderThread *renderThread,
                        std::atomic<int> *sceneVersion)
        : _scene(scene), _device(device)
        , _renderThread(renderThread), _sceneVersion(sceneVersion)
    {}

    /// Accessor for the top-level embree scene.
    RTCScene AcquireSceneForEdit() {
        _renderThread->StopRender();
        (*_sceneVersion)++;
        return _scene;
    }
    /// Accessor for the top-level embree device (library handle).
    RTCDevice GetEmbreeDevice() { return _device; }

private:
    /// A handle to the top-level embree scene.
    RTCScene _scene;
    /// A handle to the top-level embree device (library handle).
    RTCDevice _device;
    /// A handle to the global render thread.
    HdRenderThread *_renderThread;
    /// A version counter for edits to _scene.
    std::atomic<int> *_sceneVersion;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_PARAM_H
