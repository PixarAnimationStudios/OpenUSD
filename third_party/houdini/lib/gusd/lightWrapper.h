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
#ifndef __GUSD_LIGHTWRAPPER_H__
#define __GUSD_LIGHTWRAPPER_H__

#include <RE/RE_Light.h>
#include <OP/OP_Network.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdLux/light.h>

PXR_NAMESPACE_OPEN_SCOPE

using TransformMapping = std::map<SdfPath, std::string>;

class UsdLightWrapper
{
public:
    static UsdPrim write(const UsdStagePtr& stage, OP_Node* node, const float time, const UsdTimeCode& timeCode);
    static OP_Node* read(const UsdPrim& prim, const UT_String& overridePolicy, const bool useNetboxes, const TransformMapping& transformMapping);
    static bool canBeWritten(const OP_Node* node);

private:
    static OP_Network* findPrimParentNetwork(const UsdPrim& prim, const TransformMapping& transformMapping);
    static std::string getParentNetworkPath(const UsdPrim& prim, const TransformMapping& transformMapping);

private:
    static OP_Network* getRootScene();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_LIGHTWRAPPER_H__

