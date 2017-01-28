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

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/commandBuffer.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/task.h"

#include <boost/shared_ptr.hpp>

#include <unordered_map>

class HdCommandBuffer;
class HdRenderIndex;
class HdSceneDelegate;


typedef boost::shared_ptr<class HdDirtyList> HdDirtyListSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;

/// \class HdRenderPass
///
/// A single draw pass to a render target/buffer.
///
class HdRenderPass : boost::noncopyable {
public:
    HdRenderPass(HdRenderIndex *index);
    HdRenderPass(HdRenderIndex *index, const HdRprimCollection &collection);
    virtual ~HdRenderPass();

    /// Returns the HdRprimCollection to be drawn by this RenderPass.
    HdRprimCollection const& GetRprimCollection() const { return _collection; }

    /// Sets the HdRprimCollection, note that this may invalidate internal
    /// caches used to accelerate drawing.
    void SetRprimCollection(HdRprimCollection const& col);

    /// Returns the dirty list (maintained in the change tracker) for
    /// efficient traversal
    HdDirtyListSharedPtr const &GetDirtyList() const {
        return _dirtyList;
    }

    /// Returns the most recent list of render tags that this render pass
    /// has found in the render items included in the collection.
    TfTokenVector const &GetRenderTags();

    /// Execute render pass task
    void Execute(HdRenderPassStateSharedPtr const &renderPassState);

    /// Execute a specific render bucket specified by the 
    /// render tag.
    void Execute(HdRenderPassStateSharedPtr const &renderPassState,
                 TfToken const &renderTag);

    /// Sync the render pass resources
    void Sync();

    /// Return the render index
    HdRenderIndex * const GetRenderIndex() const { return _renderIndex; }

private:
    void _PrepareCommandBuffer(HdRenderPassStateSharedPtr const &renderPasssState);

    // ---------------------------------------------------------------------- //
    // Change Tracking State
    // ---------------------------------------------------------------------- //
    // The renderIndex to which this renderPass belongs
    // (can't change after construction)
    HdRenderIndex * const _renderIndex;

    // The version number of the currently held collection.
    int _collectionVersion;

    // A flag indicating that the held collection changed since this renderPass
    // was last drawn.
    //
    // When _collectionChanged is true, it indicates that _collectionVersion is
    // no longer accurate, because _collectionVersion was stored for the
    // previously held collection.
    bool _collectionChanged;

    // cached dirty prims list
    HdDirtyListSharedPtr _dirtyList;

    // ---------------------------------------------------------------------- //
    // Core RenderPass State
    // ---------------------------------------------------------------------- //
    HdRprimCollection _collection;
    typedef std::unordered_map<TfToken, 
                               HdCommandBuffer,
                               boost::hash<TfToken> > _HdCommandBufferMap;
    _HdCommandBufferMap _cmdBuffers;
    TfTokenVector _renderTags;

    bool _lastCullingDisabledState;

};

#endif //HD_RENDER_PASS_H
