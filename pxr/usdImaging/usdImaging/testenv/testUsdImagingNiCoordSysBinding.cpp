//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Coord sys bindings on a native instance (or inherited by it) need to be
// propagated to the prims of the prototype, and instances with different
// bindings need to use different prototypes.

const char *const _layer = R"usda(#usda 1.0

def Camera "camA" {}
def Camera "camB" {}

class "proto"
{
    def Mesh "m" {}
}

def Xform "instA1" (
    prepend apiSchemas = ["CoordSysAPI:proj"]
    instanceable = true
    prepend references = </proto>
)
{
    rel coordSys:proj:binding = </camA>
}

def Xform "grpA" (
    prepend apiSchemas = ["CoordSysAPI:proj"]
)
{
    rel coordSys:proj:binding = </camA>

    def Xform "instA2" (
        instanceable = true
        prepend references = </proto>
    )
    {
    }
}

def Xform "instB" (
    prepend apiSchemas = ["CoordSysAPI:proj"]
    instanceable = true
    prepend references = </proto>
)
{
    rel coordSys:proj:binding = </camB>
}

def Xform "instNone" (
    instanceable = true
    prepend references = </proto>
)
{
}
)usda";

bool
TestNiCoordSysBinding()
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

    // Map from the proj binding of each mesh (empty if none) to the
    // number of meshes with that binding.
    std::map<SdfPath, int> bindingToCount;
    for (const SdfPath &primPath : HdSceneIndexPrimView(si)) {
        const HdSceneIndexPrim prim = si->GetPrim(primPath);
        if (prim.primType != HdPrimTypeTokens->mesh) {
            continue;
        }
        SdfPath binding;
        if (HdPathDataSourceHandle const ds =
                HdCoordSysBindingSchema::GetFromParent(prim.dataSource)
                    .GetCoordSysBinding(TfToken("proj"))) {
            binding = ds->GetTypedValue(0.0f);
        }
        std::cout << primPath << ": " << binding << "\n";
        bindingToCount[binding]++;
    }

    // One prototype per distinct binding.
    const std::map<SdfPath, int> expected = {
        { SdfPath(), 1 },
        { SdfPath("/camA"), 1 },
        { SdfPath("/camB"), 1 } };
    if (bindingToCount != expected) {
        TF_CODING_ERROR("Unexpected coord sys bindings on prototype meshes");
        return false;
    }
    return true;
}

} // namespace

int
main()
{
    TfErrorMark m;
    bool ok = true;
    ok &= TestNiCoordSysBinding();
    if (!ok || !m.IsClean()) {
        return 1;
    }
    std::cout << "All tests passed." << std::endl;
    return 0;
}
