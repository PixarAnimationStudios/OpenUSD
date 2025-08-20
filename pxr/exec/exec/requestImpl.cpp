//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/requestImpl.h"

#include "pxr/exec/exec/authoredValueInvalidationResult.h"
#include "pxr/exec/exec/cacheView.h"
#include "pxr/exec/exec/debugCodes.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/disconnectedInputsInvalidationResult.h"
#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/requestTracker.h"
#include "pxr/exec/exec/runtime.h"
#include "pxr/exec/exec/system.h"
#include "pxr/exec/exec/timeChangeInvalidationResult.h"
#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/exec/types.h"
#include "pxr/exec/exec/valueExtractor.h"
#include "pxr/exec/exec/valueKey.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/exec/ef/leafNode.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/types.h"

#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

Exec_RequestImpl::Exec_RequestImpl(
    ExecSystem * const system,
    ExecRequestComputedValueInvalidationCallback &&valueCallback,
    ExecRequestTimeChangeInvalidationCallback &&timeCallback)
    : _system(system)
    , _lastInvalidatedInterval(EfTimeInterval::GetFullInterval())
    , _valueCallback(std::move(valueCallback))
    , _timeCallback(std::move(timeCallback))
{
    if (TF_VERIFY(_system)) {
        _system->_requestTracker->Insert(this);
    }
}

Exec_RequestImpl::~Exec_RequestImpl()
{
    if (_system) {
        _system->_requestTracker->Remove(this);
    }
}

static void
_OutputInvalidationResultDebugMsg(
    const std::string_view label,
    const ExecRequestIndexSet &indices)
{
    TF_DEBUG(EXEC_REQUEST_INVALIDATION).Msg(
        "[%.*s]\n",
        static_cast<int>(label.size()), label.data());

    std::vector<int> sortedIndices(indices.begin(), indices.end());
    std::sort(sortedIndices.begin(), sortedIndices.end());
    TF_DEBUG(EXEC_REQUEST_INVALIDATION).Msg("    indices:");
    for (const int index : sortedIndices) {
        TF_DEBUG(EXEC_REQUEST_INVALIDATION).Msg(" %d", index);
    }
}

static void
_OutputInvalidationResultDebugMsg(
    const std::string_view label,
    const ExecRequestIndexSet &indices,
    const EfTimeInterval &interval)
{
    _OutputInvalidationResultDebugMsg(label, indices);

    TF_DEBUG(EXEC_REQUEST_INVALIDATION).Msg(
        "\n    interval: %s\n",
        interval.GetAsString().c_str());
}

void 
Exec_RequestImpl::DidInvalidateComputedValues(
    const Exec_AuthoredValueInvalidationResult &invalidationResult)
{
    if (!_valueCallback || _leafOutputs.empty()) {
        TF_DEBUG(EXEC_REQUEST_INVALIDATION).Msg(
            "[%s] %s\n", TF_FUNC_NAME().c_str(),
            !_valueCallback
            ? "No value invalidation callback"
            : "Request has not been prepared");
        return;
    }

    TRACE_FUNCTION();

    // This is considered new invalidation only if the invalidation interval
    // isn't already fully contained in the last invalidation interval.
    const EfTimeInterval &invalidInterval = invalidationResult.invalidInterval;
    const bool isNewlyInvalidInterval = invalidInterval.IsFullInterval()
        ? !_lastInvalidatedInterval.IsFullInterval()
        : !_lastInvalidatedInterval.Contains(invalidInterval);
    if (isNewlyInvalidInterval) {
        _lastInvalidatedInterval |= invalidInterval;
    }

    // Build a set of invalid indices from the provided invalid leaf nodes.
    ExecRequestIndexSet invalidIndices;
    _InvalidateLeafOutputs(
        isNewlyInvalidInterval,
        invalidationResult.invalidLeafNodes,
        &invalidIndices);

    // TODO: Handle invalid properties which are not computed through exec.
    // In doing so we must dispatch to the derived class in order to let the
    // specific scene description library determine properties, which do not
    // require execution.

    // Only invoke the invalidation callback if there are any invalid indices
    // from this request.
    if (!invalidIndices.empty()) {
        if (ARCH_UNLIKELY(TfDebug::IsEnabled(EXEC_REQUEST_INVALIDATION))) {
            _OutputInvalidationResultDebugMsg(
                TF_FUNC_NAME(), invalidIndices, invalidInterval);
        }
        TRACE_FUNCTION_SCOPE("value invalidation callback");
        _valueCallback(invalidIndices, invalidInterval);
    }
}

void 
Exec_RequestImpl::DidInvalidateComputedValues(
    const Exec_DisconnectedInputsInvalidationResult &invalidationResult)
{
    if (!_valueCallback || _leafOutputs.empty()) {
        TF_DEBUG(EXEC_REQUEST_INVALIDATION).Msg(
            "[%s] %s\n", TF_FUNC_NAME().c_str(),
            !_valueCallback
            ? "No value invalidation callback"
            : "Request has not been prepared");
        return;
    }

    TRACE_FUNCTION();

    // For topological edits like disconnected inputs we always invalidate over
    // the entire time range. This is considered new invalidation if the last
    // invalidation interval isn't already over the entire time range.
    const EfTimeInterval &invalidInterval = EfTimeInterval::GetFullInterval();
    const bool isNewlyInvalidInterval =
        !_lastInvalidatedInterval.IsFullInterval();
    if (isNewlyInvalidInterval) {
        _lastInvalidatedInterval = invalidInterval;
    }

    // Build a set of invalid indices from the provided invalid leaf nodes.
    ExecRequestIndexSet invalidIndices;
    _InvalidateLeafOutputs(
        isNewlyInvalidInterval,
        invalidationResult.invalidLeafNodes,
        &invalidIndices);
    _InvalidateLeafOutputs(
        isNewlyInvalidInterval,
        invalidationResult.disconnectedLeafNodes,
        &invalidIndices);

    // Only invoke the invalidation callback if there are any invalid indices
    // from this request.
    if (!invalidIndices.empty()) {
        if (ARCH_UNLIKELY(TfDebug::IsEnabled(EXEC_REQUEST_INVALIDATION))) {
            _OutputInvalidationResultDebugMsg(
                TF_FUNC_NAME(), invalidIndices, invalidInterval);
        }
        TRACE_FUNCTION_SCOPE("value invalidation callback");
        _valueCallback(invalidIndices, invalidInterval);
    }
}

void
Exec_RequestImpl::DidChangeTime(
    const Exec_TimeChangeInvalidationResult &invalidationResult)
{
    if (!_timeCallback || _leafOutputs.empty()) {
        TF_DEBUG(EXEC_REQUEST_INVALIDATION).Msg(
            "[%s] %s\n", TF_FUNC_NAME().c_str(),
            !_valueCallback
            ? "No time invalidation callback"
            : "Request has not been prepared");
        return;
    }

    TRACE_FUNCTION();

    // Build a set of invalid indices from the provided invalid leaf nodes.
    ExecRequestIndexSet invalidIndices;
    for (const VdfNode *const leafNode : invalidationResult.invalidLeafNodes) {
        // All requests are notified about all time changes, but not all the
        // invalid leaf nodes may be included in this particular request.
        const auto it = _leafNodeToIndex.find(leafNode->GetId());
        if (it == _leafNodeToIndex.end()) {
            continue;
        }

        // Insert the index into the set of invalid indices.
        invalidIndices.insert(it->second);
    }

    // TODO: Handle all time-dependent properties which are not compiled in
    // exec. In doing so we must dispatch to the derived class in order to let
    // the specific scene description library determine properties, which do not
    // require execution, and which are time-dependent and changing between
    // invalidationResult.oldTime and invalidationResult.newTime.
    
    // Only invoke the invalidation callback if there are any invalid indices
    // from this request.
    if (!invalidIndices.empty()) {
        if (ARCH_UNLIKELY(TfDebug::IsEnabled(EXEC_REQUEST_INVALIDATION))) {
            _OutputInvalidationResultDebugMsg(
                TF_FUNC_NAME(), invalidIndices);
        }
        TRACE_FUNCTION_SCOPE("time change callback");
        _timeCallback(invalidIndices);
    }
}

void
Exec_RequestImpl::Expire()
{
    TF_DEBUG(EXEC_REQUEST_EXPIRATION)
        .Msg("[%s] Expiring request %p\n", TF_FUNC_NAME().c_str(), this);

    if (!TF_VERIFY(_system, "Attempted to expire an expired request")) {
        return;
    }

    TRACE_FUNCTION();

    // If the request has never been prepared (or just contains no values to
    // compute) there's no need push index-specific expiration.
    if (!_leafOutputs.empty()) {
        TRACE_FUNCTION_SCOPE("Expiring all indices");

        ExecRequestIndexSet allIndices;
        const size_t numLeafOutputs = _leafOutputs.size();
        allIndices.reserve(numLeafOutputs);
        for (size_t i=0; i<numLeafOutputs; ++i) {
            allIndices.insert(i);
        }
        _ExpireIndices(allIndices);
    }

    // Because we're expiring the whole request, we do more than just expiring
    // all the indices.  It is guaranteed that no more invalidation can occur
    // so the request removes itself from the system's tracker and drops all
    // its data structures.
    _Discard();
}

// Returns a value extractor suitable for the given value key according to its
// computation definition.
//
// If any errors occur (e.g. invalid provider, invalid computation name,
// unhandled provider type,) returns an invalid extractor.
//
static Exec_ValueExtractor
_GetValueExtractor(
    const Exec_DefinitionRegistry &defReg,
    const ExecTypeRegistry &typeReg,
    const ExecValueKey &vk)
{
    EsfJournal *const nullJournal = nullptr;

    const EsfObject &provider = vk.GetProvider();
    if (!provider->IsValid(nullJournal)) {
        TF_CODING_ERROR("Invalid provider");
        return Exec_ValueExtractor();
    }

    const TfToken &computationName = vk.GetComputationName();
    const Exec_ComputationDefinition *def = defReg.GetComputationDefinition(
        *provider, computationName,
        EsfSchemaConfigKey(),
        nullJournal);

    if (!def) {
        TF_CODING_ERROR("Failed to find computation '%s' on provider '%s'",
                        computationName.GetText(),
                        provider->GetPath(nullJournal).GetText());
        return Exec_ValueExtractor();
    }

    return typeReg.GetExtractor(def->GetExtractionType(*provider));
}

void
Exec_RequestImpl::_Compile(
    TfSpan<const ExecValueKey> valueKeys)
{
    if (!TF_VERIFY(_system)) {
        return;
    }

    // Even if the request is already compiled, we always need to perform
    // recompilation, because doing so might make new connections that
    // invalidate the request's schedule.
    //
    // TODO: If the network doesn't need to be modified at all, then we should
    // avoid repopulating _leafOutputs.

    TRACE_FUNCTION();

    // Compile the value keys.
    WorkWithScopedDispatcher([this, valueKeys] (WorkDispatcher &d) {

        d.Run([valueKeys, system = _system, &leafOutputs = _leafOutputs] {
            leafOutputs = system->_Compile(valueKeys);
        });

        {
            TRACE_FUNCTION_SCOPE("collect value extractors");

            // Collect the extractors.  This is redundant work as compilation
            // must also look up the computation definitions for each value
            // key.  However, it is more direct and easier to understand than
            // carving a special-purpose return path for the definition
            // through the generic compilation tasks.
            const auto &defReg = Exec_DefinitionRegistry::GetInstance();
            const auto &typeReg = ExecTypeRegistry::GetInstance();
            _extractors.assign(valueKeys.size(), Exec_ValueExtractor());
            WorkParallelForN(
                valueKeys.size(),
                [valueKeys, &defReg, &typeReg, &extractors = _extractors]
                (size_t i, size_t n) {
                    for (; i<n; ++i) {
                        extractors[i] = _GetValueExtractor(
                            defReg, typeReg, valueKeys[i]);
                    }
                });
        }
    });

    if (!TF_VERIFY(_leafOutputs.size() == valueKeys.size()) ||
        !TF_VERIFY(_extractors.size() == valueKeys.size())) {
        // If we somehow got the wrong number of outputs from compilation or
        // the wrong number of extractors, we have no idea if the indices
        // correspond correctly so zero out all the outputs & extractors.
        _leafOutputs.assign(valueKeys.size(), VdfMaskedOutput());
        _extractors.assign(valueKeys.size(), Exec_ValueExtractor());
    }

    // If the schedule is still valid, then we are done.
    if (_schedule && _schedule->IsValid()) {
        return;
    }

    // After rescheduling, we need to invalidate all data dependent on
    // the compiled network and the set of compiled leaf outputs.
    _computeRequest.reset();
    _schedule.reset();
    _lastInvalidatedIndices.Resize(_leafOutputs.size());
    // These bits are set instead of cleared because clients are notified of
    // invalidation only after computing values.
    _lastInvalidatedIndices.SetAll();

    // We must greedily build the leaf node to index map. When requests are
    // informed of network edits, some leaf nodes may have already been
    // disconnected from their source output.
    _BuildLeafNodeToIndexMap();
}

void
Exec_RequestImpl::_Schedule()
{
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    // If there's nothing to compute, there's nothing to schedule.
    if (_leafOutputs.empty()) {
        _schedule.reset();
        return;
    }

    // The compute request only needs to be rebuilt if the compiled outputs
    // change.
    if (!_computeRequest) {
        // All outputs received from compilation are expected to be valid.  If
        // they are not, an error should have already been issued.
        std::vector<VdfMaskedOutput> outputs;
        outputs.reserve(_leafOutputs.size());
        for (const VdfMaskedOutput &mo : _leafOutputs) {
            if (mo) {
                outputs.push_back(mo);
            }
        }
        _computeRequest = std::make_unique<VdfRequest>(std::move(outputs));
    }

    // We only need to schedule if there isn't already a valid schedule.
    if (!_schedule || !_schedule->IsValid()) {
        _schedule = std::make_unique<VdfSchedule>();
        VdfScheduler::Schedule(
            *_computeRequest, _schedule.get(), /* topologicallySort */ false);
    }
}

Exec_CacheView
Exec_RequestImpl::_Compute()
{
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    if (!TF_VERIFY(_system) || !_schedule) {
        return Exec_CacheView();
    }

    // Reset the last invalidation state so that new invalidation is properly
    // sent out as clients renew their interest in the computed values included
    // in this request.
    _lastInvalidatedIndices.ClearAll();
    _lastInvalidatedInterval.Clear();

    // Compute the values.
    _system->_Compute(*_schedule, *_computeRequest);

    // Return an exec cache view for the computed values.
    return Exec_CacheView(
        _system->_runtime->GetDataManager(), _leafOutputs, _extractors);
}

bool
Exec_RequestImpl::_RequiresCompilation() const
{
    return !_schedule
        || !_schedule->IsValid()
        || (TF_VERIFY(_system) && _system->_HasPendingRecompilation());
}

void
Exec_RequestImpl::_ExpireIndices(const ExecRequestIndexSet &expired)
{
    // If the request has never been prepared, there's no invalidation to push
    // to clients.
    if (_leafOutputs.empty()) {
        return;
    }

    TRACE_FUNCTION();

    TF_DEBUG(EXEC_REQUEST_EXPIRATION)
        .Msg("[%s] Expiring %zu indices\n",
             TF_FUNC_NAME().c_str(), expired.size());

    const bool isNewlyInvalidInterval =
        !_lastInvalidatedInterval.IsFullInterval();
    ExecRequestIndexSet newlyInvalidIndices;
    newlyInvalidIndices.reserve(expired.size());
    for (const int idx : expired) {
        if (isNewlyInvalidInterval || !_lastInvalidatedIndices.IsSet(idx)) {
            newlyInvalidIndices.insert(idx);
            _lastInvalidatedIndices.Set(idx);
        }
        _leafOutputs[idx] = VdfMaskedOutput();
        _extractors[idx] = Exec_ValueExtractor();
    }
    _schedule.reset();
    _computeRequest.reset();

    if (_valueCallback && !newlyInvalidIndices.empty()) {
        if (ARCH_UNLIKELY(TfDebug::IsEnabled(EXEC_REQUEST_INVALIDATION))) {
            _OutputInvalidationResultDebugMsg(
                TF_FUNC_NAME(), expired, EfTimeInterval::GetFullInterval());
        }
        TRACE_FUNCTION_SCOPE("value invalidation callback");
        _valueCallback(expired, EfTimeInterval::GetFullInterval());
    }

    _lastInvalidatedInterval = EfTimeInterval::GetFullInterval();
}

void
Exec_RequestImpl::_Discard()
{
    _system->_requestTracker->Remove(this);
    _system = nullptr;
    TfReset(_leafOutputs);
    TfReset(_extractors);
    TfReset(_leafNodeToIndex);
    _valueCallback = nullptr;
    _timeCallback = nullptr;
}

void
Exec_RequestImpl::_BuildLeafNodeToIndexMap()
{
    // We only need to populate this map for client notification, so if there
    // are no callbacks registered, we can avoid doing the work.
    if (!_valueCallback && !_timeCallback) {
        return;
    }

    TRACE_FUNCTION();

    // Invalid leaf nodes will need to be converted into indices for client
    // notification. Here, we build a data structure for efficient lookup.
    _leafNodeToIndex.clear();
    _leafNodeToIndex.reserve(_leafOutputs.size());
    for (size_t i = 0; i < _leafOutputs.size(); ++i) {
        const VdfMaskedOutput &sourceOutput = _leafOutputs[i];
        if (!sourceOutput) {
            continue;
        }
        for (const VdfConnection *const connection :
                sourceOutput.GetOutput()->GetConnections()) {
            const VdfNode &targetNode = connection->GetTargetNode();
            if (EfLeafNode::IsALeafNode(targetNode)) {
                _leafNodeToIndex.emplace(targetNode.GetId(), i);
            }
        }
    }
}

void
Exec_RequestImpl::_InvalidateLeafOutputs(
    const bool isNewlyInvalidInterval,
    TfSpan<const VdfNode *const> leafNodes,
    ExecRequestIndexSet *const invalidIndices)
{
    if (leafNodes.empty() || !TF_VERIFY(invalidIndices)) {
        return;
    }

    TRACE_FUNCTION();

    // Build a set of invalid indices from the provided invalid leaf nodes.
    for (const VdfNode *const leafNode : leafNodes) {
        // All requests are notified about all computed value invalidation, but
        // not all the invalid leaf nodes may be included in this particular
        // request.
        const auto it = _leafNodeToIndex.find(leafNode->GetId());
        if (it == _leafNodeToIndex.end()) {
            continue;
        }

        // Determine if invalidation has already been sent out for the invalid
        // index. If not, record this index as being invalid.
        const size_t index = it->second;
        if (isNewlyInvalidInterval || !_lastInvalidatedIndices.IsSet(index)) {
            invalidIndices->insert(index);
        }
        _lastInvalidatedIndices.Set(index); 
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
