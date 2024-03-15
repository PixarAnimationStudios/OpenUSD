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

#include "pxr/usdImaging/plugin/hdUsdWriter/curves.h"

#include "pxr/usd/usdGeom/basisCurves.h"

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((displayStyleRefineLevel, "displayStyle:refineLevel"))
    ((displayStyleFlatShadingEnabled, "displayStyle:flatShadingEnabled"))
    ((displayStyleDisplacementEnabled, "displayStyle:displacementEnabled"))
);
// clang-format on

HdUsdWriterBasisCurves::HdUsdWriterBasisCurves(SdfPath const& id) : HdUsdWriterPointBased<HdBasisCurves>(id)
{
}

HdDirtyBits HdUsdWriterBasisCurves::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::Clean | HdChangeTracker::DirtyTopology |
        HdChangeTracker::DirtyDisplayStyle | _GetInitialDirtyBitsMask();
}

void HdUsdWriterBasisCurves::Sync(HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    TfToken const& reprToken)
{
    TF_UNUSED(reprToken);

    const auto& id = GetId();
    _Sync(sceneDelegate, id, dirtyBits);

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id))
    {
        _topology = GetBasisCurvesTopology(sceneDelegate);
    }

    if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id))
    {
        _displayStyle = sceneDelegate->GetDisplayStyle(id);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void HdUsdWriterBasisCurves::SerializeToUsd(const UsdStagePtr &stage)
{
    auto curves = UsdGeomBasisCurves::Define(stage, GetId());
    HdUsdWriterRprim<HdBasisCurves>::_SerializeToUsd(
        curves.GetPrim(),
        [&curves](const HdUsdWriterPrimvar& primvar) -> auto
        {
            if (primvar.descriptor.name == HdTokens->widths)
            {
                if (primvar.value.IsHolding<VtFloatArray>())
                {
                    const auto widths = curves.CreateWidthsAttr();
                    widths.Set(primvar.value.UncheckedGet<VtFloatArray>());
                    widths.SetMetadata(
                        UsdGeomTokens->interpolation, HdUsdWriterGetTokenFromHdInterpolation(primvar.descriptor.interpolation));
                }
                return true;
            }
            return HdUsdWriterPointBased<HdBasisCurves>::_HandlePointBasedPrimvars(curves, primvar);
        });

    HdUsdWriterPopOptional(_topology,
        [&](const auto& topology)
        {
            curves.CreateCurveVertexCountsAttr().Set(topology.GetCurveVertexCounts());
            curves.CreateTypeAttr().Set(topology.GetCurveType());
            curves.CreateWrapAttr().Set(topology.GetCurveWrap());
            curves.CreateBasisAttr().Set(topology.GetCurveBasis());
        });
    HdUsdWriterPopOptional(_displayStyle,
        [&](const auto& displayStyle)
        {
            auto prim = curves.GetPrim();
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
}

PXR_NAMESPACE_CLOSE_SCOPE
