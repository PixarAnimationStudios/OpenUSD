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

#ifndef HD_USD_WRITER_POINT_BASED_H
#define HD_USD_WRITER_POINT_BASED_H

#include "pxr/usdImaging/plugin/hdUsdWriter/rprim.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"
#include "pxr/pxr.h"

#include "pxr/usd/usdGeom/pointBased.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename BASE>
class HdUsdWriterPointBased : public HdUsdWriterRprim<BASE>
{
public:
    /// HdUsdWriterPointBased constructor.
    ///   \param id The scene-graph path to this PointBased Rprim.
    HdUsdWriterPointBased(const SdfPath& id) : HdUsdWriterRprim<BASE>(id)
    {
    }

protected:
    /// Get the initial list of dirty bits handled by this base class.
    ///   \return Initial list of dirty bits.
    HdDirtyBits _GetInitialDirtyBitsMask() const
    {
        return HdUsdWriterRprim<BASE>::_GetInitialDirtyBitsMask() | HdChangeTracker::DirtyPoints |
            HdChangeTracker::DirtyNormals;
    }

    /// Handle primvars specific to UsdGeomPointBased primitives.
    ///
    /// Takes over handling of points, velocities, accelerations and normals.
    ///
    ///   \param points UsdGeomPointBased primitive.
    ///   \param primvar Primvar descriptor and value.
    ///   \return Whether or not the function handles the primvar.
    static bool _HandlePointBasedPrimvars(UsdGeomPointBased& points, const HdUsdWriterPrimvar& primvar)
    {
        if (primvar.descriptor.name == HdTokens->points)
        {
            if (primvar.value.IsHolding<VtVec3fArray>())
            {
                points.CreatePointsAttr().Set(primvar.value.UncheckedGet<VtVec3fArray>());
            }
        }
        else if (primvar.descriptor.name == HdTokens->velocities)
        {
            if (primvar.value.IsHolding<VtVec3fArray>())
            {
                points.CreateVelocitiesAttr().Set(primvar.value.UncheckedGet<VtVec3fArray>());
            }
        }
        else if (primvar.descriptor.name == HdTokens->accelerations)
        {
            if (primvar.value.IsHolding<VtVec3fArray>())
            {
                points.CreateAccelerationsAttr().Set(primvar.value.UncheckedGet<VtVec3fArray>());
            }
        }
        else if (primvar.descriptor.name == HdTokens->normals)
        {
            if (primvar.value.IsHolding<VtVec3fArray>())
            {
                points.CreateNormalsAttr().Set(primvar.value.UncheckedGet<VtVec3fArray>());
                if (primvar.descriptor.interpolation == HdInterpolation::HdInterpolationVertex)
                {
                    points.SetNormalsInterpolation(UsdGeomTokens->vertex);
                }
                else if (primvar.descriptor.interpolation == HdInterpolation::HdInterpolationUniform)
                {
                    points.SetNormalsInterpolation(UsdGeomTokens->uniform);
                }
                else if (primvar.descriptor.interpolation == HdInterpolation::HdInterpolationFaceVarying)
                {
                    points.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
                }
            }
        }
        else
        {
            return false;
        }
        return true;
    }

    /// Serialize the primitive to USD.
    ///
    ///   \param prim Reference to the Usd primitive.
    void _SerializeToUsd(const UsdPrim& prim)
    {
        UsdGeomPointBased points(prim);
        HdUsdWriterRprim<BASE>::_SerializeToUsd(
            prim, [&points](const auto& primvar) -> auto { return _HandlePointBasedPrimvars(points, primvar); });
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif