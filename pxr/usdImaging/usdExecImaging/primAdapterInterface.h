//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_PRIM_ADAPTER_INTERFACE_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_PRIM_ADAPTER_INTERFACE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/api.h"
#include "pxr/usdImaging/usdExecImaging/requestAccessorInterface.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdDataSourceLocatorSet;
class SdfPath;
class UsdExecImagingRequestBuilderInterface;
class UsdExecImagingValueKey;
class UsdPrim;

/// Interface to be implemented by all exec imaging prim adapters.
///
/// Plugins implement and register classes derived from
/// UsdExecImagingPrimAdapterInterface to customize the imaging of prims that
/// depend on exec-computed values.
///
class UsdExecImagingPrimAdapterInterface
{
public:
    USDEXECIMAGING_API
    virtual ~UsdExecImagingPrimAdapterInterface();

    /// Invoked by UsdExecImaging when building its exec request.
    ///
    /// Implementations use this method to register that the imaging of \p prim
    /// depends on one or more computed values. Each required computed value is
    /// registered by invoking AddValueKey on the provided \p requestBuilder.
    /// The value keys may be provided by \p prim, any of its attributes, or any
    /// other object in the scene.
    ///
    virtual void BuildRequest(
        const UsdPrim &prim,
        UsdExecImagingRequestBuilderInterface &requestBuilder) const = 0;

    /// Invoked by UsdExecImaging to insert computed values into the hydra scene.
    ///
    /// Implementations use this method to produce data sources for the prim
    /// located at \p primPath. This implementation should only produce data
    /// sources for exec-computed values. The data sources produced by this
    /// method sparsely override data sources produced by the
    /// UsdImagingStageSceneIndex.
    ///
    /// Any sampled data sources that depend on computed values should be
    /// provided from either UsdExecImagingComputedSampledDataSource or
    /// UsdExecImagingComputedTypedSampledDataSource<T>. These data sources
    /// are constructible from the provided \p requestAccessor.
    ///
    virtual HdContainerDataSourceHandle GetPrimData(
        const SdfPath &primPath,
        const UsdExecImagingRequestAccessorInterfaceSharedPtr &requestAccessor)
            const = 0;

    /// Invoked by UsdExecImaging when a computed value is invalidated by
    /// authoring or by a time change.
    ///
    /// This method is invoked when the computed value for \p valueKey has
    /// changed. To be notified of a change to \p valueKey, it must have been
    /// added to the request during BuildRequest. \p primPath holds the path
    /// to the prim that was being adapted when \p valueKey was added to the
    /// request.
    ///
    /// Implementations of this method add data source locators to
    /// \p invalidLocators corresponding to data sources that depend on the
    /// invalidated \p valueKey.
    ///
    virtual void InvalidatePrimData(
        const SdfPath &primPath,
        const UsdExecImagingValueKey &valueKey,
        HdDataSourceLocatorSet *invalidLocators) const = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif