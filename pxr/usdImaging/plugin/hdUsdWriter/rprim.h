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

#ifndef HD_USD_WRRITER_RPRIM_H
#define HD_USD_WRRITER_RPRIM_H

#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/instancer.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"
#include "pxr/pxr.h"

#include "pxr/base/js/json.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdUsdWriterRprim
///
/// Base class to handle common Rprim serialization.
template <typename BASE>
class HdUsdWriterRprim : public BASE
{
public:
    /// HdUsdWriterRprim constructor.
    ///   \param id The scene-graph path to this Rprim.
    HdUsdWriterRprim(const SdfPath& id) : BASE(id)
    {
    }

protected:

    /// Serialize the primitive to USD.
    ///
    /// Pass a function or lambda to take over handling primvars from HdUsdWriterRprim.
    /// The function takes a single const reference to a HdUsdWriterPrimvar, like:
    ///
    /// [](const HdUsdWriterPrimvar& primvar) -> bool {
    ///     if (primvar.descriptor.name == HdTokens->points) {
    ///         // Handle points primvar.
    ///         return true;
    ///     } else {
    ///         return false;
    ///     }
    /// }
    ///
    ///   \tparam F Function type
    ///   \param prim Reference to the Usd primitive.
    ///   \param f Function to take over handling primvars.
    template <typename F>
    void _SerializeToUsd(const UsdPrim& prim, F&& f)
    {
        if (!prim)
        {
            return;
        }
        HdUsdWriterPopOptional(_transform,
            [&](const auto& transform)
            {
                HdUsdWriterSetTransformOp(UsdGeomXformable(prim), transform);
            });

        if (!_primvars.empty())
        {
            UsdGeomPrimvarsAPI primvarsAPI(prim);
            for (const auto& primvar : _primvars)
            {
                // Ignoring the primvar and allowing the calling function to handle the value.
                if (f(primvar))
                {
                    continue;
                }
                if (!SdfValueHasValidType(primvar.value))
                {
                    continue;
                }
                const auto sdfType = SdfGetValueTypeNameForValue(primvar.value);
                const auto interpolation = HdUsdWriterGetTokenFromHdInterpolation(primvar.descriptor.interpolation);
                auto pv = primvarsAPI.CreatePrimvar(primvar.descriptor.name, sdfType, interpolation);
                pv.Set(primvar.value);
                // TODO: Add support for writing indexed primvars, currently we always write flattened.
                if (primvar.descriptor.interpolation == HdInterpolationFaceVarying && primvar.value.IsArrayValued())
                {
                    const auto arraySize = primvar.value.GetArraySize();
                    VtIntArray indices(arraySize);
                    std::iota(indices.begin(), indices.end(), 0);
                    pv.SetIndices(indices);
                }
            }
            _primvars.clear();
        }

        HdUsdWriterSetVisible(_visible, prim);

        HdUsdWriterPopOptional(_materialId,
            [&](const auto& materialId)
            {
                // How should we handle zeroing out the material?
                HdUsdWriterAssignMaterialToPrim(materialId, prim, true);
            });

        HdUsdWriterPopOptional(_renderTag,
            [&](const auto& renderTag)
            {
                UsdGeomImageable imageable(prim);
                // the geometry render tag should translate to "default"
                if (renderTag == HdRenderTagTokens->geometry)
                {
                    imageable.CreatePurposeAttr().Set(UsdGeomTokens->default_);
                }
                else
                {
                    imageable.CreatePurposeAttr().Set(renderTag);
                }
            });
    }

    // Initialize the given representation of this Rprim.
    // See HdRprim::InitRepr()
    void _InitRepr(const TfToken& reprToken, HdDirtyBits* dirtyBits) override
    {
        TF_UNUSED(reprToken);
        TF_UNUSED(dirtyBits);
    }

    /// Propagates dirty bits.
    /// See HdRprim::_PropagateDirtyBits.
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override
    {
        return bits;
    }

    /// Get the initial list of dirty bits handled by this base class.
    ///   \return Initial list of dirty bits.
    HdDirtyBits _GetInitialDirtyBitsMask() const
    {
        return HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyVisibility | HdChangeTracker::DirtyMaterialId;
    }

    /// Update the render tag.
    ///
    /// This function is called by the HdRenderIndex.
    ///
    ///   \param sceneDelegate Pointer to the Hydra scene delegate.
    ///   \param renderParam Hydra render parameter.
    void UpdateRenderTag(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam) override
    {
        // The HdRprim class already has a render tag attribute, which needs to be set, otherwise the render index skips
        // the primitive.
        BASE::_renderTag = sceneDelegate->GetRenderTag(BASE::GetId());
        _renderTag = BASE::_renderTag;
    }

    /// Sync dirty bits available to every rprim, like transform, instances etc.
    ///   \param sceneDelegate Pointer to the Hydra scene delegate.
    ///   \param id Reference to the SdfPath of the primitive.
    ///   \param dirtyBits Pointer to the Hydra dirty bits of the changes.
    void _Sync(HdSceneDelegate* sceneDelegate, const SdfPath& id, HdDirtyBits* dirtyBits)
    {
        BASE::_UpdateInstancer(sceneDelegate, dirtyBits);
        // Some _Sync calls will arrive with an instance selection as variant selection
        const auto instancerId = BASE::GetInstancerId().StripAllVariantSelections();
        if (sceneDelegate->GetRenderIndex().GetInstancer(instancerId))
        {
            HdInstancer::_SyncInstancerAndParents(sceneDelegate->GetRenderIndex(), instancerId);
        }

        // TODO: We could use IsInstancerDirty and IsInstancerIndexDirty.
        if (!instancerId.IsEmpty())
        {
            // Retrieve instance transforms from the instancer.
            auto& renderIndex = sceneDelegate->GetRenderIndex();
            auto* instancer = static_cast<HdUsdWriterInstancer*>(renderIndex.GetInstancer(instancerId));
            if (instancer != nullptr)
            {
                instancer->AddInstancedPrim(id);
            }
        }

        if (HdChangeTracker::IsTransformDirty(*dirtyBits, id))
        {
            _transform = sceneDelegate->GetTransform(id);
        }

        if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id))
        {
            _primvars.clear();
            for (auto interpolation : { HdInterpolationConstant, HdInterpolationUniform, HdInterpolationVarying,
                                        HdInterpolationVertex, HdInterpolationFaceVarying, HdInterpolationInstance })
            {
                for (const auto& primvarDescriptor : sceneDelegate->GetPrimvarDescriptors(id, interpolation))
                {
                    _primvars.emplace_back(primvarDescriptor, sceneDelegate->Get(id, primvarDescriptor.name));
                }
            }
            /// Sorting interpolations by type and name.
            std::sort(_primvars.begin(), _primvars.end(),
                      [](const auto& a, const auto& b) -> bool { return a.descriptor.name < b.descriptor.name; });
        }

        if (*dirtyBits & HdChangeTracker::DirtyMaterialId)
        {
            _materialId = sceneDelegate->GetMaterialId(id);
        }

        if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id))
        {
            _visible = sceneDelegate->GetVisible(id);
        }
    }

private:
    HdUsdWriterOptional<GfMatrix4d> _transform;
    HdUsdWriterOptional<VtMatrix4dArray> _instanceTransforms;
    std::vector<HdUsdWriterPrimvar> _primvars;
    HdUsdWriterOptional<TfToken> _renderTag;
    HdUsdWriterOptional<SdfPath> _materialId;
    HdUsdWriterOptional<bool> _visible;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif