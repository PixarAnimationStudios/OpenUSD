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
#ifndef HDST_RENDER_PASS_H
#define HDST_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hd/renderPass.h"

#include <boost/shared_ptr.hpp>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdSt_RenderPass> HdSt_RenderPassSharedPtr;

/// \class HdSt_RenderPass
///
/// A single draw pass to a render target/buffer. Stream implementation.
///
class HdSt_RenderPass : public HdRenderPass {
public:
    HDST_API
    HdSt_RenderPass(HdRenderIndex *index, HdRprimCollection const &collection);
    HDST_API
    virtual ~HdSt_RenderPass();

protected:
    /// Execute the buckets corresponding to renderTags
    virtual void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                          TfTokenVector const &renderTags) override;

    virtual void _MarkCollectionDirty() override;

private:
    void _PrepareCommandBuffer(
            HdStRenderPassStateSharedPtr const &renderPasssState);

    // -----------------------------------------------------------------------
    // Drawing state

    typedef std::unordered_map<TfToken, 
                               HdStCommandBuffer,
                               boost::hash<TfToken> > _HdStCommandBufferMap;
    _HdStCommandBufferMap _cmdBuffers;

    bool _lastCullingDisabledState;

    // -----------------------------------------------------------------------
    // Change tracking state.

    // The version number of the currently held collection.
    int _collectionVersion;

    // A flag indicating that the held collection changed since this renderPass
    // was last drawn.
    //
    // When _collectionChanged is true, it indicates that _collectionVersion is
    // no longer accurate, because _collectionVersion was stored for the
    // previously held collection.
    bool _collectionChanged;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_RENDER_PASS_H
