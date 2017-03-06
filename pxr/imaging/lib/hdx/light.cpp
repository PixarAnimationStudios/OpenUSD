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
#include "pxr/imaging/hdx/light.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/rprimCollection.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(HdxLightTokens, HDX_LIGHT_TOKENS);

HdxLight::HdxLight(SdfPath const &id)
 : HdSprim(id)
{
}

HdxLight::~HdxLight()
{
}

/* virtual */
void
HdxLight::Sync(HdSceneDelegate *sceneDelegate,
               HdRenderParam   *renderParam,
               HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const &id = GetID();

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    // HdxLight communicates to the scene graph and caches all interesting
    // values within this class.

    // later on Get() is called from TaskState (RenderPass) to perform
    // aggregation/pre-computation, in order to make the shader execution
    // efficient.

    HdDirtyBits bits = *dirtyBits;

    // Change tracking

    // Transform
    if (bits & DirtyTransform) {
        VtValue transform = sceneDelegate->Get(id, HdxLightTokens->transform);
        if (transform.IsHolding<GfMatrix4d>()) {
            _params[HdxLightTokens->transform] = transform;
        } else {
            _params[HdxLightTokens->transform] = GfMatrix4d(1);
        }
    }

    // Lighting Params
    if (bits & DirtyParams) {
        _params[HdxLightTokens->params] =
                sceneDelegate->Get(id, HdxLightTokens->params);
    }

    // Shadow Params
    if (bits & DirtyShadowParams) {
        _params[HdxLightTokens->shadowParams] =
                sceneDelegate->Get(id, HdxLightTokens->shadowParams);
    }

    // Shadow Collection
    if (bits & DirtyCollection) {
        VtValue vtShadowCollection =
                sceneDelegate->Get(id, HdxLightTokens->shadowCollection);

        // Optional
        if (vtShadowCollection.IsHolding<HdRprimCollection>()) {
            HdRprimCollection newCollection =
                vtShadowCollection.UncheckedGet<HdRprimCollection>();

            if (_params[HdxLightTokens->shadowCollection] != newCollection) {
                _params[HdxLightTokens->shadowCollection] = newCollection;

                HdChangeTracker& changeTracker =
                             sceneDelegate->GetRenderIndex().GetChangeTracker();

                changeTracker.MarkCollectionDirty(newCollection.GetName());
            }

        } else {
            _params[HdxLightTokens->shadowCollection] = HdRprimCollection();
        }
    }

    *dirtyBits = Clean;
}

/* virtual */
VtValue
HdxLight::Get(TfToken const &token) const
{
    VtValue val;
    TfMapLookup(_params, token, &val);
    return val;
}

/* virtual */
HdDirtyBits
HdxLight::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE

