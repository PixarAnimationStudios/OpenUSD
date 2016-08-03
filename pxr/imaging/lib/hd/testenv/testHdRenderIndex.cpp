#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

#define _VERIFY_DIRTY_SIZE(pass, count) \
        TF_VERIFY(pass->GetDirtyList()->GetSize() == count, \
                    "expected %d found %zu", \
                    count,\
                    pass->GetDirtyList()->GetSize());

static
bool
BasicTest()
{
    Hd_TestDriver driver;
    Hd_UnitTestDelegate &delegate = driver.GetDelegate();
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

static
bool
ChangePointsAndTopoTest()
{
    Hd_TestDriver driver;
    Hd_UnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex& index = delegate.GetRenderIndex();
    HdChangeTracker& tracker = index.GetChangeTracker();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    GfMatrix4f identity;
    identity.SetIdentity();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->collectionsRefreshed) == 0);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 1);
    SdfPath id("/cube");
    delegate.AddCube(id, identity);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 2);

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 0);
    // Baseline sanity check, expect no cache misses.
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 0);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 0);

    // ---------------------------------------------------------------------- //
    // DRAW 1
    // ---------------------------------------------------------------------- //
    // Upon first draw, expect a cache miss.
    driver.Draw();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->collectionsRefreshed) == 1);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->totalItemCount) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    // Mark points and topology as dirty, expect cache misses to increment.
    delegate.MarkRprimDirty(id, HdChangeTracker::DirtyPoints);
    delegate.MarkRprimDirty(id, HdChangeTracker::DirtyTopology);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 2);
    // ---------------------------------------------------------------------- //
    // DRAW 2
    // ---------------------------------------------------------------------- //
    driver.Draw();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->collectionsRefreshed) == 1);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 2);
    // note that HD_ENABLE_SMOOTH_NORMALS is set to 0 to pass this test.
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 2);
    TF_VERIFY(perfLog.GetCounter(HdTokens->totalItemCount) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);
    
    // ---------------------------------------------------------------------- //
    // DRAW 3
    // ---------------------------------------------------------------------- //
    // We expect all data for this draw call to be cache hits.
    driver.Draw();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->collectionsRefreshed) == 1);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 2);
    TF_VERIFY(perfLog.GetCounter(HdTokens->totalItemCount) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    // Add a second cube behind the camera
    // For some insane reason, there is no SetTranslate, or SetRotate call for GfMatrix4f
    // even though the documentation at the top of matrix4f refers to them.
    GfMatrix4f trans = GfMatrix4f().SetIdentity();
    trans.SetRow(3, GfVec4f(0.0f, -5000.0f, 0.0f, 1.0f));
    delegate.AddCube(SdfPath("/Cube2"), trans);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 3);
    
    // ---------------------------------------------------------------------- //
    // DRAW 4
    // ---------------------------------------------------------------------- //
    // Expect that second cube is culled by frustum culling
    // Note that GPU frustum culling has to be disabled for this test.
    driver.Draw();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->collectionsRefreshed) == 2);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 3);
    TF_VERIFY(perfLog.GetCounter(HdTokens->totalItemCount) == 2);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    // Mark collection dirty, expect collections to refresh.
    tracker.MarkCollectionDirty(HdTokens->geometry);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 4);
    // ---------------------------------------------------------------------- //
    // DRAW 5
    // ---------------------------------------------------------------------- //
    driver.Draw();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->collectionsRefreshed) == 3);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 4);
    TF_VERIFY(perfLog.GetCounter(HdTokens->totalItemCount) == 2);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    // Mark ALL collections dirty, expect collections to refresh.
    tracker.MarkAllCollectionsDirty();
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 5);
    // ---------------------------------------------------------------------- //
    // DRAW 6
    // ---------------------------------------------------------------------- //
    driver.Draw();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->collectionsRefreshed) == 4);
    TF_VERIFY(tracker.GetCollectionVersion(HdTokens->geometry) == 5);
    TF_VERIFY(perfLog.GetCounter(HdTokens->totalItemCount) == 2);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    return true;
}

// minimalistic delegate
class Delegate : public HdSceneDelegate {
public:
    Delegate() { }
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
    virtual GfVec4f GetColorAndOpacity(SdfPath const & id)
        { return GfVec4f(1); }
    virtual bool GetDoubleSided(SdfPath const & id)
        { return true; }
    virtual int GetRefineLevel(SdfPath const & id)
        { return 0; }
    virtual VtValue Get(SdfPath const& id, TfToken const& key) {
        if (key == HdTokens->points) {
            return VtValue(0.0f);
        } else {
            return VtValue();
        }
    }
    virtual TfTokenVector GetPrimVarVertexNames(SdfPath const& id) {
        TfTokenVector tokens;
        tokens.push_back(HdTokens->points);
        return tokens;
    }
    virtual TfTokenVector GetPrimVarVaryingNames(SdfPath const& id)
        { return TfTokenVector(); }
    virtual TfTokenVector GetPrimVarFacevaryingNames(SdfPath const& id)
        { return TfTokenVector(); }
    virtual TfTokenVector GetPrimVarUniformNames(SdfPath const& id)
        { return TfTokenVector(); }
    virtual TfTokenVector GetPrimVarConstantNames(SdfPath const& id)
        { return TfTokenVector(); }
    virtual TfTokenVector GetPrimVarInstanceNames(SdfPath const& id)
        { return TfTokenVector(); }
    virtual int GetPrimVarDataType(SdfPath const& id, TfToken const& key)
        { return 0; }
    virtual int GetPrimVarComponents(SdfPath const& id, TfToken const& key)
        { return 0; }

    virtual VtIntArray GetInstanceIndices(SdfPath const& instancerId,
                                          SdfPath const& prototypeId)
        { return VtIntArray(); }
    virtual GfMatrix4d GetInstancerTransform(SdfPath const& instancerId,
                                             SdfPath const& prototypeId)
        { return GfMatrix4d(); }
};

static bool
SyncTest()
{
    HdRprimCollection collection(HdTokens->geometry, HdTokens->hull);

    Hd_UnitTestDelegate *delegate = new Hd_UnitTestDelegate;
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    HdRenderPassSharedPtr renderPass(new HdRenderPass(&renderIndex, collection));

    SdfPath id("/prim");

    HdRprimCollection col(HdTokens->geometry, HdTokens->hull);
    HdRenderPassSharedPtr renderPass0 = 
        HdRenderPassSharedPtr(new HdRenderPass(&renderIndex, col));


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
    TF_FOR_ALL(it, primList) delegate->AddMesh(*it);
    _VERIFY_DIRTY_SIZE(renderPass, 8);

    // ------- sync /A --------
    SdfPathVector rootA;
    rootA.push_back(SdfPath("/A"));
    collection.SetRootPaths(rootA);
    renderPass->SetRprimCollection(collection);
    renderIndex.Sync(renderPass->GetDirtyList());
    renderIndex.SyncAll();

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
    renderIndex.Sync(renderPass->GetDirtyList());
    renderIndex.SyncAll();

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
    renderIndex.Sync(renderPass->GetDirtyList());
    renderIndex.SyncAll();

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
    // Transition from root </> to </A>, should be left with 2 elements
    roots.clear();
    roots.push_back(SdfPath("/A"));
    collection.SetRootPaths(roots);
    renderPass->SetRprimCollection(collection);
    _VERIFY_DIRTY_SIZE(renderPass, 2);

    // --

    // invalidate all
    changeTracker.ResetVaryingState();
    TF_FOR_ALL(it, primList) changeTracker.MarkRprimDirty(*it);

    roots.clear();
    roots.push_back(SdfPath("/A"));
    collection.SetRootPaths(roots);
    renderPass->SetRprimCollection(collection);
    _VERIFY_DIRTY_SIZE(renderPass, 2);
    // Transition from root </A> to </>, should be left with 2 elements
    roots.clear();
    roots.push_back(SdfPath("/"));
    collection.SetRootPaths(roots);
    renderPass->SetRprimCollection(collection);
    _VERIFY_DIRTY_SIZE(renderPass, 8);

    return true;
}

int main()
{
    TfErrorMark mark;
    bool success = BasicTest()
               and ChangePointsAndTopoTest()
               and SyncTest();

    TF_VERIFY(mark.IsClean());

    if (success and mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

