#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"

#include <iostream>

static
void MakeMesh(UsdStageRefPtr stage, SdfPath path)
{
    UsdGeomMesh prim = UsdGeomMesh::Define(stage, path);
    VtVec3fArray points;
    prim.GetPointsAttr().Set(points);
    TF_VERIFY(prim);
}

static 
UsdStageRefPtr 
BuildUsdStage()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Foo"));
    UsdGeomXform::Define(stage, SdfPath("/Bar"));
    MakeMesh(stage, SdfPath("/Foo/F1"));
    MakeMesh(stage, SdfPath("/Foo/F2"));
    MakeMesh(stage, SdfPath("/Bar/B1"));
    MakeMesh(stage, SdfPath("/Bar/B2"));
    MakeMesh(stage, SdfPath("/Bar/B3"));
    return stage;
}

static
void
TestRootPrim(UsdPrim const& prim, 
             SdfPathVector const& excluded, 
             int expectedCount)
{
    TfToken populatedPrimCount = UsdImagingTokens->usdPopulatedPrimCount;
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    
    // Reset all counters.
    perfLog.ResetCounters();

    UsdImagingDelegate delegate;
    delegate.Populate(prim, excluded);

    TF_VERIFY(perfLog.GetCounter(populatedPrimCount) == expectedCount, 
              "expected %d but found %d",
              expectedCount,
              int(perfLog.GetCounter(populatedPrimCount)));
}

int main()
{
    UsdStageRefPtr stage = BuildUsdStage();

    SdfPathVector excludedPaths;
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/")), excludedPaths, 5);

    excludedPaths.clear();
    excludedPaths.push_back(SdfPath("/Bar"));
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/")), excludedPaths, 2);

    excludedPaths.clear();
    excludedPaths.push_back(SdfPath("/Foo"));
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/")), excludedPaths, 3);

    excludedPaths.clear();
    excludedPaths.push_back(SdfPath("/Foo"));
    excludedPaths.push_back(SdfPath("/Bar"));
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/")), excludedPaths, 0);

    excludedPaths.clear();
    excludedPaths.push_back(SdfPath("/Foo"));
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/Foo")), excludedPaths, 0);

    excludedPaths.clear();
    excludedPaths.push_back(SdfPath("/Bar"));
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/Foo")), excludedPaths, 2);


    std::cout << "OK" << std::endl;
}

