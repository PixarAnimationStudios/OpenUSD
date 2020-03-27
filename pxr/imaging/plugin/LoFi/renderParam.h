//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#ifndef PXR_IMAGING_PLUGIN_HD_LOFI_RENDER_PARAM_H
#define PXR_IMAGING_PLUGIN_HD_LOFI_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"

#include "pxr/imaging/plugin/LoFi/scene.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class LoFiRenderParam
///
/// The render delegate can create an object of type HdRenderParam, to pass
/// to each prim during Sync(). LoFi uses this class to pass top-level
/// embree state around.
/// 
class LoFiRenderParam final : public HdRenderParam {
public:
    LoFiRenderParam(LoFiScene* scene, HdRenderThread *renderThread,
                        std::atomic<int> *sceneVersion)
        : _scene(scene), _renderThread(renderThread), 
        _sceneVersion(sceneVersion)
        {}
    virtual ~LoFiRenderParam() = default;

    /// Accessor for the top-level embree scene.
    LoFiScene* AcquireSceneForEdit() {
        _renderThread->StopRender();
        (*_sceneVersion)++;
        return _scene;
    }

private:
    /// A handle to the top-level embree scene.
    LoFiScene* _scene;
    /// A handle to the global render thread.
    HdRenderThread *_renderThread;
    /// A version counter for edits to _scene.
    std::atomic<int> *_sceneVersion;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_PARAM_H
