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

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/base/tf/stl.h"

HdLight::HdLight(HdSceneDelegate *delegate, SdfPath const &id)
    : _delegate(delegate)
    , _id(id)
    , _transform(1.0)
    , _params()
    , _shadowParams()
    , _shadowCollection()
{
}

HdLight::~HdLight()
{
}

void
HdLight::Sync()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (not TF_VERIFY(_delegate)) {
        return;  
    } 

    // HdLight communicates to the scene graph and caches all interesting
    // values within this class.

    // later on Get() is called from TaskState (RenderPass) to perform
    // aggregation/pre-computation, in order to make the shader execution
    // efficient.

    HdChangeTracker& changeTracker = 
                                _delegate->GetRenderIndex().GetChangeTracker();
    HdChangeTracker::DirtyBits bits = changeTracker.GetLightDirtyBits(_id);

    if (bits & HdChangeTracker::DirtyTransform) {
        VtValue vtTransform = _delegate->Get(_id, HdTokens->lightTransform);

        // Optional
        if (vtTransform.IsHolding<GfMatrix4d>()) {
            _transform = vtTransform.UncheckedGet<GfMatrix4d>();
        } else {
            _transform.SetIdentity();
        }
    }
    
    if (bits & HdChangeTracker::DirtyParams) {
        VtValue vtLightParams = _delegate->Get(_id, HdTokens->lightParams);

        if (TF_VERIFY(vtLightParams.IsHolding<GlfSimpleLight>())) {
            _params = vtLightParams.UncheckedGet<GlfSimpleLight>();
        } else {
            _params = GlfSimpleLight();
        }
    }

    if (bits & HdChangeTracker::DirtyShadowParams) {
        _shadowParams = _delegate->Get(_id, HdTokens->lightShadowParams);
    }

    if (bits & HdChangeTracker::DirtyCollection) {
        VtValue vtShadowCollection =
                           _delegate->Get(_id, HdTokens->lightShadowCollection);

        // Optional
        if (vtShadowCollection.IsHolding<HdRprimCollection>()) {
            HdRprimCollection newCollection =
                vtShadowCollection.UncheckedGet<HdRprimCollection>();

            if (_shadowCollection != newCollection) {
                _shadowCollection = newCollection;
                changeTracker.MarkCollectionDirty(_shadowCollection.GetName());
            }

        } else {
            _shadowCollection = HdRprimCollection();
        }
    }
}
