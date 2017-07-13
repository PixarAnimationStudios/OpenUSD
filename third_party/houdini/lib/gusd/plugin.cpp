//
// Copyright 2017 Pixar
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
/**
 * \file houdinipkg/GUSD/plugin.cpp
 * \brief main plugin file
 */

#include "gusd.h"
#include "GEO_IOTranslator.h"
#include "GT_PackedUSD.h"
#include "GT_Utils.h"
#include "GU_PackedUSD.h"
#include "gusd/GT_PointInstancer.h"
#include "curvesWrapper.h"
#include "NURBSCurvesWrapper.h"

#include "meshWrapper.h"
#include "packedUsdWrapper.h"
#include "pointsWrapper.h"
#include "xformWrapper.h"
#include "instancerWrapper.h"
#include "USD_Traverse.h"

#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/kind/registry.h"

#include <GT/GT_PrimitiveTypes.h>
#include <OP/OP_OperatorTable.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_IOTable.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::endl;

static bool libInitialized = false;

void
GusdInit() 
{
    if( libInitialized )
        return;

    // register GT_USD conversion functions keyed on GT type id
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GT_PRIM_CURVE_MESH, 
            &GusdCurvesWrapper::defineForWrite);
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GT_PRIM_POINT_MESH,
            &GusdPointsWrapper::defineForWrite);
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GT_PRIM_PARTICLE,
            &GusdPointsWrapper::defineForWrite);
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GT_PRIM_POLYGON_MESH,
            &GusdMeshWrapper::defineForWrite);
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GT_PRIM_SUBDIVISION_MESH,
            &GusdMeshWrapper::defineForWrite);
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GT_GEO_PACKED,
            &GusdXformWrapper::defineForWrite);
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GusdGT_PackedUSD::getStaticPrimitiveType(),
            &GusdPackedUsdWrapper::defineForWrite);
    GusdPrimWrapper::registerPrimDefinitionFuncForWrite(
            GusdGT_PointInstancer::getStaticPrimitiveType(),
            &GusdInstancerWrapper::defineForWrite);


    GusdPrimWrapper::registerPrimDefinitionFuncForRead(
            TfToken("Mesh"), &GusdMeshWrapper::defineForRead);
    GusdPrimWrapper::registerPrimDefinitionFuncForRead(
            TfToken("Points"), &GusdPointsWrapper::defineForRead);
    GusdPrimWrapper::registerPrimDefinitionFuncForRead(
            TfToken("BasisCurves"), &GusdCurvesWrapper::defineForRead);
    GusdPrimWrapper::registerPrimDefinitionFuncForRead(
            TfToken("NurbsCurves"), &GusdNURBSCurvesWrapper::defineForRead);
    GusdPrimWrapper::registerPrimDefinitionFuncForRead(
            TfToken("Xform"), &GusdXformWrapper::defineForRead);
    GusdPrimWrapper::registerPrimDefinitionFuncForRead(
            TfToken("PointInstancer"), &GusdInstancerWrapper::defineForRead);

    GusdUSD_TraverseTable::GetInstance().SetDefault("std:components");
    libInitialized = true;
}

void 
GusdNewGeometryPrim( GA_PrimitiveFactory *f ) {

    GusdGU_PackedUSD::install(*f);
}

static bool geomIOInitialized = false;

void
GusdNewGeometryIO()
{
    if( geomIOInitialized )
        return;

    GU_Detail::registerIOTranslator(new GusdGEO_IOTranslator());

    UT_ExtensionList* geoextension;
    geoextension = UTgetGeoExtensions();
    if (!geoextension->findExtension("usd"))
       geoextension->addExtension("usd");
   geomIOInitialized = true;
}

static GusdPathComputeFunc gusdPathComputeFunc;

void 
GusdRegisterComputeRelativeSearchPathFunc( const GusdPathComputeFunc &func )
{
    gusdPathComputeFunc = func;
}

std::string 
GusdComputeRelativeSearchPath( const std::string &path ) 
{
    if( gusdPathComputeFunc ) {
        return gusdPathComputeFunc( path );
    }
    return path;
}

static TfToken gusdAssetKind = KindTokens->component;

void
GusdSetAssetKind( const TfToken &kind ) 
{
    gusdAssetKind = kind;
}

TfToken
GusdGetAssetKind()
{
    return gusdAssetKind;
}

PXR_NAMESPACE_CLOSE_SCOPE
