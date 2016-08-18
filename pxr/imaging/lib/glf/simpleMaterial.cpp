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
/// \file simpleMaterial.cpp

#include "pxr/imaging/garch/gl.h"
#include "pxr/imaging/glf/simpleMaterial.h"


GlfSimpleMaterial::GlfSimpleMaterial() :
    _ambient(0.2, 0.2, 0.2, 1),
    _diffuse(0.8, 0.8, 0.8, 1),
    _specular(0.5, 0.5, 0.5, 1),
    _emission(0, 0, 0, 1),
    _shininess(32.0)
{
}

GlfSimpleMaterial::~GlfSimpleMaterial()
{
}

GfVec4f const &
GlfSimpleMaterial::GetAmbient() const
{
    return _ambient;
}

void
GlfSimpleMaterial::SetAmbient(GfVec4f const & ambient)
{
    _ambient = ambient;
}


GfVec4f const &
GlfSimpleMaterial::GetDiffuse() const
{
    return _diffuse;
}

void
GlfSimpleMaterial::SetDiffuse(GfVec4f const & diffuse)
{
    _diffuse = diffuse;
}


GfVec4f const &
GlfSimpleMaterial::GetSpecular() const
{
    return _specular;
}

void
GlfSimpleMaterial::SetSpecular(GfVec4f const & specular)
{
    _specular = specular;
}

GfVec4f const &
GlfSimpleMaterial::GetEmission() const
{
    return _emission;
}

void
GlfSimpleMaterial::SetEmission(GfVec4f const & emission)
{
    _emission = emission;
}

double
GlfSimpleMaterial::GetShininess() const
{
    return _shininess;
}

void
GlfSimpleMaterial::SetShininess(double shininess)
{
    _shininess = shininess;
}

void
GlfSimpleMaterial::Bind()
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, GetAmbient().GetArray());
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, GetDiffuse().GetArray());
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, GetSpecular().GetArray());
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, GetEmission().GetArray());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, GetShininess());
}

void
GlfSimpleMaterial::Unbind()
{
}

bool
GlfSimpleMaterial::operator ==(GlfSimpleMaterial const & other) const
{
    return (_ambient == other._ambient
        and _diffuse == other._diffuse
        and _specular == other._specular
        and _emission == other._emission
        and _shininess == other._shininess);
}

bool
GlfSimpleMaterial::operator !=(GlfSimpleMaterial const & other) const
{
    return not (*this == other);
}
