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
#include "usdMaya/translatorUtil.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/translatorXformable.h"
#include "usdMaya/util.h"
#include "usdMaya/xformStack.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/xformable.h"

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MString.h>


PXR_NAMESPACE_OPEN_SCOPE


const MString _DEFAULT_TRANSFORM_TYPE("transform");


/* static */
bool
UsdMayaTranslatorUtil::CreateTransformNode(
        const UsdPrim& usdPrim,
        MObject& parentNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    if (!usdPrim || !usdPrim.IsA<UsdGeomXformable>()) {
        return false;
    }

    if (!CreateNode(usdPrim,
                    _DEFAULT_TRANSFORM_TYPE,
                    parentNode,
                    context,
                    status,
                    mayaNodeObj)) {
        return false;
    }

    // Read xformable attributes from the UsdPrim on to the transform node.
    UsdGeomXformable xformable(usdPrim);
    UsdMayaTranslatorXformable::Read(xformable, *mayaNodeObj, args, context);

    return true;
}

/* static */
bool
UsdMayaTranslatorUtil::CreateDummyTransformNode(
        const UsdPrim& usdPrim,
        MObject& parentNode,
        bool importTypeName,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    if (!usdPrim) {
        return false;
    }

    if (!CreateNode(usdPrim,
                    _DEFAULT_TRANSFORM_TYPE,
                    parentNode,
                    context,
                    status,
                    mayaNodeObj)) {
        return false;
    }

    MFnDagNode dagNode(*mayaNodeObj);

    // Set the typeName on the adaptor.
    if (UsdMayaAdaptor adaptor = UsdMayaAdaptor(*mayaNodeObj)) {
        VtValue typeName;
        if (!usdPrim.HasAuthoredTypeName()) {
            // A regular typeless def.
            typeName = TfToken();
        }
        else if (importTypeName) {
            // Preserve type info for round-tripping.
            typeName = usdPrim.GetTypeName();
        }
        else {
            // Unknown type name; treat this as though it were a typeless def.
            typeName = TfToken();

            // If there is a typename that we're ignoring, leave a note so that
            // we know where it came from.
            const std::string notes = TfStringPrintf(
                    "Imported from @%s@<%s> with type '%s'",
                    usdPrim.GetStage()->GetRootLayer()->GetIdentifier().c_str(),
                    usdPrim.GetPath().GetText(),
                    usdPrim.GetTypeName().GetText());
            UsdMayaUtil::SetNotes(dagNode, notes);
        }
        adaptor.SetMetadata(SdfFieldKeys->TypeName, typeName);
    }

    // Lock all the transform attributes.
    for (const UsdMayaXformOpClassification& opClass :
            UsdMayaXformStack::MayaStack().GetOps()) {
        if (!opClass.IsInvertedTwin()) {
            MPlug plug = dagNode.findPlug(opClass.GetName().GetText(), true);
            if (!plug.isNull()) {
                if (plug.isCompound()) {
                    for (unsigned int i = 0; i < plug.numChildren(); ++i) {
                        MPlug child = plug.child(i);
                        child.setKeyable(false);
                        child.setLocked(true);
                        child.setChannelBox(false);
                    }
                }
                else {
                    plug.setKeyable(false);
                    plug.setLocked(true);
                    plug.setChannelBox(false);
                }
            }
        }
    }

    return true;
}

/* static */
bool
UsdMayaTranslatorUtil::CreateNode(
        const UsdPrim& usdPrim,
        const MString& nodeTypeName,
        MObject& parentNode,
        UsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    return CreateNode(usdPrim.GetPath(), nodeTypeName, parentNode,
                      context, status, mayaNodeObj);
}

/* static */
bool
UsdMayaTranslatorUtil::CreateNode(
        const SdfPath& path,
        const MString& nodeTypeName,
        MObject& parentNode,
        UsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    if (!CreateNode(MString(path.GetName().c_str(), path.GetName().size()),
                       nodeTypeName,
                       parentNode,
                       status,
                       mayaNodeObj)) {
        return false;
    }

    if (context) {
        context->RegisterNewMayaNode(path.GetString(), *mayaNodeObj);
    }

    return true;
}

/* static */
bool
UsdMayaTranslatorUtil::CreateNode(
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

/* static */
bool
UsdMayaTranslatorUtil::CreateShaderNode(
        const MString& nodeName,
        const MString& nodeTypeName,
        const UsdMayaShadingNodeType shadingNodeType,
        MStatus* status,
        MObject* shaderObj,
        const MObject parentNode)
{
    MString typeFlag;
    switch (shadingNodeType) {
        case UsdMayaShadingNodeType::Light:
            typeFlag = "-al"; // -asLight
            break;
        case UsdMayaShadingNodeType::PostProcess:
            typeFlag = "-app"; // -asPostProcess
            break;
        case UsdMayaShadingNodeType::Rendering:
            typeFlag = "-ar"; // -asRendering
            break;
        case UsdMayaShadingNodeType::Shader:
            typeFlag = "-as"; // -asShader
            break;
        case UsdMayaShadingNodeType::Texture:
            typeFlag = "-icm -at"; // -isColorManaged -asTexture
            break;
        case UsdMayaShadingNodeType::Utility:
            typeFlag = "-au"; // -asUtility
            break;
        default: {
            MFnDependencyNode depNodeFn;
            depNodeFn.create(nodeTypeName, nodeName, status);
            CHECK_MSTATUS_AND_RETURN(*status, false);
            *shaderObj = depNodeFn.object(status);
            CHECK_MSTATUS_AND_RETURN(*status, false);
            return true;
        }
    }

    MString parentFlag;
    if (!parentNode.isNull()) {
        const MFnDagNode parentDag(parentNode, status);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        *status = parentFlag.format(" -p \"^1s\"", parentDag.fullPathName());
        CHECK_MSTATUS_AND_RETURN(*status, false);
    }

    MString cmd;
    // ss = skipSelect
    *status = cmd.format("shadingNode ^1s^2s -ss -n \"^3s\" \"^4s\"",
               typeFlag, parentFlag, nodeName, nodeTypeName);
    CHECK_MSTATUS_AND_RETURN(*status, false);

    const MString createdNode =
        MGlobal::executeCommandStringResult(cmd, false, false, status);
    CHECK_MSTATUS_AND_RETURN(*status, false);

    *status = UsdMayaUtil::GetMObjectByName(createdNode.asChar(), *shaderObj);
    CHECK_MSTATUS_AND_RETURN(*status, false);

    // Lights are unique in that they're the only DAG nodes we might create in
    // this function, so they also involve a transform node. The shadingNode
    // command unfortunately seems to return the transform node for the light
    // and not the light node itself, so we may need to manually find the light
    // so we can return that instead.
    if (shaderObj->hasFn(MFn::kDagNode)) {
        const MFnDagNode dagNodeFn(*shaderObj, status);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        MDagPath dagPath;
        *status = dagNodeFn.getPath(dagPath);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        unsigned int numShapes = 0u;
        *status = dagPath.numberOfShapesDirectlyBelow(numShapes);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        if (numShapes == 1u) {
            *status = dagPath.extendToShape();
            CHECK_MSTATUS_AND_RETURN(*status, false);
            *shaderObj = dagPath.node(status);
            CHECK_MSTATUS_AND_RETURN(*status, false);
        }
    }

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
