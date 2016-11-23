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
#include "usdMaya/MayaPrimWriter.h"

#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"
#include "usdMaya/translatorGprim.h"
#include "usdMaya/primWriterContext.h"
#include "usdMaya/AttributeConverter.h"
#include "usdMaya/AttributeConverterRegistry.h"

#include "pxr/base/gf/gamma.h"

#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usd/inherits.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MObjectArray.h>
#include <maya/MColorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MPlugArray.h>
#include <maya/MUintArray.h>
#include <maya/MColor.h>
#include <maya/MFnSet.h>

MayaPrimWriter::MayaPrimWriter(MDagPath & iDag, 
                               UsdStageRefPtr stage, 
                               const JobExportArgs & iArgs) :
    mDagPath(iDag),
    mStage(stage),
    mIsValid(true),
    mArgs(iArgs)
{
    // XXX: see MayaTransformWriter where it will eventually muck with
    // this path to get the right behavior.  Ideally, we should have all the
    // mergeTransformAndShape logic in one spot.
    mUsdPath = PxrUsdMayaUtil::MDagPathToUsdPath(iDag, false);

    if (!mArgs.usdModelRootOverridePath.IsEmpty() ) {
        mUsdPath = mUsdPath.ReplacePrefix(mUsdPath.GetPrefixes()[0], mArgs.usdModelRootOverridePath);
    }
}

bool
MayaPrimWriter::writePrimAttrs(const MDagPath &dagT, const UsdTimeCode &usdTime, UsdGeomImageable &primSchema) 
{
    MStatus status;
    MFnDependencyNode depFn(getDagPath().node());
    MFnDependencyNode depFnT(dagT.node()); // optionally also scan a shape's transform if merging transforms

    if (getArgs().exportVisibility) {
        bool isVisible  = true;   // if BOTH shape or xform is animated, then visible
        bool isAnimated = false;  // if either shape or xform is animated, then animated

        PxrUsdMayaUtil::getPlugValue(depFn, "visibility", &isVisible, &isAnimated);

        if ( dagT.isValid() ) {
            bool isVis, isAnim;
            if (PxrUsdMayaUtil::getPlugValue(depFnT, "visibility", &isVis, &isAnim)){
                isVisible = isVisible and isVis;
                isAnimated = isAnimated or isAnim;
            }
        }

        TfToken const &visibilityTok = (isVisible ? UsdGeomTokens->inherited : 
                                        UsdGeomTokens->invisible);
        if (usdTime.IsDefault() != isAnimated ) {
            if (usdTime.IsDefault())
                primSchema.CreateVisibilityAttr(VtValue(visibilityTok), true);
            else
                primSchema.CreateVisibilityAttr().Set(visibilityTok, usdTime);
        }
    }

    UsdPrim usdPrim = primSchema.GetPrim();

    // There is no Gprim abstraction in this module, so process the few
    // gprim attrs here.
    UsdGeomGprim gprim = UsdGeomGprim(usdPrim);
    if (gprim and usdTime.IsDefault()){

        PxrUsdMayaPrimWriterContext* unused = NULL;
        PxrUsdMayaTranslatorGprim::Write(
                getDagPath().node(),
                gprim,
                unused);

    }

    // Process special "USD_" attributes.
    std::vector<const AttributeConverter*> converters =
            AttributeConverterRegistry::GetAllConverters();
    for (const AttributeConverter* converter : converters) {
        // We want the node for the xform (depFnT).
        converter->MayaToUsd(depFnT, usdPrim, usdTime);
    }
    
    // Write user-tagged export attributes. Write attributes on the transform
    // first, and then attributes on the shape node. This means that attribute
    // name collisions will always be handled by taking the shape node's value
    // if we're merging transforms and shapes.
    if (dagT.isValid() and !(dagT == getDagPath())) {
        PxrUsdMayaWriteUtil::WriteUserExportedAttributes(dagT, usdPrim, usdTime);
    }
    PxrUsdMayaWriteUtil::WriteUserExportedAttributes(getDagPath(), usdPrim, usdTime);

    return true;
}

bool
MayaPrimWriter::exportsGprims() const
{
    return false;
}
    
bool
MayaPrimWriter::exportsReferences() const
{
    return false;
}

bool
MayaPrimWriter::shouldPruneChildren() const
{
    return false;
}

