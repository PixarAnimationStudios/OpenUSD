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
#include "pxr/imaging/hdSt/light.h"

#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/rprimCollection.h"

#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStLight::HdStLight(SdfPath const &id, TfToken const &lightType)
 : HdLight(id)
 , _lightType(lightType)
{
}

HdStLight::~HdStLight()
{
}

GlfSimpleLight
HdStLight::_ApproximateAreaLight(SdfPath const &id, 
                                 HdSceneDelegate *sceneDelegate)
{
    // Get the color of the light
    GfVec3f hdc = sceneDelegate->GetLightParamValue(id, HdStLightTokens->color)
            .Get<GfVec3f>();

    // Extract intensity
    float intensity = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity)
            .Get<float>();

    // Extract the exposure of the light
    float exposure = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->exposure)
            .Get<float>();
    intensity *= powf(2.0f, GfClamp(exposure, -50.0f, 50.0f));

    // Calculate the final color of the light
    GfVec4f c(hdc[0]*intensity, hdc[1]*intensity, hdc[2]*intensity, 1.0f); 

    // Get the transform of the light
    GfMatrix4d transform = _params[HdTokens->transform].Get<GfMatrix4d>();
    GfVec3d hdp = transform.ExtractTranslation();
    GfVec4f p = GfVec4f(hdp[0], hdp[1], hdp[2], 1.0f);

    // Create the Glf Simple Light object that will be used by the rest
    // of the pipeline. No support for shadows for this translated light.
    GlfSimpleLight l;
    l.SetPosition(p);
    l.SetDiffuse(c);
    l.SetHasShadow(false);
    return l;
}

/* virtual */
void
HdStLight::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const &id = GetId();

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    // HdStLight communicates to the scene graph and caches all interesting
    // values within this class. Later on Get() is called from 
    // TaskState (RenderPass) to perform aggregation/pre-computation, 
    // in order to make the shader execution efficient.

    // Change tracking
    HdDirtyBits bits = *dirtyBits;

    // Transform
    if (bits & DirtyTransform) {
        VtValue transform = sceneDelegate->Get(id, HdLightTokens->transform);
        if (transform.IsHolding<GfMatrix4d>()) {
            _params[HdLightTokens->transform] = transform;
        } else {
            _params[HdLightTokens->transform] = GfMatrix4d(1);
        }
    }

    // Lighting Params
    if (bits & DirtyParams) {
        // If it is an area light we will extract the parameters and convert
        // them to a gl friendly representation.
        if (_lightType == HdPrimTypeTokens->simpleLight) {
            _params[HdLightTokens->params] =
                sceneDelegate->Get(id, HdLightTokens->params);
        } else {
            _params[HdLightTokens->params] =
                _ApproximateAreaLight(id, sceneDelegate);
        }
    }

    // Shadow Params
    if (bits & DirtyShadowParams) {
        _params[HdLightTokens->shadowParams] =
                sceneDelegate->Get(id, HdLightTokens->shadowParams);
    }

    // Shadow Collection
    if (bits & DirtyCollection) {
        VtValue vtShadowCollection =
                sceneDelegate->Get(id, HdLightTokens->shadowCollection);

        // Optional
        if (vtShadowCollection.IsHolding<HdRprimCollection>()) {
            HdRprimCollection newCollection =
                vtShadowCollection.UncheckedGet<HdRprimCollection>();

            if (_params[HdLightTokens->shadowCollection] != newCollection) {
                _params[HdLightTokens->shadowCollection] = newCollection;

                HdChangeTracker& changeTracker =
                             sceneDelegate->GetRenderIndex().GetChangeTracker();

                changeTracker.MarkCollectionDirty(newCollection.GetName());
            }

        } else {
            _params[HdLightTokens->shadowCollection] = HdRprimCollection();
        }
    }

    *dirtyBits = Clean;
}

VtValue
HdStLight::Get(TfToken const &token) const
{
    VtValue val;
    TfMapLookup(_params, token, &val);
    return val;
}

/* virtual */
HdDirtyBits
HdStLight::GetInitialDirtyBitsMask() const
{
    // In the case of regular lights we want to sync all dirty bits, but
    // for area lights coming from the scenegraph we just want to extract
    // the Transform and Params for now.
    if (_lightType == HdPrimTypeTokens->simpleLight) {
        return AllDirty;
    } else {
        return (DirtyParams | DirtyTransform);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
