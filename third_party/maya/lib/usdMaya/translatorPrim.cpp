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
#include "usdMaya/translatorPrim.h"

#include "usdMaya/translatorUtil.h"
#include "usdMaya/util.h"
#include "usdMaya/AttributeConverter.h"
#include "usdMaya/AttributeConverterRegistry.h"

#include "pxr/usd/usdGeom/imageable.h"

#include <maya/MFnAnimCurve.h>
#include <maya/MFnDagNode.h>

PXR_NAMESPACE_OPEN_SCOPE


void
PxrUsdMayaTranslatorPrim::Read(
        const UsdPrim& prim,
        MObject mayaNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    UsdGeomImageable primSchema(prim);
    if (!primSchema) {
        TF_CODING_ERROR("Prim %s is not UsdGeomImageable.", 
                prim.GetPath().GetText());
        return;
    }

    // Gather visibility
    // If args.GetReadAnimData() is TRUE,
    // pick the first avaiable sample or default
    UsdTimeCode visTimeSample=UsdTimeCode::EarliestTime();
    std::vector<double> visTimeSamples;
    size_t visNumTimeSamples = 0;
    if (args.GetReadAnimData()) {
        PxrUsdMayaTranslatorUtil::GetTimeSamples(primSchema.GetVisibilityAttr(),
                args, &visTimeSamples);
        visNumTimeSamples = visTimeSamples.size();
        if (visNumTimeSamples>0) {
            visTimeSample = visTimeSamples[0];
        }
    }

    MStatus status;
    MFnDependencyNode depFn(mayaNode);
    TfToken visibilityTok;

    if (primSchema.GetVisibilityAttr().Get(&visibilityTok, visTimeSample)){
        PxrUsdMayaUtil::setPlugValue(depFn, "visibility",
                           visibilityTok != UsdGeomTokens->invisible);
    }

    // == Animation ==
    if (visNumTimeSamples > 0) {
        size_t numTimeSamples = visNumTimeSamples;
        MDoubleArray valueArray(numTimeSamples);

        // Populate the channel arrays
        for (unsigned int ti=0; ti < visNumTimeSamples; ++ti) {
            primSchema.GetVisibilityAttr().Get(&visibilityTok, visTimeSamples[ti]);
            valueArray[ti] = (visibilityTok != UsdGeomTokens->invisible);
        }

        // == Write to maya node ==
        MFnDagNode depFn(mayaNode);
        MPlug plg;
        MFnAnimCurve animFn;

        // Construct the time array to be used for all the keys
        MTimeArray timeArray;
        timeArray.setLength(numTimeSamples);
        for (unsigned int ti=0; ti < numTimeSamples; ++ti) {
            timeArray.set( MTime(visTimeSamples[ti]), ti);
        }

        // Add the keys
        plg = depFn.findPlug( "visibility" );
        if ( !plg.isNull() ) {
            MObject animObj = animFn.create(plg, NULL, &status);
            animFn.addKeys(&timeArray, &valueArray);
            if (context) {
                context->RegisterNewMayaNode(
                        animFn.name().asChar(), animObj ); // used for undo/redo
            }
        }
    }
    
    // Set "USD_" attributes to store USD-specific info on the Maya node.
    // XXX: Handle animation properly in attribute converters.
    std::vector<const AttributeConverter*> converters =
            AttributeConverterRegistry::GetAllConverters();
    for (const AttributeConverter* converter : converters) {
        converter->UsdToMaya(prim, depFn, UsdTimeCode::EarliestTime());
    }

    // XXX What about all the "user attributes" that PrimWriter exports???
}

PXR_NAMESPACE_CLOSE_SCOPE

