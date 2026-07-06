//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/piPrototypePropagatingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

class PrimListener : public HdSceneIndexObserver
{
public:
    void PrimsAdded(
        const HdSceneIndexBase& sender,
        const AddedPrimEntries& entries) override
    {
        for (const AddedPrimEntry& entry : entries) {
            _prims.insert(entry.primPath);
        }
        _added.insert(_added.end(), entries.cbegin(), entries.cend());
    }

    void PrimsRemoved(
        const HdSceneIndexBase& sender,
        const RemovedPrimEntries& entries) override
    {
        for (const RemovedPrimEntry& entry : entries) {
            for (SdfPathSet::iterator it = _prims.begin();
                 it != _prims.end();) {
                if (it->HasPrefix(entry.primPath)) {
                    it = _prims.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }

    void PrimsDirtied(
        const HdSceneIndexBase& sender,
        const DirtiedPrimEntries& entries) override
    {
        _dirtied.insert(_dirtied.end(), entries.cbegin(), entries.cend());
    }

    void PrimsRenamed(
        const HdSceneIndexBase& sender,
        const RenamedPrimEntries& entries) override
    {
        ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
    }

    SdfPathSet const& GetPrimPaths()
    {
        return _prims;
    }
    AddedPrimEntries const& GetAdded()
    {
        return _added;
    }
    DirtiedPrimEntries const& GetDirtied()
    {
        return _dirtied;
    }

    void ResetEntries()
    {
        _added.clear();
        _dirtied.clear();
    }

private:
    SdfPathSet _prims;
    AddedPrimEntries _added;
    DirtiedPrimEntries _dirtied;
};

static UsdStageRefPtr
_StageFromUsda(const std::string& usda)
{
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");
    TF_VERIFY(rootLayer->ImportFromString(usda));
    return UsdStage::Open(rootLayer);
}

static void
ChangeSpecifierTest()
{
    // This is a test that asserts some assumptions that the
    // UsdImaging_PiPrototypeSceneIndex is making.
    //
    // Specifically, after a prim is no longer a prototype (i.e. if an "over"
    // becomes a "def" or a prim changes type from PointInstancer to Scope),
    // the UsdImaging_PiPrototypeSceneIndex is assuming the upstream scene
    // index will send added entries for the children.  
    UsdStageRefPtr stage = _StageFromUsda(R"(#usda 1.0
over "Over"
{
    def Cube "Prototype0"
    {
    }
}

def PointInstancer "PointInstancer"
{
    def Cube "Prototype1"
    {
    }
}

)");

    if (!TF_VERIFY(stage)) {
        return;
    }

    UsdImagingStageSceneIndexRefPtr inputSi = UsdImagingStageSceneIndex::New();
    if (!TF_VERIFY(inputSi)) {
        return;
    }

    auto si = UsdImagingPiPrototypePropagatingSceneIndex::New(inputSi);
    PrimListener primListener;
    si->AddObserver(HdSceneIndexObserverPtr(&primListener));

    primListener.ResetEntries();
    inputSi->SetStage(stage);

    auto _GetPrimTypeForAddedEntry
        = [](const HdSceneIndexObserver::AddedPrimEntries& entries,
             const SdfPath& primPath) -> std::optional<TfToken> {
        for (const auto& entry : entries) {
            if (entry.primPath == primPath) {
                return entry.primType;
            }
        }
        return std::nullopt;
    };

    const SdfPath proto0("/Over/Prototype0");

    // since `/Over` is an over, if we encounter `Prototype0`, it should *not*
    // have primType==mesh.  For now, we assert that it is empty (or not added
    // at all).
    {
        TfToken origProto0Type
            = _GetPrimTypeForAddedEntry(primListener.GetAdded(), proto0)
                  .value_or(TfToken());
        TF_VERIFY(origProto0Type.IsEmpty());
    }
    primListener.ResetEntries();

    UsdPrim overPrim = stage->GetPrimAtPath(SdfPath("/Over"));
    TF_VERIFY(overPrim);
    overPrim.SetSpecifier(SdfSpecifierDef);
    inputSi->ApplyPendingUpdates();
    
    {
        auto maybeProto0Type = _GetPrimTypeForAddedEntry(primListener.GetAdded(), proto0);
        // Make sure we got an added entry for our prototype
        TF_VERIFY(maybeProto0Type.has_value());
        // Make sure it was some non-empty type
        TF_VERIFY(!maybeProto0Type.value().IsEmpty());

        TF_VERIFY(!si->GetPrim(proto0).primType.IsEmpty());
    }
    primListener.ResetEntries();

    // Same test as above, but here, we're changing the PointInstancer to being
    // just a normal Scope
    const SdfPath proto1("/PointInstancer/Prototype1");

    UsdPrim piPrim = stage->GetPrimAtPath(SdfPath("/PointInstancer"));
    TF_VERIFY(piPrim);
    piPrim.SetTypeName(TfToken("Scope"));
    inputSi->ApplyPendingUpdates();
    
    {
        auto maybeProto1Type = _GetPrimTypeForAddedEntry(primListener.GetAdded(), proto1);
        // Make sure we got an added entry for our prototype
        TF_VERIFY(maybeProto1Type.has_value());
        // Make sure it was some non-empty type
        TF_VERIFY(!maybeProto1Type.value().IsEmpty());

        TF_VERIFY(!si->GetPrim(proto1).primType.IsEmpty());
    }
}

int
main()
{
    ChangeSpecifierTest();

    return 0;
}
