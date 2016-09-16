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
#ifndef USDIMAGING_DEFAULTSHADERADAPTER_H
#define USDIMAGING_DEFAULTSHADERADAPTER_H

#include "pxr/usdImaging/usdImaging/shaderAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

/// \class UsdImagingDefaultShaderAdapter
/// \brief An implementation of the shader adapter that uses Usd objects to
/// build up a shading network.
///
/// Currently, in the absence of any other shaderAdapter registered the delegate
/// will default to using this implementation.
class UsdImagingDefaultShaderAdapter : public UsdImagingShaderAdapter
{
public:
    UsdImagingDefaultShaderAdapter(UsdImagingDelegate* delegate);

    /// \brief Traverses the shading prims and if any of the attributes are time
    /// varying, returns true.
    bool GetSurfaceShaderIsTimeVarying(SdfPath const& usdPath) const override;

    /// \brief Gets the shading source via the \c UsdHydrShader schema.
    std::string GetSurfaceShaderSource(SdfPath const& usdPath) const override;

    TfTokenVector GetSurfaceShaderParamNames(SdfPath const& usdPath) const override;

    VtValue GetSurfaceShaderParamValue(SdfPath const& usdPath, TfToken const& paramName) const override;
    HdShaderParamVector GetSurfaceShaderParams(SdfPath const& usdPath) const override;
    SdfPathVector GetSurfaceShaderTextures(SdfPath const& usdPath) const override;

private:
    UsdImagingDelegate* _delegate;
};

#endif // USDIMAGING_DEFAULTSHADERADAPTER_H
