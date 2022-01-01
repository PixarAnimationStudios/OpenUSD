//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/plugin/LoFi/renderer.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

LoFiRenderPass::LoFiRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection,
    LoFiRenderer* renderer)
    : HdRenderPass(index, collection)
{
  _renderer = renderer;
}

LoFiRenderPass::~LoFiRenderPass()
{
   
}

void
LoFiRenderPass::_SetupDrawTarget(int width, int height)
{
  GfVec2i attachmentSize(width,height);
  GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(attachmentSize);

  // create initial attachments
  drawTarget->Bind();
  drawTarget->AddAttachment(
      "color", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
  drawTarget->AddAttachment(
      "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);
  drawTarget->Unbind();
}

void
LoFiRenderPass::_Execute( HdRenderPassStateSharedPtr const& renderPassState,
                          TfTokenVector const &renderTags)
{
  _renderer->SetCamera(
      renderPassState->GetWorldToViewMatrix(), 
      renderPassState->GetProjectionMatrix()
  );
  _renderer->Render();
}

PXR_NAMESPACE_CLOSE_SCOPE
