//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/coordSys.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hf/diagnostic.h"
#include "RiTypesHelper.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrmanCoordSys::HdPrmanCoordSys(SdfPath const& id)
    : HdCoordSys(id)
    , _coordSysId(riley::CoordinateSystemId::InvalidId())
{
    /* NOTHING */
}

HdPrmanCoordSys::~HdPrmanCoordSys() = default;

void
HdPrmanCoordSys::Finalize(HdRenderParam *renderParam)
{
    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);
    _ResetCoordSys(param);
}

void
HdPrmanCoordSys::_ResetCoordSys(HdPrman_RenderParam *param)
{
    riley::Riley *riley = param->AcquireRiley();
    if (_coordSysId != riley::CoordinateSystemId::InvalidId()) {
        riley->DeleteCoordinateSystem(_coordSysId);
        _coordSysId = riley::CoordinateSystemId::InvalidId();
    }
}

/* virtual */
void
HdPrmanCoordSys::Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits)
{
    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    SdfPath id = GetId();
    // Save state of dirtyBits before HdCoordSys::Sync clears them.
    const HdDirtyBits bits = *dirtyBits;

    riley::Riley *riley = param->AcquireRiley();

    HdCoordSys::Sync(sceneDelegate, renderParam, dirtyBits);

    if (bits & AllDirty) {
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
        // The coordSys name is the final component of the id,
        // after stripping namespaces.
        RtUString coordSysName(GetName().GetText());
        attrs.SetString(RixStr.k_name, coordSysName);
        if (_coordSysId != riley::CoordinateSystemId::InvalidId()) {
            riley->ModifyCoordinateSystem(_coordSysId, &xform, &attrs);
        } else {
          _coordSysId = riley->CreateCoordinateSystem(
              riley::UserId(
                  stats::AddDataLocation(id.GetText()).GetValue()),
              xform, attrs);
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

#if HD_API_VERSION < 53
/* virtual */
HdDirtyBits
HdPrmanCoordSys::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}
#endif

bool
HdPrmanCoordSys::IsValid() const
{
    return _coordSysId != riley::CoordinateSystemId::InvalidId();
}

PXR_NAMESPACE_CLOSE_SCOPE
