//
// Copyright 2019 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_GL_MATERIAL_TEXTURE_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_GL_MATERIAL_TEXTURE_ADAPTER_H

/// \file usdImagingGL/materialTextureAdapater.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/usdImaging/usdImaging/materialAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingGLMaterialTextureAdapter
///
/// This adapter inherits most of its behavior from UsdImagingMaterialAdapter to
/// translate material networks. The exception is loading of texture resources,
/// which is implemented in this adapter specifically for HdSt.
///
class UsdImagingGLMaterialTextureAdapter : public UsdImagingMaterialAdapter {
public:
    typedef UsdImagingMaterialAdapter BaseAdapter;

    UsdImagingGLMaterialTextureAdapter()
        : UsdImagingMaterialAdapter()
    {}

    USDIMAGINGGL_API
    virtual ~UsdImagingGLMaterialTextureAdapter();
    
    // ---------------------------------------------------------------------- //
    /// \name Texture resources
    // ---------------------------------------------------------------------- //

    USDIMAGINGGL_API
    HdTextureResourceSharedPtr
    GetTextureResource(
        UsdPrim const& usdPrim, 
        SdfPath const &id, 
        UsdTimeCode time) const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
