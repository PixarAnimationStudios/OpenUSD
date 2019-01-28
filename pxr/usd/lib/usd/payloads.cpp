//
// Copyright 2019 Pixar
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
#include "pxr/usd/usd/payloads.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/valueUtils.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdPayloads
// ------------------------------------------------------------------------- //

namespace
{

bool
_TranslatePath(SdfPayload* payload, const UsdEditTarget& editTarget)
{
    // We do not map prim paths across the edit target for non-internal 
    // payloads, as these paths are supposed to be in the namespace of
    // the payload layer stack.
    if (!payload->GetAssetPath().empty()) {
        return true;
    }

    // Non-sub-root payloads aren't expected to be mappable across 
    // non-local edit targets, so we can just use the given payload as-is.
    if (payload->GetPrimPath().IsEmpty() ||
        payload->GetPrimPath().IsRootPrimPath()) {
        return true;
    }

    const SdfPath mappedPath = editTarget.MapToSpecPath(payload->GetPrimPath());
    if (mappedPath.IsEmpty()) {
        TF_CODING_ERROR(
            "Cannot map <%s> to current edit target.", 
            payload->GetPrimPath().GetText());
        return false;
    }

    // If the edit target points inside a variant, the mapped path may 
    // contain a variant selection. We need to strip this out, since
    // inherit paths may not contain variant selections.
    payload->SetPrimPath(mappedPath.StripAllVariantSelections());
    return true;
}

}

bool
UsdPayloads::AddPayload(const SdfPayload& refIn, UsdListPosition position)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    SdfPayload payload = refIn;
    if (!_TranslatePath(&payload, _prim.GetStage()->GetEditTarget())) {
        return false;
    }

    SdfChangeBlock block;
    bool success = false;
    {
        TfErrorMark mark;
        if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
            Usd_InsertListItem( spec->GetPayloadList(), payload, position );
            // mark *should* contain only errors from adding the payload, NOT
            // any recomposition errors, because the SdfChangeBlock handily
            // defers composition till after we leave this scope.
            success = mark.IsClean();
        }
    }
    return success;
}

bool
UsdPayloads::AddPayload(const std::string &assetPath,
                        const SdfPath &primPath,
                        const SdfLayerOffset &layerOffset,
                        UsdListPosition position)
{
    return AddPayload(
        SdfPayload(assetPath, primPath, layerOffset), position);
}

bool
UsdPayloads::AddPayload(const std::string &assetPath,
                        const SdfLayerOffset &layerOffset,
                        UsdListPosition position)
{
    return AddPayload(assetPath, SdfPath(), layerOffset, position);
}

bool 
UsdPayloads::AddInternalPayload(const SdfPath &primPath,
                                const SdfLayerOffset &layerOffset,
                                UsdListPosition position)
{
    return AddPayload(std::string(), primPath, layerOffset, position);
}


bool
UsdPayloads::RemovePayload(const SdfPayload& refIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    SdfPayload payload = refIn;
    if (!_TranslatePath(&payload, _prim.GetStage()->GetEditTarget())) {
        return false;
    }

    SdfChangeBlock block;
    TfErrorMark mark;
    bool success = false;

    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfPayloadsProxy payloads = spec->GetPayloadList();
        payloads.Remove(payload);
        success = mark.IsClean();
    }
    mark.Clear();
    return success;
}

bool
UsdPayloads::ClearPayloads()
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    SdfChangeBlock block;
    TfErrorMark mark;
    bool success = false;

    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfPayloadsProxy payloads = spec->GetPayloadList();
        success = payloads.ClearEdits() && mark.IsClean();
    }
    mark.Clear();
    return success;
}

bool 
UsdPayloads::SetPayloads(const SdfPayloadVector& itemsIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    const UsdEditTarget& editTarget = _prim.GetStage()->GetEditTarget();

    TfErrorMark mark;

    SdfPayloadVector items;
    items.reserve(itemsIn.size());
    for (SdfPayload item : itemsIn) {
        if (_TranslatePath(&item, editTarget)) {
            items.push_back(item);
        }
    }

    if (!mark.IsClean()) {
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfPayloadsProxy payloads = spec->GetPayloadList();
        payloads.GetExplicitItems() = items;
    }

    bool success = mark.IsClean();
    mark.Clear();
    return success;
}

// ---------------------------------------------------------------------- //
// UsdPayloads: Private Methods and Members 
// ---------------------------------------------------------------------- //

SdfPrimSpecHandle
UsdPayloads::_CreatePrimSpecForEditing()
{
    if (!TF_VERIFY(_prim)) {
        return SdfPrimSpecHandle();
    }

    return _prim.GetStage()->_CreatePrimSpecForEditing(_prim);
}

PXR_NAMESPACE_CLOSE_SCOPE

