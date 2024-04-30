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

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/gf/vec3f.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

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
PrimvarsTest()
{
    // This test is based on testHdStDrawBatching, specifically the 
    // IndirectDrawBatchMigrationTest()
    std::cout << "==== PrimvarsTest:\n";

    HdPerfLog &perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdSt_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    driver.SetupAovs(256, 256);

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        delegate.GetRenderIndex().GetResourceRegistry());

    VtDictionary dict = resourceRegistry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    delegate.AddCube(SdfPath("/subdiv1"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->catmullClark);
    delegate.AddCube(SdfPath("/bilinear1"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->bilinear);
    delegate.AddCube(SdfPath("/subdiv2"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->catmullClark);
    delegate.AddCube(SdfPath("/bilinear2"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->bilinear);

    
    // create 2 renderpasses
    HdRenderPassSharedPtr flatPass(
        new HdSt_RenderPass(&delegate.GetRenderIndex(),
                            HdRprimCollection(HdTokens->geometry,
                              HdReprSelector(HdReprTokens->hull))));
    HdRenderPassSharedPtr smoothPass(
        new HdSt_RenderPass(&delegate.GetRenderIndex(),
                            HdRprimCollection(HdTokens->geometry,
                              HdReprSelector(HdReprTokens->smoothHull))));

    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Draw flat pass. This produces 1 buffer array containing both catmullClark
    // and bilinear mesh since we don't need normals.
    driver.Draw(flatPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw flat -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Add primvars (even though the shader doesn't use them)
    // Note: HDST_ENABLE_MATERIAL_PRIMVAR_FILTERING is set to false.
    delegate.AddPrimvar(SdfPath("/subdiv1"), TfToken("cFoo"),
                VtValue(VtVec3fArray(1, GfVec3f(1,2,3))),
                HdInterpolationConstant, HdPrimvarRoleTokens->none);
    
    delegate.AddPrimvar(SdfPath("/bilinear2"), TfToken("vBar"),
                VtValue(VtFloatArray(8, 42.0)), HdInterpolationVertex,
                HdPrimvarRoleTokens->none);

    driver.Draw(flatPass, false);
    // The subdiv meshes with new primvars need to be migrated into new
    // buffer array.

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw flat : primvars added -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Draw smooth pass. Then subdiv meshes need to be migrated into new
    // buffer array, while bilinear meshes remain. This is just to test repr
    // changes after primvar addition.

    driver.Draw(smoothPass, false);
    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw smooth -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Resize a primvar and draw smooth pass again.

    delegate.UpdatePrimvarValue(SdfPath("/subdiv1"), TfToken("cFoo"),
        VtValue(VtVec3fArray(2, GfVec3f(1,2,3))));
    driver.Draw(smoothPass, false);
    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw smooth : primvar resized -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Remove one of the primvars and draw smooth pass again.
    // Batches will be rebuilt due to BAR migration.
    delegate.RemovePrimvar(SdfPath("/subdiv1"), TfToken("cFoo"));

    driver.Draw(smoothPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw smooth : primvar removed -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // -------------------------------------------------------------------------
    // Add a facevarying primvar and remove it. Since a facevarying primvar
    // hasn't been added yet, this should result in the following transition for
    // the associated BAR.
    // [no BAR] ---add primvar-> [valid fv BAR] --remove primvar-> [no BAR]
    delegate.AddPrimvar(SdfPath("/bilinear2"), TfToken("fvBaz"),
                VtValue(VtVec3fArray(24, GfVec3f(1,2,3))),
                HdInterpolationFaceVarying, HdPrimvarRoleTokens->none);
    
    driver.Draw(smoothPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw smooth : facevarying primvar added -----\n", dict,
         perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    delegate.RemovePrimvar(SdfPath("/bilinear2"), TfToken("fvBaz"));

    driver.Draw(smoothPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("\n----- draw smooth : facevarying primvar removed -----\n", dict,
         perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);
}


int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    PrimvarsTest();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

