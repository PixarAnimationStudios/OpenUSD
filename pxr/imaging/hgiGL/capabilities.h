//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_CAPABILITIES_H
#define PXR_IMAGING_HGI_GL_CAPABILITIES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/capabilities.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiGLCapabilities
///
/// Reports the capabilities of the HgiGL device.
///
class HgiGLCapabilities final : public HgiCapabilities
{
public:
    HGIGL_API
    ~HgiGLCapabilities() override;

public:
    friend class HgiGL;

    HGIGL_API
    HgiGLCapabilities();

    HGIGL_API
    int GetAPIVersion() const override;

    HGIGL_API
    int GetShaderVersion() const override;

private:
    void _LoadCapabilities();

    // GL version
    int _glVersion;   // 400 (4.0), 410 (4.1), ...
    
    // GLSL version 
    int _glslVersion; // 400, 410, ...
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
