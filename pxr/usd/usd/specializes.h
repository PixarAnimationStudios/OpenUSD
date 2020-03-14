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
#ifndef PXR_USD_USD_SPECIALIZES_H
#define PXR_USD_USD_SPECIALIZES_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfPrimSpec);

/// \class UsdSpecializes
///
/// A proxy class for applying listOp edits to the specializes list for a
/// prim.
///
/// All paths passed to the UsdSpecializes API are expected to be in the 
/// namespace of the owning prim's stage. Subroot prim specializes paths  
/// will be translated from this namespace to the namespace of the current 
/// edit target, if necessary. If a path cannot be translated, a coding error 
/// will be issued and no changes will be made. Root prim specializes paths 
/// will not be translated.
///
class UsdSpecializes {
    friend class UsdPrim;

    explicit UsdSpecializes(const UsdPrim& prim) : _prim(prim) {}

public:

    /// Adds a path to the specializes listOp at the current EditTarget,
    /// in the position specified by \p position.
    USD_API
    bool AddSpecialize(const SdfPath &primPath,
               UsdListPosition position=UsdListPositionBackOfPrependList);

    /// Removes the specified path from the specializes listOp at the
    /// current EditTarget.
    USD_API
    bool RemoveSpecialize(const SdfPath &primPath);

    /// Removes the authored specializes listOp edits at the current edit
    /// target.
    USD_API
    bool ClearSpecializes();

    /// Explicitly set specializes paths, potentially blocking weaker opinions
    /// that add or remove items, returning true on success, false if the edit
    /// could not be performed.
    USD_API
    bool SetSpecializes(const SdfPathVector& items);

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

#endif //PXR_USD_USD_SPECIALIZES_H
