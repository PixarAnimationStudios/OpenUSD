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

#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/gf/vec3f.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (frontCullRepr)
    );

template <typename T>
static VtValue
_BuildArrayValue(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return VtValue(result);
}

static void
PrintPerfCounter(HdPerfLog &perfLog, TfToken const &token)
{
    std::cout << token << " = " << perfLog.GetCounter(token) << "\n";
}

static void
Dump(std::string const &message, VtDictionary dict, HdPerfLog &perfLog)
{
    // Get the keys in sorted order.  This ensures consistent reporting
    // regardless of the sort order of dict.
    std::set<std::string> keys;
    for (auto v: dict) {
        keys.insert(v.first);
    }

    std::cout << message;
    for (auto key: keys) {
        std::cout << key << ", ";
        const VtValue& value = dict[key];
        if (value.IsHolding<size_t>()) {
            std::cout << value.Get<size_t>();
        }
        std::cout << "\n";
    }
    PrintPerfCounter(perfLog, HdPerfTokens->garbageCollected);
    PrintPerfCounter(perfLog, HdPerfTokens->drawCalls);
}

static void
FaceCullingTest()
{
    std::cout << "==== FaceCullingTest:\n";

    HdPerfLog &perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdSt_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        delegate.GetRenderIndex().GetResourceRegistry());

    VtDictionary dict = resourceRegistry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    const SdfPath cube1("/cube1"), cube2("/cube2"), cube3("/cube3"), 
                  cube4("/cube4"), cube5("/cube5");;
    delegate.AddCube(cube1, GfMatrix4f(1));
    delegate.AddCube(cube2, GfMatrix4f(1));
    delegate.AddCube(cube3, GfMatrix4f(1));
    delegate.AddCube(cube4, GfMatrix4f(1));

    HdRenderPassSharedPtr renderPass(
        new HdSt_RenderPass(&delegate.GetRenderIndex(),
                            HdRprimCollection(HdTokens->geometry,
                              HdReprSelector(HdReprTokens->smoothHull))));

    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);

    // Draw initial state
    driver.Draw(renderPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw initial state -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);


    // Change cube2's transform to be left-handed and draw again
    GfMatrix4f transform(1);
    transform.SetScale(GfVec3f(-1, 1, 1));
    delegate.UpdateTransform(cube2, transform);

    driver.Draw(renderPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- add left handed transform to cube2 -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);


    // Change cube3's repr to cull front faces
    HdMesh::ConfigureRepr(_tokens->frontCullRepr,
                          HdMeshReprDesc(HdMeshGeomStyleHull,
                                         HdCullStyleFront,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/true));
    delegate.SetReprSelector(cube3, 
                             HdReprSelector(_tokens->frontCullRepr));
    driver.Draw(renderPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- change cube3's repr to cull front -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);


    // Add instancer to cube4
    const SdfPath instancer("/instancer");
    delegate.AddInstancer(instancer);
    VtVec3fArray scale = { GfVec3f(1, 1, 1) };
    VtVec4fArray rotate = { GfVec4f(0, 0, 0, 0) };
    VtVec3fArray translate = { GfVec3f(0, 0, 0) };
    VtIntArray prototypeIndex = { 0 }; 
    delegate.SetInstancerProperties(
        instancer, prototypeIndex, scale, rotate, translate);
    delegate.UpdateInstancer(cube4, instancer);

    driver.Draw(renderPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- add instancer to cube4 -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);

    // Add another left-handed cube
    transform.SetScale(GfVec3f(1, 1, -2));
    delegate.AddCube(cube5, transform);

    driver.Draw(renderPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- add additional left-handed cube -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
}


int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    FaceCullingTest();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

