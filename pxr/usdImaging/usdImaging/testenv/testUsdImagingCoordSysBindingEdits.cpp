//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/imaging/hd/coordSysBindingSchema.h"

#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Editing the targets of an existing coord sys binding relationship is a
// property change, not a resync, so the new target has to come through the
// data sources already handed out for the prim, including the ones cached by
// the flattening scene index.

const char *const _layer = R"usda(#usda 1.0

def Camera "camL" {}
def Camera "camR" {}

def Mesh "m" (
    prepend apiSchemas = ["CoordSysAPI:proj"]
)
{
    rel coordSys:proj:binding = </camL>
}
)usda";

// Returns the proj binding of /m in the given scene index, or the empty path
// if there is none.
SdfPath
_GetBinding(const HdSceneIndexBaseRefPtr &si)
{
    HdPathDataSourceHandle const ds =
        HdCoordSysBindingSchema::GetFromParent(
            si->GetPrim(SdfPath("/m")).dataSource)
                .GetCoordSysBinding(TfToken("proj"));
    return ds ? ds->GetTypedValue(0.0f) : SdfPath();
}

bool
_Check(const HdSceneIndexBaseRefPtr &si,
       const std::string &label,
       const SdfPath &expected)
{
    const SdfPath actual = _GetBinding(si);
    std::cout << label << ": " << actual << "\n";
    if (actual != expected) {
        TF_CODING_ERROR("%s: expected <%s>, got <%s>",
                        label.c_str(), expected.GetText(), actual.GetText());
        return false;
    }
    return true;
}

bool
TestCoordSysBindingEdits()
{
    UsdStageRefPtr const stage = UsdStage::CreateInMemory();
    if (!stage->GetRootLayer()->ImportFromString(_layer)) {
        TF_CODING_ERROR("Failed to import layer");
        return false;
    }

    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);
    HdSceneIndexBaseRefPtr const si = sceneIndices.finalSceneIndex;

    const SdfPath camL("/camL"), camR("/camR");
    UsdRelationship rel =
        stage->GetPrimAtPath(SdfPath("/m"))
            .GetRelationship(TfToken("coordSys:proj:binding"));

    bool ok = _Check(si, "initial", camL);

    rel.SetTargets({ camR });
    sceneIndices.stageSceneIndex->ApplyPendingUpdates();
    ok &= _Check(si, "retarget to /camR", camR);

    rel.ClearTargets(/* removeSpec = */ false);
    sceneIndices.stageSceneIndex->ApplyPendingUpdates();
    ok &= _Check(si, "clear targets", SdfPath());

    rel.AddTarget(camL);
    sceneIndices.stageSceneIndex->ApplyPendingUpdates();
    ok &= _Check(si, "add target /camL", camL);

    return ok;
}

} // namespace

int
main()
{
    TfErrorMark m;
    bool ok = true;
    ok &= TestCoordSysBindingEdits();
    if (!ok || !m.IsClean()) {
        return 1;
    }
    std::cout << "All tests passed." << std::endl;
    return 0;
}
