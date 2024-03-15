//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#include "pxr/usdImaging/plugin/hdUsdWriter/mesh.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/instancer.h"

#include "pxr/usd/usdSkel/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((displayStyleRefineLevel, "displayStyle:refineLevel"))
    ((displayStyleFlatShadingEnabled, "displayStyle:flatShadingEnabled"))
    ((displayStyleDisplacementEnabled, "displayStyle:displacementEnabled"))
    (restPoints)
    (skinningTransforms)
    (skelLocalToWorld)
    (primWorldToLocal)
);
// clang-format on

HdUsdWriterMesh::HdUsdWriterMesh(SdfPath const& id, bool writeExtent)
    : HdUsdWriterPointBased<HdMesh>(id), _writeExtent(writeExtent)
{
}

HdDirtyBits HdUsdWriterMesh::GetInitialDirtyBitsMask() const
{
    const HdDirtyBits mask = HdChangeTracker::Clean | HdChangeTracker::DirtyTopology |
        HdChangeTracker::DirtyDoubleSided |
        HdChangeTracker::DirtyDisplayStyle | HdChangeTracker::DirtySubdivTags |
        _GetInitialDirtyBitsMask();
    return _writeExtent ? mask | HdChangeTracker::DirtyExtent : mask;
}

HdDirtyBits HdUsdWriterMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void HdUsdWriterMesh::Sync(HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    TfToken const& reprToken)
{
    TF_UNUSED(reprToken);

    const auto& id = GetId();
    _Sync(sceneDelegate, id, dirtyBits);

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id))
    {
        if (_topology)
        {
            // When pulling a new topology, we don't want to overwrite the
            // refine level or subdiv tags, which are provided separately by the
            // scene delegate, so we save and restore them.
            const auto subdivTags = _topology->GetSubdivTags();
            const auto refineLevel = _topology->GetRefineLevel();
            _topology = HdMeshTopology(GetMeshTopology(sceneDelegate), refineLevel);
            _topology->SetSubdivTags(subdivTags);
        }
        else
        {
            _topology = GetMeshTopology(sceneDelegate);
        }
    }

    if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id) && _topology)
    {
        _topology->SetSubdivTags(sceneDelegate->GetSubdivTags(id));
    }

    if (HdChangeTracker::IsDoubleSidedDirty(*dirtyBits, id))
    {
        _doubleSided = sceneDelegate->GetDoubleSided(id);
    }

    if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id))
    {
        _displayStyle = sceneDelegate->GetDisplayStyle(id);
    }

    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id) && _writeExtent)
    {
        _extent = sceneDelegate->GetExtent(id);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void HdUsdWriterMesh::SerializeToUsd(const UsdStagePtr &stage)
{
    SdfPath id = HdUsdWriterGetFlattenPrototypePath(GetId());

    bool createOverrideParent = false;
    if (!GetInstancerId().IsEmpty())
    {
        // This is probably an instance prototype path such as instancer1.proto0_cube1_id0
        HdUsdWriterInstancer::GetPrototypePath(id, GetInstancerId(), id);
        // Try to create an override prim for the parent so the prototype meshes don't show up
        createOverrideParent = true;
    }
    auto mesh = UsdGeomMesh::Define(stage, id);
    if (createOverrideParent)
    {
        CreateParentOverride(stage, id);
    }

    if (!mesh)
    {
        return;
    }
    _SerializeToUsd(mesh.GetPrim());

    HdUsdWriterPopOptional(
        _topology,
        [&](const auto& topology)
        {
            mesh.CreateFaceVertexIndicesAttr().Set(topology.GetFaceVertexIndices());
            mesh.CreateFaceVertexCountsAttr().Set(topology.GetFaceVertexCounts());
            mesh.CreateOrientationAttr().Set(topology.GetOrientation());
            mesh.CreateSubdivisionSchemeAttr().Set(topology.GetScheme());
            const auto& subdivTags = topology.GetSubdivTags();
            const auto vertexInterpolationRule = subdivTags.GetVertexInterpolationRule();
            if (!vertexInterpolationRule.IsEmpty())
            {
                mesh.CreateInterpolateBoundaryAttr().Set(vertexInterpolationRule);
            }
            const auto faceVaryingInterpolationRule = subdivTags.GetFaceVaryingInterpolationRule();
            if (!faceVaryingInterpolationRule.IsEmpty())
            {
                mesh.CreateFaceVaryingLinearInterpolationAttr().Set(faceVaryingInterpolationRule);
            }
            const auto triangleSubdivision = subdivTags.GetTriangleSubdivision();
            if (!triangleSubdivision.IsEmpty())
            {
                mesh.CreateTriangleSubdivisionRuleAttr().Set(triangleSubdivision);
            }
            const auto& creaseIndices = subdivTags.GetCreaseIndices();
            if (!creaseIndices.empty())
            {
                mesh.CreateCreaseIndicesAttr().Set(creaseIndices);
            }
            const auto& creaseLengths = subdivTags.GetCreaseLengths();
            if (!creaseLengths.empty())
            {
                mesh.CreateCreaseLengthsAttr().Set(creaseLengths);
            }
            const auto& creaseWeights = subdivTags.GetCreaseWeights();
            if (!creaseWeights.empty())
            {
                mesh.CreateCreaseSharpnessesAttr().Set(creaseWeights);
            }
            const auto& cornerIndices = subdivTags.GetCornerIndices();
            if (!cornerIndices.empty())
            {
                mesh.CreateCornerIndicesAttr().Set(cornerIndices);
            }
            const auto& cornerWeights = subdivTags.GetCornerWeights();
            if (!cornerWeights.empty())
            {
                mesh.CreateCornerSharpnessesAttr().Set(cornerWeights);
            }
            // We are sorting the arrays because there are no requirement for ordering subsets.
            auto subsets = topology.GetGeomSubsets();
            std::sort(subsets.begin(), subsets.end(), [](const auto& a, const auto& b) -> bool { return a.id < b.id; });
            for (const auto& subset : topology.GetGeomSubsets())
            {
                // Subsets has to be direct children of the mesh primitive.
                if (subset.id.GetParentPath() == id)
                {
                    auto usdSubset = UsdGeomSubset::Define(stage, subset.id);
                    if (!usdSubset)
                    {
                        continue;
                    }
                    auto indices = subset.indices;
                    // No requirement for the indices array to be sorted, so scene delegates can return the data in
                    // whatever order they prefer.
                    std::sort(indices.begin(), indices.end());
                    usdSubset.CreateIndicesAttr().Set(subset.indices);
                    HdUsdWriterAssignMaterialToPrim(subset.materialId, usdSubset.GetPrim(), true);
                }
            }
        });
    HdUsdWriterPopOptional(_doubleSided, [&](const auto& doubleSided) { mesh.CreateDoubleSidedAttr().Set(doubleSided); });
    HdUsdWriterPopOptional(_displayStyle,
        [&](const auto& displayStyle)
        {
            auto prim = mesh.GetPrim();
            prim.CreateAttribute(_tokens->displayStyleRefineLevel, SdfValueTypeNames->Int,
                                SdfVariability::SdfVariabilityUniform)
                .Set(displayStyle.refineLevel);
            prim.CreateAttribute(_tokens->displayStyleFlatShadingEnabled, SdfValueTypeNames->Bool,
                                SdfVariability::SdfVariabilityUniform)
                .Set(displayStyle.flatShadingEnabled);
            prim.CreateAttribute(_tokens->displayStyleDisplacementEnabled, SdfValueTypeNames->Bool,
                                SdfVariability::SdfVariabilityUniform)
                .Set(displayStyle.displacementEnabled);
        });
    HdUsdWriterPopOptional(_extent,
        [&](const auto& extent)
        {
            VtVec3fArray extentArray(2);
            extentArray[0] = GfVec3f(extent.GetMin());
            extentArray[1] = GfVec3f(extent.GetMax());
            mesh.CreateExtentAttr().Set(extentArray);
        });
    HdUsdWriterPopOptional(
        _skelGeom,
        [&](const auto& skelGeom)
        {
            auto prim = mesh.GetPrim();

            if (skelGeom.isSkelMesh)
            {
                UsdGeomPrimvarsAPI primvarsAPI(prim);

                prim.CreateAttribute(_tokens->restPoints, SdfValueTypeNames->Vector3fArray).Set(skelGeom.restPoints);

                primvarsAPI
                    .CreatePrimvar(UsdSkelTokens->primvarsSkelGeomBindTransform,
                                   SdfValueTypeNames->Matrix4d, UsdGeomTokens->constant)
                    .Set(VtArray<GfMatrix4d>(1, skelGeom.geomBindingTransform));

                TfToken interpolation =
                    skelGeom.hasConstantInfluences ? UsdGeomTokens->constant : UsdGeomTokens->vertex;

                primvarsAPI
                    .CreatePrimvar(UsdSkelTokens->primvarsSkelJointIndices, SdfValueTypeNames->IntArray,
                                   interpolation, skelGeom.numInfluencesPerPoint)
                    .Set(skelGeom.jointIndices);
                primvarsAPI
                    .CreatePrimvar(UsdSkelTokens->primvarsSkelJointWeights,
                                   SdfValueTypeNames->FloatArray, interpolation, skelGeom.numInfluencesPerPoint)
                    .Set(skelGeom.jointWeights);
            }
            else
            {
                UsdAttribute attr = prim.GetAttribute(_tokens->restPoints);
                if (attr)
                    attr.Block();

                attr = prim.GetAttribute(UsdSkelTokens->primvarsSkelGeomBindTransform);
                if (attr)
                    attr.Block();

                attr = prim.GetAttribute(UsdSkelTokens->primvarsSkelJointIndices);
                if (attr)
                    attr.Block();

                attr = prim.GetAttribute(UsdSkelTokens->primvarsSkelJointWeights);
                if (attr)
                    attr.Block();
            }
        });
    HdUsdWriterPopOptional(_skelAnimXformValues,
        [&](const auto& skelAnimXformValue)
        {
            auto prim = mesh.GetPrim();
            size_t skinningXformsSize = skelAnimXformValue.skinningXforms.size();
            VtMatrix4dArray skinningTransforms(skinningXformsSize);
            for (size_t i = 0; i < skinningXformsSize; i++)
            {
                skinningTransforms[i] = GfMatrix4d(skelAnimXformValue.skinningXforms[i]);
            }

            prim.CreateAttribute(_tokens->skinningTransforms, SdfValueTypeNames->Matrix4dArray)
                .Set(skinningTransforms);
            prim.CreateAttribute(_tokens->skelLocalToWorld, SdfValueTypeNames->Matrix4d)
                .Set(skelAnimXformValue.skelLocalToWorld);
            prim.CreateAttribute(_tokens->primWorldToLocal, SdfValueTypeNames->Matrix4d)
                .Set(skelAnimXformValue.primWorldToLocal);
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
