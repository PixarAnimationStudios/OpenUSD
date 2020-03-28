//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

class LoFiScene;
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
                    LoFiScene* scene,
                    LoFiRenderer* renderer);

    /// Renderpass destructor.
    virtual ~LoFiRenderPass();

protected:

    /// Draw the scene with the bound renderpass state.
    ///   \param renderPassState Input parameters (including viewer parameters)
    ///                          for this renderpass.
    ///   \param renderTags Which rendertags should be drawn this pass.
    void _Execute(HdRenderPassStateSharedPtr const& renderPassState,
                  TfTokenVector const &renderTags) override;

private:
  LoFiScene*    _scene;
  LoFiRenderer* _renderer;

};

PXR_NAMESPACE_CLOSE_SCOPE
