//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "generativeProceduralFilteringSceneIndex.h"
#include "generativeProceduralPluginRegistry.h"

#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

static bool
_ComputeAllowAny(const std::set<TfToken>& allowedProceduralTypes)
{
    return allowedProceduralTypes.count(
               HdGpGenerativeProceduralTokens->anyProceduralType)
        > 0;
}

HdGpGenerativeProceduralFilteringSceneIndex::
        HdGpGenerativeProceduralFilteringSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const TfTokenVector &allowedProceduralTypes)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _allowedProceduralTypes(allowedProceduralTypes.begin(), allowedProceduralTypes.end())
, _allowAny(_ComputeAllowAny(_allowedProceduralTypes))
, _targetPrimTypeName(HdGpGenerativeProceduralTokens->generativeProcedural)
, _allowedPrimTypeName(_targetPrimTypeName)
, _skippedPrimTypeName(HdGpGenerativeProceduralTokens->skippedGenerativeProcedural)
{
}

HdGpGenerativeProceduralFilteringSceneIndex::
    HdGpGenerativeProceduralFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const TfTokenVector &allowedProceduralTypes,
        const std::optional<TfToken> &maybeTargetPrimTypeName,
        const std::optional<TfToken> &maybeAllowedPrimTypeName,
        const std::optional<TfToken> &maybeSkippedPrimTypeName)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _allowedProceduralTypes(
        allowedProceduralTypes.begin(), allowedProceduralTypes.end())
, _allowAny(_ComputeAllowAny(_allowedProceduralTypes))
, _targetPrimTypeName(
    maybeTargetPrimTypeName
        ? *maybeTargetPrimTypeName
        : HdGpGenerativeProceduralTokens->generativeProcedural)
, _allowedPrimTypeName(
    maybeAllowedPrimTypeName
        ? *maybeAllowedPrimTypeName
        : _targetPrimTypeName)
, _skippedPrimTypeName(
    maybeSkippedPrimTypeName
        ? *maybeSkippedPrimTypeName
        : HdGpGenerativeProceduralTokens->skippedGenerativeProcedural)
{
}

HdSceneIndexPrim
HdGpGenerativeProceduralFilteringSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (auto maybePrimType = _GetOverridePrimTypeForPrim(prim)) {
        prim.primType = maybePrimType.value();
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

    TRACE_FUNCTION();

    // Apply filtering
    HdSceneIndexObserver::AddedPrimEntries filteredEntries = entries;
    for (HdSceneIndexObserver::AddedPrimEntry &entry : filteredEntries) {
        if (entry.primType != _targetPrimTypeName) {
            continue;
        }
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(entry.primPath);
        switch (_ShouldSkipPrim(prim)) {
            case _ShouldSkipResult::Ignore: {
                // leave it alone
            } break;
            case _ShouldSkipResult::Skip: {
                entry.primType = _skippedPrimTypeName;
            } break;
            case _ShouldSkipResult::Allow: {
                entry.primType = _allowedPrimTypeName;
            } break;
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

HdGpGenerativeProceduralFilteringSceneIndex::_ShouldSkipResult
HdGpGenerativeProceduralFilteringSceneIndex::_ShouldSkipPrim(
    HdSceneIndexPrim const& prim) const
{
    if (prim.primType != _targetPrimTypeName) {
        // Not a target procedural type
        return _ShouldSkipResult::Ignore;
    }

    if (_allowAny) {
        return _ShouldSkipResult::Allow;
    }

    const TfToken procType = _GetProceduralType(prim);
    return _allowedProceduralTypes.count(procType) > 0
        ? _ShouldSkipResult::Allow
        : _ShouldSkipResult::Skip;
}

std::optional<TfToken>
HdGpGenerativeProceduralFilteringSceneIndex::_GetOverridePrimTypeForPrim(
    HdSceneIndexPrim const& prim) const
{
    switch (_ShouldSkipPrim(prim)) {
    case _ShouldSkipResult::Ignore:
        // leave it alone
        return std::nullopt;
    case _ShouldSkipResult::Skip:
        return _skippedPrimTypeName;
    case _ShouldSkipResult::Allow:
        return _allowedPrimTypeName;
    }

    return std::nullopt;
}

void
HdGpGenerativeProceduralFilteringSceneIndex::SetAllowedProceduralTypes(
    const TfTokenVector &allowedProceduralTypesList)
{
    const std::set<TfToken> allowedProceduralTypes(
        allowedProceduralTypesList.begin(),
        allowedProceduralTypesList
            .end());

    if (allowedProceduralTypes == _allowedProceduralTypes)
    {
        // early out if these match.
        return;
    }

    _allowedProceduralTypes = allowedProceduralTypes;
    _allowAny = _ComputeAllowAny(_allowedProceduralTypes);

    auto inputSi = _GetInputSceneIndex();
    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    for (const SdfPath& primPath : HdSceneIndexPrimView(inputSi)) {
        if (auto maybePrimType
            = _GetOverridePrimTypeForPrim(inputSi->GetPrim(primPath))) {
            addedEntries.emplace_back(primPath, maybePrimType.value());
        }
    }

    if (!addedEntries.empty()) {
        _SendPrimsAdded(addedEntries);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
