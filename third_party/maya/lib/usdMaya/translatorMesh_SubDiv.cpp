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
#include "usdMaya/translatorMesh.h"

#include "usdMaya/util.h"

#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MDagPath.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnPartition.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshVertex.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MUintArray.h>

#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE



bool
_AddCreaseSet(
    const std::string &rootName,
    double creaseLevel,
    MSelectionList &componentList,
    MStatus *statusOK )
{
    // Crease Set functionality is native to Maya, but undocumented and not
    // directly supported in the API. The below implementation is derived from
    // the editor code in the maya distro at:
    //
    // .../lib/python2.7/site-packages/maya/app/general/creaseSetEditor.py

    MObject creasePartitionObj;
    *statusOK = UsdMayaUtil::GetMObjectByName(":creasePartition",
                                                 creasePartitionObj);

    if (creasePartitionObj.isNull()) {
        statusOK->clear();
        
        // There is no documented way to create a shared node through the C++ API
        const std::string partitionName = MGlobal::executeCommandStringResult(
            "createNode \"partition\" -shared -name \":creasePartition\"").asChar();

        *statusOK = UsdMayaUtil::GetMObjectByName(partitionName,
                                                     creasePartitionObj);
        if (!*statusOK) {
            return false;
        }
    }
    
    MFnPartition creasePartition( creasePartitionObj, statusOK );
    if (!*statusOK) return false;
    
    std::string creaseSetname =
        TfStringPrintf("%s_creaseSet#",rootName.c_str());
    
    MFnDependencyNode creaseSetFn;
    MObject creaseSetObj =
        creaseSetFn.create("creaseSet", creaseSetname.c_str(), statusOK );
    if (!*statusOK) return false;
    
    MPlug levelPlug = creaseSetFn.findPlug("creaseLevel",false, statusOK);
    if (!*statusOK) return false;
    
    *statusOK = levelPlug.setValue(creaseLevel);
    if (!*statusOK) return false;
    
    *statusOK = creasePartition.addMember(creaseSetObj);
    if (!*statusOK) return false;
    
    MFnSet creaseSet( creaseSetObj, statusOK );
    if (!*statusOK) return false;
    
    *statusOK = creaseSet.addMembers( componentList );
    if (!*statusOK) return false;

    return true;
}

/* static */
bool 
UsdMayaTranslatorMesh::_AssignSubDivTagsToMesh( const UsdGeomMesh &primSchema, MObject &meshObj, MFnMesh &meshFn)
{
    // We may want to provide the option in the future, but for now, we
    // default to using crease sets when setting crease data.
    //
    static const bool USE_CREASE_SETS = true;
    
    MStatus statusOK;
    
    MDagPath meshPath;
    statusOK = MDagPath::getAPathTo(meshObj,meshPath);
    if (!statusOK) return false;
    
    // USD does not support grouped verts and edges, so combine all components
    // with the same weight into one set to reduce the overall crease set
    // count. The user can always split the sets up later if desired.
    // 
    // This structure is unused if crease sets aren't being created.
    std::unordered_map<float,MSelectionList> elemsPerWeight;
    
    // Vert Creasing
    VtArray<int>   subdCornerIndices;
    VtArray<float> subdCornerSharpnesses;
    primSchema.GetCornerIndicesAttr().Get(&subdCornerIndices); // not animatable
    primSchema.GetCornerSharpnessesAttr().Get(&subdCornerSharpnesses); // not animatable
    if (!subdCornerIndices.empty()) {
        if (subdCornerIndices.size() == subdCornerSharpnesses.size() ) {
            statusOK.clear();
            
            if (USE_CREASE_SETS) {
                MItMeshVertex vertIt(meshObj);
                for (unsigned int i=0; i < subdCornerIndices.size(); i++) {
                    
                    // Ignore zero-sharpness corners
                    if (subdCornerSharpnesses[i]==0)
                        continue;
                    
                    MSelectionList &elemList =
                        elemsPerWeight[ subdCornerSharpnesses[i] ];
                    
                    int prevIndexDummy; // dummy param
                    statusOK = vertIt.setIndex(subdCornerIndices[i], prevIndexDummy);
                    if (!statusOK)
                        break;
                    statusOK = elemList.add(meshPath,vertIt.currentItem());
                    if (!statusOK)
                        break;
                }
                
            } else {
                MUintArray   mayaCreaseVertIds;
                MDoubleArray mayaCreaseVertValues;
                mayaCreaseVertIds.setLength( subdCornerIndices.size() );
                mayaCreaseVertValues.setLength( subdCornerIndices.size() );
                for (unsigned int i=0; i < subdCornerIndices.size(); i++) {
                    
                    // Ignore zero-sharpness corners
                    if (subdCornerSharpnesses[i]==0)
                        continue;
                    
                    mayaCreaseVertIds[i] = subdCornerIndices[i];
                    mayaCreaseVertValues[i] = subdCornerSharpnesses[i];
                }
                statusOK = meshFn.setCreaseVertices(mayaCreaseVertIds, mayaCreaseVertValues);
            }
            
            if (!statusOK) {
                TF_RUNTIME_ERROR("Unable to set Crease Vertices on <%s>: %s", 
                        meshFn.fullPathName().asChar(),
                        statusOK.errorString().asChar());
                return false;
            }
            
        } else {
            TF_RUNTIME_ERROR(
                    "Mismatch between Corner Indices & Sharpness on <%s>", 
                    primSchema.GetPrim().GetPath().GetText());
            return false;
        }
    }

    // Edge Creasing
    VtArray<int>   subdCreaseLengths;
    VtArray<int>   subdCreaseIndices;
    VtArray<float> subdCreaseSharpnesses;
    primSchema.GetCreaseLengthsAttr().Get(&subdCreaseLengths);
    primSchema.GetCreaseIndicesAttr().Get(&subdCreaseIndices);
    primSchema.GetCreaseSharpnessesAttr().Get(&subdCreaseSharpnesses);
    if (!subdCreaseLengths.empty()) {
        if (subdCreaseLengths.size() == subdCreaseSharpnesses.size() ) {
            MUintArray   mayaCreaseEdgeIds;
            MDoubleArray mayaCreaseEdgeValues;
            MIntArray connectedEdges;
            unsigned int creaseIndexBase = 0;
            
            statusOK.clear();

            for (unsigned int creaseGroup=0;
                    statusOK && creaseGroup<subdCreaseLengths.size();
                    creaseIndexBase+=subdCreaseLengths[creaseGroup++]) {
                
                // Ignore zero-sharpness creases
                if (subdCreaseSharpnesses[creaseGroup]==0)
                    continue;
                        
                MItMeshVertex vertIt(meshObj);
                MItMeshEdge edgeIt(meshObj);
                
                for (int i=0; statusOK && i<subdCreaseLengths[creaseGroup]-1; i++) {
                    // Find the edgeId associated with the 2 vertIds.
                    int prevIndexDummy; // dummy param
                    statusOK = vertIt.setIndex(subdCreaseIndices[creaseIndexBase+i], prevIndexDummy);
                    if (!statusOK)
                        break;
                    statusOK = vertIt.getConnectedEdges(connectedEdges);
                    if (!statusOK)
                        break;
                    
                    int edgeIndex = -1;
                    for (unsigned int e=0; statusOK && e<connectedEdges.length(); e++) {
                        int tmpOppositeVertexId;
                        statusOK = vertIt.getOppositeVertex(tmpOppositeVertexId, connectedEdges[e]);
                        if (!statusOK)
                            break;
                        if ( subdCreaseIndices[creaseIndexBase+i+1] == tmpOppositeVertexId ) {
                            edgeIndex = connectedEdges[e];
                            break;
                        }
                    }
                    if (statusOK && edgeIndex != -1) {
                        if (USE_CREASE_SETS) {
                            int prevIndexDummy; // dummy param
                            statusOK = edgeIt.setIndex(edgeIndex,prevIndexDummy);
                            if (!statusOK)
                                break;
                            statusOK = elemsPerWeight[subdCreaseSharpnesses[creaseGroup]].
                                            add(meshPath,edgeIt.currentItem());
                            if (!statusOK)
                                break;
                        } else {
                            mayaCreaseEdgeIds.append(edgeIndex);
                            mayaCreaseEdgeValues.append(subdCreaseSharpnesses[creaseGroup]);
                        }
                    }
                }
            }
            
            if (statusOK && !USE_CREASE_SETS) {
                statusOK = meshFn.setCreaseEdges(mayaCreaseEdgeIds, mayaCreaseEdgeValues);
            }
            
            if (!statusOK) {
                TF_RUNTIME_ERROR("Unable to set Crease Edges on <%s>: %s", 
                        meshFn.fullPathName().asChar(),
                        statusOK.errorString().asChar());
                return false;
            }

        } else {
            TF_RUNTIME_ERROR(
                    "Mismatch between Crease Lengths & Sharpness on <%s>", 
                    primSchema.GetPrim().GetPath().GetText());
            return false;
        }
    }
    
    if (USE_CREASE_SETS) {
        TF_FOR_ALL(weightList, elemsPerWeight) {
            double creaseLevel = weightList->first;
            MSelectionList &elemList = weightList->second;

            if (!_AddCreaseSet( meshFn.name().asChar(),
                                creaseLevel, elemList, &statusOK )){
                TF_RUNTIME_ERROR("Unable to set crease sets on <%s>: %s", 
                        meshFn.fullPathName().asChar(),
                        statusOK.errorString().asChar());
                return false;
            }
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

