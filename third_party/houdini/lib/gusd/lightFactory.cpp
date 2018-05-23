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
#include "lightFactory.h"
#include "attributeTransfer.h"

#include "USD_Utils.h"

#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/sphereLight.h"
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdGeom/xform.h"

#include <OpenEXR/ImathEuler.h>
#include <OpenEXR/ImathVec.h>

PXR_NAMESPACE_OPEN_SCOPE

importFunctionMap LightFactory::s_importFunctionMap = {};
exportFunctionMap LightFactory::s_exportFunctionMap = {};

exportFunctionTokenCalculator LightFactory::s_exportFunctionTokenCalculator = nullptr;

static const std::map<UsdGeomXformCommonAPI::RotationOrder, std::string> s_rotationOrderMappingUsdToHou = {
        {UsdGeomXformCommonAPI::RotationOrderXYZ, "xyz"},
        {UsdGeomXformCommonAPI::RotationOrderXZY, "xzy"},
        {UsdGeomXformCommonAPI::RotationOrderYXZ, "yxz"},
        {UsdGeomXformCommonAPI::RotationOrderYZX, "yzx"},
        {UsdGeomXformCommonAPI::RotationOrderZXY, "zxy"},
        {UsdGeomXformCommonAPI::RotationOrderZYX, "zyx"}
};

static const std::map<UT_String, UsdGeomXformCommonAPI::RotationOrder> s_rotationOrderMappingHouToUsd = {
        {UT_String("xyz"), UsdGeomXformCommonAPI::RotationOrderXYZ},
        {UT_String("xzy"), UsdGeomXformCommonAPI::RotationOrderXZY},
        {UT_String("yxz"), UsdGeomXformCommonAPI::RotationOrderYXZ},
        {UT_String("yzx"), UsdGeomXformCommonAPI::RotationOrderYZX},
        {UT_String("zxy"), UsdGeomXformCommonAPI::RotationOrderZXY},
        {UT_String("zyx"), UsdGeomXformCommonAPI::RotationOrderZYX}
};


// ==========================
// Import/export entry points
// ==========================

TfToken LightFactory::getExportFunctionToken(const OP_Node* node)
{
    UT_String typeName;
    UT_String envLightTypeName("envlight");

    auto isEnvlight = node->getOperator()->getName() == envLightTypeName;
    if (isEnvlight)
    {
        typeName = envLightTypeName;
    }
    else if (node->isSubNetwork(false))
    {
        typeName = UT_String("transform");
    }
    else
    {
        auto& parm = node->getParm("light_type");
        parm.getValue(0, typeName, 0, false, 0);
    }

    return TfToken(typeName.c_str());
}

// static
UsdPrim LightFactory::create(UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    TfToken exportFunctionToken;
    if (s_exportFunctionTokenCalculator == nullptr)
    {
        exportFunctionToken = getExportFunctionToken(node);
    }
    else
    {
        exportFunctionToken = s_exportFunctionTokenCalculator(node);
    }

    auto foundResult = s_exportFunctionMap.find(exportFunctionToken);
    if(foundResult == s_exportFunctionMap.end())
    {
        std::cout << "Couldn't find export function for function token '" << exportFunctionToken.GetText() << "'" << std::endl;;
        // We don't have a function registered to handle this prim type.
        return UsdPrim();

    }

    auto func = foundResult->second;
    return func(stage, node, time, timeCode);
}

// static
OP_Node* LightFactory::create(const UsdPrim& prim, OP_Network* root, const bool useNetboxes)
{
    auto& typeName = prim.GetTypeName();

    auto foundResult = s_importFunctionMap.find(typeName);
    if(foundResult == s_importFunctionMap.end())
    {
        std::cout << "[LightFactory::create] Couldn't find import function for light type '" << typeName.GetString() << "'" << std::endl;
        // We don't have a function registered tpo handle this prim type.
        return nullptr;

    }
    auto func = foundResult->second;
    return func(prim, root, useNetboxes);
}

// static
bool LightFactory::canBeWritten(const OP_Node* node)
{
    TfToken exportFunctionToken;
    if (s_exportFunctionTokenCalculator == nullptr)
    {
        exportFunctionToken = getExportFunctionToken(node);
    }
    else
    {
        exportFunctionToken = s_exportFunctionTokenCalculator(node);
    }

    return s_exportFunctionMap.find(exportFunctionToken) != s_exportFunctionMap.end();
}


// =====================
// Function registration
// =====================

// static
void
LightFactory::registerLightExportFunctionTokenCalculator(exportFunctionTokenCalculator func)
{
    s_exportFunctionTokenCalculator = func;
}

// static
void
LightFactory::registerLightImportFunction(const TfToken& typeName, importFunction func, const bool override)
{
    if (override || s_importFunctionMap.find(typeName) == s_importFunctionMap.end())
    {
        s_importFunctionMap[typeName] = func;
    }
}

// static
void
LightFactory::registerLightExportFunction(const TfToken& typeName, exportFunction func, const bool override)
{
    if (override || s_exportFunctionMap.find(typeName) == s_exportFunctionMap.end())
    {
        s_exportFunctionMap[typeName] = func;
    }
}

// ===============================
// Light specific import functions
// ===============================

OP_Node* ReadTransform::operator()(const UsdPrim& prim, OP_Network* network, const bool useNetboxes)
{
    std::string lightName = prim.GetName();
    auto node = network->createNode("subnet", lightName.c_str());
    setCommonParameters(prim, node);

    node->moveToGoodPosition();

    return node;
}

OP_Node* ReadDiskLight::operator()(const UsdPrim& prim, OP_Network* network, const bool useNetboxes)
{
    std::string lightName = prim.GetName();
    auto node = network->createNode("hlight", lightName.c_str());

    node->setString("disk", CH_STRING_LITERAL, "light_type", 0, 0);

    setCommonParameters(prim, node, "areasize");
    setCommonLightParameters(prim, node);
    setLightShapingParameters(prim, node);

    node->moveToGoodPosition();

    return node;
}

OP_Node* ReadRectLight::operator()(const UsdPrim& prim, OP_Network* network, const bool useNetboxes)
{
    std::string lightName = prim.GetName();
    auto node = network->createNode("hlight", lightName.c_str());

    node->setString("grid", CH_STRING_LITERAL, "light_type", 0, 0);

    setCommonParameters(prim, node, "areasize");
    setCommonLightParameters(prim, node);
    setLightShapingParameters(prim, node);
    std::string textureParmName = "light_texture";
    setLightEmissionParameters(prim, node, textureParmName);

    node->moveToGoodPosition();

    return node;
}

OP_Node* ReadDistantLight::operator()(const UsdPrim& prim, OP_Network* network, const bool useNetboxes)
{
    std::string lightName = prim.GetName();
    auto node = network->createNode("hlight", lightName.c_str());

    node->setString("distant", CH_STRING_LITERAL, "light_type", 0, 0);

    setCommonParameters(prim, node, "orthowidth");
    setCommonLightParameters(prim, node);

    node->moveToGoodPosition();

    return node;
}

OP_Node* ReadSphereLight::operator()(const UsdPrim& prim, OP_Network* network, const bool useNetboxes)
{
    std::string lightName = prim.GetName();
    auto node = network->createNode("hlight", lightName.c_str());

    node->setString("sphere", CH_STRING_LITERAL, "light_type", 0, 0);

    setCommonParameters(prim, node, "areasize");
    setCommonLightParameters(prim, node);

    node->moveToGoodPosition();

    return node;
}

OP_Node* ReadDomeLight::operator()(const UsdPrim& prim, OP_Network* network, const bool useNetboxes)
{
    std::string lightName = prim.GetName();
    auto node = network->createNode("envlight", lightName.c_str());

    setCommonParameters(prim, node);
    setCommonLightParameters(prim, node);

    std::string textureParmName = "env_map";
    setLightEmissionParameters(prim, node, textureParmName);

    node->moveToGoodPosition();

    return node;
}


// ===============================
// Light specific export functions
// ===============================


UsdPrim WriteTransform::operator()(UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    setTime(time, timeCode);

    auto path = getLightPath(node);
    auto xformPrim = UsdGeomXform::Define(stage, SdfPath(path)).GetPrim();

    setCommonParameters(xformPrim, node);

    return xformPrim;
}

UsdPrim WriteRectLight::operator()(UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    setTime(time, timeCode);

    auto path = getLightPath(node);
    auto light = UsdLuxRectLight::Define(stage, SdfPath(path));
    auto lightPrim = light.GetPrim();

    setCommonParameters(lightPrim, node, "areasize");
    setCommonLightParameters(light, node);
    setLightShapingParameters(light, node);
    std::string textureParmName = "light_texture";
    setLightEmissionParameters(light, node, textureParmName);

    return light.GetPrim();
}

UsdPrim WriteDiskLight::operator()(UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    setTime(time, timeCode);

    auto path = getLightPath(node);
    auto light = UsdLuxDiskLight::Define(stage, SdfPath(path));
    auto lightPrim = light.GetPrim();

    setCommonParameters(lightPrim, node, "areasize");
    setCommonLightParameters(light, node);
    setLightShapingParameters(light, node);

    return light.GetPrim();
}

UsdPrim WriteDistantLight::operator()(UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    setTime(time, timeCode);
    auto path = getLightPath(node);
    auto light = UsdLuxDistantLight::Define(stage, SdfPath(path));
    auto lightPrim = light.GetPrim();

    setCommonParameters(lightPrim, node, "orthowidth");
    setCommonLightParameters(light, node);

    return light.GetPrim();
}

UsdPrim WriteSphereLight::operator()(UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    setTime(time, timeCode);
    auto path = getLightPath(node);
    auto light = UsdLuxSphereLight::Define(stage, SdfPath(path));
    auto lightPrim = light.GetPrim();

    setCommonParameters(lightPrim, node, "areasize");
    setCommonLightParameters(light, node);

    return light.GetPrim();
}

UsdPrim WriteDomeLight::operator()(UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    setTime(time, timeCode);
    auto path = getLightPath(node);
    auto light = UsdLuxDomeLight::Define(stage, SdfPath(path));
    auto lightPrim = light.GetPrim();

    setCommonParameters(lightPrim, node);
    setCommonLightParameters(light, node);

    std::string textureParmName = "env_map";
    setLightEmissionParameters(light, node, textureParmName);

    return light.GetPrim();
}

// =======================
// Parameter setter groups
// =======================

void ReadCommonLight::
setCommonParameters(const UsdPrim& prim, OP_Node* node, const char* scaleParmName, const char* rotateParmName,
    const char* translateParmName, const char* rotOrderParmName)
{
    auto stage = prim.GetStage();

    auto& translationParm = node->getParm(translateParmName);
    auto& rotationParm = node->getParm(rotateParmName);
    auto& scaleParm = node->getParm(scaleParmName);

    GfVec3d translation;
    GfVec3f rotation, scale, pivot;
    UsdGeomXformCommonAPI::RotationOrder rotOrder = UsdGeomXformCommonAPI::RotationOrderZXY;

    pxr::UsdGeomXformCommonAPI xform (prim);
    pxr::UsdGeomXformable xformable(prim);

    std::vector<double> timeCodes;
    xformable.GetTimeSamples(&timeCodes);

    bool setKey = true;

    // If the all the attributes are static, use the default time code to retrieve the values
    if (timeCodes.size() == 0)
    {
        timeCodes.push_back(UsdTimeCode::Default().GetValue());
        setKey = false;
    }

    bool isFirstRotation = true;
    Imath::V3f previousTempRot;

    for (auto& timeCode: timeCodes)
    {
        xform.GetXformVectorsByAccumulation(&translation, &rotation, &scale, &pivot, &rotOrder, timeCode);

        fpreal time = GusdUSD_Utils::GetNumericHoudiniTime(timeCode, stage);
        GusdUSD_AttributeTransfer::setValue(time, translationParm, translation, setKey);
        GusdUSD_AttributeTransfer::setValue(time, scaleParm, scale, setKey);

        // The returned rotation might be a calculated value. Unfortunately, it might suffer from Euler flips.
        // Trying to get rid of Euler flips as much as possible now before setting the attribute

        // Imath::V3f is working with radians so we have to convert our degree based rotation first
        // The rotation order doesn't matter as we will convert with the same indices back again.
        Imath::V3f tempRot{
                pxr::GfDegreesToRadians(rotation[0]),
                pxr::GfDegreesToRadians(rotation[1]),
                pxr::GfDegreesToRadians(rotation[2])};

        // Take the first rotation as it is
        if (isFirstRotation)
        {
            isFirstRotation = false;
        }
        else
        {
            // Get rid of Euler flip
            Imath::Euler<float>::nearestRotation(tempRot, previousTempRot);
        }
        // Cache rotation for next timeCode
        previousTempRot = tempRot;

        // Convert the radians back to degrees while maintaining the original order
        rotation[0] = pxr::GfRadiansToDegrees(tempRot[0]);
        rotation[1] = pxr::GfRadiansToDegrees(tempRot[1]);
        rotation[2] = pxr::GfRadiansToDegrees(tempRot[2]);

        // Finally we can set the rotation without euler flips
        GusdUSD_AttributeTransfer::setValue(time, rotationParm, rotation, setKey);
    }

    // Rotation order can't be animated to set it directly
    node->setString(s_rotationOrderMappingUsdToHou[rotOrder].c_str(), CH_STRING_LITERAL, rotOrderParmName, 0, 0);
}

void
ReadCommonLight::setCommonLightParameters(const UsdPrim& prim, OP_Node* node)
{
    auto stage = prim.GetStage();
    auto light = UsdLuxLight(prim);

    auto& lightColorParm = node->getParm("light_color");
    auto& lightExposureParm = node->getParm("light_exposure");
    auto& lightIntensityParm = node->getParm("light_intensity");
    auto& lightEnableParm = node->getParm("light_enable");

    auto colorAttr = light.GetColorAttr();
    auto exposureAttr = light.GetExposureAttr();
    auto intensityAttr = light.GetIntensityAttr();
    auto visibilityAttr = light.GetVisibilityAttr();

    auto fps = stage->GetFramesPerSecond();

    GusdUSD_AttributeTransfer::transferAttribute<GfVec3f>(colorAttr, lightColorParm, fps);
    GusdUSD_AttributeTransfer::transferAttribute<float>(exposureAttr, lightExposureParm, fps);
    GusdUSD_AttributeTransfer::transferAttribute<float>(intensityAttr, lightIntensityParm, fps);
    GusdUSD_AttributeTransfer::transferAttribute<TfToken>(visibilityAttr, lightEnableParm, fps, [&](TfToken input)->int{return (int)(input == "inherited");});
}

void
ReadCommonLight::setLightEmissionParameters(const UsdPrim& prim, OP_Node* node, const std::string& textureParmName)
{
    auto stage = prim.GetStage();
    auto fps = stage->GetFramesPerSecond();

    auto textureAttr = prim.GetAttribute(TfToken("texture:file"));

    auto& textureParm = node->getParm(textureParmName.c_str());
    GusdUSD_AttributeTransfer::transferAttribute<SdfAssetPath>(textureAttr, textureParm, fps);
}

void
ReadCommonLight::setLightShapingParameters(const UsdPrim& prim, OP_Node* node)
{
    auto stage = prim.GetStage();
    auto fps = stage->GetFramesPerSecond();
    auto shapingAPI = UsdLuxShapingAPI(prim);

    auto coneAngleAttr = shapingAPI.GetShapingConeAngleAttr();
    auto coneSoftnessAttr = shapingAPI.GetShapingConeSoftnessAttr();

    if ((coneAngleAttr.IsValid() && coneAngleAttr.IsAuthored()) || (coneSoftnessAttr.IsValid() && coneSoftnessAttr.IsAuthored()))
    {
        auto& coneAngleParm = node->getParm("coneenable");
        GusdUSD_AttributeTransfer::setValue(0, coneAngleParm, true);
    }

    auto& coneAngleParm = node->getParm("coneangle");
    auto& coneDeltaParm = node->getParm("conedelta");

    GusdUSD_AttributeTransfer::transferAttribute<float>(coneAngleAttr, coneAngleParm, fps);
    GusdUSD_AttributeTransfer::transferAttribute<float>(coneSoftnessAttr, coneDeltaParm, fps);
}


// =============================
// Common light export functions
// =============================

void WriteCommonLight::setCommonParameters(UsdPrim& prim, const OP_Node* node, const char* scaleParmName,
    const char* rotateParmName, const char* translateParmName, const char* rotOrderParmName)
{
    auto& translationParm = node->getParm(translateParmName);
    auto& rotationParm = node->getParm(rotateParmName);
    auto& scaleParm = node->getParm(scaleParmName);
    auto& rotationOrderParm = node->getParm(rotOrderParmName);

    UsdGeomXformCommonAPI xform (prim);

    auto translation = GusdUSD_AttributeTransfer::getVector<fpreal, GfVec3d>(translationParm, getTime());
    auto rotation = GusdUSD_AttributeTransfer::getVector<fpreal, GfVec3f>(rotationParm, getTime());
    auto scale = GusdUSD_AttributeTransfer::getVector<fpreal, GfVec3f>(scaleParm, getTime());
    auto rotOrder = GusdUSD_AttributeTransfer::getValue<UT_String>(rotationOrderParm, getTime(), 0);

    xform.SetTranslate(translation, getTimeCode());
    xform.SetRotate(rotation, s_rotationOrderMappingHouToUsd[rotOrder], getTimeCode());
    xform.SetScale(scale, getTimeCode());
}

void WriteCommonLight::setCommonLightParameters(UsdLuxLight& lightPrim, const OP_Node* node)
{
    auto& lightColorParm = node->getParm("light_color");
    auto& lightExposureParm = node->getParm("light_exposure");
    auto& lightIntensityParm = node->getParm("light_intensity");
    auto& lightEnableParm = node->getParm("light_enable");

    auto colorAttr = lightPrim.CreateColorAttr();
    auto exposureAttr = lightPrim.CreateExposureAttr();
    auto intensityAttr = lightPrim.CreateIntensityAttr();
    auto visibilityAttr = lightPrim.CreateVisibilityAttr();

    GusdUSD_AttributeTransfer::transferVectorAttribute<fpreal, GfVec3f>(lightColorParm, colorAttr, getTime(), getTimeCode());
    GusdUSD_AttributeTransfer::transferAttribute<fpreal, float>(lightExposureParm, exposureAttr, getTime(), getTimeCode());
    GusdUSD_AttributeTransfer::transferAttribute<fpreal, float>(lightIntensityParm, intensityAttr, getTime(), getTimeCode());
    GusdUSD_AttributeTransfer::transferAttributeWithConversion<exint>(lightEnableParm, visibilityAttr, getTime(), getTimeCode(), [&](exint input)->TfToken{return input == 1 ? TfToken("inherited") : TfToken("invisible");});
}

void WriteCommonLight::setLightEmissionParameters(UsdLuxLight& lightPrim, const OP_Node* node, const std::string& textureParmName)
{
    auto textureAttr = lightPrim.GetPrim().GetAttribute(TfToken("texture:file"));

    auto& textureParm = node->getParm(textureParmName.c_str());
    GusdUSD_AttributeTransfer::transferAttributeWithConversion<UT_String>(textureParm, textureAttr, getTime(), getTimeCode(), [&](UT_String input)->SdfAssetPath{return SdfAssetPath(input.c_str());});
}

void WriteCommonLight::setLightShapingParameters(UsdLuxLight& lightPrim, const OP_Node* node)
{
    auto& coneEnableParm = node->getParm("coneenable");
    int coneEnable;
    coneEnableParm.getValue(0, coneEnable, 0, 0);

    if (coneEnable == 0)
    {
        // Not enabled so we don't export it any of the shaping parameters
        return;
    }

    auto& coneAngleParm = node->getParm("coneangle");
    auto& coneDeltaParm = node->getParm("conedelta");

    auto shapingAPI = UsdLuxShapingAPI(lightPrim);

    auto coneAngleAttr = shapingAPI.CreateShapingConeAngleAttr();
    auto coneSoftnessAttr = shapingAPI.CreateShapingConeSoftnessAttr();

    GusdUSD_AttributeTransfer::transferAttribute<fpreal, float>(coneAngleParm, coneAngleAttr, getTime(), getTimeCode());
    GusdUSD_AttributeTransfer::transferAttribute<fpreal, float>(coneDeltaParm, coneSoftnessAttr, getTime(), getTimeCode());
}


std::string
WriteCommonLight::getLightPath(const OP_Node* light)
{
    UT_String subNetPath;
    light->getPathWithSubnet(subNetPath);
    std::string path = "/";
    path += subNetPath;

    return path;
}


PXR_NAMESPACE_CLOSE_SCOPE
