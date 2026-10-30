//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/coordSysPrimSceneIndex.h"

#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const char *const _layer = R"usda(#usda 1.0

def Camera "camL" {}
def Camera "camR" {}

def Mesh "m" (
    prepend apiSchemas = ["CoordSysAPI:proj1"]
)
{
    rel coordSys:proj1:binding = </camL>
}
)usda";

// Returns the binding of /m in the given scene index, or the empty path
// if there is none.
SdfPath
_GetBindingTarget(
    const HdSceneIndexBaseRefPtr &si,
    TfToken const& bindingName)
{
    HdPathDataSourceHandle const ds =
        HdCoordSysBindingSchema::GetFromParent(
            si->GetPrim(SdfPath("/m")).dataSource)
                .GetCoordSysBinding(bindingName);
    return ds ? ds->GetTypedValue(0.0f) : SdfPath();
}

void
_CheckBinding(
    const HdSceneIndexBaseRefPtr &si,
    const TfToken &bindingName,
    const SdfPath &expected)
{
    const SdfPath actual = _GetBindingTarget(si, bindingName);
    TF_VERIFY(actual == expected,
        "expected <%s>, got <%s>",
         expected.GetText(), actual.GetText());
}

class Logger : public HdSceneIndexObserver
{
public:
    std::set<SdfPath> coordSysPrims;

    void PrimsAdded(const HdSceneIndexBase &,
            const AddedPrimEntries &entries) override
    {
        for (auto &e : entries) {
            if (e.primType == HdSprimTypeTokens->coordSys) {
                coordSysPrims.insert(e.primPath);
                std::cout << "    ADDED   " << e.primPath << "\n";
            } else {
                coordSysPrims.erase(e.primPath);
            }
        }
    }
    void PrimsRemoved(const HdSceneIndexBase &,
            const RemovedPrimEntries &entries) override
    {
        for (auto &e : entries) {
            for (auto it = coordSysPrims.begin(); it != coordSysPrims.end();)
                it = it->HasPrefix(e.primPath) ? coordSysPrims.erase(it)
                                               : std::next(it);
            if (e.primPath.GetString().find("coordSys") != std::string::npos)
                std::cout << "    REMOVED " << e.primPath << "\n";
        }
    }
    void PrimsDirtied(const HdSceneIndexBase &,
            const DirtiedPrimEntries &) override {}
    void PrimsRenamed(const HdSceneIndexBase &,
            const RenamedPrimEntries &) override {}
};

void
TestCoordSysBindingEdits()
{
    UsdStageRefPtr const stage = UsdStage::CreateInMemory();
    stage->GetRootLayer()->ImportFromString(_layer);

    UsdPrim mesh = stage->GetPrimAtPath(SdfPath("/m"));
    TF_VERIFY(mesh);

    // Set up scene index chain.
    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);
    const HdSceneIndexBaseRefPtr usdSi = sceneIndices.finalSceneIndex;

    // We also test the further transformation applied by
    // HdsiCoordSysPrimSceneIndex.
    auto coordSysSi = HdsiCoordSysPrimSceneIndex::New(usdSi);

    Logger logger;
    coordSysSi->AddObserver(HdSceneIndexObserverPtr(&logger));

    const SdfPath camL("/camL"), camR("/camR");
    const TfToken proj1Binding("proj1");
    const TfToken proj2Binding("proj2");
    UsdRelationship proj1BindingRel =
        stage->GetPrimAtPath(SdfPath("/m"))
            .GetRelationship(TfToken("coordSys:proj1:binding"));

    // Confirm initial state.
    printf("Checking initial state.\n");
    _CheckBinding(usdSi, proj1Binding, camL);
    _CheckBinding(coordSysSi, proj1Binding,
        SdfPath("/camL/__coordSys_proj1"));
    TF_VERIFY(coordSysSi->GetPrim(SdfPath("/camL/__coordSys_proj1")));

    // Change the binding target.
    printf("Change proj1 binding to camR.\n");
    proj1BindingRel.SetTargets({ camR });
    sceneIndices.stageSceneIndex->ApplyPendingUpdates();
    _CheckBinding(usdSi, proj1Binding, camR);
    _CheckBinding(coordSysSi, proj1Binding,
        SdfPath("/camR/__coordSys_proj1"));
    TF_VERIFY(coordSysSi->GetPrim(SdfPath("/camR/__coordSys_proj1")));

    // Add a second binding on the same prim, to camL.
    printf("Add proj2 binding to camL.\n");
    UsdShadeCoordSysAPI proj2API =
        UsdShadeCoordSysAPI::Apply(mesh, proj2Binding);
    UsdRelationship proj2BindingRel =
        proj2API.GetBindingRel();
    proj2BindingRel.SetTargets({ camL });
    sceneIndices.stageSceneIndex->ApplyPendingUpdates();
    _CheckBinding(usdSi, proj1Binding, camR);
    _CheckBinding(usdSi, proj2Binding, camL);
    _CheckBinding(coordSysSi, proj1Binding,
        SdfPath("/camR/__coordSys_proj1"));
    _CheckBinding(coordSysSi, proj2Binding,
        SdfPath("/camL/__coordSys_proj2"));
    TF_VERIFY(coordSysSi->GetPrim(SdfPath("/camR/__coordSys_proj1")));
    TF_VERIFY(coordSysSi->GetPrim(SdfPath("/camL/__coordSys_proj2")));

    // Remove 1st binding target.
    printf("Remove proj1 binding.\n");
    proj1BindingRel.ClearTargets(/* removeSpec = */ false);
    sceneIndices.stageSceneIndex->ApplyPendingUpdates();
    _CheckBinding(usdSi, proj1Binding, SdfPath());
    _CheckBinding(usdSi, proj2Binding, camL);
    _CheckBinding(coordSysSi, proj1Binding,
        SdfPath());
    _CheckBinding(coordSysSi, proj2Binding,
        SdfPath("/camL/__coordSys_proj2"));
    TF_VERIFY(!coordSysSi->GetPrim(SdfPath("/camR/__coordSys_proj1")));
    TF_VERIFY(coordSysSi->GetPrim(SdfPath("/camL/__coordSys_proj2")));

    // Set 1st binding target back to camL.
    printf("Set proj1 binding to camL.\n");
    proj1BindingRel.AddTarget(camL);
    sceneIndices.stageSceneIndex->ApplyPendingUpdates();
    _CheckBinding(usdSi, proj1Binding, camL);
    TF_VERIFY(coordSysSi->GetPrim(SdfPath("/camL/__coordSys_proj1")));
    TF_VERIFY(coordSysSi->GetPrim(SdfPath("/camL/__coordSys_proj2")));
}

} // namespace

int
main()
{
    TfErrorMark m;
    TestCoordSysBindingEdits();
    TF_VERIFY(m.IsClean());
    std::cout << "All tests passed." << std::endl;
    return 0;
}
