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
#include "hdPrman/lightFilter.h"
#include "hdPrman/context.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

// For now, the procs in this file are boiler plate for when hdPrman needs to
// have light filters become prime citizens.  This will probably happen when
// its time to implement shared light filters.  For now, light filters are
// handled inside the lights in light.cpp.
//
// Also, for now base the HdPrmanLightFilter class on HdSprim as there
// currently is no HdLightFilter class.

HdPrmanLightFilter::HdPrmanLightFilter(SdfPath const& id,
                                       TfToken const& lightFilterType)
    : HdSprim(id)
    , _hdLightFilterType(lightFilterType)
    , _lightFilter(NULL)
{
    /* NOTHING */
}

HdPrmanLightFilter::~HdPrmanLightFilter()
{
}

void
HdPrmanLightFilter::Finalize(HdRenderParam *renderParam)
{
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();
    _ResetLightFilter(context);
}

void
HdPrmanLightFilter::_ResetLightFilter(HdPrman_Context *context)
{
    // Currently, light filters are managed in light.cpp as part
    // of the lights.  Eventually, we will probably want to add
    // code here that deletes the light filter via 
    //     if (_lightFilter) {
    //        riley->DeleteLightFilter()
    //        _lightFilter = NULL;
    //     }
    // or something like that. 
}

/* virtual */
void
HdPrmanLightFilter::Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits)
{  
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    if (*dirtyBits) {
        _ResetLightFilter(context);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

/* virtual */
HdDirtyBits
HdPrmanLightFilter::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

bool
HdPrmanLightFilter::IsValid() const
{
    return _lightFilter != NULL;
}

PXR_NAMESPACE_CLOSE_SCOPE

