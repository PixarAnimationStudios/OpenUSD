//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hd/renderSettings.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

HdRenderSettings::HdRenderSettings(
    SdfPath const& id)
    : HdBprim(id)
    , _active(false)
{
}

HdRenderSettings::~HdRenderSettings() = default;

bool
HdRenderSettings::IsActive() const
{
    return _active;
}

const HdRenderSettingsParams&
HdRenderSettings::GetParams() const
{
    return _params;
}

void
HdRenderSettings::Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdDirtyBits *dirtyBits)
{
    if (*dirtyBits & HdRenderSettings::DirtyActive) {
        const VtValue val = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->active);
        
        if (val.IsHolding<bool>()) {
            _active = val.UncheckedGet<bool>();
        }
    }

    if (*dirtyBits & HdRenderSettings::DirtyParams) {

        const VtValue vParams = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->params);

        if (vParams.IsHolding<HdRenderSettingsParams>()) {
            _params = vParams.UncheckedGet<HdRenderSettingsParams>();
        }
    }

    // Allow subclasses to do any additional processing if necessary.
    _Sync(sceneDelegate, renderParam, dirtyBits);

    *dirtyBits = HdChangeTracker::Clean;
}

HdDirtyBits HdRenderSettings::GetInitialDirtyBitsMask() const
{
    int mask = HdRenderSettings::AllDirty;
    return HdDirtyBits(mask);
}

void
HdRenderSettings::_Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    const HdDirtyBits *dirtyBits)
{
    // no-op
}

PXR_NAMESPACE_CLOSE_SCOPE
