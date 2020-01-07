//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "hdPrman/coordSys.h"
#include "hdPrman/context.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"
#include "RtParamList.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrmanCoordSys::HdPrmanCoordSys(SdfPath const& id)
    : HdCoordSys(id)
    , _coordSysId(riley::CoordinateSystemId::k_InvalidId)
{
    /* NOTHING */
}

HdPrmanCoordSys::~HdPrmanCoordSys()
{
}

void
HdPrmanCoordSys::Finalize(HdRenderParam *renderParam)
{
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();
    _ResetCoordSys(context);
}

void
HdPrmanCoordSys::_ResetCoordSys(HdPrman_Context *context)
{
    riley::Riley *riley = context->riley;
    if (_coordSysId != riley::CoordinateSystemId::k_InvalidId) {
        riley->DeleteCoordinateSystem(_coordSysId);
        _coordSysId = riley::CoordinateSystemId::k_InvalidId;
    }
}

/* virtual */
void
HdPrmanCoordSys::Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits)
{  
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    SdfPath id = GetId();

    riley::Riley *riley = context->riley;

    if (*dirtyBits) {
        _ResetCoordSys(context);

        // Sample transform
        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
        sceneDelegate->SampleTransform(id, &xf);
        TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
            xf_rt_values(xf.count);
        for (size_t i=0; i < xf.count; ++i) {
            xf_rt_values[i] = HdPrman_GfMatrixToRtMatrix(xf.values[i]);
        }
        const riley::Transform xform = {
            unsigned(xf.count), xf_rt_values.data(), xf.times.data()};

        RtParamList attrs;
        // The coordSys name is the final component of the id,
        // after stripping namespaces.
        attrs.SetString(RixStr.k_name,
            RtUString(SdfPath::StripNamespace(id.GetName()).c_str()));

        _coordSysId = riley->CreateCoordinateSystem(xform, attrs);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

/* virtual */
HdDirtyBits
HdPrmanCoordSys::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

bool
HdPrmanCoordSys::IsValid() const
{
    return _coordSysId != riley::CoordinateSystemId::k_InvalidId;
}

PXR_NAMESPACE_CLOSE_SCOPE

