//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
SetRprimCollectionTest()
{
    HdSt_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex& index = delegate.GetRenderIndex();
    HdChangeTracker& tracker = index.GetChangeTracker();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    HdRprimCollection collection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    HdRenderPassSharedPtr renderPass(new Hd_UnitTestNullRenderPass(&index, collection));
    HdStRenderPassStateSharedPtr renderPassState(new HdStRenderPassState());

    // ---------------------------------------------------------------------- //
    // Test RenderPass hash dependency on custom buffers
    // ---------------------------------------------------------------------- //
    HdResourceRegistrySharedPtr const &resourceRegistry = 
        index.GetResourceRegistry();

    HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(resourceRegistry);

    HdStRenderPassShaderSharedPtr renderPassShader
        = renderPassState->GetRenderPassShader();
    HdBufferSpecVector offsetSpecs;
    offsetSpecs.emplace_back(TfToken("offset"),
                             HdTupleType{HdTypeInt32, 1});
    HdBufferArrayRangeSharedPtr bar = 
        hdStResourceRegistry->AllocateSingleBufferArrayRange(
            /*role*/TfToken("selection"),
            offsetSpecs,
            HdBufferArrayUsageHintBitsUniform);

    HdStRenderPassShader::ID emptyId = renderPassShader->ComputeHash();
    renderPassShader->AddBufferBinding(HdStBindingRequest(HdStBinding::UBO,
                                    TfToken("uniforms"), bar,
                                    /*interleave*/true));
    std::cout << "empty: " << emptyId << std::endl;
    std::cout << "offset: " << renderPassShader->ComputeHash() << std::endl;

    TF_VERIFY(emptyId != renderPassShader->ComputeHash());
    HdStRenderPassShader::ID uniformsId = renderPassShader->ComputeHash();
    
    renderPassShader->ClearBufferBindings();
    std::cout << "empty: " << renderPassShader->ComputeHash() << std::endl;
    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());

    renderPassShader->AddBufferBinding(HdStBindingRequest(HdStBinding::SSBO,
                                    TfToken("differentName"), bar,
                                    /*interleave*/true));
    // 
    // Make sure that changing internal values produces a different hash.
    //
    std::cout << "different: " << renderPassShader->ComputeHash()<< std::endl;
    TF_VERIFY(emptyId != renderPassShader->ComputeHash());
    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());
    HdStRenderPassShader::ID differentId = renderPassShader->ComputeHash();
    // 
    // uniformsId is the renderPassShader with no custom buffers, so uniformsId
    // should match the current hash after calling clear here.
    //
    renderPassShader->ClearBufferBindings();
    TF_VERIFY(emptyId == renderPassShader->ComputeHash());
    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());
    TF_VERIFY(differentId != renderPassShader->ComputeHash());
    // 
    // Try a couple buffers.
    //
    renderPassShader->AddBufferBinding(HdStBindingRequest(HdStBinding::SSBO,
                                    TfToken("differentName"), bar,
                                    /*interleave*/true));
    renderPassShader->AddBufferBinding(HdStBindingRequest(HdStBinding::UBO,
                                    TfToken("uniforms"), bar,
                                    /*interleave*/true));
    HdStRenderPassShader::ID multiId = renderPassShader->ComputeHash();
    std::cout << "multi: " << renderPassShader->ComputeHash()<< std::endl;
    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());
    TF_VERIFY(differentId != renderPassShader->ComputeHash());
    TF_VERIFY(emptyId != renderPassShader->ComputeHash());
    TF_VERIFY(multiId == renderPassShader->ComputeHash());
    //
    // Try a couple buffers, shuffle params.
    //
    renderPassShader->AddBufferBinding(HdStBindingRequest(HdStBinding::UBO,
                                    TfToken("differentName"), bar,
                                    /*interleave*/true));
    renderPassShader->AddBufferBinding(HdStBindingRequest(HdStBinding::UBO,
                                    TfToken("uniforms"), bar,
                                    /*interleave*/false));
    HdStRenderPassShader::ID multiShuffId = renderPassShader->ComputeHash();
    std::cout << "multiShuff: " << renderPassShader->ComputeHash()<< std::endl;

    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());
    TF_VERIFY(differentId != renderPassShader->ComputeHash());
    TF_VERIFY(emptyId != renderPassShader->ComputeHash());
    TF_VERIFY(multiId != renderPassShader->ComputeHash());
    TF_VERIFY(multiShuffId == renderPassShader->ComputeHash());

    // ---------------------------------------------------------------------- //

    GfMatrix4f identity;
    identity.SetIdentity();
    delegate.AddCube(SdfPath("/Cube0"), identity);

    // Note: It used to be that each render pass owned a dirty list.
    // Instead, now, the render index manages a single dirty list.
    // To avoid test API in the render index to grab the dirty list, we create
    // a local dirty list and update it the same way the render index does
    // during SyncAll.

    HdDirtyList dirtyList(index);
    dirtyList.UpdateRenderTagsAndReprSelectors(
        TfTokenVector(/*empty render tags*/),{ collection.GetReprSelector() });

    // the dirty list has "/Cube0"
    TF_VERIFY(dirtyList.GetDirtyRprims().size() == 1);

    // clean "/Cube0"
    tracker.MarkRprimClean(SdfPath("/Cube0"));

    std::cerr << "!! : ";
    HdChangeTracker::DumpDirtyBits(tracker.GetRprimDirtyBits(SdfPath("/Cube0")));

    // add "/Cube1"
    delegate.AddCube(SdfPath("/Cube1"), identity);

    // as render index has changed the dirty list cube0
    // should have a forced sync only.
    TF_VERIFY(dirtyList.GetDirtyRprims().size() == 2);
    TF_VERIFY(tracker.GetRprimDirtyBits(SdfPath("/Cube0")) ==
                                                    HdChangeTracker::InitRepr);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube1")) == 1);

    // simulate the render pass switching to a new collection that uses
    // the repr smoothHull
    HdRprimCollection collection2(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->smoothHull));
    renderPass->SetRprimCollection(collection2);
  
    dirtyList.UpdateRenderTagsAndReprSelectors(
        TfTokenVector(/*empty render tags*/), { collection2.GetReprSelector() });

    // the new dirty list should contain all prims.
    TF_VERIFY(dirtyList.GetDirtyRprims().size() == 2);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube0")) == 1);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube1")) == 1);

    std::cerr << "------------\n";

    // mark "Cube0" as DirtyNormals
    tracker.MarkRprimDirty(SdfPath("/Cube0"),
                           HdChangeTracker::DirtyNormals);

    // the dirty list contains just "/Cube0" as it is the only one
    // in the varying state (Cube1 is dirty, but as we never ran Sync, it
    // isn't in the varying list
    SdfPathVector const& dirtyPrims = dirtyList.GetDirtyRprims();
    std::cerr << dirtyPrims.size()
              << " : " << dirtyPrims[0] << "\n";
    TF_VERIFY(dirtyPrims.size() == 1);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube0")) == 1);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube1")) == 1);

    return true;
}

int main()
{
    TfErrorMark mark;

    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    bool success = SetRprimCollectionTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

