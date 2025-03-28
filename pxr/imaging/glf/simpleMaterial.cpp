//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file simpleMaterial.cpp

#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/imaging/garch/glApi.h"

PXR_NAMESPACE_OPEN_SCOPE


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

bool
GlfSimpleMaterial::operator ==(GlfSimpleMaterial const & other) const
{
    return (_ambient == other._ambient
        && _diffuse == other._diffuse
        && _specular == other._specular
        && _emission == other._emission
        && _shininess == other._shininess);
}

bool
GlfSimpleMaterial::operator !=(GlfSimpleMaterial const & other) const
{
    return !(*this == other);
}

PXR_NAMESPACE_CLOSE_SCOPE

