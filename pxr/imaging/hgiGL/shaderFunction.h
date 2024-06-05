//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_SHADERFUNCTION_H
#define PXR_IMAGING_HGIGL_SHADERFUNCTION_H

#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgiGL/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

///
/// \class HgiGLShaderFunction
///
/// OpenGL implementation of HgiShaderFunction
///
class HgiGLShaderFunction final : public HgiShaderFunction
{
public:
    HGIGL_API
    ~HgiGLShaderFunction() override;

    HGIGL_API
    bool IsValid() const override;

    HGIGL_API
    std::string const& GetCompileErrors() override;

    HGIGL_API
    size_t GetByteSizeOfResource() const override;

    HGIGL_API
    uint64_t GetRawResource() const override;

    /// Returns the gl resource id of the shader.
    HGIGL_API
    uint32_t GetShaderId() const;

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLShaderFunction(Hgi const* hgi, HgiShaderFunctionDesc const& desc);

private:
    HgiGLShaderFunction() = delete;
    HgiGLShaderFunction & operator=(const HgiGLShaderFunction&) = delete;
    HgiGLShaderFunction(const HgiGLShaderFunction&) = delete;

private:
    std::string _errors;
    uint32_t _shaderId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
