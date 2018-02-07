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
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/valueUtils.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdSpecializes
// ------------------------------------------------------------------------- //

namespace
{

SdfPath 
_TranslatePath(const SdfPath& path, const UsdEditTarget& editTarget)
{
    if (path.IsEmpty()) {
        TF_CODING_ERROR("Invalid empty path");
        return SdfPath();
    }

    // Global specializes aren't expected to be mappable across non-local 
    // edit targets, so we can just use the given path as-is.
    if (path.IsRootPrimPath()) {
        return path;
    }

    const SdfPath mappedPath = editTarget.MapToSpecPath(path);
    if (mappedPath.IsEmpty()) {
        TF_CODING_ERROR(
            "Cannot map <%s> to current edit target.", path.GetText());
    }

    // If the edit target points inside a variant, the mapped path may 
    // contain a variant selection. We need to strip this out, since
    // inherit paths may not contain variant selections.
    return mappedPath.StripAllVariantSelections();
}

}

bool
UsdSpecializes::AddSpecialize(const SdfPath &primPathIn, 
                              UsdListPosition position)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    const SdfPath primPath = 
        _TranslatePath(primPathIn, _prim.GetStage()->GetEditTarget());
    if (primPath.IsEmpty()) {
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        Usd_InsertListItem( spec->GetSpecializesList(), primPath, position );
        return true;
    }
    return false;
}

bool
UsdSpecializes::RemoveSpecialize(const SdfPath &primPathIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    const SdfPath primPath = 
        _TranslatePath(primPathIn, _prim.GetStage()->GetEditTarget());
    if (primPath.IsEmpty()) {
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfSpecializesProxy paths = spec->GetSpecializesList();
        paths.Remove(primPath);
        return true;
    }
    return false;
}

bool
UsdSpecializes::ClearSpecializes()
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfSpecializesProxy paths = spec->GetSpecializesList();
        return paths.ClearEdits();
    }
    return false;
}

bool 
UsdSpecializes::SetSpecializes(const SdfPathVector& itemsIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    const UsdEditTarget& editTarget = _prim.GetStage()->GetEditTarget();

    TfErrorMark m;

    SdfPathVector items(itemsIn.size());
    std::transform(
        itemsIn.begin(), itemsIn.end(), items.begin(),
        [&editTarget](const SdfPath& p) { 
            return _TranslatePath(p, editTarget); 
        });

    if (!m.IsClean()) {
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfSpecializesProxy paths = spec->GetSpecializesList();
        paths.GetExplicitItems() = items;
    }

    return m.IsClean();
}

// ---------------------------------------------------------------------- //
// UsdSpecializes: Private Methods and Members 
// ---------------------------------------------------------------------- //

SdfPrimSpecHandle
UsdSpecializes::_CreatePrimSpecForEditing()
{
    if (!TF_VERIFY(_prim)) {
        return SdfPrimSpecHandle();
    }

    return _prim.GetStage()->_CreatePrimSpecForEditing(_prim);
}

PXR_NAMESPACE_CLOSE_SCOPE

