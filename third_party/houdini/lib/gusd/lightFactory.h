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
#ifndef USD_LIGHTFACTORY_H
#define USD_LIGHTFACTORY_H

// pxr
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdLux/light.h>

// houdini
#include <RE/RE_Light.h>
#include <OP/OP_Network.h>

PXR_NAMESPACE_OPEN_SCOPE

using exportFunctionTokenCalculator = std::function<TfToken(const OP_Node*)>;

using importFunction = std::function<OP_Node*(const UsdPrim&, OP_Network*, const bool)>;
using importFunctionMap = std::map<TfToken, importFunction>;

using exportFunction = std::function<UsdPrim(UsdStageRefPtr, const OP_Node*, const float, const UsdTimeCode&)>;
using exportFunctionMap = std::map<TfToken, exportFunction>;

struct ReadCommonLight
{
protected:
    void setCommonParameters(
            const UsdPrim& prim,
            OP_Node* node,
            const char* scaleParmName = "s",
            const char* rotateParmName = "r",
            const char* translateParmName = "t",
            const char* rotOrderParmName = "rOrd");
    void setCommonLightParameters(const UsdPrim& prim, OP_Node* node);
    void setLightEmissionParameters(const UsdPrim& prim, OP_Node* node, const std::string& textureParmName);
    void setLightShapingParameters(const UsdPrim& prim, OP_Node* node);
};

struct ReadTransform: public ReadCommonLight
{
    OP_Node* operator () (const UsdPrim& prim, OP_Network* network, const bool useNetboxes);
};
struct ReadRectLight: public ReadCommonLight
{
    OP_Node* operator () (const UsdPrim& prim, OP_Network* network, const bool useNetboxes);
};
struct ReadDiskLight: public ReadCommonLight
{
    OP_Node* operator () (const UsdPrim& prim, OP_Network* network, const bool useNetboxes);
};
struct ReadSphereLight: public ReadCommonLight
{
    OP_Node* operator () (const UsdPrim& prim, OP_Network* network, const bool useNetboxes);
};
struct ReadDomeLight: public ReadCommonLight
{
    OP_Node* operator () (const UsdPrim& prim, OP_Network* network, const bool useNetboxes);
};
struct ReadDistantLight: public ReadCommonLight
{
    OP_Node* operator () (const UsdPrim& prim, OP_Network* network, const bool useNetboxes);
};

struct WriteCommonLight
{
protected:
    void setCommonParameters(
            UsdPrim& prim,
            const OP_Node* node,
            const char* scaleParmName = "s",
            const char* rotateParmName = "r",
            const char* translateParmName = "t",
            const char* rotOrderParmName = "rOrd");
    void setCommonLightParameters(UsdLuxLight& lightPrim, const OP_Node* node);
    void setLightEmissionParameters(UsdLuxLight& lightPrim, const OP_Node* node, const std::string& textureParmName);
    void setLightShapingParameters(UsdLuxLight& lightPrim, const OP_Node* node);
    std::string getLightPath(const OP_Node* light);

    const float getTime() { return m_time; }
    const UsdTimeCode& getTimeCode() { return m_timeCode; }
    void setTime(const float time, const UsdTimeCode& timeCode) { m_time = time; m_timeCode = timeCode; }

private:
    float m_time = 1.0f;
    UsdTimeCode m_timeCode;
};

struct WriteTransform: public WriteCommonLight
{
    UsdPrim operator () (UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode);
};
struct WriteRectLight: public WriteCommonLight
{
    UsdPrim operator () (UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode);
};
struct WriteDiskLight: public WriteCommonLight
{
    UsdPrim operator () (UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode);
};
struct WriteSphereLight: public WriteCommonLight
{
    UsdPrim operator () (UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode);
};
struct WriteDomeLight: public WriteCommonLight
{
    UsdPrim operator () (UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode);
};
struct WriteDistantLight: public WriteCommonLight
{
    UsdPrim operator () (UsdStageRefPtr stage, const OP_Node* node, const float time, const UsdTimeCode& timeCode);
};

class LightFactory
{
public:
    static void registerLightExportFunctionTokenCalculator(exportFunctionTokenCalculator func);
    static void registerLightImportFunction(const TfToken& typeName, importFunction func, const bool override=true);
    static void registerLightExportFunction(const TfToken& typeName, exportFunction func, const bool override=true);
    static OP_Node* create(const UsdPrim& prim, OP_Network* root, const bool useNetboxes);
    static UsdPrim create(UsdStageRefPtr stage, const OP_Node* light, const float time, const UsdTimeCode& timeCode);
    static bool canBeWritten(const OP_Node* node);

private:
    static TfToken getExportFunctionToken(const OP_Node* node);

private:
    LightFactory() = default;
    static importFunctionMap s_importFunctionMap;
    static exportFunctionMap s_exportFunctionMap;
    static exportFunctionTokenCalculator s_exportFunctionTokenCalculator;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //USD_LIGHTFACTORY_H
