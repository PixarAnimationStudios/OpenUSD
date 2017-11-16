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

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdInherits
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

    // Global inherits aren't expected to be mappable across non-local 
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
UsdInherits::AddInherit(const SdfPath &primPathIn, UsdListPosition position)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(_prim).c_str());
        return false;
    }

    const SdfPath primPath = 
        _TranslatePath(primPathIn, _prim.GetStage()->GetEditTarget());
    if (primPath.IsEmpty()) {
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfInheritsProxy inhs = spec->GetInheritPathList();
        switch (position) {
        case UsdListPositionFront:
            inhs.Prepend(primPath);
            break;
        case UsdListPositionBack:
            inhs.Append(primPath);
            break;
        case UsdListPositionTempDefault:
            if (UsdAuthorOldStyleAdd()) {
                inhs.Add(primPath);
            } else {
                inhs.Prepend(primPath);
            }
            break;
        }
        return true;
    }
    return false;
}

bool
UsdInherits::RemoveInherit(const SdfPath &primPathIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(_prim).c_str());
        return false;
    }

    const SdfPath primPath = 
        _TranslatePath(primPathIn, _prim.GetStage()->GetEditTarget());
    if (primPath.IsEmpty()) {
        return false;
    }

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
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(_prim).c_str());
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfInheritsProxy inhs = spec->GetInheritPathList();
        return inhs.ClearEdits();
    }
    return false;
}

bool 
UsdInherits::SetInherits(const SdfPathVector& itemsIn)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(_prim).c_str());
        return false;
    }

    const UsdEditTarget& editTarget = _prim.GetStage()->GetEditTarget();

    TfErrorMark m;

    SdfPathVector items(itemsIn.size());
    std::transform(
        itemsIn.begin(), itemsIn.end(), items.begin(),
        [&editTarget](const SdfPath& path) {
            return _TranslatePath(path, editTarget);
        });

    if (!m.IsClean()) {
        return false;
    }

    SdfChangeBlock block;
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing()) {
        SdfInheritsProxy paths = spec->GetInheritPathList();
        paths.GetExplicitItems() = items;
    }

    return m.IsClean();
}


SdfPathVector
UsdInherits::GetAllDirectInherits() const
{
    SdfPathVector ret;
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(_prim).c_str());
        return ret;
    }

    std::unordered_set<SdfPath, SdfPath::Hash> seen;
    for (auto const &node:
             _prim.GetPrimIndex().GetNodeRange(PcpRangeTypeAllInherits)) {
        if (!node.IsDueToAncestor() && seen.insert(node.GetPath()).second) {
            ret.push_back(node.GetPath());
        }
    }
    return ret;
}

// ---------------------------------------------------------------------- //
// UsdInherits: Private Methods and Members 
// ---------------------------------------------------------------------- //

SdfPrimSpecHandle
UsdInherits::_CreatePrimSpecForEditing()
{
    if (!TF_VERIFY(_prim)) {
        return SdfPrimSpecHandle();
    }

    return _prim.GetStage()->_CreatePrimSpecForEditing(_prim);
}

PXR_NAMESPACE_CLOSE_SCOPE

