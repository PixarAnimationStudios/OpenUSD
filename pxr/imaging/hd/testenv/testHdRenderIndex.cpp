//
// Copyright 2021 Pixar
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

#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

#define _VERIFY_DIRTY_SIZE(pass, count) \
        { \
            HdDirtyListSharedPtr dirtyList = pass->GetDirtyList(); \
            SdfPathVector const& dirtyPaths = \
                       dirtyList->GetDirtyRprims(); \
            TF_VERIFY(dirtyPaths.size() == count, \
                        "expected %d found %zu", \
                        count,\
                        dirtyPaths.size());\
       }

static
bool
BasicTest()
{
    Hd_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Disable();

    GfMatrix4f identity;
    identity.SetIdentity();

    delegate.AddCube(SdfPath("/cube"), identity); 

    driver.Draw();

    // Performance logging is disabled, expect no tracking
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 0);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 0);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 0);
    return true;
}

// minimalistic scene delegate
class Delegate : public HdSceneDelegate {
public:
    Delegate(HdRenderIndex *renderIndex)
     : HdSceneDelegate(renderIndex, SdfPath("Delegate"))
    {

    }

    // ---------------------------------------------------------------------- //
    // See HdSceneDelegate for documentation of virtual methods.
    // ---------------------------------------------------------------------- //
    virtual bool IsInCollection(SdfPath const& id,
                                TfToken const& collectionName)
        { return true; }
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id)
        { return HdMeshTopology(); }
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id)
        { return HdBasisCurvesTopology(); }
    virtual PxOsdSubdivTags GetSubdivTags(SdfPath const& id)
        { return PxOsdSubdivTags(); }
    virtual GfRange3d GetExtent(SdfPath const & id)
        { return GfRange3d(); }
    virtual GfMatrix4d GetTransform(SdfPath const & id)
        { return GfMatrix4d(); }
    virtual bool GetVisible(SdfPath const & id)
        { return true; }
    virtual bool GetDoubleSided(SdfPath const & id)
        { return true; }
    virtual HdDisplayStyle GetDisplayStyle(SdfPath const & id) override
        { return HdDisplayStyle(); }
    virtual VtValue Get(SdfPath const& id, TfToken const& key) {
        if (key == HdTokens->points) {
            return VtValue(0.0f);
        } else {
            return VtValue();
        }
    }
    virtual
    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(SdfPath const& id,
                          HdInterpolation interpolation) override
    {
        HdPrimvarDescriptorVector result;
        if (interpolation == HdInterpolationVertex) {
            result.emplace_back(HdTokens->points, interpolation,
                                HdPrimvarRoleTokens->point);
        }
        return result;
    }
    virtual VtIntArray GetInstanceIndices(SdfPath const& instancerId,
                                          SdfPath const& prototypeId)
        { return VtIntArray(); }
    virtual GfMatrix4d GetInstancerTransform(SdfPath const& instancerId,
                                             SdfPath const& prototypeId)
        { return GfMatrix4d(); }

private:
    Delegate()                             = delete;
    Delegate(const Delegate &)             = delete;
    Delegate &operator =(const Delegate &) = delete;
};

// Simple task focus only on sync.
class TestTask final : public HdTask
{
public:
    TestTask(HdRenderPassSharedPtr const &renderPass)
    : HdTask(SdfPath::EmptyPath())
    , _renderPass(renderPass)
    {
    }

    virtual void Sync(HdSceneDelegate*,
                      HdTaskContext*,
                      HdDirtyBits*) override
    {
        _renderPass->Sync();
    }

    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override
    {
    }

    virtual void Execute(HdTaskContext* ctx) override
    {
    }

private:
    HdRenderPassSharedPtr _renderPass;
};

static bool
SyncTest()
{
    HdRprimCollection collection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));

    Hd_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex &renderIndex = delegate.GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    HdRenderPassSharedPtr renderPass(
        new Hd_UnitTestNullRenderPass(&renderIndex, collection));

    HdTaskSharedPtrVector tasks;
    HdTaskContext         taskContext;
    tasks.push_back(std::make_shared<TestTask>(renderPass));

    SdfPath id("/prim");

    HdRprimCollection col(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    HdRenderPassSharedPtr renderPass0(
        new Hd_UnitTestNullRenderPass(&renderIndex, col));


    SdfPathVector primList;
    primList.push_back(SdfPath("/A/a0"));
    primList.push_back(SdfPath("/A/a1"));
    primList.push_back(SdfPath("/B/b0"));
    primList.push_back(SdfPath("/B/b1"));
    primList.push_back(SdfPath("/C/c0"));
    primList.push_back(SdfPath("/C/c1"));
    primList.push_back(SdfPath("/E/e0"));
    primList.push_back(SdfPath("/E/e1"));

    // insert
    _VERIFY_DIRTY_SIZE(renderPass, 0);
    TF_FOR_ALL(it, primList) delegate.AddMesh(*it);
    _VERIFY_DIRTY_SIZE(renderPass, 8);

    // ------- sync /A --------
    SdfPathVector rootA;
    rootA.push_back(SdfPath("/A"));
    collection.SetRootPaths(rootA);
    renderPass->SetRprimCollection(collection);

    renderIndex.SyncAll(&tasks, &taskContext);

    // Render pass has been filtered to /A and we just cleaned it.
    _VERIFY_DIRTY_SIZE(renderPass, 0);

    // invalidate all
    changeTracker.ResetVaryingState();
    TF_FOR_ALL(it, primList) changeTracker.MarkRprimDirty(*it);

    // ------- sync /A and /B --------
    SdfPathVector rootAB;
    rootAB.push_back(SdfPath("/A"));
    rootAB.push_back(SdfPath("/B"));
    collection.SetRootPaths(rootAB);
    renderPass->SetRprimCollection(collection);
    renderIndex.SyncAll(&tasks, &taskContext);

    // Ok, we expect the list to be clean now.
    _VERIFY_DIRTY_SIZE(renderPass, 0);

    // invalidate all
    changeTracker.ResetVaryingState();
    TF_FOR_ALL(it, primList) changeTracker.MarkRprimDirty(*it);

    // ------- sync /B, /D, /E and /F, random order --------
    SdfPathVector rootBD;
    rootBD.push_back(SdfPath("/D"));  // not exists in middle
    rootBD.push_back(SdfPath("/B"));  // not first
    rootBD.push_back(SdfPath("/F"));  // not exists at last
    rootBD.push_back(SdfPath("/E"));
    collection.SetRootPaths(rootBD);
    renderPass->SetRprimCollection(collection);
    renderIndex.SyncAll(&tasks, &taskContext);

    // /A, /C remain
    _VERIFY_DIRTY_SIZE(renderPass, 0);

    // ---------------------------------------------------------------------- //
    // ApplyEdit Transition tests
    // ---------------------------------------------------------------------- //

    // invalidate all
    changeTracker.ResetVaryingState();
    TF_FOR_ALL(it, primList) changeTracker.MarkRprimDirty(*it);

    SdfPathVector roots;
    roots.push_back(SdfPath("/"));
    collection.SetRootPaths(roots);
    renderPass->SetRprimCollection(collection);
    _VERIFY_DIRTY_SIZE(renderPass, 8);

    // Transition from root </> to </A>, should still have 8 elements
    // as it includes all prims
    roots.clear();
    roots.push_back(SdfPath("/A"));
    collection.SetRootPaths(roots);
    renderPass->SetRprimCollection(collection);

    // As this doesn't effect the scene state, we expect no change
    _VERIFY_DIRTY_SIZE(renderPass, 0);

    // --

    // invalidate all
    changeTracker.ResetVaryingState();
    TF_FOR_ALL(it, primList) changeTracker.MarkRprimDirty(*it);

    roots.clear();
    roots.push_back(SdfPath("/A"));
    collection.SetRootPaths(roots);
    renderPass->SetRprimCollection(collection);
    _VERIFY_DIRTY_SIZE(renderPass, 8);
    // Transition from root </A> to </>, should still have 8 elements
    roots.clear();
    roots.push_back(SdfPath("/"));
    collection.SetRootPaths(roots);
    renderPass->SetRprimCollection(collection);

    // As this doesn't effect the scene state, we expect no change
    _VERIFY_DIRTY_SIZE(renderPass, 0);

    return true;
}

int main()
{
    TfErrorMark mark;
    bool success = BasicTest()
                   && SyncTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

