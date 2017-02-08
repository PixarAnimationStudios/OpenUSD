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
#include "usdMaya/stageData.h"

#include "pxr/base/gf/bbox3d.h"

#include <maya/MGlobal.h>

PXR_NAMESPACE_OPEN_SCOPE


/* This exists solely to make sure that the usdStage instance
 * gets discarded when Maya exits, so that an temporary files
 * that might have been created are unlinked.
 */
static void _cleanUp(void *gdPtr) 
{
    UsdMayaStageData *gd = (UsdMayaStageData *)gdPtr;

    gd->unregisterExitCallback();

    gd->stage = UsdStageRefPtr();
}

void* UsdMayaStageData::creator(
        const PluginStaticData& psData) {

        return new UsdMayaStageData(psData);
}

void UsdMayaStageData::copy(
    const MPxData& aDatum) {

        const UsdMayaStageData* aUsdStageData =
            dynamic_cast<const UsdMayaStageData*>(&aDatum);

        if(aUsdStageData) {
            stage = aUsdStageData->stage;
            primPath = aUsdStageData->primPath;
        }
}

MTypeId UsdMayaStageData::typeId() const {

        return _psData.typeId;
}

MString UsdMayaStageData::name() const {

        return _psData.typeName;
}

UsdMayaStageData::UsdMayaStageData(
        const PluginStaticData& psData)
    : MPxGeometryData()
      , _psData(psData)
{
    registerExitCallback();
}

void
UsdMayaStageData::registerExitCallback()
{
    _exitCallbackId = MSceneMessage::addCallback(MSceneMessage::kMayaExiting,
						 _cleanUp,
						 this);
}

void
UsdMayaStageData::unregisterExitCallback()
{
    MSceneMessage::removeCallback(_exitCallbackId);
}
    

UsdMayaStageData::~UsdMayaStageData() {

    unregisterExitCallback();
}



PXR_NAMESPACE_CLOSE_SCOPE

