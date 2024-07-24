//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_SIMPLE_MATERIAL_H
#define PXR_IMAGING_GLF_SIMPLE_MATERIAL_H

/// \file glf/simpleMaterial.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE


class GlfSimpleMaterial final {
public:
    GLF_API
    GlfSimpleMaterial();
    GLF_API
    ~GlfSimpleMaterial();

    GLF_API
    GfVec4f const & GetAmbient() const;
    GLF_API
    void SetAmbient(GfVec4f const & ambient);

    GLF_API
    GfVec4f const & GetDiffuse() const;
    GLF_API
    void SetDiffuse(GfVec4f const & diffuse);

    GLF_API
    GfVec4f const & GetSpecular() const;
    GLF_API
    void SetSpecular(GfVec4f const & specular);

    GLF_API
    GfVec4f const & GetEmission() const;
    GLF_API
    void SetEmission(GfVec4f const & specular);

    GLF_API
    double GetShininess() const;
    GLF_API
    void SetShininess(double specular);

    GLF_API
    bool operator ==(GlfSimpleMaterial const & other) const;
    GLF_API
    bool operator !=(GlfSimpleMaterial const & other) const;

private:
    GfVec4f _ambient;
    GfVec4f _diffuse;
    GfVec4f _specular;
    GfVec4f _emission;
    double _shininess;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
