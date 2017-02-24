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
#ifndef HD_SPRIM_H
#define HD_SPRIM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

/// \class HdSprim
///
/// Sprim (state prim) is a base class of managing state for non-drawable
/// scene entity (e.g. camera, light). Similar to Rprim, Sprim communicates
/// scene delegate and tracks the changes through change tracker, then updates
/// data cached in Hd (either on CPU or GPU).
///
/// Unlike Rprim, Sprim doesn't produce draw items. The data cached in HdSprim
/// may be used by HdTask or by HdShader.
///
/// The lifetime of HdSprim is owned by HdRenderIndex.
///
class HdSprim {
public:
    HdSprim(SdfPath const & id);
    virtual ~HdSprim();

    /// Returns the identifer by which this state is known. This
    /// identifier is a common associative key used by the SceneDelegate,
    /// RenderIndex, and for binding to the state (e.g. camera, light)
    SdfPath const& GetID() const { return _id; }

    /// Synchronizes state from the delegate to this object.
    virtual void Sync(HdSceneDelegate *sceneDelegate) = 0;

    /// Accessor for tasks to get the parameter cached in this sprim object.
    /// Don't communicate back to scene delegate within this function.
    virtual VtValue Get(TfToken const &token) const = 0;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const = 0;

private:
    SdfPath _id;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_SPRIM_H
