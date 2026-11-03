//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {

const SdfPath meshPath("/Mesh");
const SdfPath staticMeshPath("/StaticMesh");

struct TagSample {
    TfToken name;
    VtValue first;
    VtValue second;
};

const TagSample samples[] = {
    { UsdGeomTokens->creaseIndices,
      VtValue(VtIntArray{0, 1}), VtValue(VtIntArray{1, 2}) },
    { UsdGeomTokens->creaseLengths,
      VtValue(VtIntArray{2}), VtValue(VtIntArray{3}) },
    { UsdGeomTokens->creaseSharpnesses,
      VtValue(VtFloatArray{0}), VtValue(VtFloatArray{4}) },
    { UsdGeomTokens->cornerIndices,
      VtValue(VtIntArray{0}), VtValue(VtIntArray{1}) },
    { UsdGeomTokens->cornerSharpnesses,
      VtValue(VtFloatArray{0}), VtValue(VtFloatArray{4}) },
};

UsdStageRefPtr
MakeStage(TagSample const& sample)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    for (SdfPath const& path : {meshPath, staticMeshPath}) {
        UsdGeomMesh mesh = UsdGeomMesh::Define(stage, path);
        mesh.CreatePointsAttr().Set(
            VtVec3fArray{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}});
        mesh.CreateFaceVertexCountsAttr().Set(VtIntArray{4});
        mesh.CreateFaceVertexIndicesAttr().Set(VtIntArray{0, 1, 2, 3});
        for (TagSample const& tag : samples) {
            mesh.GetPrim().GetAttribute(tag.name).Set(tag.first);
        }
    }
    // Vary each array in isolation so that another animated tag cannot mask
    // its missing dependency. This test does not evaluate subdivision surfaces.
    UsdAttribute attr =
        stage->GetPrimAtPath(meshPath).GetAttribute(sample.name);
    attr.Set(sample.first, UsdTimeCode(1));
    attr.Set(sample.second, UsdTimeCode(2));
    return stage;
}

void
LegacyDelegateTest(TagSample const& sample)
{
    UsdStageRefPtr stage = MakeStage(sample);
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    UsdImagingDelegate delegate(renderIndex.get(), SdfPath::AbsoluteRootPath());
    delegate.Populate(stage->GetPseudoRoot());
    delegate.SetTime(UsdTimeCode(1));
    HdChangeTracker &tracker = renderIndex->GetChangeTracker();

    for (double time : {2.0, 1.0, 2.0}) {
        tracker.MarkRprimClean(meshPath);
        tracker.MarkRprimClean(staticMeshPath);
        delegate.SetTime(UsdTimeCode(time));
        HdDirtyBits const bits = tracker.GetRprimDirtyBits(meshPath);
        TF_VERIFY(bits & HdChangeTracker::DirtySubdivTags);
        // Tags vary independently of points and face topology.
        TF_VERIFY(!(bits & (HdChangeTracker::DirtyPoints |
                            HdChangeTracker::DirtyTopology)));
        TF_VERIFY(!(tracker.GetRprimDirtyBits(staticMeshPath) &
                    HdChangeTracker::DirtySubdivTags));
    }
}

} // anonymous namespace

int main()
{
    TfErrorMark mark;
    for (TagSample const& sample : samples) {
        std::cout << "Testing " << sample.name << std::endl;
        LegacyDelegateTest(sample);
    }
    if (!mark.IsClean()) {
        std::cout << "FAILED" << std::endl;
        return 1;
    }
    std::cout << "OK" << std::endl;
    return 0;
}
