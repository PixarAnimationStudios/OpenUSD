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

#ifndef HD_USD_WRITER_UTILS_H
#define HD_USD_WRITER_UTILS_H

#include "pxr/base/js/json.h"
#include "pxr/base/vt/array.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include <optional>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct HdUsdWriterPrimvar
{
    HdUsdWriterPrimvar(HdPrimvarDescriptor _descriptor, VtValue _value) :
        descriptor(_descriptor),
        value(_value) {}

    HdPrimvarDescriptor descriptor;
    VtValue value;
};

template <typename T>
using HdUsdWriterOptional = std::optional<T>;
constexpr auto HdUsdWriterNone = std::nullopt;

/// Get value from the scene delegate and write it into an optional value.
///
///   \tparam T Type stored in the optional value.
///   \param sceneDelegate Pointer to the Hydra Scene Delegate.
///   \param id Path to the primitive in the scene graph.
///   \param paramName Name of the parameter to query.
///   \param out Output value to store the optional value.
template <typename T>
HdUsdWriterOptional<T> HdUsdWriterGet(HdSceneDelegate* sceneDelegate, const SdfPath& id, const TfToken& paramName)
{
    const auto value = sceneDelegate->Get(id, paramName);
    return !value.IsEmpty() && value.IsHolding<T>() ? 
       std::make_optional(value.UncheckedGet<T>()) :
       HdUsdWriterNone;
}

/// Get camera param value from the scene delegate and write it into an optional value.
///
///   \tparam T Type stored in the optional value.
///   \param sceneDelegate Pointer to the Hydra Scene Delegate.
///   \param id Path to the primitive in the scene graph.
///   \param paramName Name of the parameter to query.
///   \param out Output value to store the optional value.
template <typename T>
HdUsdWriterOptional<T> HdUsdWriterGetCameraParamValue(HdSceneDelegate* sceneDelegate,
                               const SdfPath& id,
                               const TfToken& paramName)
{
    const auto value = sceneDelegate->GetCameraParamValue(id, paramName);
    return !value.IsEmpty() && value.IsHolding<T>() ? 
       std::make_optional(value.UncheckedGet<T>()) :
       HdUsdWriterNone;
}


/// Pop optional value and pass it to a function.
///
/// Sets the optional value to HdUsdWriterNone if it held a value previously.
///
/// \tparam T Type stored in the optional value.
/// \tparam F Type of the function that operations on the optional value.
/// \param optional Optional value.
/// \param f Function that receives optional value as its only parameter.
template <typename T, typename F>
void HdUsdWriterPopOptional(HdUsdWriterOptional<T>& optional, F&& f)
{
    if (optional)
    {
        f(*optional);
        optional = HdUsdWriterNone;
    }
}

/// Get a prim at a given path.
///
///   \tparam SchemaType Type of the primitive.
///   \param path Path to the primitive.
///   \return The primitive in the stage, either concrete or an override.
template <typename SchemaType>
inline SchemaType GetPrimAtPath(UsdStagePtr stage, const SdfPath& path)
{
    return SchemaType::Get(stage, path);
}

/// @brief Create an override parent if the parent is not the root.
/// @param stage 
/// @param path Path to the primitive.
inline void CreateParentOverride(UsdStagePtr stage, const SdfPath& path)
{
    SdfPath parent = path.GetParentPath();
    if (parent != SdfPath::AbsoluteRootPath())
    {
        stage->GetPrimAtPath(parent).SetSpecifier(SdfSpecifierOver);
    }
}

/// Get UsdGeom interpolation token from HdInterpolation.
///
///   \param interpolation HdInterpolation from Hydra.
///   \return UsdGeom TfToken of the HdInterpolation.
inline TfToken HdUsdWriterGetTokenFromHdInterpolation(const HdInterpolation interpolation)
{
    if (interpolation == HdInterpolationUniform)
    {
        return UsdGeomTokens->uniform;
    }
    else if (interpolation == HdInterpolationVarying)
    {
        return UsdGeomTokens->varying;
    }
    else if (interpolation == HdInterpolationVertex || interpolation == HdInterpolationInstance)
    {
        return UsdGeomTokens->vertex;
    }
    else if (interpolation == HdInterpolationFaceVarying)
    {
        return UsdGeomTokens->faceVarying;
    }
    else if (interpolation == HdInterpolationConstant)
    {        
        return UsdGeomTokens->constant;
    }
    TF_WARN("Unknown HdInterpolation: %i", interpolation);
    return UsdGeomTokens->constant;
}

template <typename T>
void HdUsdWriterSetOrWarn(const UsdAttribute& attr, const T& value)
{
    if (!attr.Set(value))
    {
        TF_WARN("Failed to set '%s'", attr.GetPath().GetAsString().c_str());
    }
}

inline SdfPath HdUsdWriterGetFlattenPrototypePath(const SdfPath& path)
{
    auto p = SdfPath(path);
    static const std::string prototypePrefix = "/__Prototype_";
    static const size_t prototypePrefixLength = prototypePrefix.size();
    static const std::string flattenPrototypePrefix = "/Flattened_Prototype_";
    if (p.ContainsPrimVariantSelection())
    {
        // This can happen for some instance paths, we don't want these here
        p = p.StripAllVariantSelections();
    }

    if (TfStringStartsWith(p.GetAsString(), prototypePrefix))
    {
        return SdfPath(flattenPrototypePrefix + p.GetAsString().substr(prototypePrefixLength));
    }
    return p;
}

inline void HdUsdWriterSetTransformOp(const UsdGeomXformable& xform, GfMatrix4d transform)
{
    auto transformOp = xform.MakeMatrixXform();
    transformOp.Set(transform);
}

/// Assing material to a primitive.
///
/// \param materialId Path to the material.
/// \param prim Usd Primitive.
/// \param unbindIfEmptyId Whether or not to unbind if the materialId is empty.
inline void HdUsdWriterAssignMaterialToPrim(const SdfPath& materialId, const UsdPrim& prim, bool unbindIfEmptyId)
{
    // If the materialId is empty, we check to see if there is an existing material binding and remove it, without
    // changing the prim if no materials are assigned to the prim or the material binding relationship is already
    // empty.
    if (materialId.IsEmpty() && unbindIfEmptyId)
    {
        UsdShadeMaterialBindingAPI bindingAPI(prim);
        if (bindingAPI)
        {
            if (!bindingAPI.GetDirectBinding().GetMaterialPath().IsEmpty())
            {
                bindingAPI.UnbindDirectBinding();
            }
        }
    }
    else
    {
        // Calling bind requires the UsdShadeMaterial to be initialized before the primitive.
        // Calling GetDirectBindingRel() creates the binding relationship with the "custom" tag.
        // So as a workaround we are manually creating the relationship for now.
        // TODO: Create the primitives in a different loop, so we can use the Bind function
        // with the material existing in the scene.
        UsdShadeMaterialBindingAPI::Apply(prim);
        if (!prim.CreateRelationship(UsdShadeTokens->materialBinding, false)
            .SetTargets(SdfPathVector{ HdUsdWriterGetFlattenPrototypePath(materialId) }))
        {
            TF_WARN("Failed to set material binding targets for %s", materialId.GetAsString().c_str());
        }
    }
}

/// Sets visibility on a UsdPrim.
///
/// \param visible HdUsdWriterOptional holding a boolean, true if visible, false otherwise.
/// \param prim UsdPrim inheriting from UsdGeomImageable.
inline void HdUsdWriterSetVisible(HdUsdWriterOptional<bool>& visible, const UsdPrim& prim)
{
    HdUsdWriterPopOptional(visible,
        [&](const auto& visible)
        {
            // There is no actual visibility attribute on Imageable prims, just a visibility token
            // that can be either "inherited" or "invisible". So in case of true we set it to
            // inherited, otherwise use invisible. Since hierarchy is flattened, this should not
            // cause any issues.
            UsdGeomImageable imageable(prim);
            if (!imageable)
            {
                return;
            }

            if (visible)
            {
                if (!imageable.CreateVisibilityAttr().Set(UsdGeomTokens->inherited))
                {
                    TF_WARN("Failed to set visibility attr to inherited for %s", prim.GetPath().GetAsString().c_str());
                }
            }
            else
            {
                if (!imageable.CreateVisibilityAttr().Set(UsdGeomTokens->invisible))
                {
                    TF_WARN("Failed to set visibility attr to invisible for %s", prim.GetPath().GetAsString().c_str());
                }
            }
        });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif