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
#ifndef HD_RENDER_PASS_H
#define HD_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/task.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdSceneDelegate;

typedef std::shared_ptr<class HdDirtyList> HdDirtyListSharedPtr;
typedef std::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef std::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;

/// \class HdRenderPass
///
/// A single draw pass to a render target/buffer.
///
class HdRenderPass : boost::noncopyable {
public:
    HD_API
    HdRenderPass(HdRenderIndex *index, HdRprimCollection const& collection);
    HD_API
    virtual ~HdRenderPass();

    /// Returns the HdRprimCollection to be drawn by this RenderPass.
    HdRprimCollection const& GetRprimCollection() const { return _collection; }

    /// Sets the HdRprimCollection, note that this may invalidate internal
    /// caches used to accelerate drawing.
    HD_API
    void SetRprimCollection(HdRprimCollection const& col);

    /// Returns the dirty list (maintained in the change tracker) for
    /// efficient traversal
    HdDirtyListSharedPtr const &GetDirtyList() const {
        return _dirtyList;
    }

    /// Returns the most recent list of render tags that this render pass
    /// has found in the render items included in the collection.
    HD_API
    TfTokenVector const &GetRenderTags();

    /// Sync the render pass resources
    HD_API
    void Sync();

    /// Return the render index
    HdRenderIndex * const GetRenderIndex() const { return _renderIndex; }

    /// Execute all of the buckets in this renderpass.
    HD_API
    void Execute(HdRenderPassStateSharedPtr const &renderPassState);
    /// Execute a subset of buckets of this renderpass.
    HD_API
    void Execute(HdRenderPassStateSharedPtr const &renderPassState,
                 TfTokenVector const &renderTags);
    /// Execute a single bucket of this renderpass.
    HD_API
    void Execute(HdRenderPassStateSharedPtr const &renderPassState,
                 TfToken const &renderTag);

    /// Optional API: Hooks for progressive rendering.
    virtual void ResetImage() {}
    virtual bool IsConverged() const { return true; }

protected:
    /// Virtual API: Execute the buckets corresponding to renderTags;
    /// renderTags.empty() implies execute everything.
    virtual void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                         TfTokenVector const &renderTags) = 0;

    /// Optional API: let derived classes mark their collection tracking as dirty.
    virtual void _MarkCollectionDirty() {}

    /// Optional API: let derived classes sync data.
    virtual void _Sync() {}

private:
    // ---------------------------------------------------------------------- //
    // Change Tracking State
    // ---------------------------------------------------------------------- //
    // The renderIndex to which this renderPass belongs
    // (can't change after construction)
    HdRenderIndex * const _renderIndex;

    // cached dirty prims list
    HdDirtyListSharedPtr _dirtyList;

    // ---------------------------------------------------------------------- //
    // Core RenderPass State
    // ---------------------------------------------------------------------- //
    HdRprimCollection _collection;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RENDER_PASS_H
