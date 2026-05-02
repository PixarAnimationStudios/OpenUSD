//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/imaging/hdx/selectionSceneIndexObserver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

std::string _SetToString(const std::set<int>& values)
{
    std::ostringstream out;
    out << "[";
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (it != values.begin()) {
            out << ", ";
        }
        out << *it;
    }
    out << "]";
    return out.str();
}

std::string _PathsToString(const SdfPathVector& paths)
{
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << paths[i].GetText();
    }
    out << "]";
    return out.str();
}

void TestSelectionSceneIndexNestedNativeInstancing()
{
    UsdStageRefPtr stage = UsdStage::Open("scene.usda");
    if (!TF_VERIFY(stage)) {
        return;
    }

    UsdImagingCreateSceneIndicesInfo createInfo;
    createInfo.stage = stage;

    UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(createInfo);
    if (!TF_VERIFY(sceneIndices.selectionSceneIndex)) {
        return;
    }

    HdxSelectionSceneIndexObserver observer;
    observer.SetSceneIndex(sceneIndices.selectionSceneIndex);

    sceneIndices.selectionSceneIndex->AddSelection(SdfPath("/Root/Tower_1"));

    HdSelectionSharedPtr selection = observer.GetSelection();
    if (!TF_VERIFY(selection)) {
        return;
    }

    const HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;
    const SdfPathVector selectedPrimPaths =
        selection->GetSelectedPrimPaths(mode);
    if (!TF_VERIFY(
            selectedPrimPaths.size() == 1,
            "Expected one selected prim, got %s",
            _PathsToString(selectedPrimPaths).c_str())) {
        return;
    }

    // Native-instanced descendants are selected on the propagated prototype
    // rprim, so selecting /Root/Tower_1 resolves to the shared Cube rprim.
    const SdfPath& selectedPrimPath = selectedPrimPaths[0];
    if (!TF_VERIFY(
            selectedPrimPath.GetNameToken() == TfToken("Cube"),
            "Expected the selected prim to be a cube, got <%s>",
            selectedPrimPath.GetText())) {
        return;
    }

    const HdSelection::PrimSelectionState*  primSelectionState =
        selection->GetPrimSelectionState(mode, selectedPrimPath);
    if (!TF_VERIFY(primSelectionState)) {
        return;
    }

    if (!TF_VERIFY(!primSelectionState->fullySelected)) {
        return;
    }

    if (!TF_VERIFY(
            !primSelectionState->instanceIndices.empty(),
            "Expected flattened instance indices for <%s>",
            selectedPrimPath.GetText())) {
        return;
    }

    std::set<int> actual;
    for (const VtIntArray &instanceIndices : primSelectionState->instanceIndices) {
        actual.insert(instanceIndices.begin(), instanceIndices.end());
    }

    // The fixture has 2 towers * 2 floors * 2 rows * 2 cubes = 16 flattened
    // cube instances total. Flattening follows:
    //   ((tower * 2 + floor) * 2 + row) * 2 + cube
    // so the tower index is the outermost block of 8 values: Tower_0 maps to
    // ids 0..7 and Tower_1 maps to ids 8..15.
    static const std::set<int> expected({8, 9, 10, 11, 12, 13, 14, 15});
    TF_VERIFY(
        actual == expected,
        "Expected Tower_1 to select indices %s on <%s>, got %s",
        _SetToString(expected).c_str(),
        selectedPrimPath.GetText(),
        _SetToString(actual).c_str());
}

} // namespace

int main()
{
    TfErrorMark mark;

    TestSelectionSceneIndexNestedNativeInstancing();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
