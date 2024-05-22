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

#include "pxr/usdImaging/plugin/hdUsdWriter/light.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdLux/cylinderLight.h"
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/shadowAPI.h"
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usdLux/sphereLight.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// HdLightToken and the corresponding Usd serialization token
using LightParamList = std::vector<std::pair<TfToken, TfToken>>;

// Taking a simple and quick approach here, as UsdLux is still in a state of flux.
const LightParamList _baseLightParams{
    { HdLightTokens->intensity, UsdLuxTokens->inputsIntensity },
    { HdLightTokens->exposure, UsdLuxTokens->inputsExposure },
    { HdLightTokens->diffuse, UsdLuxTokens->inputsDiffuse },
    { HdLightTokens->specular, UsdLuxTokens->inputsSpecular },
    { HdLightTokens->normalize, UsdLuxTokens->inputsNormalize },
    { HdLightTokens->color, UsdLuxTokens->inputsColor },
    { HdLightTokens->enableColorTemperature, UsdLuxTokens->inputsEnableColorTemperature },
    { HdLightTokens->colorTemperature, UsdLuxTokens->inputsColorTemperature },
    { HdLightTokens->shadowEnable, UsdLuxTokens->inputsShadowEnable },
    { HdLightTokens->shadowColor, UsdLuxTokens->inputsShadowColor },
    { HdLightTokens->shadowDistance, UsdLuxTokens->inputsShadowDistance },
    { HdLightTokens->shadowFalloff, UsdLuxTokens->inputsShadowFalloff },
    { HdLightTokens->shadowFalloffGamma, UsdLuxTokens->inputsShadowFalloffGamma }
};

const LightParamList _lightShapingParams{
    { HdLightTokens->shapingFocus, UsdLuxTokens->inputsShapingFocus },
    { HdLightTokens->shapingFocusTint, UsdLuxTokens->inputsShapingFocusTint },
    { HdLightTokens->shapingConeAngle, UsdLuxTokens->inputsShapingConeAngle },
    { HdLightTokens->shapingConeSoftness, UsdLuxTokens->inputsShapingConeSoftness },
    { HdLightTokens->shapingIesFile, UsdLuxTokens->inputsShapingIesFile },
    { HdLightTokens->shapingIesAngleScale, UsdLuxTokens->inputsShapingIesAngleScale },
    { HdLightTokens->shapingIesNormalize, UsdLuxTokens->inputsShapingIesNormalize },
};

using LightParams = std::pair<TfToken, LightParamList>;
const std::vector<LightParams> _lightParams{
    { HdPrimTypeTokens->distantLight, { { HdLightTokens->angle, UsdLuxTokens->inputsAngle } } },
    { HdPrimTypeTokens->diskLight, { { HdLightTokens->radius, UsdLuxTokens->inputsRadius } } },
    { HdPrimTypeTokens->rectLight,
      {
          { HdLightTokens->width, UsdLuxTokens->inputsWidth },
          { HdLightTokens->height, UsdLuxTokens->inputsHeight },
          { HdLightTokens->textureFile, UsdLuxTokens->inputsTextureFile },
      } },
    { HdPrimTypeTokens->sphereLight,
      {
          { HdLightTokens->radius, UsdLuxTokens->inputsRadius },
          { UsdLuxTokens->treatAsPoint, UsdLuxTokens->treatAsPoint },
      } },
    { HdPrimTypeTokens->cylinderLight,
      {
          { HdLightTokens->length, UsdLuxTokens->inputsLength },
          { HdLightTokens->radius, UsdLuxTokens->inputsRadius },
          { UsdLuxTokens->treatAsLine, UsdLuxTokens->treatAsLine },
      } },
    { HdPrimTypeTokens->domeLight,
      {
          { HdLightTokens->textureFile, UsdLuxTokens->inputsTextureFile },
          { HdLightTokens->textureFormat, UsdLuxTokens->inputsTextureFormat },
      } },
};

}

HdUsdWriterLight::HdUsdWriterLight(TfToken const& typeId, SdfPath const& id) : HdLight(id), _type(typeId)
{
}

HdDirtyBits HdUsdWriterLight::GetInitialDirtyBitsMask() const
{
    int mask = HdLight::Clean | HdLight::DirtyParams | HdLight::DirtyTransform;

    // Marking material as dirty for dome lights that have a material that can be baked.
    if (_type == HdPrimTypeTokens->domeLight)
        mask |= HdChangeTracker::DirtyMaterialId;

    return (HdDirtyBits)mask;
}

void HdUsdWriterLight::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(renderParam);

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    SdfPath const& id = GetId();

    // DomeLight prim needs to deal with MaterialId
    if (_type == HdPrimTypeTokens->domeLight && *dirtyBits & HdChangeTracker::DirtyMaterialId)
    {
        _materialId = sceneDelegate->GetMaterialId(id);
    }

    if (*dirtyBits & HdLight::DirtyParams)
    {
        auto setParams = [&](const LightParamList& paramNames)
        {
            for (const auto& paramName : paramNames)
            {
                _params[paramName.second] = sceneDelegate->GetLightParamValue(id, paramName.first);
            }
        };
        setParams(_baseLightParams);
        auto it = std::find_if(
            _lightParams.begin(), _lightParams.end(), [&](const LightParams& a) -> bool { return a.first == _type; });
        if (it != _lightParams.end())
        {
            setParams(it->second);
        }
        if (_type == HdPrimTypeTokens->sphereLight || _type == HdPrimTypeTokens->rectLight ||
            _type == HdPrimTypeTokens->diskLight)
        {
            setParams(_lightShapingParams);
        }

        // Visibility and Transforms on Sprims are part of DirtyParams
        _visible = sceneDelegate->GetVisible(id);
        _transform = sceneDelegate->GetTransform(id);
    }

    // Still need to handle DirtyVisibility and DirtyTransform independently of DirtyParams
    if (*dirtyBits & HdChangeTracker::DirtyVisibility)
    {
        _visible = sceneDelegate->GetVisible(id);
    }
    if (*dirtyBits & HdLight::DirtyTransform)
    {
        _transform = sceneDelegate->GetTransform(id);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void HdUsdWriterLight::SerializeToUsd(const UsdStagePtr &stage)
{
    TfToken primType;
    if (_type == HdPrimTypeTokens->distantLight)
    {
        primType = UsdSchemaRegistry::GetSchemaTypeName<UsdLuxDistantLight>();
    }
    else if (_type == HdPrimTypeTokens->diskLight)
    {
        primType = UsdSchemaRegistry::GetSchemaTypeName<UsdLuxDiskLight>();
    }
    else if (_type == HdPrimTypeTokens->rectLight)
    {
        primType = UsdSchemaRegistry::GetSchemaTypeName<UsdLuxRectLight>();
    }
    else if (_type == HdPrimTypeTokens->cylinderLight)
    {
        primType = UsdSchemaRegistry::GetSchemaTypeName<UsdLuxCylinderLight>();
    }
    else if (_type == HdPrimTypeTokens->domeLight)
    {
        primType = UsdSchemaRegistry::GetSchemaTypeName<UsdLuxDomeLight>();
    }
    else if (_type == HdPrimTypeTokens->sphereLight)
    {
        primType = UsdSchemaRegistry::GetSchemaTypeName<UsdLuxSphereLight>();
    }
    else
    {
        TF_WARN("Unrecognized light type %s", _type.GetText());
        return;
    }

    const auto prim = stage->DefinePrim(GetId(), primType);
    HdUsdWriterPopOptional(_transform,
        [&](const auto& transform)
        {
            HdUsdWriterSetTransformOp(UsdGeomXformable(prim), transform);
        });
    HdUsdWriterSetVisible(_visible, prim);

    if (_type == HdPrimTypeTokens->domeLight)
    {
        HdUsdWriterPopOptional(_materialId,
            [&](const auto& materialId)
            {
                // How should we handle zeroing out the material?
                HdUsdWriterAssignMaterialToPrim(materialId, prim, false);
            });
    }

    const auto& schemaRegistry = UsdSchemaRegistry::GetInstance();
    auto writeSchema = [&prim, this](const UsdPrimDefinition* schemaDefinition)
    {
        if (schemaDefinition == nullptr)
        {
            return false;
        }
        auto wroteSchema = false;
        for (const auto& param : schemaDefinition->GetPropertyNames())
        {
            const auto paramIt = _params.find(param);
            if (paramIt == _params.end() || paramIt->second.IsEmpty())
            {
                continue;
            }
            const auto& attributeDef = schemaDefinition->GetAttributeDefinition(param);
            auto attr = prim.CreateAttribute(param, attributeDef->GetTypeName(), false, attributeDef->GetVariability());
            attr.Set(paramIt->second);
            wroteSchema = true;
        }
        return wroteSchema;
    };
    writeSchema(schemaRegistry.FindConcretePrimDefinition(primType)); 
    auto wroteShadowSchema = writeSchema(
        schemaRegistry.FindAppliedAPIPrimDefinition(UsdSchemaRegistry::GetSchemaTypeName<UsdLuxShadowAPI>()));
    auto wroteShapingSchema = writeSchema(
        schemaRegistry.FindAppliedAPIPrimDefinition(UsdSchemaRegistry::GetSchemaTypeName<UsdLuxShapingAPI>()));

    if (wroteShadowSchema)
    {
        UsdLuxShadowAPI::Apply(prim);
    }
    if (wroteShapingSchema)
    {
        UsdLuxShapingAPI::Apply(prim);
    }

    _params.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
