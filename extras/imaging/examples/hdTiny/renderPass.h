//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXTRAS_IMAGING_EXAMPLES_HD_TINY_RENDER_PASS_H
#define EXTRAS_IMAGING_EXAMPLES_HD_TINY_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdTinyRenderPass
///
/// HdRenderPass represents a single render iteration, rendering a view of the
/// scene (the HdRprimCollection) for a specific viewer (the camera/viewport
/// parameters in HdRenderPassState) to the current draw target.
///
class HdTinyRenderPass final : public HdRenderPass 
{
public:
    /// Renderpass constructor.
    ///   \param index The render index containing scene data to render.
    ///   \param collection The initial rprim collection for this renderpass.
    HdTinyRenderPass(HdRenderIndex *index,
                       HdRprimCollection const &collection);

    /// Renderpass destructor.
    virtual ~HdTinyRenderPass();

protected:

    /// Draw the scene with the bound renderpass state.
    ///   \param renderPassState Input parameters (including viewer parameters)
    ///                          for this renderpass.
    ///   \param renderTags Which rendertags should be drawn this pass.
    void _Execute(
        HdRenderPassStateSharedPtr const& renderPassState,
        TfTokenVector const &renderTags) override;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXTRAS_IMAGING_EXAMPLES_HD_TINY_RENDER_PASS_H
