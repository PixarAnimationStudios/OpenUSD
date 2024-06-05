//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DYNAMIC_UV_TEXTURE_IMPLEMENTATION_H
#define PXR_IMAGING_HD_ST_DYNAMIC_UV_TEXTURE_IMPLEMENTATION_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDynamicUvTextureObject;

/// \class HdStDynamicUvTextureImplementation
///
/// Allows external clients to specify how a UV texture is loaded from, e.g.,
/// a file and how it is committed to the GPU.
///
class HdStDynamicUvTextureImplementation
{
public:
    /// Called during the load phase of the Storm texture system
    /// when a texture file is supposed to be loaded to the CPU.
    ///
    /// This method has to be thread-safe.
    ///
    virtual void Load(HdStDynamicUvTextureObject *textureObject) = 0;

    /// Called during the commit phase of the Storm texture system
    /// when the CPU texture is committed to the GPU.
    virtual void Commit(HdStDynamicUvTextureObject *textureObject) = 0;

    /// Queried by, e.g., the material system to determine whether
    /// to use, e.g., the fallback value of a texture node.
    virtual bool IsValid(const HdStDynamicUvTextureObject *textureObject) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
