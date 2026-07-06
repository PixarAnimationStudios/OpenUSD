//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/request.h"

#include "pxr/usdImaging/usdExecImaging/adapterRegistry.h"
#include "pxr/usdImaging/usdExecImaging/debugCodes.h"
#include "pxr/usdImaging/usdExecImaging/primAdapterInterface.h"
#include "pxr/usdImaging/usdExecImaging/requestBuilder.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/exec/systemDiagnostics.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"

#include <functional>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// The request owns the listener by a TfRefPtr, and the TfNotice API requires
// listeners be provided as TfWeakPtr.
//
class UsdExecImaging_Request::_ObjectsChangedListener
    : public TfRefBase
    , public TfWeakBase
{
public:
    // Static factory method constructs a new notice listener.
    static _ObjectsChangedListenerRefPtr New(
        UsdExecImaging_Request *const request) {
        return TfCreateRefPtr(new _ObjectsChangedListener(request));
    }

private:
    // The constructor registers itself as a notice listener.
    _ObjectsChangedListener(
        UsdExecImaging_Request *const request)
        : _request(request) {
        _noticeKey = TfNotice::Register(
            TfCreateWeakPtr(this),
            &_ObjectsChangedListener::_ObjectsChangedCallback,
            UsdStageConstPtr(_request->_stage));
    }

    ~_ObjectsChangedListener() {
        TfNotice::RevokeAndWait(_noticeKey);
    }

    // Upon receiving a notice, forward the notice to the request.
    void _ObjectsChangedCallback(
        const UsdNotice::ObjectsChanged &objectsChanged) {
        _request->_ObjectsChangedCallback(objectsChanged);
    }

private:
    TfNotice::Key _noticeKey;
    UsdExecImaging_Request *_request;
};

UsdExecImaging_RequestSharedPtr
UsdExecImaging_Request::New(UsdStageRefPtr stage)
{
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST, "[%s]\n", TF_FUNC_NAME().c_str());

    if (!TF_VERIFY(stage)) {
        return nullptr;
    }

    return std::shared_ptr<UsdExecImaging_Request>(
        new UsdExecImaging_Request(std::move(stage)));
}

UsdExecImaging_Request::UsdExecImaging_Request(UsdStageRefPtr stage)
    : _stage(std::move(stage))
    , _graphFileIndex(0)
    , _requiresRebuild(true)
    , _requiresRecompute(true)
{
    _system.emplace(_stage);

    // Construct the change listener after the system, in case it immediately
    // receives an ObjectsChanged notice.
    _objectsChangedListener = _ObjectsChangedListener::New(this);
}

void
UsdExecImaging_Request::Refresh()
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Refreshing UsdExecImaging request");
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST, "[%s]\n", TF_FUNC_NAME().c_str());

    if (_requiresRebuild || !_request || !_request->IsValid()) {
        _Rebuild();
    }
    if (_requiresRecompute) {
        _Recompute();
    }
}

void
UsdExecImaging_Request::SetTime(const UsdTimeCode timeCode)
{
    TRACE_FUNCTION();
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST,
        "[%s] %s\n",
        TF_FUNC_NAME().c_str(),
        TfStringify(timeCode).c_str());

    // This may invoke _TimeChangedInvalidationCallback.
    _system->ChangeTime(timeCode);
}

VtValue
UsdExecImaging_Request::GetComputedValue(const UsdExecImagingValueKey &valueKey)
{
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST,
        "[%s] %s %s\n",
        TF_FUNC_NAME().c_str(),
        valueKey.providerPath.GetText(),
        valueKey.computationName.GetText());

    // The request must be ready for extraction.
    if (!TF_VERIFY(!_requiresRebuild) ||
        !TF_VERIFY(!_requiresRecompute) ||
        !TF_VERIFY(_request) ||
        !TF_VERIFY(_request->IsValid())) {
        return {};
    }

    // Extract the value key from the cache view, using the value key map to
    // determine the appropriate index. We expect the value key to be found in
    // the map.
    const auto it = _valueKeyMap.valueKeyToIndexMap.find(valueKey);
    if (!TF_VERIFY(it != _valueKeyMap.valueKeyToIndexMap.end())) {
        return {};
    }
    return _cacheView->Get(it->second);
}

HdContainerDataSourceHandle
UsdExecImaging_Request::GetPrimData(const SdfPath &primPath)
{
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST,
        "[%s] %s\n",
        TF_FUNC_NAME().c_str(),
        primPath.GetText());

    // Check if there is a prim adapter for the prim at primPath. If not, there
    // is no data source for computed values. This is not an error.
    const auto it = _valueKeyMap.primToAdapterMap.find(primPath);
    if (it == _valueKeyMap.primToAdapterMap.end()) {
        return nullptr;
    }

    // Construct a shared pointer to a UsdExecImagingRequestAccessorInterface
    // which holds a shared reference to this request.
    UsdExecImagingRequestAccessorInterfaceSharedPtr requestAccessor(
        shared_from_this(),
        static_cast<UsdExecImagingRequestAccessorInterface *>(this));

    // Get data sources by delegating to the prim adapter. The adapter may make
    // copies of the requestAccessor, and each copy holds a shared reference to
    // this request.
    return it->second->GetPrimData(primPath, requestAccessor);
}

HdSceneIndexObserver::DirtiedPrimEntries
UsdExecImaging_Request::TakeDirtiedPrimEntries()
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::DirtiedPrimEntries result;

    // Move the set of dirty locators from the map into the result vector. We
    // need to do manual iteration instead of range-for, because the operator*
    // for robin_map::iterator returns a const reference. We need to use the
    // robin_map::iterator::value() method to get a non-const reference to the
    // mapped value, in order to move it out of the entry.
    for (auto it = _primToDirtyDataSourcesMap.begin();
        it != _primToDirtyDataSourcesMap.end(); ++it) {
        if (!it->second.IsEmpty()) {
            result.emplace_back(
                it->first,
                std::move(it.value()));
        }
    }

    // The dirty locators have been moved out of the map.
    _primToDirtyDataSourcesMap.clear();

    return result;
}

void
UsdExecImaging_Request::_Rebuild()
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Rebuilding UsdExecImaging request");
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST, "[%s]\n", TF_FUNC_NAME().c_str());

    // This method will rebuild the set of dirty data sources.
    _primToDirtyDataSourcesMap.clear();

    // Prim adapters use this object to add value keys to the request.
    UsdExecImaging_RequestBuilder requestBuilder;

    for (const UsdPrim &prim : _stage->Traverse()) {
        // Get the adpater for this prim. If there is no adapter, then skip
        // this prim.
        const UsdExecImagingPrimAdapterInterface *const primAdapter =
            UsdExecImaging_AdapterRegistry::GetPrimAdapter(prim);
        if (!primAdapter) {
            continue;
        }

        // Add value keys for this prim.
        TF_DEBUG_MSG(USDEXECIMAGING_REQUEST,
            "[%s] Adapting prim %s\n",
            TF_FUNC_NAME().c_str(),
            prim.GetPath().GetText());
        requestBuilder.SetAdaptedPrim(prim, *primAdapter);
        primAdapter->BuildRequest(prim, requestBuilder);

        // TODO: Even though the exec request only provides some data sources
        // for the prim, we conservatively invalidate all data sources of the
        // prim when we rebuild the request. This can be optimized in the
        // future.
        _primToDirtyDataSourcesMap[prim.GetPath()] =
            HdDataSourceLocatorSet::UniversalSet();
    }

    // Build the exec request.
    using namespace std::placeholders;
    using This = UsdExecImaging_Request;
    _request = _system->BuildRequest(
        requestBuilder.TakeValueKeys(),
        std::bind(&This::_InvalidateRequestIndices, this, _1),
        std::bind(&This::_InvalidateRequestIndices, this, _1));

    // Save the value key map gathered by the request builder.
    _valueKeyMap = requestBuilder.TakeValueKeyMap();

    // The request no longer requires rebuilding, but must be recomputed.
    _requiresRebuild = false;
    _requiresRecompute = true;

    // If enabled, write the exec network to a file.
    if (TfDebug::IsEnabled(USDEXECIMAGING_GRAPH_AFTER_REBUILD)) {
        TF_DESCRIBE_SCOPE("Writing exec network to file");
        TRACE_FUNCTION_SCOPE("Write exec network to file");

        const std::string filename =
            TfStringPrintf("usdExecImaging_Request_%d.dot", _graphFileIndex++);

        TF_DEBUG_MSG(USDEXECIMAGING_GRAPH_AFTER_REBUILD,
            "[%s] Writing %s\n",
            TF_FUNC_NAME().c_str(),
            filename.c_str());
        
        // Ensure the request has been compiled or else the graph may be empty.
        _system->PrepareRequest(*_request);
        
        ExecSystem::Diagnostics diagnostics(&_system.value());
        diagnostics.GraphNetwork(filename.c_str());
    }
}

void
UsdExecImaging_Request::_Recompute()
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Recomputing UsdExecImaging request");
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST, "[%s]\n", TF_FUNC_NAME().c_str());

    if (!TF_VERIFY(_request->IsValid())) {
        return;
    }

    _cacheView.emplace(_system->Compute(*_request));
    _requiresRecompute = false;
}

void
UsdExecImaging_Request::_InvalidateRequestIndices(
    const ExecRequestIndexSet &invalidIndices)
{
    TRACE_FUNCTION();
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST,
        "[%s] %zu indices\n",
        TF_FUNC_NAME().c_str(),
        invalidIndices.size());

    const int numValueKeys =
        static_cast<int>(_valueKeyMap.indexToValueKeyInfo.size());
    for (const int valueKeyIndex : invalidIndices) {
        // Look up information about the value key at this index.
        if (!TF_VERIFY(0 <= valueKeyIndex && valueKeyIndex < numValueKeys)) {
            continue;
        }
        const UsdExecImaging_ValueKeyMap::ValueKeyInfo &valueKeyInfo =
            _valueKeyMap.indexToValueKeyInfo[valueKeyIndex];

        // Get or create the set of invalid data source locators for the adapted
        // prim that added the value key at this index.
        HdDataSourceLocatorSet &dirtyDataSourceLocators =
            _primToDirtyDataSourcesMap[valueKeyInfo.adaptedPrimPath];

        // Notify the prim adapter of the invalidated value key. The prim
        // adapter responds by adding data source locators to
        // dirtyDataSourceLocators.
        valueKeyInfo.primAdapter->InvalidatePrimData(
            valueKeyInfo.adaptedPrimPath,
            valueKeyInfo.valueKey,
            &dirtyDataSourceLocators);
    }

    // The request needs to be recomputed.
    _requiresRecompute = true;
}

void
UsdExecImaging_Request::_ObjectsChangedCallback(
    const UsdNotice::ObjectsChanged &objectsChanged)
{
    TRACE_FUNCTION();
    TF_DEBUG_MSG(USDEXECIMAGING_REQUEST, "[%s]\n", TF_FUNC_NAME().c_str());

    // The purpose of this method is to detect when the request must be rebuilt
    // due to the addition of new prims that require exec for imaging. If the
    // request is already slated to be rebuilt, then we can avoid re-traversing
    // the scene entirely.
    if (_requiresRebuild || !_request || !_request->IsValid()) {
        return;
    }

    for (const SdfPath &resyncedPath : objectsChanged.GetResyncedPaths()) {
        // Get the resynced prim. If the prim is inactive, undefined, not
        // loaded, or abstract, then we do not traverse this prim's hierarchy.
        // If this prim or any of its descendants are providers for value keys
        // in the exec request, then exec already expires the request for us.
        const UsdPrim &resyncedPrim = _stage->GetPrimAtPath(resyncedPath);
        if (!resyncedPrim.IsValid() || !UsdPrimDefaultPredicate(resyncedPrim)) {
            continue;
        }

        // Otherwise, the prim still exists or it has been added by the resync.
        // Traverse the hierarchy rooted at the resynced prim in search of
        // descendant prims whose adapters have changed.
        for (const UsdPrim &prim : UsdPrimRange(resyncedPrim)) {

            // Get the old prim adapter.
            const auto it = _valueKeyMap.primToAdapterMap.find(prim.GetPath());
            const UsdExecImagingPrimAdapterInterface *const oldPrimAdapter =
                it == _valueKeyMap.primToAdapterMap.end()
                ? nullptr
                : it->second;

            // Get the new prim adapter.
            const UsdExecImagingPrimAdapterInterface *const newPrimAdapter =
                UsdExecImaging_AdapterRegistry::GetPrimAdapter(prim);

            // If an adapted prim has been resynced, then the leaf nodes for the
            // prim's value keys have likely been uncompiled. The request should
            // be recompiled.
            if (oldPrimAdapter) {
                _requiresRecompute = true;
            }

            // If the adapter has changed, then the request must be rebuilt, and
            // there is no need to continue traversing the scene.
            if (oldPrimAdapter != newPrimAdapter) {
                TF_DEBUG_MSG(USDEXECIMAGING_REQUEST,
                    "[%s] Adapter changed for prim %s \n",
                    TF_FUNC_NAME().c_str(),
                    prim.GetPath().GetText());

                _requiresRebuild = true;
                return;
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
