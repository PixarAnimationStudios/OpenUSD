//
// Copyright 2016 Pixar
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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdx/drawTargetRenderPass.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdSt/drawTargetRenderPassState.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"

PXR_NAMESPACE_OPEN_SCOPE

// _renderPass's collection is populated after build time, during Sync().
HdxDrawTargetRenderPass::HdxDrawTargetRenderPass(HdRenderIndex *index)
 : _renderPass(index, HdRprimCollection())
 , _drawTargetRenderPassState(nullptr)
 , _collectionObjectVersion(0)
{
}


HdxDrawTargetRenderPass::~HdxDrawTargetRenderPass() = default;

void
HdxDrawTargetRenderPass::SetDrawTargetRenderPassState(
    const HdStDrawTargetRenderPassState *drawTargetRenderPassState)
{
    _drawTargetRenderPassState = drawTargetRenderPassState;
}

void
HdxDrawTargetRenderPass::SetRprimCollection(HdRprimCollection const& col)
{
    _renderPass.SetRprimCollection(col);
}

void
HdxDrawTargetRenderPass::Sync()
{
    // Update the collection object if necessary.
    unsigned int newCollectionVersion =
        _drawTargetRenderPassState->GetRprimCollectionVersion();

    if (_collectionObjectVersion != newCollectionVersion) {
        SetRprimCollection(_drawTargetRenderPassState->GetRprimCollection());

        _collectionObjectVersion = newCollectionVersion;
    }

    _renderPass.Sync();
}

void
HdxDrawTargetRenderPass::Prepare()
{
}

void
HdxDrawTargetRenderPass::Execute(
    HdRenderPassStateSharedPtr const &renderPassState,
    TfTokenVector const &renderTags)
{
    // XXX: Should the Raster State or Renderpass set and restore this?
    // save the current viewport
    GLint originalViewport[4];
    glGetIntegerv(GL_VIEWPORT, originalViewport);

    // Perform actual draw
    _renderPass.Execute(renderPassState, renderTags);

    // restore viewport
    glViewport(originalViewport[0],
               originalViewport[1],
               originalViewport[2],
               originalViewport[3]);
}

PXR_NAMESPACE_CLOSE_SCOPE

