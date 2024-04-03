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
