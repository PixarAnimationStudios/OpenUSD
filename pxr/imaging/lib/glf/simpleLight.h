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
#ifndef GLF_SIMPLE_LIGHT_H
#define GLF_SIMPLE_LIGHT_H

/// \file glf/simpleLight.h

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/usd/sdf/path.h"

class GlfSimpleLight {
public:
    GlfSimpleLight(GfVec4f const & position = GfVec4f(0.0, 0.0, 0.0, 1.0));
    virtual ~GlfSimpleLight();

    GfMatrix4d const & GetTransform() const;
    void SetTransform(GfMatrix4d const & mat);

    GfVec4f const & GetAmbient() const;
    void SetAmbient(GfVec4f const & ambient);

    GfVec4f const & GetDiffuse() const;
    void SetDiffuse(GfVec4f const & diffuse);

    GfVec4f const & GetSpecular() const;
    void SetSpecular(GfVec4f const & specular);

    GfVec4f const & GetPosition() const;
    void SetPosition(GfVec4f const & position);

    GfVec3f const & GetSpotDirection() const;
    void SetSpotDirection(GfVec3f const & spotDirection);

    float const & GetSpotCutoff() const;
    void SetSpotCutoff(float const & spotCutoff);

    float const & GetSpotFalloff() const;
    void SetSpotFalloff(float const & spotFalloff);

    GfVec3f const & GetAttenuation() const;
    void SetAttenuation(GfVec3f const & attenuation);

    GfMatrix4d const & GetShadowMatrix() const;
    void SetShadowMatrix(GfMatrix4d const &matrix);

    int GetShadowResolution() const;
    void SetShadowResolution(int resolution);

    float GetShadowBias() const;
    void SetShadowBias(float bias);

    float GetShadowBlur() const;
    void SetShadowBlur(float blur);

    int GetShadowIndex() const;
    void SetShadowIndex(int shadowIndex);

    bool HasShadow() const;
    void SetHasShadow(bool hasShadow);

    bool IsCameraSpaceLight() const;
    void SetIsCameraSpaceLight(bool isCameraSpaceLight);

    SdfPath const & GetID() const;
    void SetID(SdfPath const & id);

    virtual void Draw() const;

    bool operator ==(GlfSimpleLight const & other) const;
    bool operator !=(GlfSimpleLight const & other) const;

private:
    friend std::ostream & operator <<(std::ostream &out,
                                      const GlfSimpleLight& v);
    GfVec4f _ambient;
    GfVec4f _diffuse;
    GfVec4f _specular;
    GfVec4f _position;
    GfVec3f _spotDirection;
    float _spotCutoff;
    float _spotFalloff;
    GfVec3f _attenuation;
    bool _isCameraSpaceLight;

    bool _hasShadow;
    int _shadowResolution;
    float _shadowBias;
    float _shadowBlur;
    int _shadowIndex;

    GfMatrix4d _transform;
    GfMatrix4d _shadowMatrix;

    SdfPath _id;
};

// VtValue requirements
std::ostream& operator<<(std::ostream& out, const GlfSimpleLight& v);

typedef std::vector<class GlfSimpleLight> GlfSimpleLightVector;

// VtValue requirements
std::ostream& operator<<(std::ostream& out, const GlfSimpleLightVector& pv);

#endif
