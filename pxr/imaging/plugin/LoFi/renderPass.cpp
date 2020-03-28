//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/plugin/LoFi/renderPassState.h"
#include "pxr/imaging/plugin/LoFi/renderer.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

LoFiRenderPass::LoFiRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection,
    LoFiScene* scene,
    LoFiRenderer* renderer)
    : HdRenderPass(index, collection)
{
  _scene = scene;
  _renderer = renderer;
}

LoFiRenderPass::~LoFiRenderPass()
{
    //std::cout << "Destroying renderPass" << std::endl;
}

void
LoFiRenderPass::_Execute( HdRenderPassStateSharedPtr const& renderPassState,
                          TfTokenVector const &renderTags)
{
  LoFiRenderPassState* state = 
    reinterpret_cast<LoFiRenderPassState*>(renderPassState.get());

   // std::cout << "=> Execute RenderPass" << std::endl;
   _renderer->SetCamera(state->GetWorldToViewMatrix(),
                        state->GetProjectionMatrix());
   _renderer->Render();
}

PXR_NAMESPACE_CLOSE_SCOPE
