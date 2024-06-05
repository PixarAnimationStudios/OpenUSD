//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
HdIndirectDrawBatchTest()
{
    HdSt_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex& index = delegate.GetRenderIndex();
    index.Clear();

    HdRprimSharedData *sharedData = new HdRprimSharedData(1, true);
    sharedData->instancerLevels = 1;
    HdStDrawItem *drawItem = new HdStDrawItem(sharedData);
    drawItem->GetDrawingCoord()->SetInstancePrimvarBaseIndex(
        HdDrawingCoord::CustomSlotsBegin);
    HdStDrawItemInstance *drawItemInstance = new HdStDrawItemInstance(drawItem);
    HdSt_IndirectDrawBatch *batch = new HdSt_IndirectDrawBatch(drawItemInstance);

    TF_VERIFY(batch->IsEnabledGPUFrustumCulling());
    TF_VERIFY(batch->IsEnabledGPUInstanceFrustumCulling());
    return true;
}

int main()
{
    TfErrorMark mark;
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    bool success = HdIndirectDrawBatchTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

