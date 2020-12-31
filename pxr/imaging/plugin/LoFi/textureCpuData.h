//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_TEXTURE_CPU_DATA_H
#define PXR_IMAGING_LOFI_TEXTURE_CPU_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HgiTextureDesc;

/// \class LoFiTextureCpuData
///
/// Represents CPU data that can be stored in a LoFiUvTextureObject, mostly,
/// likely during the load phase to be committed to the GPU.
///
class LoFiTextureCpuData {
public:
    LOFI_API
    virtual ~LoFiTextureCpuData();

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
