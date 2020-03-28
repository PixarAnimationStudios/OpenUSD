//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_RENDER_PASS_STATE_H
#define PXR_IMAGING_PLUGIN_LOFI_RENDER_PASS_STATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/resourceRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdCamera;

/// \class LoFiRenderPassState
///
/// A set of rendering parameters used among render passes.
///
/// Parameters are expressed as GL states, uniforms or shaders.
///
class LoFiRenderPassState : public HdRenderPassState {
public:
    LoFiRenderPassState();
    virtual ~LoFiRenderPassState();

    /// Schedule to update renderPassState parameters.
    /// e.g. camera matrix, override color, id blend factor.
    /// Prepare, called once per frame after the sync phase, but prior to
    /// the commit phase.
    virtual void Prepare(HdResourceRegistrySharedPtr const &resourceRegistry);

    // Bind, called once per frame before drawing.
    HD_API
    virtual void Bind();

    // Unbind, called once per frame after drawing.
    HD_API
    virtual void Unbind();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_PLUGIN_LOFI_RENDER_PASS_STATE_H
