//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_INTERFACE_H
#define PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_INTERFACE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/type.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/usd/usd/timeCode.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdStage);
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndexInterface);

/// An interface for initial scene indices that populate the Hydra scene with
/// prims and data sources from a UsdStage.
///
class UsdImagingStageSceneIndexInterface : public HdSceneIndexBase
{
public:
    USDIMAGING_API
    ~UsdImagingStageSceneIndexInterface() override;

    /// Set the USD stage to pull data from. Note that this will delete all
    /// scene index prims and reset stage global data.
    ///
    virtual void SetStage(UsdStageRefPtr stage) = 0;

    /// Set the time, and call PrimsDirtied for any time-varying attributes.
    ///
    /// PrimsDirtied is only called if the time is different from the last call.
    ///
    virtual void SetTime(UsdTimeCode time) = 0;

    /// Return the current time.
    virtual UsdTimeCode GetTime() const = 0;

    /// Apply queued stage edits to imaging scene.
    ///
    /// If the USD stage is edited while the scene index is pulling from it,
    /// those edits get queued and deferred.  Calling ApplyPendingUpdates will
    /// turn resync requests into PrimsAdded/PrimsRemoved, and property changes
    /// into PrimsDirtied.
    ///
    virtual void ApplyPendingUpdates() = 0;
};

/// A factory interface that constructs instances of
/// UsdImagingStageSceneIndexInterface.
///
/// Instances can be constructed without arguments.
///
class UsdImagingStageSceneIndexInterfaceFactoryBase : public TfType::FactoryBase
{
public:
    virtual UsdImagingStageSceneIndexInterfaceRefPtr New() const = 0;
};

/// A trivial implementation of UsdImagingStageSceneIndexInterfaceFactoryBase
/// that manufactures instances of \tparam T.
///
template <class T>
class UsdImagingStageSceneIndexInterfaceFactory
    : public UsdImagingStageSceneIndexInterfaceFactoryBase
{
    static_assert(std::is_base_of_v<UsdImagingStageSceneIndexInterface, T>);
public:
    UsdImagingStageSceneIndexInterfaceRefPtr New() const final {
        return TfCreateRefPtr(new T);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif