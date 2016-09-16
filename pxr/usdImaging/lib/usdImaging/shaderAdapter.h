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

#include "pxr/imaging/hd/shaderParam.h"

/// \class UsdImagingShaderAdapter
/// \brief Interface that provides information that can be used to generate a
/// surface shader in hydra.
///
/// Shaders are identified by a \p shaderPath which is a usd stage path..  All
/// the pure virtual methods in this interface will likely need to be
/// coordinated.  For example, \c GetSurfaceShaderParams declares all the
/// parameters that the shader uses.  Hydra will later query the \c
/// UsdImagingShaderAdapter for the values (\c
/// UsdImagingShaderAdapater::GetSurfaceShaderParamValue).
///
/// \sa UsdImagingIndexProxy::AddShaderAdapter
/// \sa UsdImagingDefaultShaderAdapter
class UsdImagingShaderAdapter
{
public:
    /// \brief Return true if \p usdPath is time varying.
    virtual bool GetSurfaceShaderIsTimeVarying(SdfPath const& usdPath) const = 0;

    /// \brief Returns the glsl source string for the shader at \p usdPath.
    virtual std::string GetSurfaceShaderSource(SdfPath const& usdPath) const = 0;

    /// \brief Returns the parameter names for \p usdPath.
    /// \deprecated This is now replaced by UsdImagingShaderAdapter::GetSurfaceShaderParams
    virtual TfTokenVector GetSurfaceShaderParamNames(SdfPath const& usdPath) const = 0;

    /// \brief Returns the value of param \p paramName for \p usdPath.
    virtual VtValue GetSurfaceShaderParamValue(SdfPath const& usdPath, TfToken const& paramName) const = 0;

    /// \brief Returns the parameters that \p usdPath users.  Hydra will built
    /// the appropriate internal data structures so that these values are
    /// available in the shader.
    ///
    /// \sa HdShaderParam
    virtual HdShaderParamVector GetSurfaceShaderParams(SdfPath const& usdPath) const = 0;

    /// \brief Returns the textures (identified by \c SdfPath objects) that \p
    /// usdPath users.
    virtual SdfPathVector GetSurfaceShaderTextures(SdfPath const& usdPath) const = 0;

};

#endif // USDIMAGING_SHADERADAPTER_H
