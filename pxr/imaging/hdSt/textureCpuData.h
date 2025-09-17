//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
