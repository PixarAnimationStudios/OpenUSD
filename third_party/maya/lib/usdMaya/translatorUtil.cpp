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
#include "usdMaya/translatorUtil.h"

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/translatorXformable.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/xformable.h"

#include <maya/MDagModifier.h>
#include <maya/MObject.h>
#include <maya/MString.h>


/* static */
bool
PxrUsdMayaTranslatorUtil::CreateTransformNode(
        const UsdPrim& usdPrim,
        MObject& parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    static const MString _defaultTypeName("transform");

    if (!usdPrim || !usdPrim.IsA<UsdGeomXformable>()) {
        return false;
    }

    if (!CreateNode(usdPrim,
                       _defaultTypeName,
                       parentNode,
                       context,
                       status,
                       mayaNodeObj)) {
        return false;
    }

    // Read xformable attributes from the UsdPrim on to the transform node.
    UsdGeomXformable xformable(usdPrim);
    PxrUsdMayaTranslatorXformable::Read(xformable, *mayaNodeObj, args, context);

    return true;
}

/* static */
bool
PxrUsdMayaTranslatorUtil::CreateNode(
        const UsdPrim& usdPrim,
        const MString& nodeTypeName,
        MObject& parentNode,
        PxrUsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    if (!CreateNode(MString(usdPrim.GetName().GetText()),
                       nodeTypeName,
                       parentNode,
                       status,
                       mayaNodeObj)) {
        return false;
    }

    if (context) {
        context->RegisterNewMayaNode(usdPrim.GetPath().GetString(), *mayaNodeObj);
    }

    return true;
}

/* static */
bool
PxrUsdMayaTranslatorUtil::CreateNode(
        const MString& nodeName,
        const MString& nodeTypeName,
        MObject& parentNode,
        MStatus* status,
        MObject* mayaNodeObj)
{
    // XXX:
    // Using MFnDagNode::create() results in nodes that are not properly
    // registered with parent scene assemblies. For now, just massaging the
    // transform code accordingly so that child scene assemblies properly post
    // their edits to their parents-- if this is indeed the best pattern for
    // this, all Maya*Reader node creation needs to be adjusted accordingly (for
    // much less trivial cases like MFnMesh).
    MDagModifier dagMod;
    *mayaNodeObj = dagMod.createNode(nodeTypeName, parentNode, status);
    CHECK_MSTATUS_AND_RETURN(*status, false);
    *status = dagMod.renameNode(*mayaNodeObj, nodeName);
    CHECK_MSTATUS_AND_RETURN(*status, false);
    *status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(*status, false);

    return TF_VERIFY(!mayaNodeObj->isNull());
}
