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
#include "pxr/imaging/hd/renderPass.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRenderPass::HdRenderPass(HdRenderIndex *index,
                           HdRprimCollection const& collection)
    : _renderIndex(index)
{
    SetRprimCollection(collection);
}

HdRenderPass::~HdRenderPass()
{
    /*NOTHING*/
}

void 
HdRenderPass::SetRprimCollection(HdRprimCollection const& col)
{
    if (col == _collection){
        return;
    }
         
    _collection = col; 

    // update dirty list subscription for the new collection.
    // holding shared_ptr for the lifetime of the dirty list.
    bool isMinorChange = true;
    if (!_dirtyList || !_dirtyList->ApplyEdit(col)) {
        _dirtyList.reset(new HdDirtyList(_collection, *_renderIndex));
        isMinorChange = false;
    }

    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
        std::stringstream s;
        s << "  Include: \n";
        for (auto i : col.GetRootPaths()) {
            s << "    - " << i << "\n";
        }
        s << "  Exclude: \n";
        for (auto i : col.GetExcludePaths()) {
            s << "    - " << i << "\n";
        }
        s << "  Repr: " << col.GetReprName() << "\n";
        s << "  Render Tags: \n";
        for (auto i : col.GetRenderTags()) {
            s << "    - " << i << "\n";
        }

        TF_DEBUG(HD_DIRTY_LIST).Msg("RenderPass(%p)::SetRprimCollection (%s) - "
            "constructing new DirtyList(%p) minorChange(%d) \n%s\n",
            (void*)this,
            col.GetName().GetText(),
            (void*)&*_dirtyList,
            isMinorChange,
            s.str().c_str());
    }

    // Mark the collection dirty in derived classes.
    _MarkCollectionDirty();
}

void
HdRenderPass::Execute(HdRenderPassStateSharedPtr const &renderPassState)
{
    TfTokenVector renderTags = {};
    _Execute(renderPassState, renderTags);
}

void
HdRenderPass::Execute(HdRenderPassStateSharedPtr const &renderPassState,
                      TfToken const &renderTag)
{
    TfTokenVector renderTags = { renderTag };
    _Execute(renderPassState, renderTags);
}

void
HdRenderPass::Execute(HdRenderPassStateSharedPtr const &renderPassState,
                      TfTokenVector const &renderTags)
{
    _Execute(renderPassState, renderTags);
}

void
HdRenderPass::Sync()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Sync the dirty list of prims
    _renderIndex->Sync(_dirtyList);

    // Give derived classes a chance to sync.
    _Sync();
}

TfTokenVector const &
HdRenderPass::GetRenderTags()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    return _collection.GetRenderTags();
}

PXR_NAMESPACE_CLOSE_SCOPE
