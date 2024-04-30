//
// Copyright 2024 Pixar
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

PXR_NAMESPACE_USING_DIRECTIVE

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

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
            new UsdImagingDelegate(renderIndex.get(), SdfPath("TestDelegate")));

    delegate->Populate(prim, excluded);

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

