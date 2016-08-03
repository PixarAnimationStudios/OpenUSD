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
void MakeXf(UsdStageRefPtr stage, SdfPath path, GfVec3d trans, GfRotation rot)
{
    UsdGeomXform prim = UsdGeomXform::Define(stage, path);
    TF_VERIFY(prim);
    GfMatrix4d t(1), r(1), mat(1);
    mat = t.SetTranslate(trans) * r.SetRotate(rot);
    TF_VERIFY(prim.MakeMatrixXform().Set(mat, 1.0));
}

static 
UsdStageRefPtr 
BuildUsdStage()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    GfVec3d trans(1,1,1);
    GfRotation rot(GfVec3d(1,0,0), 90);
    MakeXf(stage, SdfPath("/Foo"), trans, rot);
    UsdGeomCube::Define(stage, SdfPath("/Foo/C"));

    SdfPath mesh1Path("/Foo/C/Mesh");
    UsdGeomMesh mesh1 = UsdGeomMesh::Define(stage, mesh1Path);
    TF_VERIFY(mesh1);
    mesh1.GetPointsAttr().Set(VtVec3fArray());

    stage->DefinePrim(SdfPath("/Untyped"), TfToken());
    UsdGeomCube::Define(stage, SdfPath("/Untyped/C"));

    MakeXf(stage, SdfPath("/Foo/Bar"), trans, rot);
    UsdGeomCube::Define(stage, SdfPath("/Foo/Bar/C"));

    SdfPath mesh2Path("/Foo/Bar/C/Mesh");
    UsdGeomMesh mesh2 = UsdGeomMesh::Define(stage, mesh2Path);
    TF_VERIFY(mesh2);
    mesh2.GetPointsAttr().Set(VtVec3fArray());

    MakeXf(stage, SdfPath("/Foo/Bar/Baz"), trans, rot);
    UsdGeomCube::Define(stage, SdfPath("/Foo/Bar/Baz/C"));

    SdfPath mesh3Path("/Foo/Bar/Baz/C/Mesh");
    UsdGeomMesh mesh3 = UsdGeomMesh::Define(stage, mesh3Path);
    TF_VERIFY(mesh3);
    mesh3.GetPointsAttr().Set(VtVec3fArray());

    return stage;
}

static
void
_IsClose(GfMatrix4d const& lhs, GfMatrix4d const& rhs)
{
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            TF_VERIFY(fabs(lhs[i][j] - rhs[i][j]) < .001);
        }
    }
}

static
void
TestRootPrim(UsdPrim const& prim)
{
    UsdImagingDelegate delegate;
    delegate.Populate(prim);
    delegate.SetTime(1.0);
    // Root compensation is actually being set implicitly by Populate(), no need
    // to set it here, but it also doesn't hurt.
    delegate.SetRootCompensation(prim.GetPath());
    delegate.SyncAll(true);

    // Always expect the root transform to be zero, since it is expected to be
    // drawn in local space.
    GfVec3d trans = delegate.GetTransform(prim.GetPath()).ExtractTranslation();
    TF_VERIFY(GfIsClose(trans, GfVec3d(0,0,0), 1e-6), 
            "Expected no translation for %s, but got (%f, %f, %f)\n",
            prim.GetPath().GetText(),
            trans[0],trans[1],trans[2]);
    std::cout << prim.GetPath() << " Translation: " << trans << std::endl;
    
    // Expect the nested mesh transform to be relative to the root.
    UsdGeomXformCache xfCache;
    xfCache.SetTime(1.0);
    UsdPrim m = prim.GetStage()->GetPrimAtPath(prim.GetPath().AppendChild(TfToken("Mesh")));
    GfMatrix4d rootXf = xfCache.GetLocalToWorldTransform(prim).GetInverse();
    GfMatrix4d localXf = xfCache.GetLocalToWorldTransform(m);

    GfMatrix4d mat = delegate.GetTransform(m.GetPath());
    _IsClose(mat, (localXf * rootXf));
    std::cout << m.GetPath() << " GetTransform: " << mat << std::endl;
    std::cout << m.GetPath() << " local * root: " 
                                            << (localXf * rootXf) << std::endl;
}

static
void
TestVis(UsdPrim const& prim)
{
    UsdImagingDelegate delegate;
    SdfPath absRoot = SdfPath::AbsoluteRootPath();
    delegate.Populate(prim.GetStage()->GetPrimAtPath(absRoot));
    delegate.SetTime(1.0);
    delegate.SyncAll(true);
    bool visible = delegate.GetVisible(prim.GetPath());
    TfToken vis = UsdGeomImageable(prim).ComputeVisibility(1.0);
    TF_VERIFY(visible == bool(vis == UsdGeomTokens->inherited));
    visible = delegate.GetVisible(prim.GetPath());
    TF_VERIFY(visible == bool(vis == UsdGeomTokens->inherited));
}

int main()
{
    UsdStageRefPtr stage = BuildUsdStage();

    TestRootPrim(stage->GetPrimAtPath(SdfPath("/Foo/C")));
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/Foo/Bar/C")));
    TestRootPrim(stage->GetPrimAtPath(SdfPath("/Foo/Bar/Baz/C")));

    TestVis(stage->GetPrimAtPath(SdfPath("/Untyped/C")));

    std::cout << "OK" << std::endl;
}

