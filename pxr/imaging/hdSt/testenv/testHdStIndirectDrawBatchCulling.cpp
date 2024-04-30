//
// Copyright 2023 Pixar
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

