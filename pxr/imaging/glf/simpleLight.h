//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_SIMPLE_LIGHT_H
#define PXR_IMAGING_GLF_SIMPLE_LIGHT_H

/// \file glf/simpleLight.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/assetPath.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


class GlfSimpleLight final {
public:
    GLF_API
    GlfSimpleLight(GfVec4f const & position = GfVec4f(0.0, 0.0, 0.0, 1.0));
    GLF_API
    ~GlfSimpleLight();

    GLF_API
    GfMatrix4d const & GetTransform() const;
    GLF_API
    void SetTransform(GfMatrix4d const & mat);

    /// The ambient light value is in the renderer's working color space
    GLF_API
    GfVec4f const & GetAmbient() const;
    GLF_API
    void SetAmbient(GfVec4f const & ambient);

    /// The diffuse light value is in the renderer's working color space
    GLF_API
    GfVec4f const & GetDiffuse() const;
    GLF_API
    void SetDiffuse(GfVec4f const & diffuse);

    /// The specular light value is in the renderer's working color space
    GLF_API
    GfVec4f const & GetSpecular() const;
    GLF_API
    void SetSpecular(GfVec4f const & specular);

    GLF_API
    GfVec4f const & GetPosition() const;
    GLF_API
    void SetPosition(GfVec4f const & position);

    GLF_API
    GfVec3f const & GetSpotDirection() const;
    GLF_API
    void SetSpotDirection(GfVec3f const & spotDirection);

    GLF_API
    float const & GetSpotCutoff() const;
    GLF_API
    void SetSpotCutoff(float const & spotCutoff);

    GLF_API
    float const & GetSpotFalloff() const;
    GLF_API
    void SetSpotFalloff(float const & spotFalloff);

    GLF_API
    GfVec3f const & GetAttenuation() const;
    GLF_API
    void SetAttenuation(GfVec3f const & attenuation);

    GLF_API
    std::vector<GfMatrix4d> const & GetShadowMatrices() const;
    GLF_API
    void SetShadowMatrices(std::vector<GfMatrix4d> const &matrix);

    GLF_API
    int GetShadowResolution() const;
    GLF_API
    void SetShadowResolution(int resolution);

    GLF_API
    float GetShadowBias() const;
    GLF_API
    void SetShadowBias(float bias);

    GLF_API
    float GetShadowBlur() const;
    GLF_API
    void SetShadowBlur(float blur);

    GLF_API
    int GetShadowIndexStart() const;
    GLF_API
    void SetShadowIndexStart(int shadowStart);
    
    GLF_API
    int GetShadowIndexEnd() const;
    GLF_API
    void SetShadowIndexEnd(int shadowEnd);

    GLF_API
    bool HasShadow() const;
    GLF_API
    void SetHasShadow(bool hasShadow);

    GLF_API
    bool HasIntensity() const;
    GLF_API
    void SetHasIntensity(bool hasIntensity);

    GLF_API
    bool IsCameraSpaceLight() const;
    GLF_API

    void SetIsCameraSpaceLight(bool isCameraSpaceLight);
    GLF_API
    SdfPath const & GetID() const;
    GLF_API
    void SetID(SdfPath const & id);

    GLF_API
    bool IsDomeLight() const;
    GLF_API
    void SetIsDomeLight(bool isDomeLight);

    /// The path to the (unprocessed) environment map texture.
    ///
    /// All textures actually used by the dome light (irradiance, prefilter,
    /// brdf) are derived from this texture in a pre-calculation step.
    GLF_API
    const SdfAssetPath &GetDomeLightTextureFile() const;
    GLF_API
    void SetDomeLightTextureFile(const SdfAssetPath &);


    /// \name Post Surface Lighting
    ///
    /// Post surface lighting is evaluated after other surface illumination
    /// and can be used to implement lighting effects beyond those that
    /// correspond to basic positional lighting, e.g. range base fog, etc.
    ///
    /// @{

    GLF_API
    TfToken const & GetPostSurfaceIdentifier() const;
    GLF_API
    std::string const & GetPostSurfaceShaderSource() const;
    GLF_API
    VtUCharArray const & GetPostSurfaceShaderParams() const;

    GLF_API
    void SetPostSurfaceParams(TfToken const & identifier,
                              std::string const & shaderSource,
                              VtUCharArray const & shaderParams);

    /// @}
                       
    GLF_API
    bool operator ==(GlfSimpleLight const &other) const;
    GLF_API
    bool operator !=(GlfSimpleLight const &other) const;

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
    bool _hasIntensity;

    bool _hasShadow;
    int _shadowResolution;
    float _shadowBias;
    float _shadowBlur;
    int _shadowIndexStart;
    int _shadowIndexEnd;

    GfMatrix4d _transform;
    std::vector<GfMatrix4d> _shadowMatrices;

    // domeLight specific parameters 
    bool _isDomeLight;
    // path to texture for dome light.
    SdfAssetPath _domeLightTextureFile;

    TfToken _postSurfaceIdentifier;
    std::string _postSurfaceShaderSource;
    VtUCharArray _postSurfaceShaderParams;

    SdfPath _id;
};

// VtValue requirements
GLF_API
std::ostream& operator<<(std::ostream& out, const GlfSimpleLight& v);

typedef std::vector<class GlfSimpleLight> GlfSimpleLightVector;

// VtValue requirements
GLF_API
std::ostream& operator<<(std::ostream& out, const GlfSimpleLightVector& pv);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
