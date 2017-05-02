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
#ifndef HD_SHADER_H
#define HD_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: Temporary until Rprim moves to HdSt.
typedef boost::shared_ptr<class HdShaderCode> HdShaderCodeSharedPtr;

///
/// Hydra Schema for a shader object.
///
class HdShader : public HdSprim {
public:
    // change tracking for HdShader prim
    enum DirtyBits {
        Clean                 = 0,
        // XXX: Got to skip varying and force sync bits for now
        DirtySurfaceShader    = 1 << 2,
        DirtyParams           = 1 << 3,
        AllDirty              = (DirtySurfaceShader
                                 |DirtyParams)
    };

    HD_API
    virtual ~HdShader();

    /// Obtain the source code for the Surface Shader for this prim from
    /// the scene delegate.
    inline std::string GetSurfaceShaderSource(
        HdSceneDelegate* sceneDelegate) const;

    /// Obtain the source code for the Displacement Shader for this prim from
    /// the scene delegate.
    inline std::string GetDisplacementShaderSource(
        HdSceneDelegate* sceneDelegate) const;

    /// Obtain the collection of shader Primvar descriptions for this prim from
    /// the scene delegate.
    inline HdShaderParamVector GetSurfaceShaderParams(
        HdSceneDelegate* sceneDelegate) const;

    /// Obtain the value of the specified Primvar for this prim from the
    /// scene delegate.
    inline VtValue GetSurfaceShaderParamValue(HdSceneDelegate* sceneDelegate,
                                              TfToken const &paramName) const;

    /// Obtain the scene delegates's globally unique id for the texture
    /// resource identified by textureId.
    inline HdTextureResource::ID GetTextureResourceID(
        HdSceneDelegate* sceneDelegate,
        SdfPath const& textureId) const;

    /// Causes the shader to be reloaded.
    virtual void Reload() = 0;

    // XXX: Temporary until Rprim moves to HdSt.
    // Obtains the render delegate specific representation of the shader.
    virtual HdShaderCodeSharedPtr GetShaderCode() const = 0;

protected:
    HD_API
    HdShader(SdfPath const& id);

private:
    // Class can not be default constructed or copied.
    HdShader()                             = delete;
    HdShader(const HdShader &)             = delete;
    HdShader &operator =(const HdShader &) = delete;
};

inline std::string
HdShader::GetSurfaceShaderSource(HdSceneDelegate* sceneDelegate) const
{
    return sceneDelegate->GetSurfaceShaderSource(GetID());
}

inline std::string
HdShader::GetDisplacementShaderSource(HdSceneDelegate* sceneDelegate) const
{
    return sceneDelegate->GetDisplacementShaderSource(GetID());
}

inline HdShaderParamVector
HdShader::GetSurfaceShaderParams(HdSceneDelegate* sceneDelegate) const
{
    return sceneDelegate->GetSurfaceShaderParams(GetID());
}

inline VtValue
HdShader::GetSurfaceShaderParamValue(HdSceneDelegate* sceneDelegate,
                                          TfToken const &paramName) const
{
    return sceneDelegate->GetSurfaceShaderParamValue(GetID(), paramName);
}

inline HdTextureResource::ID
HdShader::GetTextureResourceID(HdSceneDelegate* sceneDelegate,
                               SdfPath const& textureId) const
{
    return sceneDelegate->GetTextureResourceID(textureId);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_SHADER_H
