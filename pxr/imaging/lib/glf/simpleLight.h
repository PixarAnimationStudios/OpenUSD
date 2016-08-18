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

#include "pxr/imaging/glf/api.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/usd/sdf/path.h"

class GlfSimpleLight {
public:
    GLF_API GlfSimpleLight(GfVec4f const & position = GfVec4f(0.0, 0.0, 0.0, 1.0));
    GLF_API virtual ~GlfSimpleLight();

    GLF_API GfMatrix4d const & GetTransform() const;
    GLF_API void SetTransform(GfMatrix4d const & mat);

    GLF_API GfVec4f const & GetAmbient() const;
    GLF_API void SetAmbient(GfVec4f const & ambient);

    GLF_API GfVec4f const & GetDiffuse() const;
    GLF_API void SetDiffuse(GfVec4f const & diffuse);

    GLF_API GfVec4f const & GetSpecular() const;
    GLF_API void SetSpecular(GfVec4f const & specular);

    GLF_API GfVec4f const & GetPosition() const;
    GLF_API void SetPosition(GfVec4f const & position);

    GLF_API GfVec3f const & GetSpotDirection() const;
    GLF_API void SetSpotDirection(GfVec3f const & spotDirection);

    GLF_API float const & GetSpotCutoff() const;
    GLF_API void SetSpotCutoff(float const & spotCutoff);

    GLF_API float const & GetSpotFalloff() const;
    GLF_API void SetSpotFalloff(float const & spotFalloff);

    GLF_API GfVec3f const & GetAttenuation() const;
    GLF_API void SetAttenuation(GfVec3f const & attenuation);

    GLF_API GfMatrix4d const & GetShadowMatrix() const;
    GLF_API void SetShadowMatrix(GfMatrix4d const &matrix);

    GLF_API int GetShadowResolution() const;
    GLF_API void SetShadowResolution(int resolution);

    GLF_API float GetShadowBias() const;
    GLF_API void SetShadowBias(float bias);

    GLF_API float GetShadowBlur() const;
    GLF_API void SetShadowBlur(float blur);

    GLF_API int GetShadowIndex() const;
    GLF_API void SetShadowIndex(int shadowIndex);

    GLF_API bool HasShadow() const;
    GLF_API void SetHasShadow(bool hasShadow);

    GLF_API bool IsCameraSpaceLight() const;
    GLF_API void SetIsCameraSpaceLight(bool isCameraSpaceLight);

    GLF_API SdfPath const & GetID() const;
    GLF_API void SetID(SdfPath const & id);

    GLF_API virtual void Draw() const;

    GLF_API bool operator ==(GlfSimpleLight const & other) const;
    GLF_API bool operator !=(GlfSimpleLight const & other) const;

private:
	GLF_API
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
GLF_API
std::ostream& operator<<(std::ostream& out, const GlfSimpleLight& v);

typedef std::vector<class GlfSimpleLight> GlfSimpleLightVector;

// VtValue requirements
GLF_API
std::ostream& operator<<(std::ostream& out, const GlfSimpleLightVector& pv);

#endif
