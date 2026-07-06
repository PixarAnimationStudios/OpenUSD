//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/requestAccessorInterface.h"
#include "pxr/usdImaging/usdExecImaging/valueKeyMap.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/exec/exec/request.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/timeCode.h"

#include <memory>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class EfTimeInterval;

TF_DECLARE_REF_PTRS(UsdStage);

/// Requests are always referred to by a shared pointer.
///
/// This allows data sources to share ownership of the underlying request.
///
using UsdExecImaging_RequestSharedPtr =
    std::shared_ptr<class UsdExecImaging_Request>;

/// The UsdExecImaging_Request manages an ExecUsdRequest used to compute values
/// for imaging.
///
/// UsdExecImaging_Request is the mediator between the scene index and the
/// exec request. It manages building, rebuilding, and computing the
/// contained ExecUsdRequest, as well as tracking the invalidation state of
/// computed values.
///
class UsdExecImaging_Request
    : public std::enable_shared_from_this<UsdExecImaging_Request> 
    , public UsdExecImagingRequestAccessorInterface
{
public:
    static UsdExecImaging_RequestSharedPtr New(UsdStageRefPtr stage);

    /// Ensures the request is up-to-date and ready to extract computed values.
    ///
    /// This rebuilds and recomputes the request if necessary.
    ///
    void Refresh();

    /// Changes the time on the exec system.
    ///
    /// This will invalidate time-dependent computed values, calling into
    /// prim adapters as needed to accumulate the set of dirtied data source
    /// locators. The resulting dirtied prim entries can be obtained by calling
    /// TakeDirtedPrimEntries.
    ///
    void SetTime(UsdTimeCode timeCode);

    // Implements ExecUsdImagingRequestAccessorInterface.
    VtValue GetComputedValue(const UsdExecImagingValueKey &valueKey) override;

    /// Returns a container data source for \p primPath that can access computed
    /// values from this request.
    ///
    /// This function delegates to the GetPrimData method of the corresponding
    /// UsdExecImagingPrimAdapterInterface for \p primPath. That method uses a
    /// shared pointer to a UsdExecImagingRequestAccessorInterface to construct
    /// data sources that lazily extract values from this request. Since
    /// UsdExecImaging_Request IS_A UsdExecImagingRequestAccessorInterface,
    /// data sources returned from this method hold shared pointers to this
    /// request.
    ///
    /// If there is no adapter for the prim at \p primPath, this returns a
    /// null data source handle.
    ///
    HdContainerDataSourceHandle GetPrimData(const SdfPath &primPath);

    /// Returns a vector of dirtied prim entries that identify data sources
    /// dirtied by computed value invalidation, which itself may be caused by
    /// authored value changes or time changes.
    ///
    /// The dirtied entries are moved out of this request, and subsequent calls
    /// will return an empty vector unless additional invalidation has occurred.
    ///
    HdSceneIndexObserver::DirtiedPrimEntries TakeDirtiedPrimEntries();

private:
    UsdExecImaging_Request(UsdStageRefPtr stage);

    // Traverses the stage and builds a new exec request for all adapted prims.
    void _Rebuild();

    // Recomputes the request. This potentially recompiles the network.
    void _Recompute();

    // Common implementation for exec invalidation callbacks. This notifies
    // the necessary prim adapters for the invalidated indices.
    void _InvalidateRequestIndices(
        const ExecRequestIndexSet &invalidIndices);

    // Invoked when the underlying stage emits an ObjectsChanged notice.
    void _ObjectsChangedCallback(
        const UsdNotice::ObjectsChanged &objectsChanged);

    // The request owns a dedicated notice listener to detect ObjectsChanged
    // notices from the underlying stage. The notice listener forwards the
    // notices to _ObjectsChangedCallback on the request.
    class _ObjectsChangedListener;
    TF_DECLARE_WEAK_AND_REF_PTRS(_ObjectsChangedListener);

private:
    UsdStageRefPtr _stage;
    std::optional<ExecUsdSystem> _system;
    std::optional<ExecUsdRequest> _request;
    std::optional<ExecUsdCacheView> _cacheView;
    UsdExecImaging_ValueKeyMap _valueKeyMap;
    using _PrimToDirtyDataSourcesMap =
        pxr_tsl::robin_map<SdfPath, HdDataSourceLocatorSet, TfHash>;
    _PrimToDirtyDataSourcesMap _primToDirtyDataSourcesMap;
    _ObjectsChangedListenerRefPtr _objectsChangedListener;
    unsigned _graphFileIndex;
    bool _requiresRebuild;
    bool _requiresRecompute;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif