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

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdReferences
// ------------------------------------------------------------------------- //
bool
UsdReferences::AddReference(const SdfReference& ref, UsdListPosition position)
{
    SdfChangeBlock block;
    bool success = false;
    {
        TfErrorMark mark;
        if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
            SdfReferencesProxy refs = spec->GetReferenceList();
            switch (position) {
            case UsdListPositionFront:
                refs.Prepend(ref);
                break;
            case UsdListPositionBack:
                refs.Append(ref);
                break;
            case UsdListPositionTempDefault:
                if (UsdAuthorOldStyleAdd()) {
                    refs.Add(ref);
                } else {
                    refs.Prepend(ref);
                }
                break;
            }
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
UsdReferences::RemoveReference(const SdfReference& ref)
{
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
UsdReferences::SetReferences(const SdfReferenceVector& items)
{
    SdfChangeBlock block;
    TfErrorMark mark;
    bool success = false;

    // Proxy editor has no clear way of setting explicit items in a single
    // call, so instead, just set the field directly.
    SdfReferenceListOp refs;
    refs.SetExplicitItems(items);
    success = GetPrim().SetMetadata(SdfFieldKeys->References, refs) && 
        mark.IsClean();

    mark.Clear();
    return success;
}

// ---------------------------------------------------------------------- //
// UsdReferences: Private Methods and Members 
// ---------------------------------------------------------------------- //

SdfPrimSpecHandle
UsdReferences::_CreatePrimSpecForEditing()
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim.");
        return SdfPrimSpecHandle();
    }

    return _prim.GetStage()->_CreatePrimSpecForEditing(_prim);
}

PXR_NAMESPACE_CLOSE_SCOPE

