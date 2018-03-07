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
#ifndef HD_BPRIM_H
#define HD_BPRIM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdRenderParam;

/// \class HdBprim
///
/// Bprim (buffer prim) is a base class of managing a blob of data that is
/// used to communicate between the scene delegate and render.
///
/// Like other prim types (Rprim and Sprim), the Bprim communicates with the
/// scene delegate got get buffer properties (e.g. the size of the buffer) as
/// well as the contents of the buffer.
///
/// Changes to the properties and contents are change tracked and updates
/// are cached in the renderer.  The Render Delegate may choose to transform
/// the data into a renderer specific form on download.
///
/// BPrims are sync'ed first and thus, Bprims should not be
/// Dependent on the state of any other prim.
///
/// The most typical use of a Bprim would be a Texture.
class HdBprim {
public:
    HD_API
    HdBprim(SdfPath const & id);
    HD_API
    virtual ~HdBprim();

    /// Returns the identifier by which this buffer is known. This
    /// identifier is a common associative key used by the SceneDelegate,
    /// RenderIndex, and for binding to the buffer
    SdfPath const& GetID() const { return _id; }

    /// Synchronizes state from the delegate to this object.
    /// @param[in, out]  dirtyBits: On input specifies which state is
    ///                             is dirty and can be pulled from the scene
    ///                             delegate.
    ///                             On output specifies which bits are still
    ///                             dirty and were not cleaned by the sync. 
    ///                             
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) = 0;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const = 0;

private:
    SdfPath _id;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_BPRIM_H
