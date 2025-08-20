//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/requestImpl.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/exec/execUsd/visitValueKey.h"

#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/exec/esfUsd/sceneAdapter.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/debugCodes.h"
#include "pxr/exec/exec/valueKey.h"

#include <tbb/concurrent_vector.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Constructs an ExecValueKey that corresponds to the computed value specified
// by an ExecUsdValueKey.  Currently, this is very straightforward.  However,
// there is expected future complexity when dealing with attribute values that
// can be obtained without involving the underlying exec system.
// 
struct _ValueKeyVisitor
{
    ExecValueKey operator()(
        const ExecUsd_ExpiredValueKey &) const {
        return ExecValueKey(
            EsfUsdSceneAdapter::AdaptObject(UsdObject()),
            TfToken());
    }

    ExecValueKey operator()(
        const ExecUsd_AttributeComputationValueKey &key) const {
        return ExecValueKey(
            EsfUsdSceneAdapter::AdaptObject(key.provider),
            key.computation);
    }

    ExecValueKey operator()(
        const ExecUsd_PrimComputationValueKey &key) const {
        return ExecValueKey(
            EsfUsdSceneAdapter::AdaptObject(key.provider),
            key.computation);
    }
};

// Visitor that returns true if a value key's provider is valid.
struct _IsValidVisitor
{
    bool operator()(const ExecUsd_ExpiredValueKey &) const {
        return false;
    }

    bool operator()(const ExecUsd_AttributeComputationValueKey &key) const {
        return key.provider.IsValid();
    }

    bool operator()(const ExecUsd_PrimComputationValueKey &key) const {
        return key.provider.IsValid();
    }
};

struct _DebugStringVisitor
{
    std::string operator()(
        const ExecUsd_ExpiredValueKey &key) const {
        return _Format("[expired]", key.path, key.computation);
    }

    std::string operator()(
        const ExecUsd_AttributeComputationValueKey &key) const {
        return _Format("[attribute]", key.provider.GetPath(), key.computation);
    }

    std::string operator()(
        const ExecUsd_PrimComputationValueKey &key) const {
        return _Format("[prim]", key.provider.GetPath(), key.computation);
    }

private:
    static std::string _Format(
        std::string_view tag,
        const SdfPath &providerPath,
        const TfToken &computation) {
        std::string s(tag);
        s += ' ';
        s += providerPath.GetAsString();
        s += " (";
        s += computation.GetString();
        s += ')';
        return s;
    }
};

}

ExecUsd_RequestImpl::ExecUsd_RequestImpl(
    ExecUsdSystem *const system,
    std::vector<ExecUsdValueKey> &&valueKeys,
    ExecRequestComputedValueInvalidationCallback &&valueCallback,
    ExecRequestTimeChangeInvalidationCallback &&timeCallback)
    : Exec_RequestImpl(
        system, std::move(valueCallback), std::move(timeCallback))
    , _valueKeys(std::move(valueKeys))
    , _expired(_valueKeys.size())
{
    // Because request expiration is driven by change processing, we must
    // check the initial validity of value keys and immediately expire indices
    // for invalid keys.
    ExpireInvalidIndices();
}

ExecUsd_RequestImpl::~ExecUsd_RequestImpl() = default;

void
ExecUsd_RequestImpl::Compile()
{
    if (!_RequiresCompilation()) {
        return;
    }

    TRACE_FUNCTION();

    const size_t numValueKeys = _valueKeys.size();

    std::vector<ExecValueKey> valueKeys;
    valueKeys.reserve(numValueKeys);
    for (const ExecUsdValueKey &uvk : _valueKeys) {
        valueKeys.push_back(ExecUsd_VisitValueKey(_ValueKeyVisitor{}, uvk));
    }

    _Compile(valueKeys);
}

void
ExecUsd_RequestImpl::Schedule()
{
    _Schedule();
}

ExecUsdCacheView
ExecUsd_RequestImpl::Compute()
{
    return ExecUsdCacheView(_Compute());
}

void
ExecUsd_ExpireValueKey(ExecUsdValueKey *uvk)
{
    auto& key = uvk->_key;

    if (const auto *attrKey =
            std::get_if<ExecUsd_AttributeComputationValueKey>(&key)) {
        key = ExecUsd_ExpiredValueKey{
            attrKey->provider.GetPath(), std::move(attrKey->computation)};
    }
    else if (const auto *primKey =
                 std::get_if<ExecUsd_PrimComputationValueKey>(&key)) {
        key = ExecUsd_ExpiredValueKey{
            primKey->provider.GetPath(), std::move(primKey->computation)};
    }
    else {
        const std::string heldTypeName = std::visit(
            [](const auto &k) { return ArchGetDemangled<decltype(k)>(); },
            key);
        TF_VERIFY(false, "Attempted to expire unhandled key variant '%s'",
                  heldTypeName.c_str());
    }
}

void
ExecUsd_RequestImpl::ExpireInvalidIndices()
{
    void ExecUsd_ExpireValueKey(ExecUsdValueKey *);

    tbb::concurrent_vector<size_t> newExpired;
    WorkWithScopedParallelism([&] {
        WorkParallelForN(
            _valueKeys.size(),
            [&newExpired, this](size_t i, size_t n) {
                for (; i<n; ++i) {
                    // Each index only expires once.
                    if (_expired.IsSet(i)) {
                        continue;
                    }

                    ExecUsdValueKey &uvk = _valueKeys[i];
                    const bool isKeyValid = ExecUsd_VisitValueKey(
                        _IsValidVisitor{}, uvk);
                    if (!isKeyValid) {
                        newExpired.push_back(i);
                        ExecUsd_ExpireValueKey(&uvk);
                    }
                }
            });
    });

    if (TfDebug::IsEnabled(EXEC_REQUEST_EXPIRATION)) {
        TF_DEBUG(EXEC_REQUEST_EXPIRATION)
            .Msg("[%s] Expiring %zu indices:\n",
                 TF_FUNC_NAME().c_str(),
                 newExpired.size());
        for (const size_t i : newExpired) {
            const ExecUsdValueKey &uvk = _valueKeys[i];

            TF_DEBUG(EXEC_REQUEST_EXPIRATION).Msg(
                " ... %zu : %s\n",
                i, ExecUsd_VisitValueKey(_DebugStringVisitor{}, uvk).c_str());
        }
    }

    if (!newExpired.empty()) {
        for (const size_t i : newExpired) {
            _expired.Set(i);
        }
        _ExpireIndices(
            ExecRequestIndexSet(newExpired.begin(), newExpired.end()));
    }
}

void
ExecUsd_RequestImpl::Discard()
{
    _Discard();
    _expired.Resize(0);
}

PXR_NAMESPACE_CLOSE_SCOPE
