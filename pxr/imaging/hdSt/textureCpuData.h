//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HD_ST_TEXTURE_CPU_DATA_H
#define PXR_IMAGING_HD_ST_TEXTURE_CPU_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HgiTextureDesc;

/// \class HdStTextureCpuData
///
/// Represents CPU data that can be stored in a HdStUvTextureObject, mostly,
/// likely during the load phase to be committed to the GPU.
///
class HdStTextureCpuData {
public:
    HDST_API
    virtual ~HdStTextureCpuData();

    /// The metadata of the texture (width, height, ...) including a
    /// pointer to the CPU data (as initialData).
    virtual const HgiTextureDesc &GetTextureDesc() const = 0;

    /// Make GPU generate mipmaps. The number of mipmaps is specified
    /// in the texture descriptor and the mipmaps are generate from
    /// the mip level 0 data.
    virtual bool GetGenerateMipmaps() const = 0;
    
    /// Are the data valid (e.g., false if file could not be found).
    virtual bool IsValid() const = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
