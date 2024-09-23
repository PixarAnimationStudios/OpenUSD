//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/lightFilter.h"

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

// For now, the procs in this file are boiler plate for when hdPrman needs to
// have light filters become prime citizens.  This will probably happen when
// its time to implement shared light filters.  For now, light filters are
// handled inside the lights in light.cpp.
//
// Also, for now base the HdPrmanLightFilter class on HdSprim as there
// currently is no HdLightFilter class.

HdPrmanLightFilter::HdPrmanLightFilter(SdfPath const& id,
                                       TfToken const& /*lightFilterType*/)
    : HdSprim(id)
    , _coordSysId(riley::CoordinateSystemId::InvalidId())
    , _rileyIsInSync(false)
{
    // XXX The lightFilterType parameter is always "lightFilter", so
    // we ignore it.
}

HdPrmanLightFilter::~HdPrmanLightFilter() = default;

void
HdPrmanLightFilter::Finalize(HdRenderParam *renderParam)
{
    std::lock_guard<std::mutex> lock(_syncToRileyMutex);
    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley *riley = param->AcquireRiley();
    if (_coordSysId != riley::CoordinateSystemId::InvalidId()) {
        riley->DeleteCoordinateSystem(_coordSysId);
        _coordSysId = riley::CoordinateSystemId::InvalidId();
    }
}

void
HdPrmanLightFilter::Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits)
{
    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);

#if HD_API_VERSION >= 70
    if (*dirtyBits & HdLight::DirtyTransform) {
#else
    if (*dirtyBits & HdChangeTracker::DirtyTransform) {
#endif
        std::lock_guard<std::mutex> lock(_syncToRileyMutex);
        _rileyIsInSync = false;
        _SyncToRileyWithLock(
            sceneDelegate, param, param->AcquireRiley());
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdPrmanLightFilter::SyncToRiley(
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
HdPrmanLightFilter::_SyncToRileyWithLock(
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
    for (size_t i=0; i < xf.count; ++i) {
        xf_rt_values[i] = HdPrman_Utils::GfMatrixToRtMatrix(xf.values[i]);
    }
    const riley::Transform xform = {
        unsigned(xf.count), xf_rt_values.data(), xf.times.data()};

    RtParamList attrs;

    // Use the full path to identify this coordinate system, which
    // is not user-named but implicitly part of the light filter.
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
HdPrmanLightFilter::GetCoordSysId()
{
    std::lock_guard<std::mutex> lock(_syncToRileyMutex);
    TF_VERIFY(_rileyIsInSync, "Must call SyncToRiley() first");
    return _coordSysId;
}

/* virtual */
HdDirtyBits
HdPrmanLightFilter::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE

