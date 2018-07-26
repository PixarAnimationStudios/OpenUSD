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
#include "usdMaya/editUtil.h"
#include "usdMaya/referenceAssembly.h"

#include <maya/MGlobal.h>
#include <maya/MItEdits.h>
#include <maya/MEdit.h>

#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


using std::unordered_map;
using std::string;
using std::pair;
using std::vector;

static const unordered_map< string, pair<PxrUsdMayaEditUtil::EditOp, PxrUsdMayaEditUtil::EditSet>>
    _attrToOpMap {
        {"translate",  {PxrUsdMayaEditUtil::OP_TRANSLATE,   PxrUsdMayaEditUtil::SET_ALL }},
        {"translateX", {PxrUsdMayaEditUtil::OP_TRANSLATE,   PxrUsdMayaEditUtil::SET_X   }},
        {"translateY", {PxrUsdMayaEditUtil::OP_TRANSLATE,   PxrUsdMayaEditUtil::SET_Y   }},
        {"translateZ", {PxrUsdMayaEditUtil::OP_TRANSLATE,   PxrUsdMayaEditUtil::SET_Z   }},
        {"rotate",     {PxrUsdMayaEditUtil::OP_ROTATE,      PxrUsdMayaEditUtil::SET_ALL }},
        {"rotateX",    {PxrUsdMayaEditUtil::OP_ROTATE,      PxrUsdMayaEditUtil::SET_X   }},
        {"rotateY",    {PxrUsdMayaEditUtil::OP_ROTATE,      PxrUsdMayaEditUtil::SET_Y   }},
        {"rotateZ",    {PxrUsdMayaEditUtil::OP_ROTATE,      PxrUsdMayaEditUtil::SET_Z   }},
        {"scale",      {PxrUsdMayaEditUtil::OP_SCALE,       PxrUsdMayaEditUtil::SET_ALL }},
        {"scaleX",     {PxrUsdMayaEditUtil::OP_SCALE,       PxrUsdMayaEditUtil::SET_X   }},
        {"scaleY",     {PxrUsdMayaEditUtil::OP_SCALE,       PxrUsdMayaEditUtil::SET_Y   }},
        {"scaleZ",     {PxrUsdMayaEditUtil::OP_SCALE,       PxrUsdMayaEditUtil::SET_Z   }}};

bool
PxrUsdMayaEditUtil::GetEditFromString(
    const MFnAssembly &assemblyFn,
    const string &editString,
    SdfPath *outEditPath,
    RefEdit *outEdit )
{
    const string absRepNS =
        string(assemblyFn.getAbsoluteRepNamespace().asChar()) + ":";
    const string repNS =
        string(assemblyFn.getRepNamespace().asChar()) + ":";

    outEdit->editString = editString;
    string simpleEditString = editString;

    if( UsdMayaUseUsdAssemblyNamespace() )
    {
        // The namespaces are used for Maya edit uniquification -- we
        // don't need them when processing the editStrings
        if( TfStringContains(simpleEditString, absRepNS) )
        {
            simpleEditString =
                TfStringReplace(simpleEditString, absRepNS, "");
        }
        else if( TfStringContains(simpleEditString, repNS) )
        {
            simpleEditString =
                TfStringReplace(simpleEditString, repNS, "");
        }
        else
        {
            // Skip edits that haven't been namespaced. Due to the way
            // Maya manages them on assemblies, they are not able to
            // be reliably attached to the assembly they were intended
            // for.
            //
            return false;
        }
    }

    // The code below is not very tolerant of bad edits...
    //
    // expected format here is 'setAttr "StairRot.rotateY" -7.2' or
    // 'setAttr "SimpleSphere.translate" -type "double3" 1.0 1.0 1.0'
    vector<string> editSplit =
        TfStringTokenize(simpleEditString);
    
    // We only support setAttr right now.
    if( editSplit[0]!="setAttr" )
        return false;
    
    vector<string> attrSplit =
        TfStringTokenize(
            editSplit[1].substr(1, editSplit[1].size()-2), ".");

    string pathStr = TfStringReplace(attrSplit[0], "|", "/");
    if( !SdfPath::IsValidPathString(pathStr) )
        return false;

    // Our output path must be a relative path.
    SdfPath path(pathStr);
    if (path.IsAbsolutePath()) {
        return false;
    }

    *outEditPath = path;

    // Figure out what operation we're doing from the attribute name.
    //
    auto *opSetPair = TfMapLookupPtr( _attrToOpMap, attrSplit[1] );
    if( !opSetPair )
        return false;
    
    outEdit->op = opSetPair->first;
    outEdit->set = opSetPair->second;

    if( outEdit->set ==  SET_ALL )
    {
        outEdit->value = GfVec3d(
                            atof(editSplit[editSplit.size()-3].c_str()),
                            atof(editSplit[editSplit.size()-2].c_str()),
                            atof(editSplit[editSplit.size()-1].c_str()));
    }
    else
    {
        outEdit->value = atof(editSplit[2].c_str());
    }

    return true;
}


void
PxrUsdMayaEditUtil::GetEditsForAssembly(
    const MObject &assemblyObj,
    PathEditMap *refEdits,
    vector<string> *invalidEdits )
{
    MStatus status;
    
    MFnAssembly assemblyFn(assemblyObj,&status);
    if( !status )
        return;
    
    MObject editsOwner(assemblyObj);
    MObject targetNode(assemblyObj);
    
    MItEdits assemEdits(editsOwner, targetNode);
    
    while( !assemEdits.isDone() )
    {
        string editString = assemEdits.currentEditString().asChar();
        
        SdfPath editPath;
        RefEdit curEdit;
        
        if( GetEditFromString( assemblyFn, editString, &editPath, &curEdit) )
        {
            (*refEdits)[ editPath ].push_back( curEdit );
        }
        else if( invalidEdits )
        {
            invalidEdits->push_back( editString );
        }
        
        assemEdits.next();
    }
}

void
PxrUsdMayaEditUtil::ApplyEditsToProxy(
    const PathEditMap &refEdits,
    const UsdStagePtr &stage,
    const UsdPrim &proxyRootPrim,
    vector<string> *failedEdits  )
{
    if( !stage || !proxyRootPrim.IsValid() )
        return;
    
    // refEdits is a container of lists of ordered edits sorted by path
    // This outer loop is per path...
    //
    TF_FOR_ALL(itr, refEdits)
    {
        SdfPath editPath =
                    itr->first.IsAbsolutePath() ? itr->first :
                    proxyRootPrim.GetPrimPath().AppendPath( itr->first );
        
        bool gotXform = false;
        
        GfVec3d translation;
        GfVec3f rotation;
        GfVec3f scale;
        GfVec3f pivot;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;
        
        UsdGeomXformCommonAPI transform =
                UsdGeomXformCommonAPI::Get( stage, editPath );
        if( transform )
        {
            // The UsdGeomXformCommonAPI will populate the data without us
            // having to know exactly how the data is set.
            //
            gotXform = 
                transform.GetXformVectors(
                    &translation, &rotation, &scale,
                    &pivot, &rotOrder, UsdTimeCode::Default());
        }
        
        if( !gotXform )
        {
            TF_FOR_ALL(refEdit, itr->second)
                failedEdits->push_back(refEdit->editString);
            continue;
        }
        
        // Apply all edits for the particular path in order.
        //
        TF_FOR_ALL(refEdit, itr->second)
        {
            if( refEdit->set==SET_ALL )
            {
                const GfVec3d &toSet = refEdit->value.Get<GfVec3d>();
                switch( refEdit->op )
                {
                    default:
                    case OP_TRANSLATE: translation = toSet; break;
                    case OP_ROTATE: rotation = GfVec3f(toSet); break;
                    case OP_SCALE: scale = GfVec3f(toSet); break;
                };
            }
            else
            {
                // We're taking advantage of the enum values for EditSet here...
                //
                const double &toSet = refEdit->value.Get<double>();
                switch( refEdit->op )
                {
                    default:
                    case OP_TRANSLATE:  translation[refEdit->set] = toSet;
                                        break;
                    case OP_ROTATE:     rotation[refEdit->set] = toSet;
                                        break;
                    case OP_SCALE:      scale[refEdit->set] = toSet;
                                        break;
                };
            }
        }
        
        transform.SetXformVectors(
                translation, rotation, scale,
                pivot, rotOrder, UsdTimeCode::Default());
    }
}

void
PxrUsdMayaEditUtil::_ApplyEditToAvar(
    EditOp op,
    EditSet set,
    double value,
    AvarValueMap *valueMap )
{
    switch( op )
    {
        case OP_TRANSLATE:
            switch( set )
            {
                case SET_X: (*valueMap)["Tx"] = value;      return;
                case SET_Y: (*valueMap)["Ty"] = value;      return;
                case SET_Z: (*valueMap)["Tz"] = value;      return;
                default:                                    return;
            };
        case OP_ROTATE:
            switch( set )
            {
                case SET_X: (*valueMap)["Rx"] = value;      return;
                case SET_Y: (*valueMap)["Ry"] = value;      return;
                case SET_Z: (*valueMap)["Rz"] = value;      return;
                default:                                    return;
            };
        case OP_SCALE:
            switch( set )
            {
                case SET_X: (*valueMap)["Swide"] = value;   return;
                case SET_Y: (*valueMap)["Sthick"] = value;  return;
                case SET_Z: (*valueMap)["Shigh"] = value;   return;
                default:                                    return;
            };
    };
}


void
PxrUsdMayaEditUtil::_ApplyEditToAvars(
    const RefEdit &refEdit,
    AvarValueMap *valueMap )
{
    if( refEdit.set==SET_ALL )
    {
        const GfVec3d &toSet = refEdit.value.Get<GfVec3d>();
        
        _ApplyEditToAvar( refEdit.op, SET_X, toSet[0], valueMap );
        _ApplyEditToAvar( refEdit.op, SET_Y, toSet[1], valueMap );
        _ApplyEditToAvar( refEdit.op, SET_Z, toSet[2], valueMap );
    }
    else
    {
        const double &toSet = refEdit.value.Get<double>();
        
        _ApplyEditToAvar( refEdit.op, refEdit.set, toSet, valueMap );
    }
}


void
PxrUsdMayaEditUtil::GetAvarEdits(
    const PathEditMap &refEdits,
    PathAvarMap *avarMap )
{
    // refEdits is a container of lists of ordered edits sorted by path
    // This outer loop is per path...
    //
    TF_FOR_ALL(itr, refEdits)
    {
        const SdfPath &editPath = itr->first;
        
        AvarValueMap &valueMap = (*avarMap)[editPath];
        
        // Apply all edits for the particular path in order.
        //
        TF_FOR_ALL(refEdit, itr->second)
        {
            _ApplyEditToAvars( *refEdit, &valueMap );
        }
    }
}



PXR_NAMESPACE_CLOSE_SCOPE

