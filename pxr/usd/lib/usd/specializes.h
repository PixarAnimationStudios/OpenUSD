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
#ifndef USD_SPECIALIZES_H
#define USD_SPECIALIZES_H

#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

SDF_DECLARE_HANDLES(SdfPrimSpec);

/// \class UsdSpecializes
///
/// A proxy class for applying listOp edits to the specializes list for a
/// prim.
///
class UsdSpecializes {
    friend class UsdPrim;

    explicit UsdSpecializes(const UsdPrim& prim) : _prim(prim) {}

public:

    /// Add a path to the specializes listOp at the current EditTarget.
    USD_API
    bool Add(const SdfPath &primPath);

    /// Removes the specified path from the specializes listOp at the
    /// current EditTarget.
    USD_API
    bool Remove(const SdfPath &primPath);

    /// Removes the authored specializes listOp edits at the current edit
    /// target.
    USD_API
    bool Clear();

    /// Explicitly set specializes paths, potentially blocking weaker opinions
    /// that add or remove items, returning true on success, false if the edit
    /// could not be performed.
    USD_API
    bool SetItems(const SdfPathVector& items);

    /// Return the prim this object is bound to.
    const UsdPrim &GetPrim() const { return _prim; }
    UsdPrim GetPrim() { return _prim; }

    // TODO: use safe bool idiom
    operator bool() { return bool(_prim); }

    // ---------------------------------------------------------------------- //
    // Private Methods and Members
    // ---------------------------------------------------------------------- //
private:

    SdfPrimSpecHandle _CreatePrimSpecForEditing();
    UsdPrim _prim;
};

#endif //USD_SPECIALIZES_H
