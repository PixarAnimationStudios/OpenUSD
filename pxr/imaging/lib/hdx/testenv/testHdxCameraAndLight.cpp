//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdx/camera.h"
#include "pxr/imaging/hdx/light.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

#define VERIFY_PERF_COUNT(token, count) \
            TF_VERIFY(perfLog.GetCounter(token) == count, \
                    "expected %d found %.0f", \
                    count,\
                    perfLog.GetCounter(token));

PXR_NAMESPACE_USING_DIRECTIVE

static void CameraAndLightTest()
{
    Hdx_UnitTestDelegate delegate;
    HdRenderIndex& index = delegate.GetRenderIndex();
    HdChangeTracker& tracker = index.GetChangeTracker();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    HdRprimCollection collection(HdTokens->geometry, HdTokens->hull);
    HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());
    HdRenderPassSharedPtr renderPass(new HdRenderPass(&index, collection));
    HdEngine engine;

    GfMatrix4d tx(1.0f);
    tx.SetRow(3, GfVec4f(5, 0, 5, 1.0));
    SdfPath cube("geometry");
    delegate.AddCube(cube, tx);

    SdfPath camera("camera");
    SdfPath light("light");

    delegate.AddCamera(camera);
    delegate.AddLight(light, GlfSimpleLight());
    delegate.SetLight(light, HdxLightTokens->shadowCollection,
                      VtValue(HdRprimCollection(HdTokens->geometry,
                                                HdTokens->hull)));

    engine.Draw(index, renderPass, renderPassState);

    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 1);

    // Update camera matrix
    delegate.SetCamera(camera, GfMatrix4d(2), GfMatrix4d(2));
    tracker.MarkSprimDirty(camera, HdxCamera::DirtyMatrices);

    engine.Draw(index, renderPass, renderPassState);

    // batch should not be rebuilt
    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 1);

    // Update shadow collection
    delegate.SetLight(light, HdxLightTokens->shadowCollection,
                      VtValue(HdRprimCollection(HdTokens->geometry,
                                                HdTokens->refined)));
    tracker.MarkSprimDirty(light, HdxLight::DirtyCollection);

    engine.Draw(index, renderPass, renderPassState);

    // batch rebuilt
    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 2);

    // Update shadow collection again with the same data
    delegate.SetLight(light, HdxLightTokens->shadowCollection,
                      VtValue(HdRprimCollection(HdTokens->geometry,
                                                HdTokens->refined)));
    tracker.MarkSprimDirty(light, HdxLight::DirtyCollection);

    engine.Draw(index, renderPass, renderPassState);

    // batch should not be rebuilt
    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 2);
}

int main()
{
    TfErrorMark mark;

    CameraAndLightTest();

    TF_VERIFY(mark.IsClean());

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

