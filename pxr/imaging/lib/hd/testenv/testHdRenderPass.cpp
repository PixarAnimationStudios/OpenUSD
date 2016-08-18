#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

static bool
SetRprimCollectionTest()
{
    Hd_TestDriver driver;
    Hd_UnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex& index = delegate.GetRenderIndex();
    HdChangeTracker& tracker = index.GetChangeTracker();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    HdRprimCollection collection(HdTokens->geometry, HdTokens->hull);
    HdRenderPassSharedPtr renderPass(new HdRenderPass(&index, collection));
    HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());

    // ---------------------------------------------------------------------- //
    // Test RenderPass hash dependency on custom buffers
    // ---------------------------------------------------------------------- //
    HdResourceRegistry* resourceRegistry = &HdResourceRegistry::GetInstance();
    HdRenderPassShaderSharedPtr renderPassShader
        = renderPassState->GetRenderPassShader();
    HdBufferSpecVector offsetSpecs;
    offsetSpecs.push_back(HdBufferSpec(TfToken("offset"), GL_INT, 1));
    HdBufferArrayRangeSharedPtr bar = resourceRegistry->AllocateSingleBufferArrayRange(
                                        /*role*/TfToken("selection"),
                                        offsetSpecs);

    HdRenderPassShader::ID emptyId = renderPassShader->ComputeHash();
    renderPassShader->AddBufferBinding(HdBindingRequest(HdBinding::UBO,
                                    TfToken("uniforms"), bar,
                                    /*interleave*/true));
    std::cout << "empty: " << emptyId << std::endl;
    std::cout << "offset: " << renderPassShader->ComputeHash() << std::endl;

    TF_VERIFY(emptyId != renderPassShader->ComputeHash());
    HdRenderPassShader::ID uniformsId = renderPassShader->ComputeHash();
    
    renderPassShader->ClearBufferBindings();
    std::cout << "empty: " << renderPassShader->ComputeHash() << std::endl;
    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());

    renderPassShader->AddBufferBinding(HdBindingRequest(HdBinding::SSBO,
                                    TfToken("differentName"), bar,
                                    /*interleave*/true));
    // 
    // Make sure that changing internal values produces a different hash.
    //
    std::cout << "different: " << renderPassShader->ComputeHash()<< std::endl;
    TF_VERIFY(emptyId != renderPassShader->ComputeHash());
    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());
    HdRenderPassShader::ID differentId = renderPassShader->ComputeHash();
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
    renderPassShader->AddBufferBinding(HdBindingRequest(HdBinding::SSBO,
                                    TfToken("differentName"), bar,
                                    /*interleave*/true));
    renderPassShader->AddBufferBinding(HdBindingRequest(HdBinding::UBO,
                                    TfToken("uniforms"), bar,
                                    /*interleave*/true));
    HdRenderPassShader::ID multiId = renderPassShader->ComputeHash();
    std::cout << "multi: " << renderPassShader->ComputeHash()<< std::endl;
    TF_VERIFY(uniformsId != renderPassShader->ComputeHash());
    TF_VERIFY(differentId != renderPassShader->ComputeHash());
    TF_VERIFY(emptyId != renderPassShader->ComputeHash());
    TF_VERIFY(multiId == renderPassShader->ComputeHash());
    //
    // Try a couple buffers, shuffle params.
    //
    renderPassShader->AddBufferBinding(HdBindingRequest(HdBinding::UBO,
                                    TfToken("differentName"), bar,
                                    /*interleave*/true));
    renderPassShader->AddBufferBinding(HdBindingRequest(HdBinding::UBO,
                                    TfToken("uniforms"), bar,
                                    /*interleave*/false));
    HdRenderPassShader::ID multiShuffId = renderPassShader->ComputeHash();
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

    // create a dirtylist for this render pass
    HdDirtyListSharedPtr dirtyList = renderPass->GetDirtyList();

    // the dirty list has "/Cube0"
    TF_VERIFY(dirtyList->GetSize() == 1);

    // clean "/Cube0"
    tracker.MarkRprimClean(SdfPath("/Cube0"));

    std::cerr << "!! : ";
    HdChangeTracker::DumpDirtyBits(tracker.GetRprimDirtyBits(SdfPath("/Cube0")));


    // add "/Cube1"
    delegate.AddCube(SdfPath("/Cube1"), identity);

    // the dirty list has "/Cube1"
    TF_VERIFY(dirtyList->GetSize() == 1);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube0")) == 0);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube1")) == 1);

    // switch to the new collection, smoothHull
    HdRprimCollection collection2(HdTokens->geometry, HdTokens->smoothHull);
    renderPass->SetRprimCollection(collection2);

    // create new dirtylist for this render pass
    dirtyList = renderPass->GetDirtyList();

    // the new dirty list contains all prims.
    TF_VERIFY(dirtyList->GetSize() == 2);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube0")) == 1);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube1")) == 1);

    std::cerr << "------------\n";

    // mark "Cube0" as DirtyNormals
    tracker.MarkRprimDirty(SdfPath("/Cube0"),
                           HdChangeTracker::DirtyNormals);

    // the dirty list contains "/Cube0" and "/Cube1"
    std::cerr << dirtyList->GetSize() << " : " << dirtyList->GetDirtyRprims()[0] << "\n";
    TF_VERIFY(dirtyList->GetSize() == 2);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube0")) == 1);
    TF_VERIFY(tracker.IsRprimDirty(SdfPath("/Cube1")) == 1);

    return true;
}

int main()
{
    TfErrorMark mark;
    bool success = SetRprimCollectionTest();

    TF_VERIFY(mark.IsClean());

    if (success and mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

