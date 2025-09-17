//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hdsi/debuggingSceneIndex.h"

PXR_NAMESPACE_USING_DIRECTIVE

//-----------------------------------------------------------------------------

TF_DECLARE_REF_PTRS(_BadSceneIndex);
// A scene index that wraps an HdRetainedSceneIndex but ignores all notices
// from the HdRetainedSceneIndex.
//
// Instead, clients need to explicitly call _BadSceneIndex::SendPrims[...].
//
// This allows us to create scenarios where the _BadSceneIndex is not
// sending necessary notices and thus let's us test the debugging scene index.
class _BadSceneIndex : public HdSceneIndexBase
{
public:
    static
    _BadSceneIndexRefPtr New(const std::string &name)
    {
        return TfCreateRefPtr(new _BadSceneIndex(name));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &path) const {
        return retainedSceneIndex->GetPrim(path);
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &path) const {
        return retainedSceneIndex->GetChildPrimPaths(path);
    }
        
    void
    SendPrimsAdded(const HdSceneIndexObserver::AddedPrimEntries &entries)
    {
        _SendPrimsAdded(entries);
    }

    void
    SendPrimsRemoved(const HdSceneIndexObserver::RemovedPrimEntries &entries)
    {
        _SendPrimsRemoved(entries);
    }

    void
    SendPrimsDirtied(const HdSceneIndexObserver::DirtiedPrimEntries &entries)
    {
        _SendPrimsDirtied(entries);
    }

    void
    SendPrimsRenamed(const HdSceneIndexObserver::RenamedPrimEntries &entries)
    {
        _SendPrimsRenamed(entries);
    }

    HdRetainedSceneIndexRefPtr retainedSceneIndex;

private:
    _BadSceneIndex(const std::string &name)
     : retainedSceneIndex(HdRetainedSceneIndex::New())
    {
        SetDisplayName(name);
    }
};

struct _SceneIndices
{
    _SceneIndices(const std::string &sceneIndexName)
     : badSceneIndex(_BadSceneIndex::New(sceneIndexName))
     , retainedSceneIndex(badSceneIndex->retainedSceneIndex)
     , debuggingSceneIndex(HdsiDebuggingSceneIndex::New(badSceneIndex, nullptr))
    {
    }
    
    _BadSceneIndexRefPtr const badSceneIndex;
    HdRetainedSceneIndexRefPtr const retainedSceneIndex;
    HdsiDebuggingSceneIndexRefPtr const debuggingSceneIndex;
};

static void
_TestPrimAddedWithoutNotice()
{
    _SceneIndices sceneIndices("Scene index adding prim without notice");

    sceneIndices.debuggingSceneIndex->GetChildPrimPaths(SdfPath("/"));
    sceneIndices.retainedSceneIndex->AddPrims({{SdfPath("/A/B"), TfToken("scope")}});
    sceneIndices.debuggingSceneIndex->GetChildPrimPaths(SdfPath("/"));
}

int
main(int argc, char** argv)
{
    _TestPrimAddedWithoutNotice();
}
