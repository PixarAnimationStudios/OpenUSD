//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/legacyPrimSceneIndex.h"
#include "pxr/imaging/hd/dataSourceLegacyPrim.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

void
HdLegacyPrimSceneIndex::AddLegacyPrim(SdfPath const &id, TfToken const &type,
                                      HdSceneDelegate *sceneDelegate)
{
    AddPrims({{id, type,
        HdDataSourceLegacyPrim::New(id, type, sceneDelegate)}});
}

void
HdLegacyPrimSceneIndex::RemovePrim(SdfPath const &id)
{
    if (!GetChildPrimPaths(id).empty()) {
        AddPrims({{id, TfToken(), nullptr}});
    }
    else {
        RemovePrims({id});
    }
}

void
HdLegacyPrimSceneIndex::DirtyPrims(
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (auto const &entry : entries) {
        if (!entry.dirtyLocators.Intersects(
                HdDataSourceLegacyPrim::GetCachedLocators())) {
            // If none of the locators are cached by the datasource,
            // PrimDirtied will be a no-op so we can skip the map lookup...
            continue;
        }
        HdDataSourceLegacyPrimHandle legacyDs =
            HdDataSourceLegacyPrim::Cast(GetPrim(entry.primPath).dataSource);
        if (legacyDs) {
            legacyDs->PrimDirtied(entry.dirtyLocators);
        }
    }
    HdRetainedSceneIndex::DirtyPrims(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
