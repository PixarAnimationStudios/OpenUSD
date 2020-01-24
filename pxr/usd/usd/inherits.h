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
#ifndef PXR_USD_USD_INHERITS_H
#define PXR_USD_USD_INHERITS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfPrimSpec);

/// \class UsdInherits
///
/// A proxy class for applying listOp edits to the inherit paths list for a
/// prim.
///
/// All paths passed to the UsdInherits API are expected to be in the 
/// namespace of the owning prim's stage. Subroot prim inherit paths
/// will be translated from this namespace to the  namespace of the current
/// edit target, if necessary. If a path cannot be translated, a coding error 
/// will be issued and no changes will be made. Root prim inherit paths will 
/// not be translated.
///
class UsdInherits {
    friend class UsdPrim;

    explicit UsdInherits(const UsdPrim& prim) : _prim(prim) {}

public:
    /// Adds a path to the inheritPaths listOp at the current EditTarget,
    /// in the position specified by \p position.
    USD_API
    bool AddInherit(const SdfPath &primPath,
                    UsdListPosition position=UsdListPositionBackOfPrependList);

    /// Removes the specified path from the inheritPaths listOp at the
    /// current EditTarget.
    USD_API
    bool RemoveInherit(const SdfPath &primPath);

    /// Removes the authored inheritPaths listOp edits at the current edit
    /// target.
    USD_API
    bool ClearInherits();

    /// Explicitly set the inherited paths, potentially blocking weaker opinions
    /// that add or remove items, returning true on success, false if the edit
    /// could not be performed.
    USD_API
    bool SetInherits(const SdfPathVector& items);

    /// Return all the paths in this prim's stage's local layer stack that would
    /// compose into this prim via direct inherits (excluding prim specs that
    /// would be composed into this prim due to inherits authored on ancestral
    /// prims) in strong-to-weak order.
    ///
    /// Note that there currently may not be any scene description at these
    /// paths on the stage.  This returns all the potential places that such
    /// opinions could appear.
    USD_API
    SdfPathVector GetAllDirectInherits() const;
    
    /// Return the prim this object is bound to.
    const UsdPrim &GetPrim() const { return _prim; }
    UsdPrim GetPrim() { return _prim; }

    explicit operator bool() { return bool(_prim); }

    // ---------------------------------------------------------------------- //
    // Private Methods and Members
    // ---------------------------------------------------------------------- //
private:

    UsdPrim _prim;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INHERITS_H
