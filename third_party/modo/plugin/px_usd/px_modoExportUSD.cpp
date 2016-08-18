//
//  px_modoExportUSD.cpp
//
//  Created by Michael B. Johnson on 9/16/14.
//

#include "px_modoExportUSD.h"


////
// methods for collecting the information on the UV maps

ModoExportUSD::CUSDMapNameVisitor::CUSDMapNameVisitor(CLxUser_MeshMap meshMap)
{
    if (meshMap.test()) {
        _meshMap.copy(meshMap);
    }
}

std::vector<std::string>
ModoExportUSD::CUSDMapNameVisitor::names() {
    return _names;
}


LxResult
ModoExportUSD::CUSDMapNameVisitor::ByIndex(unsigned index, std::string &name)
{
    if (index < _names.size ()) {
        name = _names[index];
        return LXe_OK;
    }
    return LXe_FAILED;
}

////
// the main exporter class

ModoExportUSD::CUSDSaver::CUSDSaver() {
    CLxUser_SceneService	sceneService;

    m_typeCamera		= sceneService.ItemType(LXsITYPE_CAMERA);
    m_typeMesh          = sceneService.ItemType(LXsITYPE_MESH);
    m_typeMeshInst		= sceneService.ItemType(LXsITYPE_MESHINST);
    m_typeGroupLocator	= sceneService.ItemType(LXsITYPE_GROUPLOCATOR);
    m_typeLocator		= sceneService.ItemType(LXsITYPE_LOCATOR);

    theIdentityMatrix44.SetIdentity();
    usd_modoNamespace = "modo";
    usd_modo_originalItemName = "originalItemName";
    usd_modo_originalUVName = "originalUVName";
}

/*
  XXX modo thinks in meters. 
  Currently, USD has no way of specifying what the base unit is, 
  but current convention (3/2016) is that it is cm. 
  So we transform the translation component of the 4x4.
*/
static GfMatrix4d 
_ConvertModoTransformToUSD(const GfMatrix4d &modoTransform) {
    GfMatrix4d usdTransform(modoTransform);
    usdTransform[3][0] *= s_meters2cm;
    usdTransform[3][1] *= s_meters2cm;
    usdTransform[3][2] *= s_meters2cm;
    return usdTransform;
}

/*
  XXX modo thinks in meters. 
  Currently, USD has no way of specifying what the base unit is, 
  but current convention (3/2016) is that it is cm. 
  So we transform the points here.
*/
static GfVec3f
_ScaleModoPointToUSDScale(float x, float y, float z) {
    x *= s_meters2cm;
    y *= s_meters2cm;
    z *= s_meters2cm;
    GfVec3f usdPoint(x, y, z);
    return usdPoint;
}

GfMatrix4d
ModoExportUSD::CUSDSaver::GetWorldTransformOfCurrentItem() {
    CLxUser_Item	currentItem;
    if (!CLxSceneSaver::GetItem(currentItem)) {
        // there's no current item!
        return theIdentityMatrix44;
    }
    CLxLoc_Locator locator;
    if (!locator.set(currentItem)) {
        // this is not something we can get a transform for
        return theIdentityMatrix44;
    }
    LXtMatrix xfrm;
    LXtVector offset;
    if (!CLxSceneSaver::WorldXform(xfrm, offset)) {
        return theIdentityMatrix44;
    }
    GfMatrix4d worldTransform;
    /* For clarity, let's just do direct assignments here.
       Note that we have to swap rows and columns for the 
       rotation matrix, and put the translation in the last row.
    */
    worldTransform[0][0] = xfrm[0][0]; 
    worldTransform[0][1] = xfrm[1][0]; 
    worldTransform[0][2] = xfrm[2][0]; 
    worldTransform[0][3] = 0;
    worldTransform[1][0] = xfrm[0][1]; 
    worldTransform[1][1] = xfrm[1][1]; 
    worldTransform[1][2] = xfrm[2][1]; 
    worldTransform[1][3] = 0;
    worldTransform[2][0] = xfrm[0][2]; 
    worldTransform[2][1] = xfrm[1][2]; 
    worldTransform[2][2] = xfrm[2][2]; 
    worldTransform[2][3] = 0;
    worldTransform[3][0] = offset[0]; 
    worldTransform[3][1] = offset[1]; 
    worldTransform[3][2] = offset[2]; 
    worldTransform[3][3] = 1;

    return worldTransform;
}

GfMatrix4d
ModoExportUSD::CUSDSaver::GetLocalTransformOfCurrentItem(GfMatrix4d 
                                                         parentMatrixInverse) {
/* modo's SDK doesn't give us the local space transform, 
   so we synthesize it by multiplying our item's world space xform 
   by the inverse of its parent's world space xform.
*/
    GfMatrix4d myWorldSpaceTransform = GetWorldTransformOfCurrentItem();
    GfMatrix4d localXform = myWorldSpaceTransform * parentMatrixInverse;
    return localXform;
}


void
ModoExportUSD::CUSDSaver::GatherColors() {
    std::map<std::string, GfVec3f>::iterator it;
    for (it = materialColorMap.begin(); it != materialColorMap.end(); it++) {
        const char* mask = it->first.c_str();
        if (!CLxSceneSaver::ScanMask(mask)) {
            GfVec3f white(1, 1, 1);
            materialColorMap[mask] = white;
            continue;
        }
        while (CLxSceneSaver::NextLayer()) {
            if (strcmp(CLxSceneSaver::ItemType(), LXsITYPE_ADVANCEDMATERIAL)) {
                continue;
            }
            GfVec3f rgb;
            rgb[0] = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_DIFFCOL".R");
            rgb[1] = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_DIFFCOL".G");
            rgb[2] = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_DIFFCOL".B");
            for (int i = 0; i < 3; i++) {
                if (rgb[i] < 0.0) {
                    std::cerr << "clamping rgb[" << i 
                              << "] of material \"" 
                              << mask << "\" to zero from " 
                              << rgb[i] << "\n";
                }
                if (rgb[i] > 1.0) {
                    std::cerr << "clamping rgb[" 
                              << i << "] of material \"" 
                              << mask << "\" to one from " 
                              << rgb[i] << "\n";
                }
            }
            materialColorMap[mask] = rgb;
       }
    }
}

void
ModoExportUSD::CUSDSaver::WriteItemInstance(std::string meshPath, 
                                            std::string referencePath, 
                                            GfMatrix4d localXform) {
    UsdPrim prim = myStage->DefinePrim(SdfPath(meshPath));
    UsdGeomXformable primSchema = UsdGeomXformable(prim);
    // save out the original name so that we could get it back 
    // if we round trip to modo
    if (!IsSafeName(CLxSceneSaver::ItemName())) {
        TfToken originalNameKeyToken((usd_modoNamespace + ":" 
                                      + usd_modo_originalItemName));
        TfToken originalNameValToken(CLxSceneSaver::ItemName());
        primSchema.GetPrim().SetCustomDataByKey(originalNameKeyToken, 
                                                VtValue(originalNameValToken));
    }
    primSchema.GetPrim().SetInstanceable(true);
    primSchema.GetPrim().GetReferences().AddInternal(SdfPath(referencePath));
    TF_AXIOM(primSchema);
    GfMatrix4d usdMatrix = _ConvertModoTransformToUSD(localXform);
    primSchema.MakeMatrixXform().Set(usdMatrix, UsdTimeCode::Default());
    return ;
}

bool
ModoExportUSD::CUSDSaver::WriteItemMeshInstanceAtParentPath(CLxUser_Item& item,
                                                            std::string parentPath, 
                                                            GfMatrix4d localXform,
                                                            std::string safeName) {
    std::string path = parentPath + "/" + safeName;
    UsdGeomMesh primSchema;
    CLxUser_SceneService service;
    CLxUser_Item  srcMeshItem;
    ILxItem1ID	iMesh;
    if (LXx_OK(service.GetMeshInstSourceItem ((ILxUnknownID)item, 
                                              (void**)&iMesh))) {
        SetItem ((ILxUnknownID)iMesh);
        GetItem(srcMeshItem);
        if (CLxSceneSaver::ReallySaving()) {
            std::string srcPath = meshItemNameMap[srcMeshItem];
            WriteItemInstance(path, srcPath, localXform);
        } else {
            // for this instance's src mesh, we want to add 
            // this instance to its list of instances.
            meshMasterMeshInstancesMap[srcMeshItem].push_back(item);
        }
        // restore original item back to be current item
        SetItem (item);
        return true;
    }
    std::cerr << "unable to find source mesh for instance called \"" 
              << path << std::endl;
    return false;
}

bool
ModoExportUSD::CUSDSaver::IsItemMeshInstanced(CLxUser_Item& item) {
    for (std::map<CLxUser_Item, 
             std::vector<CLxUser_Item>>::iterator i = meshMasterMeshInstancesMap.begin(); 
         i != meshMasterMeshInstancesMap.end(); i++) {
        const CLxUser_Item& meshMaster = i->first;
        if (meshMaster == item) {
            return true;
        }
    }
    return false;
}

bool
ModoExportUSD::CUSDSaver::WriteItemMeshAtParentPath(CLxUser_Item& item, 
                                                    std::string parentPath, 
                                                    GfMatrix4d localXform, 
                                                    std::string safeName) {
    std::string path = parentPath + "/" + safeName;
    UsdGeomMesh primSchema;

    if (CLxSceneSaver::ReallySaving()) {
        // first let's figure out if this mesh is instanced:
        if (IsItemMeshInstanced(item)) {
            // if it is, we need to define a top-level class scope 
            // to put this mesh data into. This will then be referenced by all
            // its instances (including one that we will make for this mesh)
            std::string classPath = "/" + safeName + "__modoMeshInstanceMaster__";
            UsdGeomXform classSchema = UsdGeomXform::Define(myStage, 
                                                            SdfPath(classPath));
            TF_AXIOM(classSchema);
            classSchema.GetPrim().SetSpecifier(SdfSpecifierClass);
            classSchema.GetPrim().SetMetadata(SdfFieldKeys->Kind, 
                                              TfToken("subcomponent"));;

            // Let's stash the class path so that instances can look 
            // it up later.Note that this path points to the top-level class,
            // not the path where this mesh is.
            meshItemNameMap[item] = classPath;
            // since we're not writing this mesh out directly, 
            // but rather stashing its info in an instance, we do that here:
            WriteItemInstance(path, meshItemNameMap[item], localXform);

            // reset the path to reparent the mesh under this top-level class
            path = classPath + "/" + safeName;
        }
        primSchema = UsdGeomMesh::Define(myStage, SdfPath(path));
        TF_AXIOM(primSchema);
        // save out the original name so that we could get it back 
        // if we round trip to modo
        if (!IsSafeName(CLxSceneSaver::ItemName())) {
            TfToken originalNameKeyToken((usd_modoNamespace + ":" 
                                          + usd_modo_originalItemName));
            TfToken originalNameValueToken(CLxSceneSaver::ItemName());
            UsdPrim prim = primSchema.GetPrim();
            prim.SetCustomDataByKey(originalNameKeyToken, 
                                    VtValue(originalNameValueToken));
        }
        if (!IsItemMeshInstanced(item)) {
            // if we're not writing this mesh out as an instance, we
            // should attach the 4x4 with it
            GfMatrix4d usdMatrix = _ConvertModoTransformToUSD(localXform);
            primSchema.MakeMatrixXform().Set(usdMatrix, 
                                             UsdTimeCode::Default());
        }
    }
    pointIndexMap.clear();
    usdPoints.clear();
    usdPoints.resize(CLxSceneSaver::PointCount());
    usdFlattenedFaceVertexIndices.clear();
    usdFaceVertexCounts.clear();
    faceIsSubdiv.clear();
    usdFaceVertexRGBs.clear();
    edgeMap.clear();
    usdEdgeCreaseLengths.clear();
    usdFlattenedEdgeCreasePointIndices.clear();
    usdFlattenedEdgeCreaseSharpnesses.clear();
    cornerWeights.clear();
    weightedCornerPointIndices.clear();
    namedUVs.clear();
    mapNames.clear();
    pointCount = 0;

    // note: unfortunately, modo could have a mesh that has a mix of
    // subdivs and non-subdivs in it, but we need to know now (before
    // we get to the faces) if this has any in order to keep track of
    // crease information. To the best of my current knowledge, this
    // check is accurate, and works for this part of the process, but
    // then later on when we write the faces we'll keep more specific
    // track, and that's the information we'll use to determine if the
    // mesh is a catmull-clark subdiv or just a polygon face (which we
    // currently export as a bilinear mesh, not a "none" one. If we do
    // decide to switch to "none", we should probably also export out
    // normals at that point.
    if (CLxSceneSaver::SetMap(LXi_VMAP_SUBDIV, NULL)) {
        exportingASubdiv = true;
    } else {
        exportingASubdiv = false;
    }
    // both the ss_Point() and ss_Edge() callbacks pay attention to
    // the exportingAsSubdiv flag
    CLxSceneSaver::WritePoints(); // will call our ss_Point() callback
    CLxSceneSaver::WriteEdges(); // will call our ss_Edge() callback

    polygonExportPassType = POLYGON_EXPORT_PASS_VERTICES;
    CLxSceneSaver::WritePolys();

    // now get UV info
    CLxUser_Mesh	mesh;
    CLxUser_MeshMap	mesh_map;
    if (GetMesh(mesh)) {
        if (mesh.GetMaps(mesh_map)) {
            CUSDMapNameVisitor	 map_visitor(mesh_map);
            // Only do this if you want a specific map type.
            // XXX eventually this might be the entry
            // point where we get other things like selection sets?
            mesh_map.FilterByType (LXi_VMAP_TEXTUREUV);	
            mesh_map.Enum(&map_visitor);
            mapNames = map_visitor.names();
            for (unsigned i = 0; i < mapNames.size(); i++) {
                bool hasUvs = CLxSceneSaver::SetMap(LXi_VMAP_TEXTUREUV, 
                                                    mapNames[i].c_str());
                if (hasUvs) {
                    uvName = mapNames[i];
                    polygonExportPassType = POLYGON_EXPORT_PASS_UVS;
                    CLxSceneSaver::SetSelMap(LXi_VMAP_TEXTUREUV);
                    CLxSceneSaver::WritePolys();
                }
            }
        }
    }

    unsigned int verticesCount = 0;
    for (size_t i = 0; i < usdFaceVertexCounts.size(); i++) {
        verticesCount += usdFaceVertexCounts[i];
    }
    if (verticesCount != usdFlattenedFaceVertexIndices.size()) {
        std::cerr << "We have a problem: " << verticesCount 
                  << " != " << usdFlattenedFaceVertexIndices.size() << "\n";
    }

    if (CLxSceneSaver::ReallySaving() && usdFaceVertexCounts.size()) {
        unsigned int currentPointIndex = 0;
        int maxVertexIndex = usdFlattenedFaceVertexIndices.size() - 1;
        for (size_t i = 0; i < usdFaceVertexCounts.size(); i++) {
            int vertextCount = usdFaceVertexCounts[i];
            for (int j = 0; j < vertextCount; j++) {
                if (currentPointIndex > static_cast<size_t>(maxVertexIndex)) {
                    std::cerr << "NOT SUPPOSED TO HAPPEN : " 
                              << currentPointIndex << " > " 
                              << maxVertexIndex << "\n";
                } else {
                    currentPointIndex++;
                }
            }
        }
        // build up our edge information
        if (edgeMap.size()) {
            for (EdgeMap::iterator it = edgeMap.begin(); 
                 it != edgeMap.end(); ++it) {
                int vertex0Index = it->first.v0;
                int vertex1Index = it->first.v1;
                float edgeSharpness = it->second;
                // XXX note: the values in modo seem to be 10X
                // smaller than we expect in the rest of the pipeline,
                // so we scale them here:
                edgeSharpness *= 10.0;
                // for each edge, we only have two vertices
                usdEdgeCreaseLengths.push_back(2); 
                usdFlattenedEdgeCreasePointIndices.push_back(vertex0Index);
                usdFlattenedEdgeCreasePointIndices.push_back(vertex1Index);
                usdFlattenedEdgeCreaseSharpnesses.push_back(edgeSharpness);
            }
        }
        // how many times did we flip face types in this mesh?
        int faceTypeMix = 0; 
        for (size_t i = 0; i < faceIsSubdiv.size(); i++) {
            bool faceFlag = faceIsSubdiv[i];
            if (i) {
                if (faceFlag != exportingASubdiv) {
                    faceTypeMix++;
                }
            }
            exportingASubdiv = faceFlag;
        }
        if (faceTypeMix) {
            // we've got some mix of subdiv and non-subdiv faces in
            // this mesh.  we don't have a way of representing those
            // right now in this exporter, so at least tell the user:
            std::string doc("in modo, this mesh has a mix of subdivision faces and non-subdivision faces - we are treating them all the same");
            primSchema.GetPrim().SetDocumentation(doc.c_str());
        }

        if (exportingASubdiv) {
            primSchema.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->catmullClark);
        } else {
            primSchema.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->bilinear);
        }
        primSchema.GetPointsAttr().Set(usdPoints, UsdTimeCode::Default());
        primSchema.GetFaceVertexCountsAttr().Set(usdFaceVertexCounts);
        primSchema.GetFaceVertexIndicesAttr().Set(usdFlattenedFaceVertexIndices);
        UsdGeomPrimvar primvar = primSchema.GetDisplayColorPrimvar();
        primvar.Set(usdFaceVertexRGBs);
        primvar.SetInterpolation(UsdGeomTokens->uniform);

        // if we have any crease sharpness info at the edges, write
        // that out here:
        if (edgeMap.size()) {
            primSchema.CreateCreaseLengthsAttr().Set(usdEdgeCreaseLengths);
            primSchema.CreateCreaseIndicesAttr().Set(usdFlattenedEdgeCreasePointIndices);
            primSchema.CreateCreaseSharpnessesAttr().Set(usdFlattenedEdgeCreaseSharpnesses);
        }
	
        // currently, we need to provide the extents for each mesh or
        // usdview gets mad
        VtVec3fArray  extent(2);
        GfRange3f     range;

        for (size_t i = 0; i < usdPoints.size(); ++i) {
            GfVec3f point = usdPoints[i];
            range.UnionWith(point);
        }
        extent[0] = range.GetMin();
        extent[1] = range.GetMax();
        primSchema.GetExtentAttr().Set(extent, UsdTimeCode::Default());

        // if we have UV, write it out here:
        std::vector<std::string> usedUVNames;
        for (unsigned int i = 0; i < namedUVs.size(); i++) {
            std::string originalUVName = mapNames[i];
            std::string safeUVName 
                = SafeNameFromExclusionList(originalUVName.c_str(), 
                                            usedUVNames);
            usedUVNames.push_back(safeUVName);
            std::string uvName = originalUVName;
            VtArray<GfVec2f> uvs = namedUVs[uvName];
            if (uvs.size())  {
                std::string uvName = safeUVName + "_uv";
                TfToken uvChannelToken(uvName.c_str());
                UsdGeomPrimvar uvSet_uv 
                    = primSchema.CreatePrimvar(uvChannelToken,
                                               SdfValueTypeNames->Float2Array,
                                               UsdGeomTokens->vertex);
                if (!IsSafeName(originalUVName.c_str())) {
                    // if we had to transform this name to be safe, we
                    // need to stash the original name out here:
                    TfToken originalNameKeyToken((usd_modoNamespace 
                                                  + ":" 
                                                  + usd_modo_originalUVName));
                    TfToken originalNameValToken(originalUVName);
                    uvSet_uv.GetAttr().SetCustomDataByKey(originalNameKeyToken,
                                                          VtValue(originalNameValToken));
                }
                uvSet_uv.Set(uvs);
            }
        }
    }
    return true;
}


bool
ModoExportUSD::CUSDSaver::WriteItemTransformAtParentPath(CLxUser_Item& item, 
                                                         std::string 
                                                         parentPath, 
                                                         GfMatrix4d 
                                                         localMatrix, 
                                                         std::string 
                                                         safeName) {
    if (CLxSceneSaver::ReallySaving()) {
        std::string path = parentPath + "/" + safeName;
        UsdGeomXform primSchema;
        SdfPath nameOfThisMatrix = SdfPath(path);
        primSchema = UsdGeomXform::Define(myStage, nameOfThisMatrix);
        TF_AXIOM(primSchema);
        // save out the original name so that we could get it back if
        // we round trip to modo
        if (!IsSafeName(CLxSceneSaver::ItemName())) {
            TfToken originalNameKeyToken((usd_modoNamespace 
                                          + ":" 
                                          + usd_modo_originalItemName));
            TfToken originalNameValueToken(CLxSceneSaver::ItemName());
            UsdPrim prim = primSchema.GetPrim();
            prim.SetCustomDataByKey(originalNameKeyToken, 
                                    VtValue(originalNameValueToken));
        }
        GfMatrix4d usdMatrix = _ConvertModoTransformToUSD(localMatrix);
        primSchema.MakeMatrixXform().Set(usdMatrix, UsdTimeCode::Default());
    }
    return true;
}


bool
ModoExportUSD::CUSDSaver::WriteItemCameraAtParentPath(CLxUser_Item& item, 
                                                      std::string parentPath, 
                                                      GfMatrix4d localMatrix, 
                                                      std::string safeName) {
    if (CLxSceneSaver::ReallySaving()) {
        std::string path = parentPath + "/" + safeName;
        float fStop = CLxSceneSaver::ChanFloat(LXsICHAN_CAMERA_FSTOP);
        // XXX In USD, focus distance is spec'ed in the base
        // unit, which as of 3/2016 is meters in modo & cm in USD.
        float focusDistanceCM 
            = CLxSceneSaver::ChanFloat(LXsICHAN_CAMERA_FOCUSDIST) 
            * s_meters2cm;
        // XXX We use the USD constants to keep the ratio
        // between the base unit and the focal length & aperture
        // consistent
        float focalLengthMM 
            = CLxSceneSaver::ChanFloat(LXsICHAN_CAMERA_FOCALLEN) 
            * s_meters2cm * GfCamera::FOCAL_LENGTH_UNIT;
        float horizontalApertureMM 
            = CLxSceneSaver::ChanFloat(LXsICHAN_CAMERA_APERTUREX) 
            * s_meters2cm * GfCamera::APERTURE_UNIT;
        float verticalApertureMM 
            = CLxSceneSaver::ChanFloat(LXsICHAN_CAMERA_APERTUREY) 
            * s_meters2cm * GfCamera::APERTURE_UNIT;
        UsdGeomCamera primSchema;
        SdfPath nameOfThisSchema = SdfPath(path);
        primSchema = UsdGeomCamera::Define(myStage, nameOfThisSchema);
        TF_AXIOM(primSchema);
        if (!IsSafeName(CLxSceneSaver::ItemName())) {
            // save out the original name so that we could get it back
            // if we round trip to modo
            TfToken originalNameKeyToken((usd_modoNamespace 
                                          + ":" 
                                          + usd_modo_originalItemName));
            TfToken originalNameValToken(CLxSceneSaver::ItemName());
            UsdPrim prim = primSchema.GetPrim();
            prim.SetCustomDataByKey(originalNameKeyToken, 
                                    VtValue(originalNameValToken));
        }

        primSchema.GetFocalLengthAttr().Set(focalLengthMM, 
                                            UsdTimeCode::Default());
        primSchema.GetFocusDistanceAttr().Set(focusDistanceCM, 
                                              UsdTimeCode::Default());
        primSchema.GetFStopAttr().Set(fStop,
                                      UsdTimeCode::Default());
        primSchema.GetHorizontalApertureAttr().Set(horizontalApertureMM, 
                                                   UsdTimeCode::Default());
        primSchema.GetVerticalApertureAttr().Set(verticalApertureMM, 
                                                 UsdTimeCode::Default());
        GfMatrix4d usdMatrix = _ConvertModoTransformToUSD(localMatrix);
        primSchema.MakeMatrixXform().Set(usdMatrix, 
                                         UsdTimeCode::Default());
   }
    return true;
}


bool
ModoExportUSD::CUSDSaver::WriteItemAtParentPath(CLxUser_Item& item, 
                                                std::string parentPath, 
                                                bool* actuallyWroteOut, 
                                                GfMatrix4d localMatrix, 
                                                std::string safeName) {
    LXtItemType itemType = item.Type();
    std::string itemName;
    item.GetUniqueName(itemName);
    *actuallyWroteOut = false;
    bool useInstancing = true;
    bool skipInvisibleItems = false;

    if (skipInvisibleItems && !ItemVisible()) {
        return true;
    }
    if (itemType == m_typeMesh) { 
        if (!WriteItemMeshAtParentPath(item, 
                                       parentPath, 
                                       localMatrix,
                                       safeName)) {
            std::cerr 
                << "ModoExportUSD's WriteItemMeshAtParentPath "
                << "failed - aborting USD export\n";
            return false;
        }
        *actuallyWroteOut = true;
        return true;
    }
    if (itemType == m_typeMeshInst) {
        if (useInstancing) {
            if (!WriteItemMeshInstanceAtParentPath(item, 
                                                   parentPath, 
                                                   localMatrix, 
                                                   safeName)) {
                std::cerr << "ModoExportUSD's "
                          << "WriteItemMeshInstanceAtParentPath "
                          << "failed - aborting USD export\n";
                return false;
            }
        } else {
            if (!WriteItemMeshAtParentPath(item, 
                                           parentPath, 
                                           localMatrix, 
                                           safeName)) {
                std::cerr << "ModoExportUSD's "
                          << "WriteItemMeshInstanceAtParentPath "
                          << "failed - aborting USD export\n";
                return false;
            }
        }
        *actuallyWroteOut = true;
        return true;
    }
    if (itemType == m_typeCamera) {
        if (!WriteItemCameraAtParentPath(item, 
                                         parentPath, 
                                         localMatrix, 
                                         safeName)) {
            std::cerr << "ModoExportUSD's "
                      << "WriteItemCameraAtParentPath "
                      << "failed - aborting USD export\n";
            return false;
        }
        *actuallyWroteOut = true;
        return true;
    }
    if ((itemType == m_typeLocator) ||
        (itemType == m_typeGroupLocator)) {
        if (!WriteItemTransformAtParentPath(item, 
                                            parentPath, 
                                            localMatrix, 
                                            safeName)) {
            std::cerr << "ModoExportUSD's "
                      << "WriteItemTransformAtParentPath "
                      << "failed - aborting USD export\n";
            return false;
        }
        *actuallyWroteOut = true;
        return true;
    }
    bool showUnknownItems = false;
    if (showUnknownItems) {
        CLxUser_SceneService sceneService;

        const char* itemTypeString;
        sceneService.ItemTypeName(itemType, &itemTypeString);
        std::cout << "found an item of unknown type \"" 
                  << itemTypeString << "\" named " << itemName << "\n";
    }
    return true;
}


bool
ModoExportUSD::CUSDSaver::SaveItemAtParentPath(CLxUser_Item& item, 
                                               std::string parentPath, 
                                               GfMatrix4d parentMatrixInverse, 
                                               std::vector<std::string>& usedNames) {
    unsigned int nChildren;
    std::string myName;
    item.GetUniqueName(myName);
    bool actuallyWroteOut = false;

    std::string mySafeName = SafeNameFromExclusionList(myName, usedNames);
    usedNames.push_back(mySafeName);
    GfMatrix4d localXform 
        = GetLocalTransformOfCurrentItem(parentMatrixInverse);

    if (!WriteItemAtParentPath(item, 
                               parentPath, 
                               &actuallyWroteOut, 
                               localXform, 
                               mySafeName)) {
        std::cerr << "failed to write item " 
                  << myName << " at " << parentPath << "\n";
        return false;
    }

    // get the world space xform of the soon-to-be parent and then
    // invert it.  as we just did above, we'll multiply this resulting
    // xform by our child's xform maxtrix to produce the local xform
    // for that child, which is what USD wants.
    GfMatrix4d currentWorldTransform = GetWorldTransformOfCurrentItem();
    GfMatrix4d inverseTransform = currentWorldTransform.GetInverse();

    // now write out our children
    item.SubCount(&nChildren);
    std::string newParentPath = parentPath + "/" + mySafeName;
    std::vector<std::string> childNamesUsedSoFar;
    childNamesUsedSoFar.clear();
    for (size_t i = 0; i < nChildren; i++) {
        CLxUser_Item	childItem;
        if (!item.GetSubItem (i, childItem)) {
            std::cerr << "failed to get child[" 
                      << i << "] at " << newParentPath << "\n";
            if (!SetItem(item)) {
                std::cerr << "unable to restore current item state\n";
            }
            return false;
        }
        if (!SetItem(childItem)) {
            std::cerr << "unable to set child item " 
                      << childItem << " as current item\n";
            return false;
        }
        if (!SaveItemAtParentPath(childItem, 
                                  newParentPath, 
                                  inverseTransform, 
                                  childNamesUsedSoFar)) {
            std::cerr << "failed to save child[" 
                      << i << "] at " << newParentPath << "\n";
            return false;
        }
    }
    if (!CLxSceneSaver::SetItem(item)) {
        std::cerr << "unable to restore current item state\n";
    }
    return true;
}


// should replace with code from Alex: 
// std::replace_if(name.begin(),name.end(), not boost::bind(isalnum, _1), '_');
std::string&
ModoExportUSD::CUSDSaver::SafeName(const char* name) {
    static std::string	safeName;

	if (isdigit(name[0])) {		// USD doesn't like names starting with digits
	  safeName.assign ("_");
	  safeName.append (name);
    } else {
        safeName.assign(name);
    }
    for (size_t i = 0; i < safeName.length(); i++) {
        if (!isalnum(safeName[i])) {
            safeName.replace(i, 1, 1, '_');
        }
    }
	return safeName;
}

std::string
ModoExportUSD::CUSDSaver::SafeNameFromExclusionList(std::string initialName,
std::vector<std::string> namesToExclude) {
    std::string newName = SafeName(initialName.c_str());
    bool done = false;

    while (!done) {
        // now check this against our list. If it's not in the list,
        // we're done. If it is, add a _ to the end and call this again.
        // eventually, we'll be done.
        for (size_t i = 0; i < namesToExclude.size(); i++) {
            std::string nameToExclude = namesToExclude[i];
            if (!nameToExclude.compare(newName)) {
                // this new name is on the list, so we need to try again:
                newName = SafeNameFromExclusionList(newName + "_", 
                                                    namesToExclude);
                continue;
            }
        }
        done = true;
    }
    return newName;

}


bool
ModoExportUSD::CUSDSaver::IsSafeName(const char* proposedName) {
    std::string currentName(proposedName);
    std::string transformedName = SafeName(proposedName);
    if (currentName.compare(transformedName)) {
        // these are different, so this is NOT a safe name
        return false;
    }
    return true;
}


LxResult
ModoExportUSD::CUSDSaver::ss_Save() {
    std::string rootPath("");
    try {
        if (CLxSceneSaver::ReallySaving()) {
            // Figure out if we're Z up or Y up:
            upAxis = 1; // 0===X, 1==Y, 2==Z
            CLxSceneSaver::StartScan(LXsITYPE_SCENE);
            if (CLxSceneSaver::NextItem()) {
                upAxis = CLxSceneSaver::ChanInt(LXsICHAN_SCENE_UPAXIS);
            }
            myStage  = UsdStage::CreateNew(file_name);
            if (!myStage) {
                std::cerr << "Failed to create stage file " 
                          << file_name << "\n";
                return LXe_FAILED;
            }
        }
        // first gather up our colors (taken from fmtgeo.cpp example)
        materialColorMap.clear();
        polygonExportPassType = POLYGON_EXPORT_PASS_COLOR;
        CLxSceneSaver::StartScan();
        while (CLxSceneSaver::NextMesh()) {
            CLxSceneSaver::WritePolys();
        }
        GatherColors();
        // we now have a map containing the diffuse RGB value for each
        // of the named maps as we export each mesh, we should check
        // what map it has applied to it and export out that diffuse
        // color with it.

        // we need to keep a map of the items we scan and what USD
        // names we give them.  we will use this so that when we find
        // an item that is an instance we know what other item to
        // reference
        meshItemNameMap.clear();
        if (!CLxSceneSaver::ReallySaving()) {
            // the first time we go through, we want to clear this map
            // out since this is where we'll store the correspondence
            // between a given mesh and all the mesh instances that
            // reference it.
            meshMasterMeshInstancesMap.clear();
        }
        pointCount = 0;
        CLxSceneSaver::StartScan();
        while (CLxSceneSaver::NextItem()) {
            LXtObjectID	parentItem;
            CLxUser_Item	item;

            if (!CLxSceneSaver::GetItem(item)) {
                // there's no current item - we're done
                continue;
            }
            // if the item has a parent, skip it.
            // We want to only grab the top-level items,
            // and then recurse down them to get their children.
            if (LXe_OK == item.Parent(&parentItem)) {
                lx::ObjRelease(parentItem);
                continue;
            }
            // okay, this item is a top-level item in the scene graph.
            // we want to call SaveItem() on it, where it will iterate
            // over its children recursively
            GfMatrix4d parentMatrixInverse;
            parentMatrixInverse.SetIdentity();
            std::vector<std::string> topLevelNames;
            if (!SaveItemAtParentPath(item, 
                                      rootPath, 
                                      parentMatrixInverse, 
                                      topLevelNames)) {
                std::cerr 
                    << "problem saving out some part "
                    << "of top level item \"" << rootPath << "\"\n";
            }
        }
        if (CLxSceneSaver::ReallySaving()) {
            switch (upAxis) {
                case 0:
                    // XXX note: As of 3/2016, USD only supports
                    // Y and Z up, so this will generate a "Coding
                    // Eror" in USD, but this is a permissible thing
                    // in modo and therefore we should attempt to pass
                    // it through the API. Current implementation does
                    // not stop the writing of the rest of the file
                    UsdGeomSetStageUpAxis(myStage, TfToken("X"));
                    break;
                case 1:
                    UsdGeomSetStageUpAxis(myStage, UsdGeomTokens->y);
                    break;
                case 2:
                    UsdGeomSetStageUpAxis(myStage, UsdGeomTokens->z);
                    break;
                default:
                    break;
            } 
            myStage->GetRootLayer()->Save();
            myStage->Close();
        }
    }
    catch (...) {
        std::cerr << "ModoExportUSD's ss_Save() threw an exception "
                  << ": aborting USD export\n";
        return LXe_FAILED;
    }
    return LXe_OK;
}


void
ModoExportUSD::CUSDSaver::ss_Point() {
    float point[3];
    CLxSceneSaver::PntPosition(point);
    LXtPointID pointID = CLxSceneSaver::PntID();
    pointIndexMap[pointID] = pointCount;
    usdPoints[pointCount++] = _ScaleModoPointToUSDScale(point[0], 
                                                        point[1], 
                                                        point[2]);

    if (!exportingASubdiv) {
        // if this isn't a subdiv, we're done
        return;
    }
    float cornerWeight;
    if (!CLxSceneSaver::PntMapValue(&cornerWeight)) {
        return ;
    }
    // we only save corners with non-zero weights
    if (cornerWeight) {
        weightedCornerPointIndices.push_back(pointCount);
        cornerWeights.push_back(cornerWeight);
    }
}


void
ModoExportUSD::CUSDSaver::ss_Edge() {
    if (!exportingASubdiv) {
        // if this isn't a subdiv, we're done
        return;
    }
    LXtPointID	pt1, pt2;
    if (!CLxSceneSaver::EdgeEndPoints(&pt1, &pt2)) {
        return;
    }
    
    float edgeWeight = 0;
    if (!CLxSceneSaver::EdgeMapValue(&edgeWeight)) {
        return ;
    }
    // if we have non-negative values for the edge weights, we store
    // them make sure that edges are always stored (and looked up)
    // small->large
    EdgeVert edge = { std::min(pointIndexMap[pt1], 
                               pointIndexMap[pt2]), 
                      std::max(pointIndexMap[pt1], 
                               pointIndexMap[pt2])
    };
    edgeMap.insert(EdgePair(edge, edgeWeight));
}

void
ModoExportUSD::CUSDSaver::ss_Polygon() {
    LXtID4 polyType = CLxSceneSaver::PolyType();
    if ((LXiPTYP_PSUB == polyType) || (LXiPTYP_SUBD == polyType)) {
        faceIsSubdiv.push_back(true);
    } else {
        faceIsSubdiv.push_back(false);
    }

    int n = CLxSceneSaver::PolyNumVerts();
    switch (polygonExportPassType) {
        case POLYGON_EXPORT_PASS_COLOR: {
            const char* mask = CLxSceneSaver::PolyTag(LXi_PTAG_MATR);
            if (mask) {
                GfVec3f black(0, 0, 0);
                materialColorMap[mask] = black;
            }
            break;
        }
        case POLYGON_EXPORT_PASS_VERTICES: {
            // how many vertices in this polygon?
            usdFaceVertexCounts.push_back(n);
            // what is the rgb value of this polygon?
            // note: we built up this map earlier in GatherColors()

            // default to white if we don't have an override
            GfVec3f rgb(1, 1, 1);  
            const char* currentMask = CLxSceneSaver::PolyTag(LXi_PTAG_MATR);
            if (currentMask) {
                // this mesh has an associated mask, let's see
                // if we've gathered a color value for that mesh:
                std::string mask(currentMask);
                rgb = materialColorMap[mask];
            }
            usdFaceVertexRGBs.push_back(rgb);
            for (int i = 0; i < n; i++) {
                LXtPointID pointID = CLxSceneSaver::PolyVertex(i);
                unsigned int vertexIndex = pointIndexMap[pointID];
                usdFlattenedFaceVertexIndices.push_back(vertexIndex);
            }
            break;
        }
        case POLYGON_EXPORT_PASS_UVS: {
            VtArray<GfVec2f> uvs;
            for (int i = 0; i < n; i++) {
                LXtPointID pointID = CLxSceneSaver::PolyVertex(i);
                GfVec2f uv;
                if (!CLxSceneSaver::PolyMapValue((float*)&uv, pointID)) {
                    uv[0] = uv[1] = 0.0;
                }
                uvs.push_back(uv);
            }
            namedUVs[uvName] = uvs;
            break;
        }
    }
}

void initialize () {
    LXx_ADD_SERVER(Saver, ModoExportUSD::CUSDSaver, "w_USD");
}

void cleanup() {
}

