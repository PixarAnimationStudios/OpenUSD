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
#include "usdMaya/editUtil.h"

#include "usdMaya/referenceAssembly.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

#include <maya/MEdit.h>
#include <maya/MFnAssembly.h>
#include <maya/MGlobal.h>
#include <maya/MItEdits.h>
#include <maya/MStatus.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


static const std::unordered_map<std::string, std::pair<UsdMayaEditUtil::EditOp, UsdMayaEditUtil::EditSet>>
    _attrToOpMap {
        {"translate",  {UsdMayaEditUtil::OP_TRANSLATE, UsdMayaEditUtil::SET_ALL}},
        {"translateX", {UsdMayaEditUtil::OP_TRANSLATE, UsdMayaEditUtil::SET_X  }},
        {"translateY", {UsdMayaEditUtil::OP_TRANSLATE, UsdMayaEditUtil::SET_Y  }},
        {"translateZ", {UsdMayaEditUtil::OP_TRANSLATE, UsdMayaEditUtil::SET_Z  }},
        {"rotate",     {UsdMayaEditUtil::OP_ROTATE,    UsdMayaEditUtil::SET_ALL}},
        {"rotateX",    {UsdMayaEditUtil::OP_ROTATE,    UsdMayaEditUtil::SET_X  }},
        {"rotateY",    {UsdMayaEditUtil::OP_ROTATE,    UsdMayaEditUtil::SET_Y  }},
        {"rotateZ",    {UsdMayaEditUtil::OP_ROTATE,    UsdMayaEditUtil::SET_Z  }},
        {"scale",      {UsdMayaEditUtil::OP_SCALE,     UsdMayaEditUtil::SET_ALL}},
        {"scaleX",     {UsdMayaEditUtil::OP_SCALE,     UsdMayaEditUtil::SET_X  }},
        {"scaleY",     {UsdMayaEditUtil::OP_SCALE,     UsdMayaEditUtil::SET_Y  }},
        {"scaleZ",     {UsdMayaEditUtil::OP_SCALE,     UsdMayaEditUtil::SET_Z  }}};


/// Returns true if the assembly edit tokenized as \p tokenizedEditString
/// applies to the assembly \p assemblyFn based on a namespace match of the
/// edited node with the assembly.
static
bool
_EditAppliesToAssembly(
        const MFnAssembly& assemblyFn,
        const std::vector<std::string>& tokenizedEditString)
{
    if (tokenizedEditString.size() < 2u) {
        return false;
    }

    const std::string editedNodeAndAttrName =
        TfStringReplace(tokenizedEditString[1], "\"", "");

    const std::string absRepNS =
        TfStringPrintf("%s:", assemblyFn.getAbsoluteRepNamespace().asChar());
    const std::string repNS =
        TfStringPrintf("%s:", assemblyFn.getRepNamespace().asChar());

    return TfStringStartsWith(editedNodeAndAttrName, absRepNS) ||
        TfStringStartsWith(editedNodeAndAttrName, repNS);
}

static
bool
_GetEditFromTokenizedString(
        const MFnAssembly& assemblyFn,
        const std::vector<std::string>& tokenizedEditString,
        SdfPath* outEditPath,
        UsdMayaEditUtil::AssemblyEdit* outEdit)
{
    // The expected format for editString is:
    //     setAttr "StairRot.rotateY" -7.2
    //     or
    //     setAttr "SimpleSphere.translate" -type "double3" 1.0 1.0 1.0

    const size_t numEditTokens = tokenizedEditString.size();

    if (numEditTokens < 3u) {
        return false;
    }

    // We only support setAttr edits right now.
    if (tokenizedEditString[0u] != "setAttr") {
        return false;
    }

    const std::string editedNodeAndAttrName =
        TfStringReplace(tokenizedEditString[1u], "\"", "");

    const std::vector<std::string> editedNodeAndAttrSplit =
        TfStringTokenize(editedNodeAndAttrName, ".");
    if (editedNodeAndAttrSplit.size() < 2u) {
        return false;
    }

    const std::string mayaNodeName = editedNodeAndAttrSplit[0u];
    const std::string mayaAttrName = editedNodeAndAttrSplit[1u];

    const SdfPath usdPath =
        UsdMayaUtil::MayaNodeNameToSdfPath(
            mayaNodeName,
            UsdMayaUseUsdAssemblyNamespace());

    // Our output path must be a relative path.
    if (usdPath.IsAbsolutePath()) {
        return false;
    }

    // Figure out what operation we're doing from the attribute name.
    const auto* opSetPair = TfMapLookupPtr(_attrToOpMap, mayaAttrName);
    if (!opSetPair) {
        return false;
    }

    *outEditPath = usdPath;
    outEdit->op = opSetPair->first;
    outEdit->set = opSetPair->second;

    if (outEdit->set == UsdMayaEditUtil::SET_ALL) {
        outEdit->value =
            GfVec3d(
                atof(tokenizedEditString[numEditTokens - 3u].c_str()),
                atof(tokenizedEditString[numEditTokens - 2u].c_str()),
                atof(tokenizedEditString[numEditTokens - 1u].c_str()));
    } else {
        outEdit->value = atof(tokenizedEditString[2u].c_str());
    }

    return true;
}

/* static */
bool
UsdMayaEditUtil::GetEditFromString(
        const MFnAssembly& assemblyFn,
        const std::string& editString,
        SdfPath* outEditPath,
        AssemblyEdit* outEdit)
{
    const std::vector<std::string> tokenizedEditString =
        TfStringTokenize(editString);

    if (!_EditAppliesToAssembly(assemblyFn, tokenizedEditString)) {
        return false;
    }

    outEdit->editString = editString;

    return _GetEditFromTokenizedString(
        assemblyFn,
        tokenizedEditString,
        outEditPath,
        outEdit);
}

/* static */
void
UsdMayaEditUtil::GetEditsForAssembly(
        const MObject& assemblyObj,
        PathEditMap* assemEdits,
        std::vector<std::string>* invalidEdits)
{
    MStatus status;
    const MFnAssembly assemblyFn(assemblyObj, &status);
    if (status != MS::kSuccess) {
        return;
    }

    // The MItEdits API doesn't seem to work exactly as the Maya docs suggest.
    // They say that passing MObject::kNullObj as the targetNode to the
    // MItEdits constructor should return an iterator that yields all edits
    // stored on editsOwner (assemblyObj in our case):
    //
    // http://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=__cpp_ref_class_m_it_edits_html
    //
    // What seems to be happening instead is that we get ALL of the edits
    // stored at the "scene" level. You can see similar behavior in the
    // "List Assembly Edits" dialog when "Show -> List Edits Stored on Selected
    // Levels" is set.
    //
    // What we want is to find all of the edits for nodes below a given
    // assembly at any arbitrary level of nesting, but we don't want to have to
    // traverse nodes below the assembly or activate any nested assemblies.
    // Previously, we would use the assembly node as both editsOwner and
    // targetNode for the MItEdits constructor, but that would only yield edits
    // to nodes in the "first level" of hierarchy below the assembly and would
    // not include edits to nodes below any nested assemblies. As a result, we
    // use MObject::kNullObj as the targetNode and look through the list of all
    // assembly edits in the scene. We determine whether each one applies to
    // the current assembly based on a namespace match of the edited node with
    // the assembly.
    MObject editsOwner(assemblyObj);
    MItEdits itAssemEdits(editsOwner, MObject::kNullObj);

    while (!itAssemEdits.isDone()) {
        const std::string editString =
            itAssemEdits.currentEditString().asChar();

        const std::vector<std::string> tokenizedEditString =
            TfStringTokenize(editString);

        if (!_EditAppliesToAssembly(assemblyFn, tokenizedEditString)) {
            // Skip any edits that do not apply to this assembly so that we
            // don't identify them as invalid.
            itAssemEdits.next();
            continue;
        }

        AssemblyEdit curEdit;
        curEdit.editString = editString;

        SdfPath editPath;
        if (_GetEditFromTokenizedString(
                assemblyFn,
                tokenizedEditString,
                &editPath,
                &curEdit)) {
            (*assemEdits)[editPath].push_back(curEdit);
        } else if (invalidEdits) {
            invalidEdits->push_back(editString);
        }

        itAssemEdits.next();
    }
}

/* static */
void
UsdMayaEditUtil::ApplyEditsToProxy(
        const PathEditMap& assemEdits,
        const UsdPrim& proxyRootPrim,
        std::vector<std::string>* failedEdits)
{
    if (!proxyRootPrim.IsValid()) {
        return;
    }

    const SdfPath proxyRootPrimPath = proxyRootPrim.GetPath();
    const UsdStageRefPtr stage = proxyRootPrim.GetStage();

    // assemEdits is a container of lists of ordered edits sorted by path
    // This outer loop is per path...
    TF_FOR_ALL(itr, assemEdits) {
        const SdfPath editPath =
            itr->first.IsAbsolutePath() ?
                itr->first :
                proxyRootPrimPath.AppendPath(itr->first);

        // The UsdGeomXformCommonAPI will populate the data without us having
        // to know exactly how the data is set.
        GfVec3d translation;
        GfVec3f rotation;
        GfVec3f scale;
        GfVec3f pivot;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;

        UsdGeomXformCommonAPI xformCommonAPI =
            UsdGeomXformCommonAPI::Get(stage, editPath);
        if (!xformCommonAPI ||
                !xformCommonAPI.GetXformVectors(
                    &translation,
                    &rotation,
                    &scale,
                    &pivot,
                    &rotOrder,
                    UsdTimeCode::Default())) {
            // We failed either to get the xformCommonAPI or to get its
            // transform data, so mark all edits as failed.
            TF_FOR_ALL(assemEdit, itr->second) {
                failedEdits->push_back(assemEdit->editString);
            }
            continue;
        }

        // Apply all edits for the particular path in order.
        TF_FOR_ALL(assemEdit, itr->second) {
            if (assemEdit->set == SET_ALL) {
                const GfVec3d& toSet = assemEdit->value.Get<GfVec3d>();
                switch (assemEdit->op) {
                    default:
                    case OP_TRANSLATE:
                        translation = toSet;
                        break;
                    case OP_ROTATE:
                        rotation = GfVec3f(toSet);
                        break;
                    case OP_SCALE:
                        scale = GfVec3f(toSet);
                        break;
                }
            } else {
                // We're taking advantage of the enum values for EditSet here...
                const double toSet = assemEdit->value.Get<double>();
                switch (assemEdit->op) {
                    default:
                    case OP_TRANSLATE:
                        translation[assemEdit->set] = toSet;
                        break;
                    case OP_ROTATE:
                        rotation[assemEdit->set] = toSet;
                        break;
                    case OP_SCALE:
                        scale[assemEdit->set] = toSet;
                        break;
                }
            }
        }

        // We're about to author an edit to a prim underneath the proxy, so we
        // need to take some care with regard to instancing. If the edited prim
        // has any parent prims that have been marked instanceable, then we
        // must set all prims along the path from the proxy root prim down to
        // the edited prim as *not* instanceable, otherwise we will not be able
        // to make the edit. If the edited prim is itself instanceable, it can
        // remain instanceable as long as it has no instanceable parent prims,
        // since the edited prim is only receiving a transform edit.
        SdfPathVector pathPrefixes;
        editPath.GetPrefixes(&pathPrefixes);
        bool instanceableChanged = false;

        for (const SdfPath& pathPrefix : pathPrefixes) {
            if (!pathPrefix.HasPrefix(proxyRootPrimPath)) {
                // Skip any paths that are above the proxy root prim.
                continue;
            }

            if (pathPrefix == editPath && !instanceableChanged) {
                // If we've reached editPath and haven't had to modify
                // instanceable for any parent prims, then we can leave the
                // instanceable state for the edited prim as it is.
                break;
            }

            const UsdPrim parentPrim = stage->GetPrimAtPath(pathPrefix);
            if (parentPrim) {
                if (parentPrim.IsInstanceable()) {
                    parentPrim.SetInstanceable(false);
                    instanceableChanged = true;
                }
            }
        }

        if (instanceableChanged) {
            // Re-Get() the API schema if there was a change in
            // instanceable-ness so that we get one based on the newly
            // non-instance proxy prim.
            xformCommonAPI = UsdGeomXformCommonAPI::Get(stage, editPath);
        }

        if (!xformCommonAPI.SetXformVectors(
                translation,
                rotation,
                scale,
                pivot,
                rotOrder,
                UsdTimeCode::Default())) {
            TF_FOR_ALL(assemEdit, itr->second) {
                failedEdits->push_back(assemEdit->editString);
            }
            continue;
        }
    }
}

/* static */
void
UsdMayaEditUtil::_ApplyEditToAvar(
        const EditOp op,
        const EditSet set,
        const double value,
        AvarValueMap* valueMap)
{
    switch (op) {
        case OP_TRANSLATE:
            switch (set) {
                case SET_X:
                    (*valueMap)["Tx"] = value;
                    return;
                case SET_Y:
                    (*valueMap)["Ty"] = value;
                    return;
                case SET_Z:
                    (*valueMap)["Tz"] = value;
                    return;
                default:
                    return;
            }
        case OP_ROTATE:
            switch (set) {
                case SET_X:
                    (*valueMap)["Rx"] = value;
                    return;
                case SET_Y:
                    (*valueMap)["Ry"] = value;
                    return;
                case SET_Z:
                    (*valueMap)["Rz"] = value;
                    return;
                default:
                    return;
            }
        case OP_SCALE:
            switch (set) {
                case SET_X:
                    (*valueMap)["Swide"] = value;
                    return;
                case SET_Y:
                    (*valueMap)["Sthick"] = value;
                    return;
                case SET_Z:
                    (*valueMap)["Shigh"] = value;
                    return;
                default:
                    return;
            }
    }
}

/* static */
void
UsdMayaEditUtil::_ApplyEditToAvars(
        const AssemblyEdit& assemEdit,
        AvarValueMap* valueMap)
{
    if (assemEdit.set == SET_ALL) {
        const GfVec3d& toSet = assemEdit.value.Get<GfVec3d>();

        _ApplyEditToAvar(assemEdit.op, SET_X, toSet[0], valueMap);
        _ApplyEditToAvar(assemEdit.op, SET_Y, toSet[1], valueMap);
        _ApplyEditToAvar(assemEdit.op, SET_Z, toSet[2], valueMap);
    } else {
        const double toSet = assemEdit.value.Get<double>();

        _ApplyEditToAvar(assemEdit.op, assemEdit.set, toSet, valueMap);
    }
}

/* static */
void
UsdMayaEditUtil::GetAvarEdits(
        const PathEditMap& assemEdits,
        PathAvarMap* avarMap)
{
    // assemEdits is a container of lists of ordered edits sorted by path
    // This outer loop is per path...
    TF_FOR_ALL(itr, assemEdits) {
        const SdfPath& editPath = itr->first;

        AvarValueMap& valueMap = (*avarMap)[editPath];

        // Apply all edits for the particular path in order.
        TF_FOR_ALL(assemEdit, itr->second) {
            _ApplyEditToAvars(*assemEdit, &valueMap);
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
