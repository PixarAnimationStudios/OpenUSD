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
#ifndef PXR_IMAGING_HD_ST_MATERIAL_NETWORK_H
#define PXR_IMAGING_HD_ST_MATERIAL_NETWORK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/material.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef std::unique_ptr<class HioGlslfx> HioGlslfxUniquePtr;


/// \class HdStMaterialNetwork
///
/// Helps HdStMaterial process a Hydra material network into shader source code
/// and parameters values.
class HdStMaterialNetwork {
public:
    HDST_API
    HdStMaterialNetwork();

    HDST_API
    virtual ~HdStMaterialNetwork();

    /// Process a material network topology and extract all the information we
    /// need from it.
    HDST_API
    void ProcessMaterialNetwork(
        SdfPath const& materialId,
        HdMaterialNetworkMap const& hdNetworkMap);

    HDST_API
    TfToken const& GetMaterialTag() const;

    HDST_API
    std::string const& GetFragmentCode() const;

    HDST_API
    std::string const& GetGeometryCode() const;

    HDST_API
    VtDictionary const& GetMetadata() const;

    HDST_API
    HdMaterialParamVector const& GetMaterialParams() const;

    /// Primarily used during reload of the material (glslfx may have changed)
    HDST_API
    void ClearGlslfx();

private:
    TfToken _materialTag;
    std::string _fragmentSource;
    std::string _geometrySource;
    VtDictionary _materialMetadata;
    HdMaterialParamVector _materialParams;
    HioGlslfxUniquePtr _surfaceGfx;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
