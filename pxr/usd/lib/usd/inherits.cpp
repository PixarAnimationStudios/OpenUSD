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
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdInherits
// ------------------------------------------------------------------------- //
bool
UsdInherits::AppendInherit(const SdfPath &primPath)
{
    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfInheritsProxy inhs = spec->GetInheritPathList();
        if (UsdAuthorAppendAsAdd()) {
            inhs.Add(primPath);
        } else {
            inhs.Append(primPath);
        }
        return true;
    }
    return false;
}

bool
UsdInherits::RemoveInherit(const SdfPath &primPath)
{
    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfInheritsProxy inhs = spec->GetInheritPathList();
        inhs.Remove(primPath);
        return true;
    }
    return false;
}

bool
UsdInherits::ClearInherits()
{
    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfInheritsProxy inhs = spec->GetInheritPathList();
        return inhs.ClearEdits();
    }
    return false;
}

bool 
UsdInherits::SetInherits(const SdfPathVector& items)
{
    // Proxy editor has no clear way of setting explicit items in a single
    // call, so instead, just set the field directly.
    SdfPathListOp paths;
    paths.SetExplicitItems(items);
    return GetPrim().SetMetadata(SdfFieldKeys->InheritPaths, paths);
}

// ---------------------------------------------------------------------- //
// UsdInherits: Private Methods and Members 
// ---------------------------------------------------------------------- //

SdfPrimSpecHandle
UsdInherits::_CreatePrimSpecForEditing()
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim.");
        return SdfPrimSpecHandle();
    }

    return _prim.GetStage()->_CreatePrimSpecForEditing(_prim);
}

PXR_NAMESPACE_CLOSE_SCOPE

