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
#include "pxr/usd/usdGeom/constraintTarget.h"

#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/modelAPI.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    
    (constraintTargets)
    (constraintTargetIdentifier)
);

UsdGeomConstraintTarget::UsdGeomConstraintTarget(const UsdAttribute &attr)
    : _attr(attr)
{
}

/* static */
bool 
UsdGeomConstraintTarget::IsValid(const UsdAttribute &attr)
{
    if (!attr)
        return false;

    static TfType matrix4dType = TfType::Find<GfMatrix4d>();

    return UsdModelAPI(attr.GetPrim()).IsModel() /* is this a model */

        /* is it in the constraintTargets namespace */
        && attr.SplitName().front() == _tokens->constraintTargets

        /* is it matrix-typed */
        && attr.GetTypeName().GetType() == matrix4dType;
}

bool
UsdGeomConstraintTarget::Get(GfMatrix4d* value, UsdTimeCode time) const
{
    return _attr.Get(value, time);
}

bool
UsdGeomConstraintTarget::Set(const GfMatrix4d& value, UsdTimeCode time) const
{
    return _attr.Set(value, time);
}

TfToken
UsdGeomConstraintTarget::GetIdentifier() const
{
    TfToken result;
    if (_attr) {
        if (_attr.GetMetadata(_tokens->constraintTargetIdentifier, &result)) {
            return result;
        }
    }

    return result;
}

void
UsdGeomConstraintTarget::SetIdentifier(const TfToken &identifier)
{
    if (_attr) {
        _attr.SetMetadata(_tokens->constraintTargetIdentifier, identifier);
    }
}

/* static */
TfToken 
UsdGeomConstraintTarget::GetConstraintAttrName(
    const std::string &constraintName)
{
    return TfToken(_tokens->constraintTargets.GetString() + ":" + 
                   constraintName);
}

GfMatrix4d 
UsdGeomConstraintTarget::ComputeInWorldSpace(
    UsdTimeCode time,
    UsdGeomXformCache *xfCache) const
{
    if (!IsDefined()) {
        TF_CODING_ERROR("Invalid constraint target.");
        return GfMatrix4d(1);
    }

    const UsdPrim &modelPrim = GetAttr().GetPrim();

    GfMatrix4d localToWorld(1);
    if (xfCache) {
        xfCache->SetTime(time);
        localToWorld = xfCache->GetLocalToWorldTransform(modelPrim);
    } else {
        UsdGeomXformCache cache;
        cache.SetTime(time);
        localToWorld = cache.GetLocalToWorldTransform(modelPrim);
    }

    GfMatrix4d localConstraintSpace(1.);
    if (!Get(&localConstraintSpace, time)) {
        TF_WARN("Failed to get value of constraint target '%s' at path <%s>.",
                GetIdentifier().GetText(), GetAttr().GetPath().GetText());
        return localConstraintSpace;
    }
    
    return localConstraintSpace * localToWorld;
}

PXR_NAMESPACE_CLOSE_SCOPE

