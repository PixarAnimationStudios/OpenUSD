//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"
#include "usdMaya/pluginStaticData.h"
#include "usdMaya/proxyShape.h"


PXR_NAMESPACE_OPEN_SCOPE


// NOTE: Since we have assets already with the typeid baked in,
// we aren't changing them.  For future usd development, we've blocked
// off the following node ids.
// 0x00126400 - 0x001264ff
PxrUsdMayaPluginStaticData PxrUsdMayaPluginStaticData::pxrUsd(
        // UsdMayaProxyShape
        MTypeId(0x0010A259), MString(PxrUsdMayaProxyShapeTokens->MayaTypeName.GetText()),

        // UsdMayaReferenceAssembly
        MTypeId(0x0010A251), MString("pxrUsdReferenceAssembly"),

        // UsdMayaStageData
        MTypeId(0x0010A257), MString("pxrUsdStageData"));

PxrUsdMayaPluginStaticData::PxrUsdMayaPluginStaticData(
        const MTypeId& proxyShapeId,
        const MString& proxyShapeName,
        const MTypeId& refAssemblyId,
        const MString& refAssemblyName,
        const MTypeId& stageDataId,
        const MString& stageDataName)
    : 
        proxyShape(proxyShapeId, proxyShapeName, stageDataId),
        referenceAssembly(refAssemblyId, refAssemblyName, stageDataId, proxyShape),
        stageData(stageDataId, stageDataName)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
