//
// Copyright 2017 Pixar
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
#include "usdMaya/translatorRfMLight.h"

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/primReaderRegistry.h"
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/translatorXformable.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/geometryLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/shadowAPI.h"
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usdLux/sphereLight.h"
#include "pxr/usd/usdRi/pxrAovLight.h"
#include "pxr/usd/usdRi/pxrEnvDayLight.h"

#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // RenderMan for Maya light types.
    ((AovLightMayaTypeName, "PxrAovLight"))
    ((DiskLightMayaTypeName, "PxrDiskLight"))
    ((DistantLightMayaTypeName, "PxrDistantLight"))
    ((DomeLightMayaTypeName, "PxrDomeLight"))
    ((EnvDayLightMayaTypeName, "PxrEnvDayLight"))
    ((GeometryLightMayaTypeName, "PxrMeshLight"))
    ((RectLightMayaTypeName, "PxrRectLight"))
    ((SphereLightMayaTypeName, "PxrSphereLight"))

    // Light plug names.
    ((IntensityPlugName, "intensity"))
    ((ExposurePlugName, "exposure"))
    ((DiffuseAmountPlugName, "diffuse"))
    ((SpecularAmountPlugName, "specular"))
    ((NormalizePowerPlugName, "areaNormalize"))
    ((ColorPlugName, "lightColor"))
    ((EnableTemperaturePlugName, "enableTemperature"))
    ((TemperaturePlugName, "temperature"))

    // Type-specific Light plug names.
    ((DistantLightAnglePlugName, "angleExtent"))
    ((TextureFilePlugName, "lightColorMap"))

    // PxrAovLight plug names.
    ((AovNamePlugName, "aovName"))
    ((InPrimaryHitPlugName, "inPrimaryHit"))
    ((InReflectionPlugName, "inReflection"))
    ((InRefractionPlugName, "inRefraction"))
    ((InvertPlugName, "invert"))
    ((OnVolumeBoundariesPlugName, "onVolumeBoundaries"))
    ((UseColorPlugName, "useColor"))
    ((UseThroughputPlugName, "useThroughput"))

    // PxrEnvDayLight plug names.
    ((DayPlugName, "day"))
    ((HazinessPlugName, "haziness"))
    ((HourPlugName, "hour"))
    ((LatitudePlugName, "latitude"))
    ((LongitudePlugName, "longitude"))
    ((MonthPlugName, "month"))
    ((SkyTintPlugName, "skyTint"))
    ((SunDirectionPlugName, "sunDirection"))
    ((SunSizePlugName, "sunSize"))
    ((SunTintPlugName, "sunTint"))
    ((YearPlugName, "year"))
    ((ZonePlugName, "zone"))

    // ShapingAPI plug names.
    ((FocusPlugName, "emissionFocus"))
    ((FocusTintPlugName, "emissionFocusTint"))
    ((ConeAnglePlugName, "coneAngle"))
    ((ConeSoftnessPlugName, "coneSoftness"))
    ((ProfileFilePlugName, "iesProfile"))
    ((ProfileScalePlugName, "iesProfileScale"))

    // ShadowAPI plug names.
    ((EnableShadowsPlugName, "enableShadows"))
    ((ShadowColorPlugName, "shadowColor"))
    ((ShadowDistancePlugName, "shadowDistance"))
    ((ShadowFalloffPlugName, "shadowFalloff"))
    ((ShadowFalloffGammaPlugName, "shadowFalloffGamma"))
);


static
bool
_ReportError(const std::string& msg, const SdfPath& primPath=SdfPath())
{
    TF_RUNTIME_ERROR(
            "%s%s",
            msg.c_str(),
            primPath.IsPrimPath()
                ? TfStringPrintf(" for Light <%s>", primPath.GetText()).c_str()
                : "");
    return false;
}


// INTENSITY

static
bool
_WriteLightIntensity(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    MStatus status;
    const MPlug lightIntensityPlug =
        depFn.findPlug(_tokens->IntensityPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightIntensity = 1.0f;
    status = lightIntensityPlug.getValue(mayaLightIntensity);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateIntensityAttr(VtValue(mayaLightIntensity), true);

    return true;
}

static
bool
_ReadLightIntensity(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    MStatus status;
    MPlug lightIntensityPlug =
        depFn.findPlug(_tokens->IntensityPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightIntensity = 1.0f;
    lightSchema.GetIntensityAttr().Get(&lightIntensity);

    status = lightIntensityPlug.setValue(lightIntensity);

    return (status == MS::kSuccess);
}


// EXPOSURE

static
bool
_WriteLightExposure(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    MStatus status;
    const MPlug lightExposurePlug =
        depFn.findPlug(_tokens->ExposurePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightExposure = 0.0f;
    status = lightExposurePlug.getValue(mayaLightExposure);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateExposureAttr(VtValue(mayaLightExposure), true);

    return true;
}

static
bool
_ReadLightExposure(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    MStatus status;
    MPlug lightExposurePlug =
        depFn.findPlug(_tokens->ExposurePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightExposure = 0.0f;
    lightSchema.GetExposureAttr().Get(&lightExposure);

    status = lightExposurePlug.setValue(lightExposure);

    return (status == MS::kSuccess);
}


// DIFFUSE

static
bool
_WriteLightDiffuse(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    MStatus status;
    const MPlug lightDiffusePlug =
        depFn.findPlug(_tokens->DiffuseAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightDiffuseAmount = 1.0f;
    status = lightDiffusePlug.getValue(mayaLightDiffuseAmount);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateDiffuseAttr(VtValue(mayaLightDiffuseAmount), true);

    return true;
}

static
bool
_ReadLightDiffuse(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    MStatus status;
    MPlug lightDiffusePlug =
        depFn.findPlug(_tokens->DiffuseAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightDiffuseAmount = 1.0f;
    lightSchema.GetDiffuseAttr().Get(&lightDiffuseAmount);

    status = lightDiffusePlug.setValue(lightDiffuseAmount);

    return (status == MS::kSuccess);
}


// SPECULAR

static
bool
_WriteLightSpecular(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    MStatus status;
    const MPlug lightSpecularPlug =
        depFn.findPlug(_tokens->SpecularAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightSpecularAmount = 1.0f;
    status = lightSpecularPlug.getValue(mayaLightSpecularAmount);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateSpecularAttr(VtValue(mayaLightSpecularAmount), true);

    return true;
}

static
bool
_ReadLightSpecular(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    MStatus status;
    MPlug lightSpecularPlug =
        depFn.findPlug(_tokens->SpecularAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightSpecularAmount = 1.0f;
    lightSchema.GetSpecularAttr().Get(&lightSpecularAmount);

    status = lightSpecularPlug.setValue(lightSpecularAmount);

    return (status == MS::kSuccess);
}


// NORMALIZE POWER

static
bool
_WriteLightNormalizePower(
        const MFnDependencyNode& depFn,
        UsdLuxLight& lightSchema)
{
    MStatus status;
    const MPlug lightNormalizePowerPlug =
        depFn.findPlug(_tokens->NormalizePowerPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool mayaLightNormalizePower = false;
    status = lightNormalizePowerPlug.getValue(mayaLightNormalizePower);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateNormalizeAttr(VtValue(mayaLightNormalizePower), true);

    return true;
}

static
bool
_ReadLightNormalizePower(
        const UsdLuxLight& lightSchema,
        MFnDependencyNode& depFn)
{
    MStatus status;
    MPlug lightNormalizePowerPlug =
        depFn.findPlug(_tokens->NormalizePowerPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightNormalizePower = false;
    lightSchema.GetNormalizeAttr().Get(&lightNormalizePower);

    status = lightNormalizePowerPlug.setValue(lightNormalizePower);

    return (status == MS::kSuccess);
}


// COLOR

static
bool
_WriteLightColor(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    MStatus status;
    const MPlug lightColorPlug =
        depFn.findPlug(_tokens->ColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    const GfVec3f lightColor(lightColorPlug.child(0).asFloat(),
                             lightColorPlug.child(1).asFloat(),
                             lightColorPlug.child(2).asFloat());
    lightSchema.CreateColorAttr(VtValue(lightColor), true);

    return true;
}

static
bool
_ReadLightColor(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    MStatus status;
    MPlug lightColorPlug =
        depFn.findPlug(_tokens->ColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightColor(1.0f);
    lightSchema.GetColorAttr().Get(&lightColor);

    status = lightColorPlug.child(0).setValue(lightColor[0]);
    status = lightColorPlug.child(1).setValue(lightColor[1]);
    status = lightColorPlug.child(2).setValue(lightColor[2]);

    return (status == MS::kSuccess);
}


// TEMPERATURE

static
bool
_WriteLightTemperature(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    MStatus status;
    const MPlug lightEnableTemperaturePlug =
        depFn.findPlug(_tokens->EnableTemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool mayaLightEnableTemperature = false;
    status = lightEnableTemperaturePlug.getValue(mayaLightEnableTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    const MPlug lightTemperaturePlug =
        depFn.findPlug(_tokens->TemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightTemperature = 6500.0f;
    status = lightTemperaturePlug.getValue(mayaLightTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateEnableColorTemperatureAttr(
        VtValue(mayaLightEnableTemperature), true);
    lightSchema.CreateColorTemperatureAttr(VtValue(mayaLightTemperature), true);

    return true;
}

static
bool
_ReadLightTemperature(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    MStatus status;
    MPlug lightEnableTemperaturePlug =
        depFn.findPlug(_tokens->EnableTemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    MPlug lightTemperaturePlug =
        depFn.findPlug(_tokens->TemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightEnableTemperature = false;
    lightSchema.GetEnableColorTemperatureAttr().Get(&lightEnableTemperature);

    status = lightEnableTemperaturePlug.setValue(lightEnableTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightTemperature = 6500.0f;
    lightSchema.GetColorTemperatureAttr().Get(&lightTemperature);

    status = lightTemperaturePlug.setValue(lightTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    return true;
}


// DISTANT LIGHT ANGLE

static
bool
_WriteDistantLightAngle(
        const MFnDependencyNode& depFn,
        UsdLuxLight& lightSchema)
{
    UsdLuxDistantLight distantLightSchema(lightSchema);
    if (!distantLightSchema) {
        return false;
    }

    MStatus status;
    const MPlug lightAnglePlug =
        depFn.findPlug(_tokens->DistantLightAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightAngle = 0.53f;
    status = lightAnglePlug.getValue(mayaLightAngle);
    if (status != MS::kSuccess) {
        return false;
    }

    distantLightSchema.CreateAngleAttr(VtValue(mayaLightAngle), true);

    return true;
}

static
bool
_ReadDistantLightAngle(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    const UsdLuxDistantLight distantLightSchema(lightSchema);
    if (!distantLightSchema) {
        return false;
    }

    MStatus status;
    MPlug lightAnglePlug =
        depFn.findPlug(_tokens->DistantLightAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightAngle = 0.53f;
    distantLightSchema.GetAngleAttr().Get(&lightAngle);

    status = lightAnglePlug.setValue(lightAngle);

    return (status == MS::kSuccess);
}


// LIGHT TEXTURE FILE

static
bool
_WriteLightTextureFile(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    UsdLuxRectLight rectLightSchema(lightSchema);
    UsdLuxDomeLight domeLightSchema(lightSchema);
    if (!rectLightSchema && !domeLightSchema) {
        return false;
    }

    MStatus status;
    const MPlug lightTextureFilePlug =
        depFn.findPlug(_tokens->TextureFilePlugName.GetText(),
                       &status);
    if (status != MS::kSuccess) {
        return false;
    }

    MString mayaLightTextureFile;
    status = lightTextureFilePlug.getValue(mayaLightTextureFile);
    if (status != MS::kSuccess) {
        return false;
    }

    if (mayaLightTextureFile.numChars() < 1u) {
        return false;
    }

    const SdfAssetPath lightTextureAssetPath(mayaLightTextureFile.asChar());
    if (rectLightSchema) {
        rectLightSchema.CreateTextureFileAttr(VtValue(lightTextureAssetPath),
                                              true);
    } else if (domeLightSchema) {
        domeLightSchema.CreateTextureFileAttr(VtValue(lightTextureAssetPath),
                                              true);
    }

    return true;
}

static
bool
_ReadLightTextureFile(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    const UsdLuxRectLight rectLightSchema(lightSchema);
    const UsdLuxDomeLight domeLightSchema(lightSchema);
    if (!rectLightSchema && !domeLightSchema) {
        return false;
    }

    MStatus status;
    MPlug lightTextureFilePlug =
        depFn.findPlug(_tokens->TextureFilePlugName.GetText(),
                       &status);
    if (status != MS::kSuccess) {
        return false;
    }

    SdfAssetPath lightTextureAssetPath;
    if (rectLightSchema) {
        rectLightSchema.GetTextureFileAttr().Get(&lightTextureAssetPath);
    } else if (domeLightSchema) {
        domeLightSchema.GetTextureFileAttr().Get(&lightTextureAssetPath);
    }
    const std::string lightTextureFile = lightTextureAssetPath.GetAssetPath();

    status = lightTextureFilePlug.setValue(MString(lightTextureFile.c_str()));

    return (status == MS::kSuccess);
}


// AOV LIGHT
static
bool
_WriteAovLight(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    UsdRiPxrAovLight aovLightSchema(lightSchema);
    if (!aovLightSchema) {
        return false;
    }

    MStatus status;

    // AOV Name.
    MPlug aovNamePlug =
        depFn.findPlug(_tokens->AovNamePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(aovNamePlug)) {
        MString mayaAovName;
        status = aovNamePlug.getValue(mayaAovName);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateAovNameAttr(VtValue(mayaAovName.asChar()), true);
    }

    // In Primary Hit.
    MPlug inPrimaryHitPlug =
        depFn.findPlug(_tokens->InPrimaryHitPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(inPrimaryHitPlug)) {
        bool mayaInPrimaryHit = true;
        status = inPrimaryHitPlug.getValue(mayaInPrimaryHit);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateInPrimaryHitAttr(VtValue(mayaInPrimaryHit), true);
    }

    // In Reflection.
    MPlug inReflectionPlug =
        depFn.findPlug(_tokens->InReflectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(inReflectionPlug)) {
        bool mayaInReflection = true;
        status = inReflectionPlug.getValue(mayaInReflection);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateInReflectionAttr(VtValue(mayaInReflection), true);
    }

    // In Refraction.
    MPlug inRefractionPlug =
        depFn.findPlug(_tokens->InRefractionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(inRefractionPlug)) {
        bool mayaInRefraction = true;
        status = inRefractionPlug.getValue(mayaInRefraction);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateInRefractionAttr(VtValue(mayaInRefraction), true);
    }

    // Invert.
    MPlug invertPlug =
        depFn.findPlug(_tokens->InvertPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(invertPlug)) {
        bool mayaInvert = true;
        status = invertPlug.getValue(mayaInvert);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateInvertAttr(VtValue(mayaInvert), true);
    }

    // On Volume Boundaries.
    MPlug onVolumeBoundariesPlug =
        depFn.findPlug(_tokens->OnVolumeBoundariesPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(onVolumeBoundariesPlug)) {
        bool mayaOnVolumeBoundaries = true;
        status = onVolumeBoundariesPlug.getValue(mayaOnVolumeBoundaries);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateOnVolumeBoundariesAttr(
            VtValue(mayaOnVolumeBoundaries),
            true);
    }

    // Use Color.
    MPlug useColorPlug =
        depFn.findPlug(_tokens->UseColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(useColorPlug)) {
        bool mayaUseColor = true;
        status = useColorPlug.getValue(mayaUseColor);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateUseColorAttr(VtValue(mayaUseColor), true);
    }

    // Use Throughput.
    MPlug useThroughputPlug =
        depFn.findPlug(_tokens->UseThroughputPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(useThroughputPlug)) {
        bool mayaUseThroughput = true;
        status = useThroughputPlug.getValue(mayaUseThroughput);
        if (status != MS::kSuccess) {
            return false;
        }

        aovLightSchema.CreateUseThroughputAttr(VtValue(mayaUseThroughput),
                                               true);
    }

    return true;
}

static
bool
_ReadAovLight(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    const UsdRiPxrAovLight aovLightSchema(lightSchema);
    if (!aovLightSchema) {
        return false;
    }

    MStatus status;

    // AOV Name.
    MPlug lightAovNamePlug =
        depFn.findPlug(_tokens->AovNamePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    std::string lightAovName;
    aovLightSchema.GetAovNameAttr().Get(&lightAovName);

    status = lightAovNamePlug.setValue(MString(lightAovName.c_str()));
    if (status != MS::kSuccess) {
        return false;
    }

    // In Primary Hit.
    MPlug lightInPrimaryHitPlug =
        depFn.findPlug(_tokens->InPrimaryHitPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightInPrimaryHit = true;
    aovLightSchema.GetInPrimaryHitAttr().Get(&lightInPrimaryHit);

    status = lightInPrimaryHitPlug.setValue(lightInPrimaryHit);
    if (status != MS::kSuccess) {
        return false;
    }

    // In Reflection.
    MPlug lightInReflectionPlug =
        depFn.findPlug(_tokens->InReflectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightInReflection = true;
    aovLightSchema.GetInReflectionAttr().Get(&lightInReflection);

    status = lightInReflectionPlug.setValue(lightInReflection);
    if (status != MS::kSuccess) {
        return false;
    }

    // In Refraction.
    MPlug lightInRefractionPlug =
        depFn.findPlug(_tokens->InRefractionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightInRefraction = true;
    aovLightSchema.GetInRefractionAttr().Get(&lightInRefraction);

    status = lightInRefractionPlug.setValue(lightInRefraction);
    if (status != MS::kSuccess) {
        return false;
    }

    // Invert.
    MPlug lightInvertPlug =
        depFn.findPlug(_tokens->InvertPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightInvert = true;
    aovLightSchema.GetInvertAttr().Get(&lightInvert);

    status = lightInvertPlug.setValue(lightInvert);
    if (status != MS::kSuccess) {
        return false;
    }

    // On Volume Boundaries.
    MPlug lightOnVolumeBoundariesPlug =
        depFn.findPlug(_tokens->OnVolumeBoundariesPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightOnVolumeBoundaries = true;
    aovLightSchema.GetOnVolumeBoundariesAttr().Get(&lightOnVolumeBoundaries);

    status = lightOnVolumeBoundariesPlug.setValue(lightOnVolumeBoundaries);
    if (status != MS::kSuccess) {
        return false;
    }

    // Use Color.
    MPlug lightUseColorPlug =
        depFn.findPlug(_tokens->UseColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightUseColor = true;
    aovLightSchema.GetUseColorAttr().Get(&lightUseColor);

    status = lightUseColorPlug.setValue(lightUseColor);
    if (status != MS::kSuccess) {
        return false;
    }

    // Use Throughput.
    MPlug lightUseThroughputPlug =
        depFn.findPlug(_tokens->UseThroughputPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightUseThroughput = true;
    aovLightSchema.GetUseThroughputAttr().Get(&lightUseThroughput);

    status = lightUseThroughputPlug.setValue(lightUseThroughput);
    return status == MS::kSuccess;
}


// ENVDAY LIGHT
static
bool
_WriteEnvDayLight(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    UsdRiPxrEnvDayLight envDayLightSchema(lightSchema);
    if (!envDayLightSchema) {
        return false;
    }

    MStatus status;

    // Day.
    MPlug dayPlug = depFn.findPlug(_tokens->DayPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(dayPlug)) {
        int mayaDay;
        status = dayPlug.getValue(mayaDay);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateDayAttr(VtValue(mayaDay), true);
    }

    // Haziness.
    MPlug hazinessPlug =
        depFn.findPlug(_tokens->HazinessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(hazinessPlug)) {
        float mayaHaziness;
        status = hazinessPlug.getValue(mayaHaziness);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateHazinessAttr(VtValue(mayaHaziness), true);
    }

    // Hour.
    MPlug hourPlug = depFn.findPlug(_tokens->HourPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(hourPlug)) {
        float mayaHour;
        status = hourPlug.getValue(mayaHour);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateHourAttr(VtValue(mayaHour), true);
    }

    // Latitude.
    MPlug latitudePlug =
        depFn.findPlug(_tokens->LatitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(latitudePlug)) {
        float mayaLatitude;
        status = latitudePlug.getValue(mayaLatitude);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateLatitudeAttr(VtValue(mayaLatitude), true);
    }

    // Longitude.
    MPlug longitudePlug =
        depFn.findPlug(_tokens->LongitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(longitudePlug)) {
        float mayaLongitude;
        status = longitudePlug.getValue(mayaLongitude);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateLongitudeAttr(VtValue(mayaLongitude), true);
    }

    // Month.
    MPlug monthPlug =
        depFn.findPlug(_tokens->MonthPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(monthPlug)) {
        int mayaMonth;
        status = monthPlug.getValue(mayaMonth);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateMonthAttr(VtValue(mayaMonth), true);
    }

    // Sky tint.
    MPlug skyTintPlug =
        depFn.findPlug(_tokens->SkyTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(skyTintPlug)) {
        const GfVec3f mayaSkyTint(skyTintPlug.child(0).asFloat(),
                                  skyTintPlug.child(1).asFloat(),
                                  skyTintPlug.child(2).asFloat());

        envDayLightSchema.CreateSkyTintAttr(VtValue(mayaSkyTint), true);
    }

    // Sun direction.
    MPlug sunDirectionPlug =
        depFn.findPlug(_tokens->SunDirectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(sunDirectionPlug)) {
        const GfVec3f mayaSunDirection(sunDirectionPlug.child(0).asFloat(),
                                       sunDirectionPlug.child(1).asFloat(),
                                       sunDirectionPlug.child(2).asFloat());

        envDayLightSchema.CreateSunDirectionAttr(VtValue(mayaSunDirection), true);
    }

    // Sun size.
    MPlug sunSizePlug =
        depFn.findPlug(_tokens->SunSizePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(sunSizePlug)) {
        float mayaSunSize;
        status = sunSizePlug.getValue(mayaSunSize);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateSunSizeAttr(VtValue(mayaSunSize), true);
    }

    // Sun tint.
    MPlug sunTintPlug =
        depFn.findPlug(_tokens->SunTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(sunTintPlug)) {
        const GfVec3f mayaSunTint(sunTintPlug.child(0).asFloat(),
                                  sunTintPlug.child(1).asFloat(),
                                  sunTintPlug.child(2).asFloat());

        envDayLightSchema.CreateSunTintAttr(VtValue(mayaSunTint), true);
    }

    // Year.
    MPlug yearPlug = depFn.findPlug(_tokens->YearPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(yearPlug)) {
        int mayaYear;
        status = yearPlug.getValue(mayaYear);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateYearAttr(VtValue(mayaYear), true);
    }

    // Zone.
    MPlug zonePlug = depFn.findPlug(_tokens->ZonePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(zonePlug)) {
        float mayaZone;
        status = zonePlug.getValue(mayaZone);
        if (status != MS::kSuccess) {
            return false;
        }

        envDayLightSchema.CreateZoneAttr(VtValue(mayaZone), true);
    }

    return true;
}

static
bool
_ReadEnvDayLight(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    const UsdRiPxrEnvDayLight envDayLightSchema(lightSchema);
    if (!envDayLightSchema) {
        return false;
    }

    MStatus status;

    // Day.
    MPlug lightDayPlug =
        depFn.findPlug(_tokens->DayPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    int lightDay = 1;
    envDayLightSchema.GetDayAttr().Get(&lightDay);

    status = lightDayPlug.setValue(lightDay);
    if (status != MS::kSuccess) {
        return false;
    }

    // Haziness.
    MPlug lightHazinessPlug =
        depFn.findPlug(_tokens->HazinessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightHaziness = 2.0f;
    envDayLightSchema.GetHazinessAttr().Get(&lightHaziness);

    status = lightHazinessPlug.setValue(lightHaziness);
    if (status != MS::kSuccess) {
        return false;
    }

    // Hour.
    MPlug lightHourPlug =
        depFn.findPlug(_tokens->HourPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightHour = 14.633333f;
    envDayLightSchema.GetHourAttr().Get(&lightHour);

    status = lightHourPlug.setValue(lightHour);
    if (status != MS::kSuccess) {
        return false;
    }

    // Latitude.
    MPlug lightLatitudePlug =
        depFn.findPlug(_tokens->LatitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightLatitude = 47.602f;
    envDayLightSchema.GetLatitudeAttr().Get(&lightLatitude);

    status = lightLatitudePlug.setValue(lightLatitude);
    if (status != MS::kSuccess) {
        return false;
    }

    // Longitude.
    MPlug lightLongitudePlug =
        depFn.findPlug(_tokens->LongitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightLongitude = -122.332f;
    envDayLightSchema.GetLongitudeAttr().Get(&lightLongitude);

    status = lightLongitudePlug.setValue(lightLongitude);
    if (status != MS::kSuccess) {
        return false;
    }

    // Month.
    MPlug lightMonthPlug =
        depFn.findPlug(_tokens->MonthPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    int lightMonth = 0;
    envDayLightSchema.GetMonthAttr().Get(&lightMonth);

    status = lightMonthPlug.setValue(lightMonth);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sky tint.
    MPlug lightSkyTintPlug =
        depFn.findPlug(_tokens->SkyTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightSkyTint(1.0f);
    envDayLightSchema.GetSkyTintAttr().Get(&lightSkyTint);

    status = lightSkyTintPlug.child(0).setValue(lightSkyTint[0]);
    status = lightSkyTintPlug.child(1).setValue(lightSkyTint[1]);
    status = lightSkyTintPlug.child(2).setValue(lightSkyTint[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sun direction.
    MPlug lightSunDirectionPlug =
        depFn.findPlug(_tokens->SunDirectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightSunDirection(0.0f, 0.0f, 1.0f);
    envDayLightSchema.GetSunDirectionAttr().Get(&lightSunDirection);

    status = lightSunDirectionPlug.child(0).setValue(lightSunDirection[0]);
    status = lightSunDirectionPlug.child(1).setValue(lightSunDirection[1]);
    status = lightSunDirectionPlug.child(2).setValue(lightSunDirection[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sun size.
    MPlug lightSunSizePlug =
        depFn.findPlug(_tokens->SunSizePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightSunSize = 1.0f;
    envDayLightSchema.GetSunSizeAttr().Get(&lightSunSize);

    status = lightSunSizePlug.setValue(lightSunSize);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sun tint.
    MPlug lightSunTintPlug =
        depFn.findPlug(_tokens->SunTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightSunTint(1.0f);
    envDayLightSchema.GetSunTintAttr().Get(&lightSunTint);

    status = lightSunTintPlug.child(0).setValue(lightSunTint[0]);
    status = lightSunTintPlug.child(1).setValue(lightSunTint[1]);
    status = lightSunTintPlug.child(2).setValue(lightSunTint[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Year.
    MPlug lightYearPlug =
        depFn.findPlug(_tokens->YearPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    int lightYear = 2015;
    envDayLightSchema.GetYearAttr().Get(&lightYear);

    status = lightYearPlug.setValue(lightYear);
    if (status != MS::kSuccess) {
        return false;
    }

    // Zone.
    MPlug lightZonePlug =
        depFn.findPlug(_tokens->ZonePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightZone = -8.0f;
    envDayLightSchema.GetZoneAttr().Get(&lightZone);

    status = lightZonePlug.setValue(lightZone);
    return status == MS::kSuccess;
}


// SHAPING API

static
bool
_WriteLightShapingAPI(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    UsdLuxShapingAPI shapingAPI =
        PxrUsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdLuxShapingAPI>(
            lightSchema.GetPrim());
    if (!shapingAPI) {
        return false;
    }

    MStatus status;

    // Focus.
    MPlug lightFocusPlug =
        depFn.findPlug(_tokens->FocusPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightFocusPlug)) {
        float mayaLightFocus = 0.0f;
        status = lightFocusPlug.getValue(mayaLightFocus);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingFocusAttr(VtValue(mayaLightFocus), true);
    }

    // Focus Tint.
    MPlug lightFocusTintPlug =
        depFn.findPlug(_tokens->FocusTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightFocusTintPlug)) {
        const GfVec3f lightFocusTint(lightFocusTintPlug.child(0).asFloat(),
                                     lightFocusTintPlug.child(1).asFloat(),
                                     lightFocusTintPlug.child(2).asFloat());

        shapingAPI.CreateShapingFocusTintAttr(VtValue(lightFocusTint), true);
    }

    // Cone Angle.
    MPlug lightConeAnglePlug =
        depFn.findPlug(_tokens->ConeAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightConeAnglePlug)) {
        float mayaLightConeAngle = 90.0f;
        status = lightConeAnglePlug.getValue(mayaLightConeAngle);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingConeAngleAttr(VtValue(mayaLightConeAngle),
                                              true);
    }

    // Cone Softness.
    MPlug lightConeSoftnessPlug =
        depFn.findPlug(_tokens->ConeSoftnessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightConeSoftnessPlug)) {
        float mayaLightConeSoftness = 0.0f;
        status = lightConeSoftnessPlug.getValue(mayaLightConeSoftness);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingConeSoftnessAttr(VtValue(mayaLightConeSoftness),
                                                 true);
    }

    // Profile File.
    MPlug lightProfileFilePlug =
        depFn.findPlug(_tokens->ProfileFilePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightProfileFilePlug)) {
        MString mayaLightProfileFile;
        status = lightProfileFilePlug.getValue(mayaLightProfileFile);
        if (status != MS::kSuccess) {
            return false;
        }

        if (mayaLightProfileFile.numChars() > 0u) {
            const SdfAssetPath lightProfileAssetPath(
                mayaLightProfileFile.asChar());
            shapingAPI.CreateShapingIesFileAttr(VtValue(lightProfileAssetPath),
                                                true);
        }
    }

    // Profile Scale.
    MPlug lightProfileScalePlug =
        depFn.findPlug(_tokens->ProfileScalePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightProfileScalePlug)) {
        float mayaLightProfileScale = 1.0f;
        status = lightProfileScalePlug.getValue(mayaLightProfileScale);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingIesAngleScaleAttr(VtValue(mayaLightProfileScale),
                                                  true);
    }

    return true;
}

static
bool
_ReadLightShapingAPI(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    const UsdLuxShapingAPI shapingAPI(lightSchema);
    if (!shapingAPI) {
        return false;
    }

    MStatus status;

    // Focus.
    MPlug lightFocusPlug =
        depFn.findPlug(_tokens->FocusPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightFocus = 0.0f;
    shapingAPI.GetShapingFocusAttr().Get(&lightFocus);

    status = lightFocusPlug.setValue(lightFocus);
    if (status != MS::kSuccess) {
        return false;
    }

    // Focus Tint.
    MPlug lightFocusTintPlug =
        depFn.findPlug(_tokens->FocusTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightFocusTint(0.0f);
    shapingAPI.GetShapingFocusTintAttr().Get(&lightFocusTint);

    status = lightFocusTintPlug.child(0).setValue(lightFocusTint[0]);
    status = lightFocusTintPlug.child(1).setValue(lightFocusTint[1]);
    status = lightFocusTintPlug.child(2).setValue(lightFocusTint[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Cone Angle.
    MPlug lightConeAnglePlug =
        depFn.findPlug(_tokens->ConeAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightConeAngle = 90.0f;
    shapingAPI.GetShapingConeAngleAttr().Get(&lightConeAngle);

    status = lightConeAnglePlug.setValue(lightConeAngle);
    if (status != MS::kSuccess) {
        return false;
    }

    // Cone Softness.
    MPlug lightConeSoftnessPlug =
        depFn.findPlug(_tokens->ConeSoftnessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightConeSoftness = 0.0f;
    shapingAPI.GetShapingConeSoftnessAttr().Get(&lightConeSoftness);

    status = lightConeSoftnessPlug.setValue(lightConeSoftness);
    if (status != MS::kSuccess) {
        return false;
    }

    // Profile File.
    MPlug lightProfileFilePlug =
        depFn.findPlug(_tokens->ProfileFilePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    SdfAssetPath lightProfileAssetPath;
    shapingAPI.GetShapingIesFileAttr().Get(&lightProfileAssetPath);
    const std::string lightProfileFile = lightProfileAssetPath.GetAssetPath();

    status = lightProfileFilePlug.setValue(MString(lightProfileFile.c_str()));
    if (status != MS::kSuccess) {
        return false;
    }

    // Profile Scale.
    MPlug lightProfileScalePlug =
        depFn.findPlug(_tokens->ProfileScalePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightProfileScale = 1.0f;
    shapingAPI.GetShapingIesAngleScaleAttr().Get(&lightProfileScale);

    status = lightProfileScalePlug.setValue(lightProfileScale);
    return status == MS::kSuccess;
}


// SHADOW API

static
bool
_WriteLightShadowAPI(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
{
    UsdLuxShadowAPI shadowAPI =
        PxrUsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdLuxShadowAPI>(
            lightSchema.GetPrim());
    if (!shadowAPI) {
        return false;
    }

    MStatus status;

    // Enable Shadows.
    MPlug lightEnableShadowsPlug =
        depFn.findPlug(_tokens->EnableShadowsPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightEnableShadowsPlug)) {
        bool mayaLightEnableShadows = true;
        status = lightEnableShadowsPlug.getValue(mayaLightEnableShadows);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowEnableAttr(VtValue(mayaLightEnableShadows), true);
    }

    // Shadow Include.
    // XXX: Not yet implemented.

    // Shadow Exclude.
    // XXX: Not yet implemented.

    // Shadow Color.
    MPlug lightShadowColorPlug =
        depFn.findPlug(_tokens->ShadowColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightShadowColorPlug)) {
        const GfVec3f lightShadowColor(lightShadowColorPlug.child(0).asFloat(),
                                       lightShadowColorPlug.child(1).asFloat(),
                                       lightShadowColorPlug.child(2).asFloat());

        shadowAPI.CreateShadowColorAttr(VtValue(lightShadowColor), true);
    }

    // Shadow Distance.
    MPlug lightShadowDistancePlug =
        depFn.findPlug(_tokens->ShadowDistancePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightShadowDistancePlug)) {
        float mayaLightShadowDistance = 0.0f;
        status = lightShadowDistancePlug.getValue(mayaLightShadowDistance);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowDistanceAttr(VtValue(mayaLightShadowDistance),
                                           true);
    }

    // Shadow Falloff.
    MPlug lightShadowFalloffPlug =
        depFn.findPlug(_tokens->ShadowFalloffPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightShadowFalloffPlug)) {
        float mayaLightShadowFalloff = 0.0f;
        status = lightShadowFalloffPlug.getValue(mayaLightShadowFalloff);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowFalloffAttr(VtValue(mayaLightShadowFalloff),
                                          true);
    }

    // Shadow Falloff Gamma.
    MPlug lightShadowFalloffGammaPlug =
        depFn.findPlug(_tokens->ShadowFalloffGammaPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (PxrUsdMayaUtil::IsAuthored(lightShadowFalloffGammaPlug)) {
        float mayaLightShadowFalloffGamma = 1.0f;
        status =
            lightShadowFalloffGammaPlug.getValue(mayaLightShadowFalloffGamma);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowFalloffGammaAttr(
            VtValue(mayaLightShadowFalloffGamma),
            true);
    }

    return true;
}

static
bool
_ReadLightShadowAPI(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
{
    const UsdLuxShadowAPI shadowAPI(lightSchema);
    if (!shadowAPI) {
        return false;
    }

    MStatus status;

    // Enable Shadows.
    MPlug lightEnableShadowsPlug =
        depFn.findPlug(_tokens->EnableShadowsPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightEnableShadows = true;
    shadowAPI.GetShadowEnableAttr().Get(&lightEnableShadows);

    status = lightEnableShadowsPlug.setValue(lightEnableShadows);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Include.
    // XXX: Not yet implemented.

    // Shadow Exclude.
    // XXX: Not yet implemented.

    // Shadow Color.
    MPlug lightShadowColorPlug =
        depFn.findPlug(_tokens->ShadowColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightShadowColor(0.0f);
    shadowAPI.GetShadowColorAttr().Get(&lightShadowColor);

    status = lightShadowColorPlug.child(0).setValue(lightShadowColor[0]);
    status = lightShadowColorPlug.child(1).setValue(lightShadowColor[1]);
    status = lightShadowColorPlug.child(2).setValue(lightShadowColor[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Distance.
    MPlug lightShadowDistancePlug =
        depFn.findPlug(_tokens->ShadowDistancePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightShadowDistance = 0.0f;
    shadowAPI.GetShadowDistanceAttr().Get(&lightShadowDistance);

    status = lightShadowDistancePlug.setValue(lightShadowDistance);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Falloff.
    MPlug lightShadowFalloffPlug =
        depFn.findPlug(_tokens->ShadowFalloffPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightShadowFalloff = 0.0f;
    shadowAPI.GetShadowFalloffAttr().Get(&lightShadowFalloff);

    status = lightShadowFalloffPlug.setValue(lightShadowFalloff);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Falloff Gamma.
    MPlug lightShadowFalloffGammaPlug =
        depFn.findPlug(_tokens->ShadowFalloffGammaPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightShadowFalloffGamma = 1.0f;
    shadowAPI.GetShadowFalloffGammaAttr().Get(&lightShadowFalloffGamma);

    status = lightShadowFalloffGammaPlug.setValue(lightShadowFalloffGamma);
    return status == MS::kSuccess;
}


static
UsdLuxLight
_DefineUsdLuxLightForMayaLight(
        const MFnDependencyNode& depFn,
        PxrUsdMayaPrimWriterContext* context)
{
    UsdLuxLight lightSchema;

    UsdStageRefPtr stage = context->GetUsdStage();
    const SdfPath& authorPath = context->GetAuthorPath();

    MStatus status;
    const MString mayaLightTypeName = depFn.typeName(&status);
    if (status != MS::kSuccess) {
        _ReportError("Failed to get Maya light type name", authorPath);
        return lightSchema;
    }

    const TfToken mayaLightTypeToken(mayaLightTypeName.asChar());

    if (mayaLightTypeToken == _tokens->AovLightMayaTypeName) {
        lightSchema = UsdRiPxrAovLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->DiskLightMayaTypeName) {
        lightSchema = UsdLuxDiskLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->DistantLightMayaTypeName) {
        lightSchema = UsdLuxDistantLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->DomeLightMayaTypeName) {
        lightSchema = UsdLuxDomeLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->EnvDayLightMayaTypeName) {
        lightSchema = UsdRiPxrEnvDayLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->GeometryLightMayaTypeName) {
        lightSchema = UsdLuxGeometryLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->RectLightMayaTypeName) {
        lightSchema = UsdLuxRectLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->SphereLightMayaTypeName) {
        lightSchema = UsdLuxSphereLight::Define(stage, authorPath);
    } else {
        _ReportError("Could not determine UsdLux schema for Maya light",
                     authorPath);
    }

    return lightSchema;
}

/* static */
bool
PxrUsdMayaTranslatorRfMLight::Write(
        const PxrUsdMayaPrimWriterArgs& args,
        PxrUsdMayaPrimWriterContext* context)
{
    const SdfPath& authorPath = context->GetAuthorPath();

    MStatus status;
    const MObject& lightObj = args.GetMObject();
    const MFnDependencyNode depFn(lightObj, &status);
    if (status != MS::kSuccess) {
        return _ReportError("Failed to get Maya light", authorPath);
    }

    UsdLuxLight lightSchema = _DefineUsdLuxLightForMayaLight(depFn, context);
    if (!lightSchema) {
        return _ReportError("Failed to create UsdLuxLight prim", authorPath);
    }

    _WriteLightIntensity(depFn, lightSchema);
    _WriteLightExposure(depFn, lightSchema);
    _WriteLightDiffuse(depFn, lightSchema);
    _WriteLightSpecular(depFn, lightSchema);
    _WriteLightNormalizePower(depFn, lightSchema);
    _WriteLightColor(depFn, lightSchema);
    _WriteLightTemperature(depFn, lightSchema);

    // XXX: Light filters not yet implemented.
    // XXX: PxrMeshLight geometry not yet implemented.
    // XXX: PxrDomeLight portals not yet implemented.

    _WriteDistantLightAngle(depFn, lightSchema);

    _WriteLightTextureFile(depFn, lightSchema);

    _WriteAovLight(depFn, lightSchema);

    _WriteEnvDayLight(depFn, lightSchema);

    _WriteLightShapingAPI(depFn, lightSchema);

    _WriteLightShadowAPI(depFn, lightSchema);

    return true;
}


static
TfToken
_GetMayaTypeTokenForUsdLuxLight(const UsdLuxLight& lightSchema)
{
    const UsdPrim& lightPrim = lightSchema.GetPrim();

    if (lightPrim.IsA<UsdRiPxrAovLight>()) {
        return _tokens->AovLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxDiskLight>()) {
        return _tokens->DiskLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxDistantLight>()) {
        return _tokens->DistantLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxDomeLight>()) {
        return _tokens->DomeLightMayaTypeName;
    } else if (lightPrim.IsA<UsdRiPxrEnvDayLight>()) {
        return _tokens->EnvDayLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxGeometryLight>()) {
        return _tokens->GeometryLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxRectLight>()) {
        return _tokens->RectLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxSphereLight>()) {
        return _tokens->SphereLightMayaTypeName;
    }

    return TfToken();
}

/* static */
bool
PxrUsdMayaTranslatorRfMLight::Read(
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    const UsdPrim& usdPrim = args.GetUsdPrim();
    if (!usdPrim) {
        return false;
    }

    const UsdLuxLight lightSchema(usdPrim);
    if (!lightSchema) {
        return _ReportError("Failed to read UsdLuxLight prim",
                            usdPrim.GetPath());
    }

    const TfToken mayaLightTypeToken =
        _GetMayaTypeTokenForUsdLuxLight(lightSchema);
    if (mayaLightTypeToken.IsEmpty()) {
        return _ReportError(
            "Could not determine Maya light type for UsdLuxLight prim",
            lightSchema.GetPath());
    }

    MObject parentNode =
        context->GetMayaNode(lightSchema.GetPath().GetParentPath(), false);

    MStatus status;
    MObject mayaNodeTransformObj;
    if (!PxrUsdMayaTranslatorUtil::CreateTransformNode(
            usdPrim,
            parentNode,
            args,
            context,
            &status,
            &mayaNodeTransformObj)) {
        return _ReportError("Failed to create transform node",
                            lightSchema.GetPath());
    }

    const MString nodeName =
        TfStringPrintf("%sShape", usdPrim.GetName().GetText()).c_str();

    MObject lightObj;
    if (!PxrUsdMayaTranslatorUtil::CreateNode(
            nodeName,
            MString(mayaLightTypeToken.GetText()),
            mayaNodeTransformObj,
            &status,
            &lightObj)) {
        return _ReportError(TfStringPrintf("Failed to create %s node",
                                           mayaLightTypeToken.GetText()),
                            lightSchema.GetPath());
    }

    const std::string nodePath = lightSchema.GetPath().AppendChild(
        TfToken(nodeName.asChar())).GetString();
    context->RegisterNewMayaNode(nodePath, lightObj);

    MFnDependencyNode depFn(lightObj, &status);
    if (status != MS::kSuccess) {
        return _ReportError("Failed to get Maya light", lightSchema.GetPath());
    }

    _ReadLightIntensity(lightSchema, depFn);
    _ReadLightExposure(lightSchema, depFn);
    _ReadLightDiffuse(lightSchema, depFn);
    _ReadLightSpecular(lightSchema, depFn);
    _ReadLightNormalizePower(lightSchema, depFn);
    _ReadLightColor(lightSchema, depFn);
    _ReadLightTemperature(lightSchema, depFn);

    // XXX: LightFilters not yet implemented.
    // XXX: GeometryLight geometry not yet implemented.
    // XXX: DomeLight LightPortals not yet implemented.

    _ReadDistantLightAngle(lightSchema, depFn);

    _ReadLightTextureFile(lightSchema, depFn);

    _ReadAovLight(lightSchema, depFn);

    _ReadEnvDayLight(lightSchema, depFn);

    _ReadLightShapingAPI(lightSchema, depFn);

    _ReadLightShadowAPI(lightSchema, depFn);

    return true;
}


// Declare/define the Maya type name for RenderMan for Maya light types as a
// dummy class so that we can register writers for them.

class PxrAovLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrAovLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdRiPxrAovLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


class PxrDiskLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrDiskLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDiskLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


class PxrDistantLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrDistantLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDistantLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


class PxrDomeLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrDomeLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDomeLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


class PxrEnvDayLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrEnvDayLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdRiPxrEnvDayLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


class PxrMeshLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrMeshLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxGeometryLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


class PxrRectLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrRectLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxRectLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


class PxrSphereLight {};
PXRUSDMAYA_DEFINE_WRITER(PxrSphereLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxSphereLight, args, context)
{
    return PxrUsdMayaTranslatorRfMLight::Read(args, context);
}


PXR_NAMESPACE_CLOSE_SCOPE
