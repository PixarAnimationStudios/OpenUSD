//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/subdivisionTagsSchema.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

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

class PrimListener : public HdSceneIndexObserver
{
public:
    void PrimsAdded(
        HdSceneIndexBase const&, AddedPrimEntries const&) override {}
    void PrimsRemoved(
        HdSceneIndexBase const&, RemovedPrimEntries const&) override {}
    void PrimsDirtied(
        HdSceneIndexBase const&, DirtiedPrimEntries const& entries) override {
        dirtied.insert(dirtied.end(), entries.begin(), entries.end());
    }
    void PrimsRenamed(
        HdSceneIndexBase const&, RenamedPrimEntries const&) override {}

    DirtiedPrimEntries dirtied;
};

void
SceneIndexTest(TagSample const& sample)
{
    UsdStageRefPtr stage = MakeStage(sample);
    UsdImagingStageSceneIndexRefPtr sceneIndex =
        UsdImagingStageSceneIndex::New();
    PrimListener listener;
    sceneIndex->AddObserver(HdSceneIndexObserverPtr(&listener));
    sceneIndex->SetStage(stage);
    sceneIndex->SetTime(UsdTimeCode(1));

    // Pull static tags as well; they must not become time dependencies.
    for (SdfPath const& path : {meshPath, staticMeshPath}) {
        HdSubdivisionTagsSchema tags = HdMeshSchema::GetFromParent(
            sceneIndex->GetPrim(path).dataSource).GetSubdivisionTags();
        for (TagSample const& tag : samples) {
            HdSampledDataSourceHandle source = HdSampledDataSource::Cast(
                tags.GetContainer()->Get(tag.name));
            TF_VERIFY(source && source->GetValue(0) == tag.first);
        }
    }

    for (double time : {2.0, 1.0, 2.0}) {
        listener.dirtied.clear();
        sceneIndex->SetTime(UsdTimeCode(time));
        HdDataSourceLocator const locator =
            HdMeshSchema::GetSubdivisionTagsLocator().Append(sample.name);
        TF_VERIFY(listener.dirtied.size() == 1);
        if (!listener.dirtied.empty()) {
            TF_VERIFY(listener.dirtied[0].primPath == meshPath);
            TF_VERIFY(listener.dirtied[0].dirtyLocators ==
                      HdDataSourceLocatorSet(locator));
        }
        HdSubdivisionTagsSchema tags = HdMeshSchema::GetFromParent(
            sceneIndex->GetPrim(meshPath).dataSource).GetSubdivisionTags();
        HdSampledDataSourceHandle source = HdSampledDataSource::Cast(
            tags.GetContainer()->Get(sample.name));
        TF_VERIFY(source && source->GetValue(0) ==
                  (time == 1 ? sample.first : sample.second));

        listener.dirtied.clear();
        sceneIndex->SetTime(UsdTimeCode(time));
        TF_VERIFY(listener.dirtied.empty());
    }
}

} // anonymous namespace

int main()
{
    TfErrorMark mark;
    for (TagSample const& sample : samples) {
        std::cout << "Testing " << sample.name << std::endl;
        SceneIndexTest(sample);
    }
    if (!mark.IsClean()) {
        std::cout << "FAILED" << std::endl;
        return 1;
    }
    std::cout << "OK" << std::endl;
    return 0;
}
