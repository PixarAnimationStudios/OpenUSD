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
#ifndef GLF_SIMPLE_LIGHTING_CONTEXT_H
#define GLF_SIMPLE_LIGHTING_CONTEXT_H

/// \file glf/simpleLightingContext.h

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"
#include "pxr/imaging/glf/simpleShadowArray.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

TF_DECLARE_WEAK_AND_REF_PTRS(GlfBindingMap);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfUniformBlock);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleLightingContext);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleShadowArray);

class GlfSimpleLightingContext : public TfRefBase, public TfWeakBase {
public:
    typedef GlfSimpleLightingContext This;

    static GlfSimpleLightingContextRefPtr New();

    void SetLights(GlfSimpleLightVector const & lights);
    GlfSimpleLightVector & GetLights();

    // returns the effective number of lights taken into account
    // in composable/compatible shader constraints
    int GetNumLightsUsed() const;

    void SetShadows(GlfSimpleShadowArrayRefPtr const & shadows);
    GlfSimpleShadowArrayRefPtr const & GetShadows();

    void SetMaterial(GlfSimpleMaterial const & material);
    GlfSimpleMaterial const & GetMaterial() const;

    void SetSceneAmbient(GfVec4f const & sceneAmbient);
    GfVec4f const & GetSceneAmbient() const;

    void SetCamera(GfMatrix4d const &worldToViewMatrix,
                   GfMatrix4d const &projectionMatrix);

    void SetUseLighting(bool val);
    bool GetUseLighting() const;

    // returns true if any light has shadow enabled.
    bool GetUseShadows() const;

    void SetUseColorMaterialDiffuse(bool val);
    bool GetUseColorMaterialDiffuse() const;

    void InitUniformBlockBindings(GlfBindingMapPtr const &bindingMap) const;
    void InitSamplerUnitBindings(GlfBindingMapPtr const &bindingMap) const;

    void BindUniformBlocks(GlfBindingMapPtr const &bindingMap);
    void BindSamplers(GlfBindingMapPtr const &bindingMap);

    void UnbindSamplers(GlfBindingMapPtr const &bindingMap);

    void SetStateFromOpenGL();

protected:
    GlfSimpleLightingContext();
    ~GlfSimpleLightingContext();

private:
    GlfSimpleLightVector _lights;
    GlfSimpleShadowArrayRefPtr _shadows;

    GfMatrix4d _worldToViewMatrix;
    GfMatrix4d _projectionMatrix;

    GlfSimpleMaterial _material;
    GfVec4f _sceneAmbient;

    bool _useLighting;
    bool _useShadows;
    bool _useColorMaterialDiffuse;

    GlfUniformBlockRefPtr _lightingUniformBlock;
    GlfUniformBlockRefPtr _shadowUniformBlock;
    GlfUniformBlockRefPtr _materialUniformBlock;

    bool _lightingUniformBlockValid;
    bool _shadowUniformBlockValid;
    bool _materialUniformBlockValid;
};

#endif
