//
// Copyright 2024 Pixar
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
#include "generativeProceduralFilteringSceneIndex.h"
#include "generativeProceduralPluginRegistry.h"

#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

PXR_NAMESPACE_OPEN_SCOPE

HdGpGenerativeProceduralFilteringSceneIndex::
        HdGpGenerativeProceduralFilteringSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const TfTokenVector &allowedProceduralTypes)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _allowedProceduralTypes(allowedProceduralTypes)
, _targetPrimTypeName(HdGpGenerativeProceduralTokens->generativeProcedural)
{
}

HdGpGenerativeProceduralFilteringSceneIndex::
        HdGpGenerativeProceduralFilteringSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const TfTokenVector &allowedProceduralTypes,
    const TfToken &targetPrimTypeName)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _allowedProceduralTypes(allowedProceduralTypes)
, _targetPrimTypeName(targetPrimTypeName)
{
}

HdSceneIndexPrim
HdGpGenerativeProceduralFilteringSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (_ShouldSkipPrim(prim)) {
        prim.primType = HdGpGenerativeProceduralTokens->skippedGenerativeProcedural;
    }
    return prim;
}

SdfPathVector
HdGpGenerativeProceduralFilteringSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdGpGenerativeProceduralFilteringSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    // Fast path: no procedurals to consider
    bool foundAnyProcedurals = false;
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        if (entry.primType == _targetPrimTypeName) {
            foundAnyProcedurals = true;
            break;
        }
    }
    if (!foundAnyProcedurals) {
        _SendPrimsAdded(entries);
        return;
    }

    // Apply filtering
    HdSceneIndexObserver::AddedPrimEntries filteredEntries = entries;
    for (HdSceneIndexObserver::AddedPrimEntry &entry : filteredEntries) {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(entry.primPath);
        if (_ShouldSkipPrim(prim)) {
            entry.primType = HdGpGenerativeProceduralTokens->skippedGenerativeProcedural;
        }
    }
    _SendPrimsAdded(filteredEntries);
}

void
HdGpGenerativeProceduralFilteringSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdGpGenerativeProceduralFilteringSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

TfToken
HdGpGenerativeProceduralFilteringSceneIndex::_GetProceduralType(
    HdSceneIndexPrim const& prim) const
{
    HdPrimvarsSchema primvars =
        HdPrimvarsSchema::GetFromParent(prim.dataSource);
    if (const HdSampledDataSourceHandle procTypeDs = primvars.GetPrimvar(
        HdGpGenerativeProceduralTokens->proceduralType).GetPrimvarValue()) {
        const VtValue v = procTypeDs->GetValue(0.0f);
        if (v.IsHolding<TfToken>()) {
            return v.UncheckedGet<TfToken>();
        }
    }
    return TfToken();
}

bool
HdGpGenerativeProceduralFilteringSceneIndex::_ShouldSkipPrim(
    HdSceneIndexPrim const& prim) const
{
    if (prim.primType == _targetPrimTypeName) {
        const TfToken procType = _GetProceduralType(prim);
        for (TfToken const& allowedType: _allowedProceduralTypes) {
            if (allowedType == HdGpGenerativeProceduralTokens->anyProceduralType ||
                allowedType == procType) {
                // Allow this procedural; do not skip
                return false;
            }
        }
        // Skip it
        return true;
    } else {
        // Not a target procedural type
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
