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
#include "usdMaya/meshUtil.h"

#include <maya/MFnMesh.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MFnStringData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>

#include "pxr/usd/usdGeom/mesh.h"

// These tokens are supported Maya attributes used for Mesh surfaces
TF_DEFINE_PRIVATE_TOKENS(
        _meshTokens, 

        // we capitalize this because it doesn't correspond to an actual attribute
        (USD_EmitNormals)  
        );

// These tokens are supported Maya attributes used for SDiv surfaces
TF_DEFINE_PRIVATE_TOKENS(
        _subdivTokens, 
        (USD_subdivisionScheme)
        (USD_interpolateBoundary)
        (USD_faceVaryingInterpolateBoundary)
        );

// This can be customized for specific pipeline
// We read the USD bool attribute, if not present we look for the mojito bool attribute
bool PxrUsdMayaMeshUtil::getEmitNormals(const MFnMesh &mesh)
{
    MPlug plug = mesh.findPlug(MString(_meshTokens->USD_EmitNormals.GetText()));
    if (plug.isNull()) {
        plug = mesh.findPlug(MString("mjtoMeshVtxNormals"));
    }
    if (not plug.isNull() and plug.asBool()) {
        return true;
    }

    return false;
}

TfToken PxrUsdMayaMeshUtil::setEmitNormals(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue)
{
    MStatus status;
    
    TfToken normalInterp = defaultValue;
    //primSchema.GetSubdivisionSchemeAttr().Get(&subdScheme, UsdTimeCode::Default());

    //primSchema.GetNormalsAttr().Set(meshNormals, UsdTimeCode::Default());
    normalInterp = primSchema.GetNormalsInterpolation();

    // If normals are not present, don't create the attribute
    if (normalInterp == UsdGeomTokens->faceVarying) {
        MFnNumericAttribute nAttr;
        MObject attr = nAttr.create(_meshTokens->USD_EmitNormals.GetText(),
                                 "", MFnNumericData::kBoolean, 1, &status);    
        if (status == MS::kSuccess) {
            meshFn.addAttribute(attr);
        }
    }
    return normalInterp;
}


// This can be customized for specific pipeline
// We read the USD string attribute, if not present we look for the mojito string attribute
// If not present we look for the renderman for maya int attribute
// Lastly we force none if we have a mojitoForcePoly bool tag set
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken PxrUsdMayaMeshUtil::getSubdivScheme(const MFnMesh &mesh, TfToken defaultValue)
{
    MString sdScheme;
    MPlug plug = mesh.findPlug(MString(_subdivTokens->USD_subdivisionScheme.GetText()));
    if (!plug.isNull()) {
        sdScheme = plug.asString();
    } else {
        plug = mesh.findPlug(MString("mjtoSdScheme"));
        if (!plug.isNull()) {
            sdScheme = plug.asString();
        } else {
            plug = mesh.findPlug(MString("rman__torattr___subdivScheme"));
            if (!plug.isNull()) {
                switch(plug.asInt()) {
                    case 0: sdScheme = "catmullClark"; break;
                    case 1: sdScheme = "loop"; break;
                    default: break;
                }
            }
        }
    }
    
    // Foce Poly Bool Mojito Attribute
    plug = mesh.findPlug(MString("mjtoSdForcePoly"));
    if (!plug.isNull()) {
        if (plug.asBool()) {
            sdScheme="none";
        }
    }
    
    if (sdScheme=="") {
        return defaultValue;
    } else if (sdScheme=="none") {
        return UsdGeomTokens->none;
    } else if (sdScheme=="catmullClark") {
        return UsdGeomTokens->catmullClark;
    } else if (sdScheme=="loop") {
        return UsdGeomTokens->loop;
    } else if (sdScheme=="bilinear") {
        return UsdGeomTokens->bilinear;
    } else {
        MGlobal::displayError("Unsupported Subdivision scheme:" +
            sdScheme + " on mesh:" + MString(mesh.fullPathName()) + 
           ". Defaulting to: " + MString(defaultValue.GetText()));
        return defaultValue;
    }
}

// This can be customized for specific pipeline
// We read the USD string attribute, if not present we look for the mojito bool attribute
// If not present we look for the renderman for maya int attribute
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken PxrUsdMayaMeshUtil::getSubdivInterpBoundary(const MFnMesh &mesh, TfToken defaultValue)
{
    MString sdInterpBound;
    MPlug plug = mesh.findPlug(MString(_subdivTokens->USD_interpolateBoundary.GetText()));
    if (!plug.isNull()) {
        sdInterpBound = plug.asString();
    } else {
        plug = mesh.findPlug(MString("mjtoSdInterpBound"));
        if (!plug.isNull()) {
            if (plug.asBool()) {
                sdInterpBound = "edgeAndCorner";
            } else {
                sdInterpBound = "none";
            }
        } else {
            plug = mesh.findPlug(MString("rman__torattr___subdivInterp"));
            if (!plug.isNull()) {
                switch(plug.asInt()) {
                    case 0: sdInterpBound = "none"; break;
                    case 1: sdInterpBound = "edgeAndCorner"; break;
                    case 2: sdInterpBound = "edgeOnly"; break;
                    default: break;
                }
            }
        }
    }
    if (sdInterpBound=="") {
        return defaultValue;
    } else if (sdInterpBound=="none") {
        return UsdGeomTokens->none;
    } else if (sdInterpBound=="edgeAndCorner") {
        return UsdGeomTokens->edgeAndCorner;
    } else if (sdInterpBound=="edgeOnly") {
        return UsdGeomTokens->edgeOnly;
    } else {
        MGlobal::displayError("Unsupported InterpBoundary Attribute:" +
            sdInterpBound + " on mesh:" + MString(mesh.fullPathName()) + 
           ". Defaulting to: " + MString(defaultValue.GetText()));
        return defaultValue;
    }
}

// This can be customize for specific pipeline
// We read the USD string attribute, if not present we look for the mojito int attribute
// If not present we look for the renderman for maya int attribute
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken PxrUsdMayaMeshUtil::getSubdivFVInterpBoundary(const MFnMesh &mesh)
{
    MString sdFVInterpBound;
    MPlug plug = mesh.findPlug(MString(_subdivTokens->USD_faceVaryingInterpolateBoundary.GetText()));
    if (!plug.isNull()) {
        sdFVInterpBound = plug.asString();
    } else {
        plug = mesh.findPlug(MString("mjtoSdSmoothFVInterp"));
        if (!plug.isNull()) {
            switch(plug.asInt()) {
                case 1: sdFVInterpBound = "cornersPlus1"; break;
                case 2: sdFVInterpBound = "all"; break;
                case 3: sdFVInterpBound = "boundaries"; break;
                default: break;
            }
        } else {
            plug = mesh.findPlug(MString("rman__torattr___subdivFacevaryingInterp"));
            if (!plug.isNull()) {
                switch(plug.asInt()) {
                    case 0: sdFVInterpBound = "all"; break;
                    case 1: sdFVInterpBound = "cornersPlus1"; break;
                    case 2: sdFVInterpBound = "none"; break;
                    case 3: sdFVInterpBound = "boundaries"; break;
                    default: break;
                }
            }
        }
    }
    if (sdFVInterpBound=="") {
        return UsdGeomTokens->cornersPlus1;
    } else if (sdFVInterpBound == "all"
               or sdFVInterpBound == "bilinear") {
        return UsdGeomTokens->all;
    } else if (sdFVInterpBound == "cornersPlus1"
               or sdFVInterpBound == "edgeAndCorner") {
        return UsdGeomTokens->cornersPlus1;
    } else if (sdFVInterpBound == "none"
               or sdFVInterpBound == "edgeOnly") {
        return UsdGeomTokens->none;
    } else if (sdFVInterpBound == "boundaries"
               or sdFVInterpBound == "alwaysSharp") {
        return UsdGeomTokens->boundaries;
    } else {
        MGlobal::displayError("Unsupported Face Varying InterpBoundary Attribute:" +
            sdFVInterpBound + " on mesh:" + MString(mesh.fullPathName()) + 
           ". Defaulting to cornersPlus1");
        return UsdGeomTokens->cornersPlus1;
    }
}

TfToken PxrUsdMayaMeshUtil::setSubdivScheme(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue)
{
    MStatus status;
    
    // Determine if PolyMesh or SubdivMesh
    TfToken subdScheme;
    primSchema.GetSubdivisionSchemeAttr().Get(&subdScheme, UsdTimeCode::Default());
        
    // If retrieved scheme is default, don't create the attribute
    if (subdScheme != defaultValue) {
        MFnTypedAttribute stringAttr;
        MFnStringData stringData;
        MObject stringVal = stringData.create(subdScheme.GetText());
        MObject attr = stringAttr.create(_subdivTokens->USD_subdivisionScheme.GetText(),
                                         "", MFnData::kString, stringVal, &status);
        if (status == MS::kSuccess) {
            meshFn.addAttribute(attr);
        }
    }
    return subdScheme;
}

TfToken PxrUsdMayaMeshUtil::setSubdivInterpBoundary(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue)
{
    MStatus status;
    
    TfToken interpBoundary;
    primSchema.GetInterpolateBoundaryAttr().Get(&interpBoundary, UsdTimeCode::Default());

    // XXXX THIS IS FOR OPENSUBDIV IN MAYA ?
    if (interpBoundary != UsdGeomTokens->none) {
        MPlug boundRulePlug = meshFn.findPlug("boundaryRule", &status);
        if (status == MS::kSuccess) {
            int value=0;
            if(interpBoundary == UsdGeomTokens->edgeAndCorner) value=1;
            else if(interpBoundary == UsdGeomTokens->edgeOnly) value=2;
            boundRulePlug.setValue(value);
        }
    }
        
    if (interpBoundary != defaultValue) {
        MFnTypedAttribute stringAttr;
        MFnStringData stringData;
        MObject stringVal = stringData.create(interpBoundary.GetText());
        MObject attr = stringAttr.create(_subdivTokens->USD_interpolateBoundary.GetText(),
                         "", MFnData::kString, stringVal, &status);
        if (status == MS::kSuccess) {
            meshFn.addAttribute(attr);
        }
    }
    return interpBoundary;
}

TfToken PxrUsdMayaMeshUtil::setSubdivFVInterpBoundary(const UsdGeomMesh &primSchema, MFnMesh &meshFn)
{
    MStatus status;
    
    TfToken fvInterpBoundary = 
        primSchema.GetFaceVaryingLinearInterpolation();
        
    if (fvInterpBoundary != UsdGeomTokens->cornersPlus1) {
        MFnTypedAttribute stringAttr;
        MFnStringData stringData;
        MObject stringVal = stringData.create(fvInterpBoundary.GetText());
        MObject attr = stringAttr.create(_subdivTokens->USD_faceVaryingInterpolateBoundary.GetText(),
                                         "", MFnData::kString, stringVal, &status);
        if (status == MS::kSuccess) {
            meshFn.addAttribute(attr);
        }
    }
    return fvInterpBoundary;
}
