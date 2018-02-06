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
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/valueUtils.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdReferences
// ------------------------------------------------------------------------- //

namespace
{

bool
_TranslatePath(SdfReference* ref, const UsdEditTarget& editTarget)
{
    // We do not map prim paths across the edit target for non-internal 
    // references, as these paths are supposed to be in the namespace of
    // the referenced layer stack.
    if (!ref->GetAssetPath().empty()) {
        return true;
    }

    // Non-sub-root references aren't expected to be mappable across 
    // non-local edit targets, so we can just use the given reference as-is.
    if (ref->GetPrimPath().IsEmpty() ||
        ref->GetPrimPath().IsRootPrimPath()) {
        return true;
    }

    const SdfPath mappedPath = editTarget.MapToSpecPath(ref->GetPrimPath());
    if (mappedPath.IsEmpty()) {
        TF_CODING_ERROR(
            "Cannot map <%s> to current edit target.", 
            ref->GetPrimPath().GetText());
        return false;
    }

    // If the edit target points inside a variant, the mapped path may 
    // contain a variant selection. We need to strip this out, since
    // inherit paths may not contain variant selections.
    ref->SetPrimPath(mappedPath.StripAllVariantSelections());
    return true;
}

}

bool
UsdReferences::AddReference(const SdfReference& refIn, UsdListPosition position)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    SdfReference ref = refIn;
    if (!_TranslatePath(&ref, _prim.GetStage()->GetEditTarget())) {
        return false;
    }

    SdfChangeBlock block;
    bool success = false;
    {
        TfErrorMark mark;
        if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
            Usd_InsertListItem( spec->GetReferenceList(), ref, position );
            // mark *should* contain only errors from adding the reference, NOT
            // any recomposition errors, because the SdfChangeBlock handily
            // defers composition till after we leave this scope.
            success = mark.IsClean();
        }
    }
    return success;
}

bool
UsdReferences::AddReference(const std::string &assetPath,
                            const SdfPath &primPath,
                            const SdfLayerOffset &layerOffset,
                            UsdListPosition position)
{
    return AddReference(
        SdfReference(assetPath, primPath, layerOffset), position);
}

bool
UsdReferences::AddReference(const std::string &assetPath,
                            const SdfLayerOffset &layerOffset,
                            UsdListPosition position)
{
    return AddReference(assetPath, SdfPath(), layerOffset, position);
}

bool 
UsdReferences::AddInternalReference(const SdfPath &primPath,
                                    const SdfLayerOffset &layerOffset,
                                    UsdListPosition position)
{
    return AddReference(std::string(), primPath, layerOffset, position);
}

bool
UsdReferences::RemoveReference(const SdfReference& refIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    SdfReference ref = refIn;
    if (!_TranslatePath(&ref, _prim.GetStage()->GetEditTarget())) {
        return false;
    }

    SdfChangeBlock block;
    TfErrorMark mark;
    bool success = false;

    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfReferencesProxy refs = spec->GetReferenceList();
        refs.Remove(ref);
        success = mark.IsClean();
    }
    mark.Clear();
    return success;
}

bool
UsdReferences::ClearReferences()
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    SdfChangeBlock block;
    TfErrorMark mark;
    bool success = false;

    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfReferencesProxy refs = spec->GetReferenceList();
        success = refs.ClearEdits() && mark.IsClean();
    }
    mark.Clear();
    return success;
}

bool 
UsdReferences::SetReferences(const SdfReferenceVector& itemsIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    const UsdEditTarget& editTarget = _prim.GetStage()->GetEditTarget();

    TfErrorMark mark;

    SdfReferenceVector items;
    items.reserve(itemsIn.size());
    for (SdfReference item : itemsIn) {
        if (_TranslatePath(&item, editTarget)) {
            items.push_back(item);
        }
    }

    if (!mark.IsClean()) {
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfReferencesProxy refs = spec->GetReferenceList();
        refs.GetExplicitItems() = items;
    }

    bool success = mark.IsClean();
    mark.Clear();
    return success;
}

// ---------------------------------------------------------------------- //
// UsdReferences: Private Methods and Members 
// ---------------------------------------------------------------------- //

SdfPrimSpecHandle
UsdReferences::_CreatePrimSpecForEditing()
{
    if (!TF_VERIFY(_prim)) {
        return SdfPrimSpecHandle();
    }

    return _prim.GetStage()->_CreatePrimSpecForEditing(_prim);
}

PXR_NAMESPACE_CLOSE_SCOPE

