//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/drawTarget.h"

PXR_NAMESPACE_OPEN_SCOPE

class LoFiRenderer;
/// \class LoFiRenderPass
///
class LoFiRenderPass final : public HdRenderPass 
{
public:
    /// Renderpass constructor.
    ///   \param index The render index containing scene data to render.
    ///   \param collection The initial rprim collection for this renderpass.
    LoFiRenderPass( HdRenderIndex *index,
                    HdRprimCollection const &collection,
                    LoFiRenderer* renderer);

    /// Renderpass destructor.
    virtual ~LoFiRenderPass();

protected:

    /// Setup the framebuffer with color and depth attachments
    ///   \param width The width of the framebuffer
    ///   \param height The height of the framebuffer
    void _SetupDrawTarget(int width, int height);

    /// Draw the scene with the bound renderpass state.
    ///   \param renderPassState Input parameters (including viewer parameters)
    ///                          for this renderpass.
    ///   \param renderTags Which rendertags should be drawn this pass.
    void _Execute(HdRenderPassStateSharedPtr const& renderPassState,
                  TfTokenVector const &renderTags) override;

private:
  LoFiRenderer*             _renderer;
  GlfDrawTargetRefPtr       _drawTarget;

};

PXR_NAMESPACE_CLOSE_SCOPE
