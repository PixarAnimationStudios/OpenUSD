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

#include "usdMaya/colorSpace.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MAnimControl.h>
#include <maya/MAnimUtil.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MColor.h>
#include <maya/MDGModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnExpression.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnSet.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTime.h>

#include <string>
#include <unordered_map>
#include <vector>


PXR_NAMESPACE_USING_DIRECTIVE


// return seconds per frame
double
UsdMayaUtil::spf()
{
    static const MTime sec(1.0, MTime::kSeconds);
    return 1.0 / sec.as(MTime::uiUnit());
}

MStatus
UsdMayaUtil::GetMObjectByName(const std::string& nodeName, MObject& mObj)
{
    MSelectionList selectionList;
    MStatus status = selectionList.add(MString(nodeName.c_str()));
    if (status != MS::kSuccess) {
        return status;
    }

    status = selectionList.getDependNode(0, mObj);

    return status;
}

MStatus
UsdMayaUtil::GetDagPathByName(const std::string& nodeName, MDagPath& dagPath)
{
    MSelectionList selectionList;
    MStatus status = selectionList.add(MString(nodeName.c_str()));
    if (status != MS::kSuccess) {
        return status;
    }

    status = selectionList.getDagPath(0, dagPath);

    return status;
}

MStatus
UsdMayaUtil::GetPlugByName(const std::string& attrPath, MPlug& plug)
{
    std::vector<std::string> comps = TfStringSplit(attrPath, ".");
    if (comps.size() != 2) {
        TF_RUNTIME_ERROR("'%s' is not a valid Maya attribute path",
                attrPath.c_str());
        return MStatus::kFailure;
    }

    MObject object;
    MStatus status = GetMObjectByName(comps[0], object);
    if (!status) {
        return status;
    }

    MFnDependencyNode depNode(object, &status);
    if (!status) {
        return status;
    }

    MPlug tmpPlug = depNode.findPlug(comps[1].c_str(), true, &status);
    if (!status) {
        return status;
    }

    plug = tmpPlug;
    return status;
}

MPlug
UsdMayaUtil::GetMayaTimePlug()
{
    MPlug timePlug;
    MStatus status;

    // As an extra sanity check, we only return a discovered plug if its
    // value matches the current time.
    MTime curTime = MAnimControl::currentTime();

    MItDependencyNodes iter(MFn::kTime, &status);
    CHECK_MSTATUS_AND_RETURN(status, timePlug);

    while (!timePlug && !iter.isDone()) {
        MObject node = iter.thisNode();
        iter.next();

        MFnDependencyNode depNodeFn(node, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MPlug outTimePlug = depNodeFn.findPlug("outTime", true, &status);
        if (status != MS::kSuccess || !outTimePlug) {
            continue;
        }

        if (outTimePlug.asMTime() != curTime) {
            continue;
        }

        timePlug = outTimePlug;
    }

    return timePlug;
}

bool
UsdMayaUtil::isAncestorDescendentRelationship(
        const MDagPath& path1,
        const MDagPath& path2)
{
    unsigned int length1 = path1.length();
    unsigned int length2 = path2.length();
    unsigned int diff;

    if (length1 == length2 && !(path1 == path2))
        return false;
    MDagPath ancestor, descendent;
    if (length1 > length2)
    {
        ancestor = path2;
        descendent = path1;
        diff = length1 - length2;
    }
    else
    {
        ancestor = path1;
        descendent = path2;
        diff = length2 - length1;
    }

    descendent.pop(diff);

    return ancestor == descendent;
}

// returns 0 if static, 1 if sampled, and 2 if a curve
int
UsdMayaUtil::getSampledType(
        const MPlug& iPlug,
        const bool includeConnectedChildren)
{
    MPlugArray conns;

    iPlug.connectedTo(conns, true, false);

    // it's possible that only some element of an array plug or
    // some component of a compound plus is connected
    if (conns.length() == 0)
    {
        if (iPlug.isArray())
        {
            unsigned int numConnectedElements = iPlug.numConnectedElements();
            for (unsigned int e = 0; e < numConnectedElements; e++)
            {
                // For now we assume that when you encounter an array of plugs,
                // we always want to include connected children.
                int retVal = getSampledType(iPlug.connectionByPhysicalIndex(e),
                                            true);
                if (retVal > 0) {
                    return retVal;
                }
            }
        }
        else if (iPlug.isCompound() && iPlug.numConnectedChildren() > 0
                && includeConnectedChildren)
        {
            unsigned int numChildren = iPlug.numChildren();
            for (unsigned int c = 0; c < numChildren; c++)
            {
                int retVal = getSampledType(iPlug.child(c), true);
                if (retVal > 0)
                    return retVal;
            }
        }
        return 0;
    }

    MObject ob;
    MFnDependencyNode nodeFn;
    for (unsigned i = 0; i < conns.length(); i++)
    {
        ob = conns[i].node();
        MFn::Type type = ob.apiType();

        switch (type)
        {
            case MFn::kAnimCurveTimeToAngular:
            case MFn::kAnimCurveTimeToDistance:
            case MFn::kAnimCurveTimeToTime:
            case MFn::kAnimCurveTimeToUnitless:
            {
                nodeFn.setObject(ob);
                MPlug incoming = nodeFn.findPlug("i", true);

                // sampled
                if (incoming.isConnected())
                    return 1;

                // curve
                else
                    return 2;
            }
            break;

            case MFn::kMute:
            {
                nodeFn.setObject(ob);
                MPlug mutePlug = nodeFn.findPlug("mute", true);

                // static
                if (mutePlug.asBool())
                    return 0;
                // curve
                else
                   return 2;
            }
            break;

            default:
            break;
        }
    }

    return 1;
}

bool
UsdMayaUtil::getRotOrder(
        const MTransformationMatrix::RotationOrder iOrder,
        unsigned int& oXAxis,
        unsigned int& oYAxis,
        unsigned int& oZAxis)
{
    switch (iOrder)
    {
        case MTransformationMatrix::kXYZ:
        {
            oXAxis = 0;
            oYAxis = 1;
            oZAxis = 2;
        }
        break;

        case MTransformationMatrix::kYZX:
        {
            oXAxis = 1;
            oYAxis = 2;
            oZAxis = 0;
        }
        break;

        case MTransformationMatrix::kZXY:
        {
            oXAxis = 2;
            oYAxis = 0;
            oZAxis = 1;
        }
        break;

        case MTransformationMatrix::kXZY:
        {
            oXAxis = 0;
            oYAxis = 2;
            oZAxis = 1;
        }
        break;

        case MTransformationMatrix::kYXZ:
        {
            oXAxis = 1;
            oYAxis = 0;
            oZAxis = 2;
        }
        break;

        case MTransformationMatrix::kZYX:
        {
            oXAxis = 2;
            oYAxis = 1;
            oZAxis = 0;
        }
        break;

        default:
        {
            return false;
        }
    }
    return true;
}

// 0 dont write, 1 write static 0, 2 write anim 0, 3 write anim -1
int
UsdMayaUtil::getVisibilityType(const MPlug& iPlug)
{
    int type = getSampledType(iPlug, true);

    // static case
    if (type == 0)
    {
        // dont write anything
        if (iPlug.asBool())
            return 0;

        // write static 0
        return 1;
    }
    else
    {
        // anim write -1
        if (iPlug.asBool())
            return 3;

        // write anim 0
        return 2;
    }
}

// does this cover all cases?
bool
UsdMayaUtil::isAnimated(MObject& object, const bool checkParent)
{
    MStatus stat;
    MItDependencyGraph iter(
        object,
        MFn::kInvalid,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kNodeLevel,
        &stat);

    if (stat!= MS::kSuccess)
    {
        TF_RUNTIME_ERROR("Unable to create DG iterator");
    }

    // MAnimUtil::isAnimated(node) will search the history of the node
    // for any animation curve nodes. It will return true for those nodes
    // that have animation curve in their history.
    // The average time complexity is O(n^2) where n is the number of history
    // nodes. But we can improve the best case by split the loop into two.
    std::vector<MObject> nodesToCheckAnimCurve;

    for (; !iter.isDone(); iter.next())
    {
        MObject node = iter.thisNode();

        if (node.hasFn(MFn::kPluginDependNode) ||
                node.hasFn( MFn::kConstraint ) ||
                node.hasFn(MFn::kPointConstraint) ||
                node.hasFn(MFn::kAimConstraint) ||
                node.hasFn(MFn::kOrientConstraint) ||
                node.hasFn(MFn::kScaleConstraint) ||
                node.hasFn(MFn::kGeometryConstraint) ||
                node.hasFn(MFn::kNormalConstraint) ||
                node.hasFn(MFn::kTangentConstraint) ||
                node.hasFn(MFn::kParentConstraint) ||
                node.hasFn(MFn::kPoleVectorConstraint) ||
                node.hasFn(MFn::kParentConstraint) ||
                node.hasFn(MFn::kTime) ||
                node.hasFn(MFn::kJoint) ||
                node.hasFn(MFn::kGeometryFilt) ||
                node.hasFn(MFn::kTweak) ||
                node.hasFn(MFn::kPolyTweak) ||
                node.hasFn(MFn::kSubdTweak) ||
                node.hasFn(MFn::kCluster) ||
                node.hasFn(MFn::kFluid) ||
                node.hasFn(MFn::kPolyBoolOp))
        {
            return true;
        }

        if (node.hasFn(MFn::kExpression))
        {
            MFnExpression fn(node, &stat);
            if (stat == MS::kSuccess && fn.isAnimated())
            {
                return true;
            }
        }

        nodesToCheckAnimCurve.push_back(node);
    }

    for (auto& node : nodesToCheckAnimCurve)
    {
        if (MAnimUtil::isAnimated(node, checkParent))
        {
            return true;
        }
    }

    return false;
}

bool
UsdMayaUtil::isPlugAnimated(const MPlug& plug)
{
    if (plug.isNull()) {
        return false;
    }
    if (plug.isKeyable() && MAnimUtil::isAnimated(plug)) {
        return true;
    }
    if (plug.isDestination()) {
        const MPlug source(GetConnected(plug));
        if (!source.isNull() && MAnimUtil::isAnimated(source.node())) {
            return true;
        }
    }
    return false;
}

bool
UsdMayaUtil::isIntermediate(const MObject& object)
{
    MStatus stat;
    MFnDagNode mFn(object);

    MPlug plug = mFn.findPlug("intermediateObject", false, &stat);
    if (stat == MS::kSuccess && plug.asBool())
        return true;
    else
        return false;
}

bool
UsdMayaUtil::isRenderable(const MObject& object)
{
    MStatus stat;
    MFnDagNode mFn(object);

    // templated turned on?  return false
    MPlug plug = mFn.findPlug("template", false, &stat);
    if (stat == MS::kSuccess && plug.asBool())
        return false;

    // visibility or lodVisibility off?  return false
    plug = mFn.findPlug("visibility", false, &stat);
    if (stat == MS::kSuccess && !plug.asBool())
    {
        // the value is off. let's check if it has any in-connection,
        // otherwise, it means it is not animated.
        MPlugArray arrayIn;
        plug.connectedTo(arrayIn, true, false, &stat);

        if (stat == MS::kSuccess && arrayIn.length() == 0)
        {
            return false;
        }
    }

    plug = mFn.findPlug("lodVisibility", false, &stat);
    if (stat == MS::kSuccess && !plug.asBool())
    {
        MPlugArray arrayIn;
        plug.connectedTo(arrayIn, true, false, &stat);

        if (stat == MS::kSuccess && arrayIn.length() == 0)
        {
            return false;
        }
    }

    // this shape is renderable
    return true;
}

MString
UsdMayaUtil::stripNamespaces(const MString& iNodeName, const int iDepth)
{
    if (iDepth == 0) {
        return iNodeName;
    }

    std::stringstream ss;
    MStringArray pathPartsArray;
    if (iNodeName.split('|', pathPartsArray) == MS::kSuccess) {
        unsigned int partsLen = pathPartsArray.length();
        for (unsigned int i = 0; i < partsLen; ++i) {
            ss << '|';
            MStringArray strArray;
            if (pathPartsArray[i].split(':', strArray) == MS::kSuccess) {
                int len = strArray.length();
                // if iDepth is -1, we don't keep any namespaces
                if (iDepth != -1) {
                    // add any ns beyond iDepth so if name is: "stripped:save1:save2:name" add "save1:save2:",
                    // but if there aren't any to save like: "stripped:name" then add nothing.
                    for (int j = iDepth; j < len - 1; ++j) {
                        ss << strArray[j] << ":";
                    }
                }
                ss << strArray[len - 1];  // add the node name
            }
        }
        auto path = ss.str();
        return MString(path.c_str());
    } else {
        return iNodeName;
    }
}

std::string
UsdMayaUtil::SanitizeName(const std::string& name)
{
    return TfStringReplace(name, ":", "_");
}

// This to allow various pipeline to sanitize the colorset name for output
std::string
UsdMayaUtil::SanitizeColorSetName(const std::string& name)
{
    // We sanitize the name since certain pipeline like Pixar's, we have rman_
    // in front of all color sets that need to be exportred. We now export all
    // colorsets.
    size_t namePos = 0u;
    static const std::string RMAN_PREFIX("rman_");
    if (name.find(RMAN_PREFIX) == 0) {
        namePos = RMAN_PREFIX.size();
    }
    return name.substr(namePos);
}

// Get array (constant or per component) of attached shaders
// Pass a non-zero value for numComponents when retrieving shaders on an object
// that supports per-component shader assignment (e.g. faces of a polymesh).
// In this case, shaderObjs will be the length of the number of shaders
// assigned to the object. assignmentIndices will be the length of
// numComponents, with values indexing into shaderObjs.
// When numComponents is zero, shaderObjs will be of length 1 and
// assignmentIndices will be empty.
static
bool
_getAttachedMayaShaderObjects(
        const MFnDagNode& node,
        const unsigned int numComponents,
        MObjectArray* shaderObjs,
        VtIntArray* assignmentIndices)
{
    bool hasShader=false;
    MStatus status;

    // This structure maps shader object names to their indices in the
    // shaderObjs array. We use this to make sure that we add each unique
    // shader to shaderObjs only once.
    TfHashMap<std::string, size_t, TfHash> shaderPlugsMap;

    shaderObjs->clear();
    assignmentIndices->clear();

    MObjectArray setObjs;
    MObjectArray compObjs;
    node.getConnectedSetsAndMembers(0, setObjs, compObjs, true); // Assuming that not using instancing

    // If we have multiple components and either multiple sets or one set with
    // only a subset of the object in it, we'll keep track of the assignments
    // for all components in assignmentIndices. We initialize all of the
    // assignments as unassigned using a value of -1.
    if (numComponents > 1 &&
            (setObjs.length() > 1 ||
             (setObjs.length() == 1 && !compObjs[0].isNull()))) {
        assignmentIndices->assign((size_t)numComponents, -1);
    }

    for (unsigned int i=0; i < setObjs.length(); ++i) {
        // Get associated Set and Shading Group
        MFnSet setFn(setObjs[i], &status);
        MPlug seSurfaceShaderPlg = setFn.findPlug("surfaceShader", &status);

        // Find connection shader->shadingGroup
        MPlugArray plgCons;
        seSurfaceShaderPlg.connectedTo(plgCons, true, false, &status);
        if (plgCons.length() == 0) {
            continue;
        }

        hasShader = true;
        MPlug shaderPlug = plgCons[0];
        MObject shaderObj = shaderPlug.node();

        auto inserted = shaderPlugsMap.insert(
            std::pair<std::string, size_t>(shaderPlug.name().asChar(),
                                           shaderObjs->length()));
        if (inserted.second) {
            shaderObjs->append(shaderObj);
        }

        // If we are tracking per-component assignments, mark all components of
        // this set as assigned to this shader.
        if (!assignmentIndices->empty()) {
            size_t shaderIndex = inserted.first->second;

            MItMeshPolygon faceIt(node.dagPath(), compObjs[i]);
            for (faceIt.reset(); !faceIt.isDone(); faceIt.next()) {
                (*assignmentIndices)[faceIt.index()] = shaderIndex;
            }
        }
    }

    return hasShader;
}

bool
_GetColorAndTransparencyFromLambert(
        const MObject& shaderObj,
        GfVec3f* rgb,
        float* alpha)
{
    MStatus status;
    MFnLambertShader lambertFn(shaderObj, &status);
    if (status == MS::kSuccess ) {
        if (rgb) {
            GfVec3f displayColor;
            MColor color = lambertFn.color();
            for (int j=0;j<3;j++) {
                displayColor[j] = color[j];
            }
            displayColor *= lambertFn.diffuseCoeff();
            *rgb = UsdMayaColorSpace::ConvertMayaToLinear(displayColor);
        }
        if (alpha) {
            MColor trn = lambertFn.transparency();
            // Assign Alpha as 1.0 - average of shader trasparency
            // and check if they are all the same
            *alpha = 1.0 - ((trn[0] + trn[1] + trn[2]) / 3.0);
        }
        return true;
    }

    return false;
}

bool
_GetColorAndTransparencyFromDepNode(
        const MObject& shaderObj,
        GfVec3f* rgb,
        float* alpha)
{
    MStatus status;
    MFnDependencyNode d(shaderObj);
    MPlug colorPlug = d.findPlug("color", &status);
    if (!status) {
        return false;
    }
    MPlug transparencyPlug = d.findPlug("transparency", &status);
    if (!status) {
        return false;
    }

    if (rgb) {
        GfVec3f displayColor;
        for (int j=0; j<3; j++) {
            colorPlug.child(j).getValue(displayColor[j]);
        }
        *rgb = UsdMayaColorSpace::ConvertMayaToLinear(displayColor);
    }

    if (alpha) {
        float trans = 0.f;
        for (int j=0; j<3; j++) {
            float t = 0.f;
            transparencyPlug.child(j).getValue(t);
            trans += t/3.f;
        }
        (*alpha) = 1.f - trans;
    }
    return true;
}

static
void
_getMayaShadersColor(
        const MObjectArray& shaderObjs,
        VtVec3fArray* RGBData,
        VtFloatArray* AlphaData)
{
    MStatus status;

    if (shaderObjs.length() == 0) {
        return;
    }

    if (RGBData) {
        RGBData->resize(shaderObjs.length());
    }
    if (AlphaData) {
        AlphaData->resize(shaderObjs.length());
    }

    for (unsigned int i = 0; i < shaderObjs.length(); ++i) {
        // Initialize RGB and Alpha to (1,1,1,1)
        if (RGBData) {
            (*RGBData)[i][0] = 1.0;
            (*RGBData)[i][1] = 1.0;
            (*RGBData)[i][2] = 1.0;
        }
        if (AlphaData) {
            (*AlphaData)[i] = 1.0;
        }

        if (shaderObjs[i].isNull()) {
            TF_RUNTIME_ERROR(
                    "Invalid Maya shader object at index %d. "
                    "Unable to retrieve shader base color.",
                    i);
            continue;
        }

        // first, we assume the shader is a lambert and try that API.  if
        // not, we try our next best guess.
        bool gotValues = _GetColorAndTransparencyFromLambert(
                shaderObjs[i],
                RGBData ?  &(*RGBData)[i] : nullptr,
                AlphaData ?  &(*AlphaData)[i] : nullptr)

            || _GetColorAndTransparencyFromDepNode(
                shaderObjs[i],
                RGBData ?  &(*RGBData)[i] : nullptr,
                AlphaData ?  &(*AlphaData)[i] : nullptr);

        if (!gotValues) {
            TF_RUNTIME_ERROR(
                    "Failed to get shaders colors at index %d. "
                    "Unable to retrieve shader base color.",
                    i);
        }
    }
}

static
bool
_GetLinearShaderColor(
        const MFnDagNode& node,
        const unsigned int numComponents,
        VtVec3fArray* RGBData,
        VtFloatArray* AlphaData,
        TfToken* interpolation,
        VtIntArray* assignmentIndices)
{
    MObjectArray shaderObjs;
    bool hasAttachedShader = _getAttachedMayaShaderObjects(
            node, numComponents, &shaderObjs, assignmentIndices);
    if (hasAttachedShader) {
        _getMayaShadersColor(shaderObjs, RGBData, AlphaData);
    }

    if (assignmentIndices && interpolation) {
        if (assignmentIndices->empty()) {
            *interpolation = UsdGeomTokens->constant;
        } else {
            *interpolation = UsdGeomTokens->uniform;
        }
    }

    return hasAttachedShader;
}

bool
UsdMayaUtil::GetLinearShaderColor(
        const MFnDagNode& node,
        VtVec3fArray* RGBData,
        VtFloatArray* AlphaData,
        TfToken* interpolation,
        VtIntArray* assignmentIndices)
{
    return _GetLinearShaderColor(node,
                                 0,
                                 RGBData,
                                 AlphaData,
                                 interpolation,
                                 assignmentIndices);
}

bool
UsdMayaUtil::GetLinearShaderColor(
        const MFnMesh& mesh,
        VtVec3fArray* RGBData,
        VtFloatArray* AlphaData,
        TfToken* interpolation,
        VtIntArray* assignmentIndices)
{
    unsigned int numComponents = mesh.numPolygons();
    return _GetLinearShaderColor(mesh,
                                 numComponents,
                                 RGBData,
                                 AlphaData,
                                 interpolation,
                                 assignmentIndices);
}

namespace {

template <typename T>
struct _ValuesHash
{
    std::size_t operator() (const T& value) const {
        return hash_value(value);
    }
};

// There is no globally defined hash_value for numeric types
// so we need an explicit opt-in here.
template <>
struct _ValuesHash<float>
{
    std::size_t operator() (const float& value) const {
        return boost::hash_value(value);
    }
};

template <typename T>
struct _ValuesEqual
{
    bool operator() (const T& a, const T& b) const {
        return GfIsClose(a, b, 1e-9);
    }
};

} // anonymous namespace

template <typename T>
static
void
_MergeEquivalentIndexedValues(
        VtArray<T>* valueData,
        VtIntArray* assignmentIndices)
{
    if (!valueData || !assignmentIndices) {
        return;
    }

    const size_t numValues = valueData->size();
    if (numValues == 0u) {
        return;
    }

    // We maintain a map of values to that value's index in our uniqueValues
    // array.
    std::unordered_map<T, size_t, _ValuesHash<T>, _ValuesEqual<T> > valuesMap;
    VtArray<T> uniqueValues;
    VtIntArray uniqueIndices;

    for (int index : *assignmentIndices) {
        if (index < 0 || static_cast<size_t>(index) >= numValues) {
            // This is an unassigned or otherwise unknown index, so just keep it.
            uniqueIndices.push_back(index);
            continue;
        }

        const T value = (*valueData)[index];

        int uniqueIndex = -1;

        auto inserted = valuesMap.insert(
            std::pair<T, size_t>(value, uniqueValues.size()));
        if (inserted.second) {
            // This is a new value, so add it to the array.
            uniqueValues.push_back(value);
            uniqueIndex = uniqueValues.size() - 1;
        } else {
            // This is an existing value, so re-use the original's index.
            uniqueIndex = inserted.first->second;
        }

        uniqueIndices.push_back(uniqueIndex);
    }

    // If we reduced the number of values by merging, copy the results back.
    if (uniqueValues.size() < numValues) {
        (*valueData) = uniqueValues;
        (*assignmentIndices) = uniqueIndices;
    }
}

void
UsdMayaUtil::MergeEquivalentIndexedValues(
        VtFloatArray* valueData,
        VtIntArray* assignmentIndices) {
    return _MergeEquivalentIndexedValues<float>(valueData, assignmentIndices);
}

void
UsdMayaUtil::MergeEquivalentIndexedValues(
        VtVec2fArray* valueData,
        VtIntArray* assignmentIndices) {
    return _MergeEquivalentIndexedValues<GfVec2f>(valueData, assignmentIndices);
}

void
UsdMayaUtil::MergeEquivalentIndexedValues(
        VtVec3fArray* valueData,
        VtIntArray* assignmentIndices) {
    return _MergeEquivalentIndexedValues<GfVec3f>(valueData, assignmentIndices);
}

void
UsdMayaUtil::MergeEquivalentIndexedValues(
        VtVec4fArray* valueData,
        VtIntArray* assignmentIndices) {
    return _MergeEquivalentIndexedValues<GfVec4f>(valueData, assignmentIndices);
}

void
UsdMayaUtil::CompressFaceVaryingPrimvarIndices(
        const MFnMesh& mesh,
        TfToken* interpolation,
        VtIntArray* assignmentIndices)
{
    if (!interpolation ||
            !assignmentIndices ||
            assignmentIndices->size() == 0u) {
        return;
    }

    // Use -2 as the initial "un-stored" sentinel value, since -1 is the
    // default unauthored value index for primvars.
    int numPolygons = mesh.numPolygons();
    VtIntArray uniformAssignments;
    uniformAssignments.assign((size_t)numPolygons, -2);

    int numVertices = mesh.numVertices();
    VtIntArray vertexAssignments;
    vertexAssignments.assign((size_t)numVertices, -2);

    // We assume that the data is constant/uniform/vertex until we can
    // prove otherwise that two components have differing values.
    bool isConstant = true;
    bool isUniform = true;
    bool isVertex = true;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        int faceIndex = itFV.faceId();
        int vertexIndex = itFV.vertId();

        int assignedIndex = (*assignmentIndices)[fvi];

        if (isConstant) {
            if (assignedIndex != (*assignmentIndices)[0]) {
                isConstant = false;
            }
        }

        if (isUniform) {
            if (uniformAssignments[faceIndex] < -1) {
                // No value for this face yet, so store one.
                uniformAssignments[faceIndex] = assignedIndex;
            } else if (assignedIndex != uniformAssignments[faceIndex]) {
                isUniform = false;
            }
        }

        if (isVertex) {
            if (vertexAssignments[vertexIndex] < -1) {
                // No value for this vertex yet, so store one.
                vertexAssignments[vertexIndex] = assignedIndex;
            } else if (assignedIndex != vertexAssignments[vertexIndex]) {
                isVertex = false;
            }
        }

        if (!isConstant && !isUniform && !isVertex) {
            // No compression will be possible, so stop trying.
            break;
        }
    }

    if (isConstant) {
        assignmentIndices->resize(1);
        *interpolation = UsdGeomTokens->constant;
    } else if (isUniform) {
        *assignmentIndices = uniformAssignments;
        *interpolation = UsdGeomTokens->uniform;
    } else if(isVertex) {
        *assignmentIndices = vertexAssignments;
        *interpolation = UsdGeomTokens->vertex;
    } else {
        *interpolation = UsdGeomTokens->faceVarying;
    }
}

bool
UsdMayaUtil::SetUnassignedValueIndex(
        VtIntArray* assignmentIndices,
        int* unassignedValueIndex)
{
    if (assignmentIndices == nullptr || unassignedValueIndex == nullptr) {
        return false;
    }

    *unassignedValueIndex = -1;
    for (auto& index: *assignmentIndices) {
        if (index < 0) {
            index = -1;
            *unassignedValueIndex = 0;
        }
    }
    return *unassignedValueIndex == 0;
}

bool
UsdMayaUtil::IsAuthored(MPlug& plug)
{
    MStatus status;

    if (plug.isNull(&status) || status != MS::kSuccess) {
        return false;
    }

    // Plugs with connections are considered authored.
    if (plug.isConnected(&status)) {
        return true;
    }

    MStringArray setAttrCmds;
    status = plug.getSetAttrCmds(setAttrCmds, MPlug::kChanged);
    CHECK_MSTATUS_AND_RETURN(status, false);

    for (unsigned int i = 0u; i < setAttrCmds.length(); ++i) {
        if (setAttrCmds[i].numChars() > 0u) {
            return true;
        }
    }

    return false;
}

MPlug
UsdMayaUtil::GetConnected(const MPlug& plug)
{
    MStatus status = MS::kFailure;
    MPlugArray conn;
    plug.connectedTo(conn, true, false, &status);
    if (!status || conn.length() != 1) {
        return MPlug();
    }
    return conn[0];
}

void
UsdMayaUtil::Connect(
        const MPlug& srcPlug,
        const MPlug& dstPlug,
        const bool clearDstPlug)
{
    MStatus status;
    MDGModifier dgMod;

    if (clearDstPlug) {
        MPlugArray plgCons;
        dstPlug.connectedTo(plgCons, true, false, &status);
        for (unsigned int i=0; i < plgCons.length(); ++i) {
            status = dgMod.disconnect(plgCons[i], dstPlug);
        }
    }

    // Execute the disconnect/connect
    status = dgMod.connect(srcPlug, dstPlug);
    dgMod.doIt();
}

MPlug
UsdMayaUtil::FindChildPlugByName(const MPlug& plug, const MString& name)
{
    unsigned int numChildren = plug.numChildren();
    for(unsigned int i = 0; i < numChildren; ++i) {
        MPlug child = plug.child(i);

        // We can't get at the name of just the *component*
        // plug.name() gives us node.plug[index].compound, etc.
        // partialName() also has no form that just gives us the name.
        MString childName = child.name();
        if(childName.length() > name.length()) {
            int index = childName.rindex('.');
            if(index >= 0) {
                MString childSuffix =
                    childName.substring(index+1, childName.length());
                if(childSuffix == name) {
                    return child;
                }
            }
        }
    }
    return MPlug();
}

// XXX: see logic in UsdMayaTransformWriter.  It's unfortunate that this
// logic is in 2 places.  we should merge.
static
bool
_IsShape(const MDagPath& dagPath)
{
    if (dagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    // go to the parent
    MDagPath parentDagPath = dagPath;
    parentDagPath.pop();
    if (!parentDagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    unsigned int numberOfShapesDirectlyBelow = 0;
    parentDagPath.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
    return (numberOfShapesDirectlyBelow == 1);
}

SdfPath
UsdMayaUtil::MDagPathToUsdPath(
        const MDagPath& dagPath,
        const bool mergeTransformAndShape,
        const bool stripNamespaces)
{
    std::string usdPathStr;

    if (stripNamespaces) {
        // drop namespaces instead of making them part of the path
        MString stripped = UsdMayaUtil::stripNamespaces(dagPath.fullPathName());
        usdPathStr = stripped.asChar();
    } else{
        usdPathStr = dagPath.fullPathName().asChar();
    }
    std::replace(usdPathStr.begin(), usdPathStr.end(), '|', '/');
    std::replace(usdPathStr.begin(), usdPathStr.end(), ':', '_'); // replace namespace ":" with "_"

    SdfPath usdPath(usdPathStr);
    if (mergeTransformAndShape && _IsShape(dagPath)) {
        usdPath = usdPath.GetParentPath();
    }

    return usdPath;
}

bool
UsdMayaUtil::GetBoolCustomData(
        const UsdAttribute& obj,
        const TfToken& key,
        const bool defaultValue)
{
    bool returnValue = defaultValue;
    VtValue data = obj.GetCustomDataByKey(key);
    if (!data.IsEmpty()) {
        if (data.IsHolding<bool>()) {
            return data.Get<bool>();
        } else {
            TF_RUNTIME_ERROR(
                    "customData at key '%s' is not of type bool. Skipping...",
                    key.GetText());
        }
    }
    return returnValue;
}

template <typename T>
static T
_GetVec(const UsdAttribute& attr, const VtValue& val)
{
    const T ret = val.UncheckedGet<T>();

    if (attr.GetRoleName() == SdfValueRoleNames->Color) {
        return UsdMayaColorSpace::ConvertMayaToLinear(ret);
    }

    return ret;
}

MMatrix
UsdMayaUtil::GfMatrixToMMatrix(const GfMatrix4d& mx)
{
    MMatrix mayaMx;
    std::copy(mx.GetArray(), mx.GetArray()+16, mayaMx[0]);
    return mayaMx;
}

bool
UsdMayaUtil::getPlugMatrix(
        const MFnDependencyNode& depNode,
        const MString& attr,
        MMatrix* outVal)
{
    MStatus status;
    MPlug plug = depNode.findPlug(attr, &status);
    if (!status) {
        return false;
    }

    MObject plugObj = plug.asMObject(MDGContext::fsNormal, &status);
    if (!status) {
        return false;
    }

    MFnMatrixData plugMatrixData(plugObj, &status);
    if (!status) {
        return false;
    }

    *outVal = plugMatrixData.matrix();
    return true;
}

bool
UsdMayaUtil::setPlugMatrix(
        const MFnDependencyNode& depNode,
        const MString& attr,
        const GfMatrix4d& mx)
{
    MStatus status;
    MPlug plug = depNode.findPlug(attr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    return setPlugMatrix(mx, plug);
}

bool
UsdMayaUtil::setPlugMatrix(const GfMatrix4d& mx, MPlug& plug)
{
    MStatus status;
    MObject mxObj = MFnMatrixData().create(GfMatrixToMMatrix(mx), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = plug.setValue(mxObj);
    CHECK_MSTATUS_AND_RETURN(status, false);
    return true;
}

bool
UsdMayaUtil::setPlugValue(const UsdAttribute& usdAttr, MPlug& attrPlug)
{
    return setPlugValue(usdAttr, UsdTimeCode::Default(), attrPlug);
}

bool
UsdMayaUtil::setPlugValue(
        const UsdAttribute& usdAttr,
        const UsdTimeCode time,
        MPlug& attrPlug)
{
    VtValue val;
    if (!usdAttr.Get(&val, time)) {
        return false;
    }

    MStatus status = MStatus::kFailure;

    if (val.IsHolding<double>()) {
        status = attrPlug.setDouble(val.UncheckedGet<double>());
    }
    else if (val.IsHolding<float>()) {
        status = attrPlug.setFloat(val.UncheckedGet<float>());
    }
    else if (val.IsHolding<int>()) {
        status = attrPlug.setInt(val.UncheckedGet<int>());
    }
    else if (val.IsHolding<short>()) {
        status = attrPlug.setShort(val.UncheckedGet<short>());
    }
    else if (val.IsHolding<bool>()) {
        status = attrPlug.setBool(val.UncheckedGet<bool>());
    }
    else if (val.IsHolding<std::string>()) {
        status = attrPlug.setString(val.UncheckedGet<std::string>().c_str());
    }
    else if (val.IsHolding<TfToken>()) {
        const TfToken token(val.UncheckedGet<TfToken>());
        const MObject attrObj = attrPlug.attribute(&status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        if (attrObj.hasFn(MFn::kEnumAttribute)) {
            MFnEnumAttribute attrEnumFn(attrObj, &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            short enumVal = attrEnumFn.fieldIndex(token.GetText(), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = attrPlug.setShort(enumVal);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    }
    else if (val.IsHolding<GfVec2d>()) {
        if (attrPlug.isCompound()) {
            const GfVec2d& vecVal = val.UncheckedGet<GfVec2d>();
            for (size_t i = 0u; i < GfVec2d::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<GfVec2f>()) {
        if (attrPlug.isCompound()) {
            const GfVec2f& vecVal = val.UncheckedGet<GfVec2f>();
            for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<GfVec3d>()) {
        if (attrPlug.isCompound()) {
            const GfVec3d vecVal = _GetVec<GfVec3d>(usdAttr, val);
            for (size_t i = 0u; i < GfVec3d::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<GfVec3f>()) {
        if (attrPlug.isCompound()) {
            const GfVec3f vecVal = _GetVec<GfVec3f>(usdAttr, val);
            for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<GfVec4d>()) {
        if (attrPlug.isCompound()) {
            const GfVec4d vecVal = _GetVec<GfVec4d>(usdAttr, val);
            for (size_t i = 0u; i < GfVec4d::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<GfVec4f>()) {
        if (attrPlug.isCompound()) {
            const GfVec4f vecVal = _GetVec<GfVec4f>(usdAttr, val);
            for (size_t i = 0u; i < GfVec4f::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<VtDoubleArray>()) {
        const VtDoubleArray& valArray = val.UncheckedGet<VtDoubleArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setDouble(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    }
    else if (val.IsHolding<VtFloatArray>()) {
        const VtFloatArray& valArray = val.UncheckedGet<VtFloatArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setFloat(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    }
    else if (val.IsHolding<VtIntArray>()) {
        const VtIntArray& valArray = val.UncheckedGet<VtIntArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setInt(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    }
    else if (val.IsHolding<VtShortArray>()) {
        const VtShortArray& valArray = val.UncheckedGet<VtShortArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setShort(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    }
    else if (val.IsHolding<VtBoolArray>()) {
        const VtBoolArray& valArray = val.UncheckedGet<VtBoolArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setBool(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    }
    else if (val.IsHolding<VtStringArray>()) {
        const VtStringArray& valArray = val.UncheckedGet<VtStringArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setString(valArray[i].c_str());
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    }
    else if (val.IsHolding<VtVec2dArray>()) {
        const VtVec2dArray valArray = val.UncheckedGet<VtVec2dArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            const GfVec2d& vecVal = valArray[i];
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec2d::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<VtVec2fArray>()) {
        const VtVec2fArray valArray = val.UncheckedGet<VtVec2fArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            const GfVec2f& vecVal = valArray[i];
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec2f::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<VtVec3dArray>()) {
        const VtVec3dArray valArray = val.UncheckedGet<VtVec3dArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec3d vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec3d::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<VtVec3fArray>()) {
        const VtVec3fArray valArray = val.UncheckedGet<VtVec3fArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec3f vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec3f::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<VtVec4dArray>()) {
        const VtVec4dArray valArray = val.UncheckedGet<VtVec4dArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec4d vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec4d::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else if (val.IsHolding<VtVec4fArray>()) {
        const VtVec4fArray valArray = val.UncheckedGet<VtVec4fArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec4f vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(
                static_cast<unsigned int>(i),
                &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec4f::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j),
                                                 &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    else {
        TF_CODING_ERROR("Unsupported type '%s' for USD attribute '%s'",
                        usdAttr.GetTypeName().GetAsToken().GetText(),
                        usdAttr.GetPath().GetText());
        return false;
    }

    CHECK_MSTATUS_AND_RETURN(status, false);

    return true;
}

bool
UsdMayaUtil::createStringAttribute(
        MFnDependencyNode& depNode,
        const MString& attr)
{
    MStatus status = MStatus::kFailure;
    MFnTypedAttribute typedAttrFn;
    MObject attrObj = typedAttrFn.create(
        attr,
        attr,
        MFnData::kString,
        MObject::kNullObj,
        &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = depNode.addAttribute(attrObj);
    CHECK_MSTATUS_AND_RETURN(status, false);

    return true;
}

bool
UsdMayaUtil::createNumericAttribute(
        MFnDependencyNode& depNode,
        const MString& attr,
        const MFnNumericData::Type type)
{
    MStatus status = MStatus::kFailure;
    MFnNumericAttribute numericAttrFn;
    MObject attrObj = numericAttrFn.create(
        attr,
        attr,
        type,
        0,
        &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = depNode.addAttribute(attrObj);
    CHECK_MSTATUS_AND_RETURN(status, false);

    return true;
}

UsdMayaUtil::MDataHandleHolder::MDataHandleHolder(
        const MPlug& plug,
        MDataHandle dataHandle) :
    _plug(plug),
    _dataHandle(dataHandle)
{
}

UsdMayaUtil::MDataHandleHolder::~MDataHandleHolder()
{
    if (!_plug.isNull()) {
        _plug.destructHandle(_dataHandle);
    }
}

TfRefPtr<UsdMayaUtil::MDataHandleHolder>
UsdMayaUtil::MDataHandleHolder::New(const MPlug& plug)
{
    MStatus status;

#if MAYA_API_VERSION >= 20180000
    MDataHandle dataHandle = plug.asMDataHandle(&status);
#else
    MDataHandle dataHandle = plug.asMDataHandle(MDGContext::fsNormal, &status);
#endif

    if (!status.error()) {
        return TfCreateRefPtr(
                new UsdMayaUtil::MDataHandleHolder(plug, dataHandle));
    }
    else {
        return nullptr;
    }
}

TfRefPtr<UsdMayaUtil::MDataHandleHolder>
UsdMayaUtil::GetPlugDataHandle(const MPlug& plug)
{
    return UsdMayaUtil::MDataHandleHolder::New(plug);
}

VtDictionary
UsdMayaUtil::GetDictionaryFromArgDatabase(
        const MArgDatabase& argData,
        const VtDictionary& guideDict)
{
    // We handle three types of arguments:
    // 1 - bools: Some bools are actual boolean flags (t/f) in Maya, and others
    //     are false if omitted, true if present (simple flags).
    // 2 - strings: Just strings!
    // 3 - vectors (multi-use args): Try to mimic the way they're passed in the
    //     Python command API. If single arg per flag, make it a vector of
    //     strings. Multi arg per flag, vector of vector of strings.
    VtDictionary args;
    for (const std::pair<std::string, VtValue>& entry : guideDict) {
        const std::string& key = entry.first;
        const VtValue& guideValue = entry.second;
        if (!argData.isFlagSet(key.c_str())) {
            continue;
        }

        // The usdExport command must handle bools, strings, and vectors.
        if (guideValue.IsHolding<bool>()) {
            // The flag should be either 0-arg or 1-arg. If 0-arg, it's true by
            // virtue of being present (getFlagArgument won't change val). If
            // it's 1-arg, then getFlagArgument will set the appropriate true
            // or false value.
            bool val = true;
            argData.getFlagArgument(key.c_str(), 0, val);
            args[key] = val;
        }
        else if (guideValue.IsHolding<std::string>()) {
            const std::string val =
                    argData.flagArgumentString(key.c_str(), 0).asChar();
            args[key] = val;
        }
        else if (guideValue.IsHolding<std::vector<VtValue>>()) {
            unsigned int count = argData.numberOfFlagUses(entry.first.c_str());
            if (!TF_VERIFY(count > 0)) {
                // There should be at least one use if isFlagSet() = true.
                continue;
            }

            std::vector<MArgList> argLists(count);
            for (unsigned int i = 0; i < count; ++i) {
                argData.getFlagArgumentList(key.c_str(), i, argLists[i]);
            }

            // The flag is either 1-arg or multi-arg. If it's 1-arg, make this
            // a 1-d vector [arg, arg, ...]. If it's multi-arg, make this a
            // 2-d vector [[arg1, arg2, ...], [arg1, arg2, ...], ...].
            std::vector<VtValue> val;
            if (argLists[0].length() == 1) {
                for (const MArgList& argList : argLists) {
                    const std::string arg = argList.asString(0).asChar();
                    val.emplace_back(arg);
                }
            }
            else {
                for (const MArgList& argList : argLists) {
                    std::vector<VtValue> subList;
                    for (unsigned int i = 0; i < argList.length(); ++i) {
                        const std::string arg = argList.asString(i).asChar();
                        subList.emplace_back(arg);
                    }
                    val.emplace_back(subList);
                }
            }
            args[key] = val;
        }
        else {
            TF_CODING_ERROR("Can't handle type '%s'",
                    guideValue.GetTypeName().c_str());
        }
    }

    return args;
}

VtValue
UsdMayaUtil::ParseArgumentValue(
        const std::string& key,
        const std::string& value,
        const VtDictionary& guideDict)
{
    // We handle two types of arguments:
    // 1 - bools: Should be encoded by translator UI as a "1" or "0" string.
    // 2 - strings: Just strings!
    // We don't handle any vectors because none of the translator UIs currently
    // pass around any of the vector flags.
    auto iter = guideDict.find(key);
    if (iter != guideDict.end()) {
        const VtValue& guideValue = iter->second;
        // The export UI only has boolean and string parameters.
        if (guideValue.IsHolding<bool>()) {
            return VtValue(TfUnstringify<bool>(value));
        }
        else if (guideValue.IsHolding<std::string>()) {
            return VtValue(value);
        }
    }
    else {
        TF_CODING_ERROR("Unknown flag '%s'", key.c_str());
    }

    return VtValue();
}

std::vector<std::string>
UsdMayaUtil::GetAllAncestorMayaNodeTypes(const std::string& ty)
{
    const MString inheritedTypesMel = TfStringPrintf(
            "nodeType -isTypeName -inherited %s", ty.c_str()).c_str();
    MStringArray inheritedTypes;
    if (!MGlobal::executeCommand(
            inheritedTypesMel, inheritedTypes, false, false)) {
        TF_RUNTIME_ERROR(
                "Failed to query ancestor types of '%s' via MEL (does the type "
                "exist?)",
                ty.c_str());
        return std::vector<std::string>();
    }

#if MAYA_API_VERSION < 201800
    // In older versions of Maya, the MEL command
    // "nodeType -isTypeName -inherited" returns an empty array (but does not
    // fail) for some built-in types.
    // The buggy built-in cases from Maya 2016 have been hard-coded below with
    // the appropriate ancestors list. (The cases below all work with 2018.)
    if (inheritedTypes.length() == 0) {
        if (ty == "file") {
            return {"shadingDependNode", "texture2d", "file"};
        }
        else if (ty == "mesh") {
            return {
                "containerBase", "entity", "dagNode", "shape", "geometryShape",
                "deformableShape", "controlPoint", "surfaceShape", "mesh"
            };
        }
        else if (ty == "nurbsCurve") {
            return {
                "containerBase", "entity", "dagNode", "shape", "geometryShape",
                "deformableShape", "controlPoint", "curveShape", "nurbsCurve"
            };
        }
        else if (ty == "nurbsSurface") {
            return {
                "containerBase", "entity", "dagNode", "shape", "geometryShape",
                "deformableShape", "controlPoint", "surfaceShape",
                "nurbsSurface"
            };
        }
        else if (ty == "time") {
            return {"time"};
        }
        else {
            TF_RUNTIME_ERROR(
                    "Type '%s' exists, but MEL returned empty ancestor type "
                    "information for it",
                    ty.c_str());
            return {ty}; // Best that we can do without ancestor type info.
        }
    }
#endif

    std::vector<std::string> inheritedTypesVector;
    inheritedTypesVector.reserve(inheritedTypes.length());
    for (unsigned int i = 0; i < inheritedTypes.length(); ++i) {
        inheritedTypesVector.emplace_back(inheritedTypes[i].asChar());
    }
    return inheritedTypesVector;
}
