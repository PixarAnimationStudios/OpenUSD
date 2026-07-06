//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/volumeFilter.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/usd/sdf/types.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/smallVector.h"

#include "pxr/pxr.h"

#include <Riley.h>
#include <RileyIds.h>
#include <RiTypesHelper.h>
#include <stats/Roz.h>

#include <cstddef>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_VolumeFilter::HdPrman_VolumeFilter(SdfPath const& id)
    : HdSprim(id)
    , _coordSysId(riley::CoordinateSystemId::InvalidId())
    , _rileyIsInSync(false)
{
}

HdPrman_VolumeFilter::~HdPrman_VolumeFilter() = default;

void
HdPrman_VolumeFilter::Finalize(HdRenderParam *renderParam)
{
    std::lock_guard<std::mutex> lock(_syncToRileyMutex);
    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley *riley = param->AcquireRiley();
    if (riley && _coordSysId != riley::CoordinateSystemId::InvalidId()) {
        riley->DeleteCoordinateSystem(_coordSysId);
        _coordSysId = riley::CoordinateSystemId::InvalidId();
    }
}

void
HdPrman_VolumeFilter::Sync(HdSceneDelegate *sceneDelegate,
                           HdRenderParam   *renderParam,
                           HdDirtyBits     *dirtyBits)
{
    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);

    if (*dirtyBits & HdChangeTracker::DirtyTransform) {
        std::lock_guard<std::mutex> lock(_syncToRileyMutex);
        _rileyIsInSync = false;
        _SyncToRileyWithLock(
            sceneDelegate, param, param->AcquireRiley());
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdPrman_VolumeFilter::SyncToRiley(
    HdSceneDelegate *sceneDelegate,
    HdPrman_RenderParam *param,
    riley::Riley *riley)
{
    std::lock_guard<std::mutex> lock(_syncToRileyMutex);
    if (!_rileyIsInSync) {
        _SyncToRileyWithLock(sceneDelegate, param, riley);
    }
}

void
HdPrman_VolumeFilter::_SyncToRileyWithLock(
    HdSceneDelegate *sceneDelegate,
    HdPrman_RenderParam * param,
    riley::Riley *riley)
{
    SdfPath const& id = GetId();

    // Sample transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(id,
#if HD_API_VERSION >= 68
                                   param->GetShutterInterval()[0],
                                   param->GetShutterInterval()[1],
#endif
                                   &xf);
    TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>
        xf_rt_values(xf.count);
    for (size_t i = 0; i < xf.count; ++i) {
        xf_rt_values[i] = HdPrman_Utils::GfMatrixToRtMatrix(xf.values[i]);
    }
    const riley::Transform xform = {
        unsigned(xf.count), xf_rt_values.data(), xf.times.data()};

    RtParamList attrs;

    // Use the full path to identify this coordinate system, which
    // is not user-named but implicitly part of the volume filter.
    RtUString coordSysName(id.GetText());
    attrs.SetString(RixStr.k_name, coordSysName);

    if (_coordSysId == riley::CoordinateSystemId::InvalidId()) {
        _coordSysId = riley->CreateCoordinateSystem(
            riley::UserId(stats::AddDataLocation(id.GetText()).GetValue()),
            xform, attrs);
    } else {
        riley->ModifyCoordinateSystem(_coordSysId, &xform, &attrs);
    }

    _rileyIsInSync = true;
}

riley::CoordinateSystemId
HdPrman_VolumeFilter::GetCoordSysId()
{
    std::lock_guard<std::mutex> lock(_syncToRileyMutex);
    TF_VERIFY(_rileyIsInSync, "Must call SyncToRiley() first");
    return _coordSysId;
}

/* virtual */
HdDirtyBits
HdPrman_VolumeFilter::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE
