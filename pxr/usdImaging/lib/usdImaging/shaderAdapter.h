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
#ifndef USDIMAGING_SHADERADAPTER_H
#define USDIMAGING_SHADERADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/shaderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingDelegate;

/// \class UsdImagingShaderAdapter
/// \brief Provides information that can be used to generate a surface shader in
/// hydra.
class UsdImagingShaderAdapter {
public:
    USDIMAGING_API
    UsdImagingShaderAdapter(UsdImagingDelegate* delegate);

    /// \brief Traverses the shading prims and if any of the attributes are time
    /// varying, returns true.
    USDIMAGING_API
    bool GetSurfaceShaderIsTimeVarying(SdfPath const& usdPath) const;

    /// \brief Returns the surface source string for the shader at \p usdPath.
    /// 
    /// This obtains the shading source via the \c UsdHydraShader schema.
    USDIMAGING_API
    std::string GetSurfaceShaderSource(SdfPath const& usdPath) const;

    /// \brief Returns the displacement source string for the shader at \p usdPath.
    /// 
    /// This obtains the shading source via the \c UsdHydraShader schema.
    USDIMAGING_API
    std::string GetDisplacementShaderSource(SdfPath const& usdPath) const;

    /// \brief Returns the value of param \p paramName for \p usdPath.
    USDIMAGING_API
    VtValue GetSurfaceShaderParamValue(SdfPath const& usdPath, TfToken const& paramName) const;

    /// \brief Returns the parameters that \p usdPath users.  Hydra will built
    /// the appropriate internal data structures so that these values are
    /// available in the shader.
    ///
    /// \sa HdShaderParam
    USDIMAGING_API
    HdShaderParamVector GetSurfaceShaderParams(SdfPath const& usdPath) const;

    /// \brief Returns the textures (identified by \c SdfPath objects) that \p
    /// usdPath users.
    USDIMAGING_API
    SdfPathVector GetSurfaceShaderTextures(SdfPath const& usdPath) const;

private:
    /// \brief Returns the source string for the specified shader
    /// terminal at \p usdPath.
    /// 
    /// This obtains the shading source via the \c UsdHydraShader schema.
    std::string _GetShaderSource(SdfPath const& usdPath,
                                 TfToken const& shaderType) const;

private:
    UsdImagingDelegate* _delegate;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGING_SHADERADAPTER_H
