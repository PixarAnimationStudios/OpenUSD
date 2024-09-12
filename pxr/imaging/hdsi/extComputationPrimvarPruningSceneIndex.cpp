//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/extComputationPrimvarPruningSceneIndex.h"

#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dataSourceLegacyPrim.h"
#include "pxr/imaging/hd/extComputationContextInternal.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

template <typename T>
T
_SafeGetTypedValue(typename HdTypedSampledDataSource<T>::Handle ds)
{
    if (ds) {
        return ds->GetTypedValue(0.0f);
    }
    return T();
}

// A fallback container data source for use when an invalid one is provided to
// avoid conditional checks in the data source overrides below.
//
class _EmptyContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_EmptyContainerDataSource);

    TfTokenVector
    GetNames() override
    {
        return {};
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        return nullptr;
    }
};

// A computation context that is shared amongst the computed primvars of a
// prim (i.e., Hydra Rprim) that executes the computation network and returns
// the computed result.
// 
// A shared computation context allows caching of the computed results until it
// is fetched. This is useful because Hydra ExtComputation's can produce 
// multiple computed primvar outputs with one invocation and we want to continue
// to take advantage of this versus executing the computation(s) for each
// computed primvar.
//
// XXX The current version is simplistic and does not provide caching, although
//     it is thread safe, since the computation network is executed (possibly 
//     redundantly) for each computed primvar.
// 
class _ExtComputationContext
{
public:
    _ExtComputationContext(const HdSceneIndexBaseRefPtr &si)
    : _si(si) {}

    ~_ExtComputationContext() = default;

    using Time = HdSampledDataSource::Time;
    using TokenValuePair  = std::pair<TfToken, VtValue>;
    using TokenValuePairs = std::vector<TokenValuePair>;

    VtValue
    GetComputedValue(
        const TfToken &primvarName,
        const SdfPath &sourceCompId,
        const TfToken &compOutputName,
        const Time &shutterOffset)
    {
        TokenValuePairs result =
            _ExecuteComputationNetwork(sourceCompId, shutterOffset);

        for (auto entry : result) {
            if (entry.first == compOutputName) {
                return entry.second;
            }
        }

        TF_WARN("Did not find computed value for primvar %s",
                primvarName.GetText());

        return VtValue();
    }
    
    bool
    GetContributingSampleTimesForInterval(
        const SdfPath& sourceCompId,
        const Time startTime,
        const Time endTime,
        std::vector<Time>* outSampleTimes)
    {
        return _GetSampleTimesFromComputationNetwork(
            sourceCompId, startTime, endTime, outSampleTimes);
    }

private:

    using _ComputationDependencyMap =
        std::unordered_map<SdfPath, SdfPathVector, SdfPath::Hash>;
    using _ComputationPathToDataSourceMap =
        std::unordered_map<SdfPath, HdContainerDataSourceHandle, SdfPath::Hash>;
    using _PathQueue = std::deque<SdfPath>;
    using TokenPair = std::pair<TfToken, TfToken>;
    using TokenPairs = std::vector<TokenPair>;

    static SdfPathVector
    _GetInputComputationPaths(
        const HdExtComputationInputComputationContainerSchema &inputComps)
    {
        SdfPathVector result;

        for (const TfToken &name : inputComps.GetNames()) {
            const HdExtComputationInputComputationSchema inputComp =
                inputComps.Get(name);
            if (HdPathDataSourceHandle const ds =
                    inputComp.GetSourceComputation()) {
                result.push_back(ds->GetTypedValue(0.0f));
            }
        }

        return result;
    }

    void
    _GatherComputationSources(
        const SdfPath& sourceCompId,
        _ComputationPathToDataSourceMap* compDsMap,
        _ComputationDependencyMap* compDepMap)
    {
        TF_VERIFY(compDsMap);
        TF_VERIFY(compDepMap);

        // Use a deque to add and subsequently visit dependent computations,
        // using the dependency map to track the ones we've visited.
        _PathQueue compsQueue = { sourceCompId };
        
        while (!compsQueue.empty()) {
            const SdfPath curCompId = compsQueue.front();
            compsQueue.pop_front();
            
            // Nothing to do since we've already processed this computation
            // previously.
            if (compDepMap->find(curCompId) != compDepMap->end()) {
                continue;
            }
            
            // Add computations directly feeding curCompId and update
            // bookkeeping.
            HdSceneIndexPrim prim = _si->GetPrim(curCompId);
            if (HdExtComputationSchema curCompSchema =
                HdExtComputationSchema::GetFromParent(prim.dataSource)) {
                
                // compId -> dataSource
                (*compDsMap)[curCompId] = curCompSchema.GetContainer();
                
                // dependency entry
                auto& entry = (*compDepMap)[curCompId];
                entry = _GetInputComputationPaths(
                    curCompSchema.GetInputComputations());
                
                // comps to visit
                compsQueue.insert(compsQueue.end(), entry.begin(), entry.end());
            }
        }
    }


    static TokenPairs
    _GetComputationInputAndSourceOutputNames(
        const HdExtComputationInputComputationContainerSchema &inputComps)
    {
        TokenPairs result;

        for (const TfToken &name : inputComps.GetNames()) {
            const HdExtComputationInputComputationSchema inputComp =
                inputComps.Get(name);
            if (HdTokenDataSourceHandle const ds =
                    inputComp.GetSourceComputationOutputName()) {
                result.emplace_back(name, ds->GetTypedValue(0.0f));
            }
        }

        return result;
    }
    
    bool _GetSampleTimesFromComputationNetwork(
        const SdfPath& sourceCompId,
        const Time& startTime,
        const Time& endTime,
        std::vector<Time>* outSampleTimes)
    {
        _ComputationPathToDataSourceMap compDsMap;
        {
            _ComputationDependencyMap compDepMap;
            _GatherComputationSources(sourceCompId, &compDsMap, &compDepMap);
        }
        std::vector<HdSampledDataSourceHandle> sources;
        for (const auto& comp : compDsMap) {
            HdExtComputationSchema cs(comp.second);
            const HdSampledDataSourceContainerSchema input =
                cs.GetInputValues();
            for (const TfToken& name : input.GetNames()) {
                if (HdSampledDataSourceHandle const ds = input.Get(name)) {
                    sources.push_back(ds);
                }
            }
        }
        return HdGetMergedContributingSampleTimesForInterval(
            sources.size(), sources.data(), startTime, endTime, outSampleTimes);
    }

    // Execute the computation network by traversing the network backwards from
    // the terminal node "sourceCompId" and return the computed results as
    // token value pairs.
    //
    TokenValuePairs
    _ExecuteComputationNetwork(
        const SdfPath &sourceCompId,
        const Time &shutterOffset)
    {
        TokenValuePairs result;

        // XXX The generic algorithm below might be overkill for the
        //     small size of ExtComputation networks (typically 2-3 nodes).
        
        // Track dependencies for each computation. Note that this map is
        // mutated when we order the computations in the subsequent step.
        _ComputationDependencyMap compDepMap;

        // Cache data sources for computation prims.
        _ComputationPathToDataSourceMap compDsMap;

        // Populate computation dependency & data source maps.
        _GatherComputationSources(sourceCompId, &compDsMap, &compDepMap);

        // Topological ordering of computations (Kahn's algorithm).
        SdfPathVector orderedComps;
        {
            _PathQueue compsQueue;

            using _MapIterator = _ComputationDependencyMap::iterator;

            // Add computations that have no dependencies to queue and remove 
            // them from the dependency map.
            _MapIterator it = compDepMap.begin();
            while (it != compDepMap.end()) {
                const SdfPath &comp = it->first;
                SdfPathVector &dependencies = it->second;
                if (dependencies.empty()) {
                    compsQueue.push_back(comp);
                    it = compDepMap.erase(it);
                } else {
                    it++;
                }
            }

            // On each iteration:
            //    pop a computation from the queue,
            //    add it to the ordered result,
            //    remove it as a dependency from computations in the dependency 
            //    map; in doing so, if a computation no longer has dependencies,
            //    add it to the queue and remove it from the map.
            //
            while (!compsQueue.empty()) {
                // Pop entry, add to ordered result
                const SdfPath indComp = compsQueue.front();
                compsQueue.pop_front();
                orderedComps.push_back(indComp);

                _MapIterator it = compDepMap.begin();
                while (it != compDepMap.end()) {
                    const SdfPath &comp = it->first;
                    SdfPathVector &deps = it->second;
                    deps.erase(std::remove(deps.begin(), deps.end(), indComp),
                               deps.end());
            
                    if (deps.empty()) {
                        compsQueue.push_back(comp);
                        it = compDepMap.erase(it);
                    } else {
                        it++;
                    }
                }
            }
        }

        // Execute computations in order, using a value store to back input
        // and computed output values.
        //
        {
            using _ValueStore =
                std::unordered_map<TfToken, VtValue, TfToken::HashFunctor>;
            _ValueStore valueStore;

            for (SdfPath const &compId : orderedComps) {
                
                HdExtComputationSchema cs(compDsMap[compId]);

                // Add (attribute) inputs to the value store.
                const HdSampledDataSourceContainerSchema inputValues =
                    cs.GetInputValues();
                for (TfToken const &name : inputValues.GetNames()) {
                    
                    if (HdSampledDataSourceHandle const ds =
                            inputValues.Get(name)) {
                        valueStore[name] = ds->GetValue(shutterOffset);
                    }
                }

                // If the computation is an input aggregator (i.e., produces no
                // outputs), there's nothing more to do.
                const HdExtComputationOutputContainerSchema outputs =
                    cs.GetOutputs();
                if (outputs.GetNames().empty()) {
                    continue;
                }

                HdExtComputationContextInternal executionContext;

                // Update the execution context with ...
                // ... (attribute) inputs
                for (TfToken const &name : inputValues.GetNames()) {
                    if (ARCH_UNLIKELY(
                        valueStore.find(name) == valueStore.end())) {
                        TF_WARN("Couldn't find input %s for computation "
                                "%s.", name.GetText(), compId.GetText());
                        continue;
                    }
                    executionContext.SetInputValue(name, valueStore.at(name));
                }

                // ... and computation inputs
                TokenPairs inputAndSourceOutputNames = 
                    _GetComputationInputAndSourceOutputNames(
                        cs.GetInputComputations());
                
                for (const auto &pair : inputAndSourceOutputNames) {
                    const TfToken &inputName = pair.first;
                    const TfToken &sourceCompOutputName = pair.second;

                    if (ARCH_UNLIKELY(valueStore.find(sourceCompOutputName)
                            == valueStore.end())) {

                        TF_WARN("Couldn't find input %s (sourceCompOutput %s) "
                                "for computation %s.", inputName.GetText(),
                                sourceCompOutputName.GetText(),
                                compId.GetText());
                        continue;
                    }
                    executionContext.SetInputValue(
                        inputName, valueStore.at(sourceCompOutputName));
                }
                    
                // Execute computation ....
                // Note: Handle only scene index emulated ext computations
                //       via the cast below.
                if (HdExtComputationCallbackDataSourceHandle callbackDs =
                        HdExtComputationCallbackDataSource::Cast(
                            cs.GetCpuCallback())) {
                    
                    callbackDs->Invoke(&executionContext);

                } else {
                    TF_WARN("Could not find CPU callback data source for %s",
                            compId.GetText());
                    continue;
                }

                // ... and add outputs to the value store.
                if (executionContext.HasComputationError()) {
                    // We could bail here, or choose to execute other 
                    // computations. Choose the latter.
                    TF_WARN("Error invoking computation %s.\n",
                        compId.GetText());

                } else {

                    const TfTokenVector outputNames =
                        outputs.GetNames();
                    
                    const bool updateResult = (compId == sourceCompId);

                    for (const auto &name : outputNames) {
                        VtValue value;
                        if (!executionContext.GetOutputValue(name, &value)) {
                            TF_WARN("Did not find computed output %s for "
                                    "computation %s.\n",
                                    name.GetText(), compId.GetText());
                        } else {
                            if (updateResult) {
                                result.push_back(
                                    std::make_pair(name, value));
                            } else {
                                valueStore[name] = value;
                            }
                        }
                    }
                }

            } // for each computation
        }

        return result;
    }


private:
    const HdSceneIndexBaseRefPtr _si;
};

using _ExtComputationContextSharedPtr = std::shared_ptr<_ExtComputationContext>;


// Wrapper data source for a computed primvar's value.
//
class _SampledExtCompPrimvarDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SampledExtCompPrimvarDataSource);

    _SampledExtCompPrimvarDataSource(
        const HdContainerDataSourceHandle &extCompPrimvarDataSource,
        const TfToken &extCompPrimvarName,
        _ExtComputationContextSharedPtr ctx)
    : _input(extCompPrimvarDataSource)
    , _primvarName(extCompPrimvarName)
    , _ctx(ctx)
    {
        if (ARCH_UNLIKELY(!extCompPrimvarDataSource)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    VtValue
    GetValue(Time shutterOffset) override
    {
        // Delegate to the context to get the computed result
        HdExtComputationPrimvarSchema s(_input);
        HdPathDataSourceHandle h1 = s.GetSourceComputation();
        HdTokenDataSourceHandle h2 = s.GetSourceComputationOutputName();

        if (h1 && h2) {
            SdfPath sourceComp = h1->GetTypedValue(0.0f);
            TfToken outputName = h2->GetTypedValue(0.0f);

            return _ctx->GetComputedValue(
                    _primvarName, sourceComp, outputName, shutterOffset);
        }

        return VtValue();
    }

    bool
    GetContributingSampleTimesForInterval(
        Time startTime, 
        Time endTime,
        std::vector<Time> * outSampleTimes) override
    {
        // Gather and combine all contributing sample times for each input.
        HdExtComputationPrimvarSchema s(_input);
        HdPathDataSourceHandle h1 = s.GetSourceComputation();
        if (h1) {
            SdfPath sourceComp = h1->GetTypedValue(0.0f);
            return _ctx->GetContributingSampleTimesForInterval(
                sourceComp, startTime, endTime, outSampleTimes);
        }
        *outSampleTimes = { 0.0f };
        return false;
    }

private:
    HdContainerDataSourceHandle _input;
    TfToken _primvarName;
    _ExtComputationContextSharedPtr const _ctx;
};


// Wrapper data source for a computed primvar that is presented as an authored
// primvar by satisfying a subset of the HdPrimvarSchema.
// primvarName
//      primvarValue
//      indexedPrimvarValue <-- unsupported by extComp
//      indices <-- unsupported by extComp
//      interpolation
//      role
//
class _ExtCompPrimvarDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ExtCompPrimvarDataSource);

    _ExtCompPrimvarDataSource(
        const HdContainerDataSourceHandle &extCompPrimvarDataSource,
        const TfToken &extCompPrimvarName,
        _ExtComputationContextSharedPtr ctx)
    : _input(extCompPrimvarDataSource)
    , _primvarName(extCompPrimvarName)
    , _ctx(ctx)
    {
        if (ARCH_UNLIKELY(!extCompPrimvarDataSource)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    TfTokenVector
    GetNames() override
    {
        static const TfTokenVector names = {
            HdPrimvarSchemaTokens->primvarValue,
            HdPrimvarSchemaTokens->interpolation,
            HdPrimvarSchemaTokens->role
        };
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdPrimvarSchemaTokens->interpolation ||
            name == HdPrimvarSchemaTokens->role) {

            return _input->Get(name);
        }

        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return _SampledExtCompPrimvarDataSource::New(
                _input, _primvarName, _ctx);
        }

        return nullptr;
    }

private:
    HdContainerDataSourceHandle _input;
    TfToken _primvarName;
    _ExtComputationContextSharedPtr const _ctx;
};

// Primvars schema data source override that adds computed primvars to the
// list of authored primvars and overrides each computed primvar's data source.
// A computation context is created and shared with the latter to allow for
// caching and reuse of computed results when multiple primvars are computed by
// the same computation.
//
class _PrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    _PrimvarsDataSource(
        const HdContainerDataSourceHandle &primvarsDs,
        const HdContainerDataSourceHandle &extCompPrimvarsDs,
        const HdSceneIndexBaseRefPtr &si)
    : _primvarsDs(primvarsDs)
    , _extCompPrimvarsDs(extCompPrimvarsDs)
    , _si(si)
    , _ctx(std::make_shared<_ExtComputationContext>(si))
    {
        if (ARCH_UNLIKELY(!primvarsDs)) {
            // Don't emit an error here. It is possible that there are no
            // authored primvars.
            _primvarsDs = _EmptyContainerDataSource::New();
        }
        if (ARCH_UNLIKELY(!extCompPrimvarsDs)) {
            TF_CODING_ERROR("Invalid container data source input provided for "
                            "extComputationPrimvars.");
            _extCompPrimvarsDs = _EmptyContainerDataSource::New();
        }
    }

    TfTokenVector
    GetNames() override
    {
        // Merge authored and computed primvar names.
        TfTokenVector res;
        TfTokenVector a = _primvarsDs->GetNames();
        TfTokenVector b = _extCompPrimvarsDs->GetNames();

        res.reserve(a.size() + b.size());
        res.insert(res.end(), a.begin(), a.end());
        res.insert(res.end(), b.begin(), b.end());
        return res;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        // If it's an authored primvar, return as-is.
        if (HdDataSourceBaseHandle authoredPrimvarDs = _primvarsDs->Get(name)) {
            return authoredPrimvarDs;
        }

        // Override the data source for computed primvars.
        if (HdContainerDataSourceHandle c = HdContainerDataSource::Cast(
                _extCompPrimvarsDs->Get(name))) {
            return _ExtCompPrimvarDataSource::New(c, name, _ctx);
        }

        return nullptr;
    }

private:
    HdContainerDataSourceHandle _primvarsDs;
    HdContainerDataSourceHandle _extCompPrimvarsDs;
    const HdSceneIndexBaseRefPtr _si;
    _ExtComputationContextSharedPtr _ctx;
};


// Prim level data source override.
// A prim container that houses extcomputation primvars has the following
// hierarchy:
//
// prim
//     primvars (primvarsSchema)
//         name : primvarSchema
//                    primvarValue, ...
//     extComputationPrimvars (extComputationPrimvarsSchema)
//          name : extComputationPrimvarSchema
//                    sourceComputation, ...
//
// Override the primvars container if there are computed primvars, and
// return an empty container for extComputationPrimvars.
//
class _PrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdContainerDataSourceHandle &input,
        const HdSceneIndexBaseRefPtr &si)
    : _input(input)
    , _si(si)
    {
        if (ARCH_UNLIKELY(!_input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();

        // Handle a less likely scenario wherein a prim has only computed
        // primvars.
        // We could remove extComputationPrimvars, but it might be cheaper to
        // return an empty data source in Get() instead.
        if (ARCH_UNLIKELY(
                std::find(names.begin(), names.end(),
                    HdExtComputationPrimvarsSchemaTokens->
                        extComputationPrimvars) != names.end()
                && std::find(names.begin(), names.end(),
                    HdPrimvarsSchemaTokens->primvars) == names.end())) {
            names.push_back(HdPrimvarsSchemaTokens->primvars);
        }

        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);

        if (name == HdPrimvarsSchemaTokens->primvars) {

            // Check for the presence of computed primvars and override the
            // data source only if that is the case.
            if (HdContainerDataSourceHandle c = HdContainerDataSource::Cast(
                _input->Get(HdExtComputationPrimvarsSchemaTokens->
                                extComputationPrimvars))) {

                HdExtComputationPrimvarsSchema es(c);
                if (!(es.GetExtComputationPrimvarNames().empty())) {

                    HdContainerDataSourceHandle primvarsDs =
                        HdContainerDataSource::Cast(result);

                    HdContainerDataSourceHandle &extCompPrimvarsDs = c;

                    return _PrimvarsDataSource::New(
                            primvarsDs, extCompPrimvarsDs, _si);

                }
            }
        }

        // Suppress computed primvars.
        if (name ==
                HdExtComputationPrimvarsSchemaTokens->extComputationPrimvars) {
            return _EmptyContainerDataSource::New();
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _input;
    const HdSceneIndexBaseRefPtr _si;
};


} // namespace anonymous

////////////////////////////////////////////////////////////////////////////////

/* static */
HdSiExtComputationPrimvarPruningSceneIndexRefPtr
HdSiExtComputationPrimvarPruningSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdSiExtComputationPrimvarPruningSceneIndex(inputSceneIndex));
}


HdSiExtComputationPrimvarPruningSceneIndex::
    HdSiExtComputationPrimvarPruningSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim
HdSiExtComputationPrimvarPruningSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    const bool isRprim =
        prim.primType == HdPrimTypeTokens->mesh ||
        prim.primType == HdPrimTypeTokens->basisCurves||
        prim.primType == HdPrimTypeTokens->points;

    if (isRprim && prim.dataSource) {
        prim.dataSource =
            _PrimDataSource::New(prim.dataSource, _GetInputSceneIndex());
    }

    return prim;
}

SdfPathVector
HdSiExtComputationPrimvarPruningSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdSiExtComputationPrimvarPruningSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdSiExtComputationPrimvarPruningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdSiExtComputationPrimvarPruningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    /// XXX Use an Intersects test to avoid copying entries when unnecessary?
    HdSceneIndexObserver::DirtiedPrimEntries editedEntries;
    editedEntries.reserve(entries.size());

    for (const auto &entry : entries) {
        const HdDataSourceLocatorSet &locators = entry.dirtyLocators;

        // Present dirty notices for computed primvars as though it were for
        // authored primvars.
        editedEntries.emplace_back(entry.primPath,
            locators.ReplacePrefix(
                HdExtComputationPrimvarsSchema::GetDefaultLocator(),
                HdPrimvarsSchema::GetDefaultLocator()));

    }

    _SendPrimsDirtied(editedEntries);
}

PXR_NAMESPACE_CLOSE_SCOPE
