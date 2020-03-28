//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#ifndef PXR_IMAGING_PLUGIN_LOFI_RENDER_PARAM_H
#define PXR_IMAGING_PLUGIN_LOFI_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"

#include "pxr/imaging/plugin/LoFi/scene.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class LoFiRenderParam
///
class LoFiRenderParam final : public HdRenderParam {
public:
    LoFiRenderParam(LoFiScene* scene): _scene(scene){};

    virtual ~LoFiRenderParam() = default;

    LoFiScene* GetScene() 
    {
        return _scene;
    }

private:
    LoFiScene*          _scene;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_RENDER_PARAM_H
