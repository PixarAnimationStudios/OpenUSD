//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/drawModeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/sceneIndexPrimView.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// The prim names UsdImaging_DrawModeStandin inserts for each draw mode.
bool
_IsDrawModeStandin(const TfToken &name)
{
    static const TfToken cardsMesh("cardsMesh");
    static const TfToken boundsCurves("boundsCurves");
    static const TfToken originCurves("originCurves");

    return name == cardsMesh
        || name == boundsCurves
        || name == originCurves;
}

// A standin on the binding scope replaces the scope's whole subtree, including
// the instancer, so its rprim is an immediate child of the scope prim.  Prims
// deeper than that -- notably the propagated prototype -- are carded normally.
bool
_IsOnInstanceBindingScope(const SdfPath &path)
{
    return path.GetParentPath().GetParentPath().GetNameToken()
        == UsdImagingTokens->niPropagatedPrototypesScope;
}

}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: testUsdImagingDrawModeAggregationScope"
                     " <file.usd>\n";
        return -1;
    }

    UsdStageRefPtr stage = UsdStage::Open(argv[1]);
    if (!TF_VERIFY(stage, "Failed to open stage at path <%s>.\n", argv[1])) {
        return -1;
    }

    // Reproduce the ordering Presto uses: no draw mode inside prototype
    // propagation, and one draw mode scene index downstream of the whole
    // graph.
    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    info.addDrawModeSceneIndex = false;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);

    const HdSceneIndexBaseRefPtr sceneIndex =
        UsdImagingDrawModeSceneIndex::New(
            sceneIndices.finalSceneIndex, /* inputArgs = */ nullptr);

    SdfPathVector standinsOnBindingScope;
    SdfPathVector otherStandins;

    for (const SdfPath &path : HdSceneIndexPrimView(sceneIndex)) {
        if (!_IsDrawModeStandin(path.GetNameToken())) {
            continue;
        }
        if (_IsOnInstanceBindingScope(path)) {
            standinsOnBindingScope.push_back(path);
        } else {
            otherStandins.push_back(path);
        }
    }

    int result = 0;

    // Instance aggregation copies the instance's geomModel onto its binding
    // scope, so a draw mode scene index running downstream of propagation used
    // to create a draw mode standin for the scope.  That standin was not
    // instanced and drew once at the origin. Verify here this standin is no
    // longer created
    if (!standinsOnBindingScope.empty()) {
        std::cerr << "ERROR: draw mode standin(s) on the instance binding "
                     "scope:\n";
        for (const SdfPath &path : standinsOnBindingScope) {
            std::cerr << "    " << path << "\n";
        }
        result = -1;
    }

    // Guards against excluding too much: the propagated prototype must still
    // be carded, since that is what the instancer draws.
    if (otherStandins.empty()) {
        std::cerr << "ERROR: no draw mode standin outside the instance "
                     "binding scope; the propagated prototype should have "
                     "one.\n";
        result = -1;
    }

    if (result == 0) {
        std::cout << "Draw mode standin(s) created, none on an instance "
                     "binding scope:\n";
        for (const SdfPath &path : otherStandins) {
            std::cout << "    " << path << "\n";
        }
    }

    return result;
}
