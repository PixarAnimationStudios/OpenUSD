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
#include "usdKatana/baseMaterialHelpers.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/sdf/relationshipSpec.h"

// This tests if a given node represents a "live" base material,
// i.e. once that hasn't been "flattened out" due to being
// pulled across a reference to a library.
static bool
_NodeRepresentsLiveBaseMaterial(const PcpNodeRef &node)
{
    bool isLiveBaseMaterial = false;
    for (PcpNodeRef n = node; n; n = n.GetOriginNode()) {
        switch(n.GetArcType()) {
        case PcpArcTypeLocalSpecializes:
        case PcpArcTypeGlobalSpecializes:
            isLiveBaseMaterial = true;
            break;
        case PcpArcTypeReference:
            if (isLiveBaseMaterial) {
                // Node is within a base material, but that is in turn
                // across a reference. That means this is a library
                // material, so it is not live and we should flatten it
                // out.  Continue iterating, however, since this
                // might be referenced into some other live base material
                // downstream.
                isLiveBaseMaterial = false;
            }
            break;
        default:
            break;
        }
    }
    return isLiveBaseMaterial;
}

bool
PxrUsdKatana_IsAttrValFromBaseMaterial(const UsdAttribute &attr)
{
    return _NodeRepresentsLiveBaseMaterial( attr.GetResolveInfo().GetNode() );
}

bool
PxrUsdKatana_IsPrimDefFromBaseMaterial(const UsdPrim &prim)
{
    for(const PcpNodeRef &n: prim.GetPrimIndex().GetNodeRange()) {
        for (const SdfLayerRefPtr &l: n.GetLayerStack()->GetLayers()) {
            if (SdfPrimSpecHandle p = l->GetPrimAtPath(n.GetPath())) {
                if (SdfIsDefiningSpecifier(p->GetSpecifier())) {
                    return _NodeRepresentsLiveBaseMaterial(n);
                }
            }
        }
    }
    return false;
}

bool
PxrUsdKatana_AreRelTargetsFromBaseMaterial(const UsdRelationship &rel)
{
    // Find the strongest opinion about the relationship targets.
    SdfRelationshipSpecHandle strongestRelSpec;
    SdfPropertySpecHandleVector propStack = rel.GetPropertyStack();
    for (const SdfPropertySpecHandle &prop: propStack) {
        if (SdfRelationshipSpecHandle relSpec =
            TfDynamic_cast<SdfRelationshipSpecHandle>(prop)) {
            if (relSpec->HasTargetPathList()) {
                strongestRelSpec = relSpec;
                break;
            }
        }
    }
    // Find which prim node introduced that opinion.
    if (strongestRelSpec) {
        for(const PcpNodeRef &node:
            rel.GetPrim().GetPrimIndex().GetNodeRange()) {
            if (node.GetPath() == strongestRelSpec->GetPath().GetPrimPath() &&
                node.GetLayerStack()->HasLayer(strongestRelSpec->GetLayer())) {
                return _NodeRepresentsLiveBaseMaterial(node);
            }
        }
    }
    return false;
}
