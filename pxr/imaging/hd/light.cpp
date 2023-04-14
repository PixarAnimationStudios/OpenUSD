//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hd/light.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdLightTokens, HD_LIGHT_TOKENS);

HdLight::HdLight(SdfPath const &id)
 : HdSprim(id)
 , _instancerId()
{
}

HdLight::~HdLight() = default;

void
HdLight::_UpdateInstancer(
    HdSceneDelegate* delegate,
    HdDirtyBits* dirtyBits)
{
    if (HdChangeTracker::IsInstancerDirty(*dirtyBits, GetId())) {
        const SdfPath& instancerId = delegate->GetInstancerId(GetId());
        if (instancerId == _instancerId) {
            return;
        }
        HdChangeTracker& tracker = delegate->GetRenderIndex().GetChangeTracker();
        if (!_instancerId.IsEmpty()) {
            tracker.RemoveInstancerSprimDependency(_instancerId, GetId());
        }
        if (!instancerId.IsEmpty()) {
            tracker.AddInstancerSprimDependency(instancerId, GetId());
        }
        _instancerId = instancerId;
    }
}

/* static */
std::string
HdLight::StringifyDirtyBits(HdDirtyBits dirtyBits) {
    if (dirtyBits == DirtyBits::Clean) {
        return std::string("Clean");
    }
    std::stringstream ss;
    if (dirtyBits & DirtyTransform) {
        ss << "Transform ";
    }
    if (dirtyBits & DirtyParams) {
        ss << "Params ";
    }
    if (dirtyBits & DirtyShadowParams) {
        ss << "ShadowParams ";
    }
    if (dirtyBits & DirtyCollection) {
        ss << "Collection ";
    }
    if (dirtyBits & DirtyResource) {
        ss << "Resource ";
    }
    if (dirtyBits & DirtyInstancer) {
        ss << "Instancer ";
    }
    return TfStringTrimRight(ss.str());
}

PXR_NAMESPACE_CLOSE_SCOPE
