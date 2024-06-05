//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/expressionVariables.h"

#include "pxr/usd/pcp/cache.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

static
VtDictionary
Pcp_ComposeExpressionVariablesOver(
    const PcpLayerStackIdentifier& identifier,
    const VtDictionary* expressionVariableOverrides)
{
    VtDictionary expressionVars;

    VtDictionaryOver(
        identifier.rootLayer->template GetFieldAs<VtDictionary>(
            SdfPath::AbsoluteRootPath(), SdfFieldKeys->ExpressionVariables),
        &expressionVars);

    if (identifier.sessionLayer) {
        VtDictionaryOver(
            identifier.sessionLayer->template GetFieldAs<VtDictionary>(
                SdfPath::AbsoluteRootPath(), SdfFieldKeys->ExpressionVariables),
            &expressionVars);
    }

    if (expressionVariableOverrides) {
        VtDictionaryOver(*expressionVariableOverrides, &expressionVars);
    }

    return expressionVars;
}

namespace
{

struct NoCache
{
public:
    PcpExpressionVariables* GetEntry(const PcpLayerStackIdentifier& id)
    { return nullptr; }

    PcpExpressionVariables* CacheEntry(
        const PcpLayerStackIdentifier& id, const PcpExpressionVariables&)
    {
        // This function gets called when we compute a PcpExpressionVariables
        // object that hasn't changed from the previous computation.
        // We can just return a pointer to our current result.
        return &result;
    }

    PcpExpressionVariables* CacheEntry(
        const PcpLayerStackIdentifier& id, PcpExpressionVariables&& s)
    {
        result = std::move(s);
        return &result;
    }

    PcpExpressionVariables result;
};

struct Cache
{
public:
    template <class Map> 
    Cache(Map* cache) : _cache(cache) { }

    PcpExpressionVariables* GetEntry(const PcpLayerStackIdentifier& id)
    { return TfMapLookupPtr(*_cache, id); }

    template <class ExpressionVarsAndSource>
    PcpExpressionVariables* CacheEntry(
        const PcpLayerStackIdentifier& id, 
        ExpressionVarsAndSource&& expressionVarsAndSource)
    {
        auto mapResult = _cache->emplace(
            id, std::forward<decltype(expressionVarsAndSource)>(
                expressionVarsAndSource));
        TF_VERIFY(mapResult.second);
        return &mapResult.first->second;
    }

private:
    std::unordered_map<PcpLayerStackIdentifier, PcpExpressionVariables, TfHash>*
        _cache;
};


} // end anonymous namespace

template <class CachePolicy>
static
const PcpExpressionVariables*
Pcp_ComposeExpressionVariables(
    const PcpLayerStackIdentifier& sourceLayerStackId,
    const PcpLayerStackIdentifier& rootLayerStackId,
    CachePolicy* cache)
{
    PcpExpressionVariables localExpressionVars;
    const PcpExpressionVariables* expressionVars = &localExpressionVars;

    // Traverse the variable override sources to collect the expression variable
    // sources from weakest to strongest.
    std::vector<PcpLayerStackIdentifier> sources;
    for (const PcpLayerStackIdentifier* currId = &sourceLayerStackId;
         sources.empty() || sources.back() != rootLayerStackId;
         currId = &currId->expressionVariablesOverrideSource
             .ResolveLayerStackIdentifier(rootLayerStackId)) {

        // If we have a cached entry for an override source, we can start
        // composing from this point.
        if (const PcpExpressionVariables* entry = cache->GetEntry(*currId)) {
            expressionVars = entry;
            break;
        }
        sources.push_back(*currId);
    }

    // Traverse the expression variable sources from strongest to weakest,
    // composing the variables from each source over the variables from
    // the next source.
    for (size_t i = sources.size(); i-- > 0; ) {
        VtDictionary overriddenVars = Pcp_ComposeExpressionVariablesOver(
            sources[i], &expressionVars->GetVariables());
        
        // If the composed expression vars up to this source match those from
        // the previous source, we use the previously-computed expression
        // variables and source. Otherwise, create a new PcpExpressionVariables
        // object.
        if (overriddenVars == expressionVars->GetVariables()) {
            expressionVars = cache->CacheEntry(sources[i], *expressionVars);
        }
        else {
            PcpExpressionVariables newExpressionVars(
                PcpExpressionVariablesSource(sources[i], rootLayerStackId),
                std::move(overriddenVars));

            expressionVars = cache->CacheEntry(
                sources[i], std::move(newExpressionVars));
        }
    }
    
    TF_VERIFY(expressionVars != &localExpressionVars);
    return expressionVars;
}

PcpExpressionVariables
PcpExpressionVariables::Compute(
    const PcpLayerStackIdentifier& sourceLayerStackId,
    const PcpLayerStackIdentifier& rootLayerStackId,
    const PcpExpressionVariables* overrideVars)
{
    if (overrideVars) {
        VtDictionary composedVars = Pcp_ComposeExpressionVariablesOver(
            sourceLayerStackId, &overrideVars->GetVariables());

        if (composedVars == overrideVars->GetVariables()) {
            return *overrideVars;
        }

        return PcpExpressionVariables(
            PcpExpressionVariablesSource(sourceLayerStackId, rootLayerStackId),
            std::move(composedVars));
    }

    // Pcp_ComposeExpressionVariables will return a pointer to an object
    // stored in the NoCache object, so we must return by value.
    NoCache noCache;
    return *Pcp_ComposeExpressionVariables(
        sourceLayerStackId, rootLayerStackId, &noCache);
}

// ------------------------------------------------------------

PcpExpressionVariableCachingComposer::PcpExpressionVariableCachingComposer(
    const PcpLayerStackIdentifier& rootLayerStackId)
    : _rootLayerStackId(rootLayerStackId)
{
}

const PcpExpressionVariables&
PcpExpressionVariableCachingComposer::ComputeExpressionVariables(
    const PcpLayerStackIdentifier& id)
{
    // Pcp_ComposeExpressionVariables will return a pointer to an object in
    // the _identifierToExpressionVars map, so it's safe to return by 
    // const reference.
    Cache cache(&_identifierToExpressionVars);
    return *Pcp_ComposeExpressionVariables(id, _rootLayerStackId, &cache);
}

PXR_NAMESPACE_CLOSE_SCOPE
