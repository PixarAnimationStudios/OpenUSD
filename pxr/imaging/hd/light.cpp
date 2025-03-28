//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
