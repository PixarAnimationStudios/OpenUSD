//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_SHADERPROGRAM_H
#define PXR_IMAGING_HGIGL_SHADERPROGRAM_H

#include "pxr/imaging/hgi/shaderProgram.h"

#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/shaderFunction.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


///
/// \class HgiGLShaderProgram
///
/// OpenGL implementation of HgiShaderProgram
///
class HgiGLShaderProgram final : public HgiShaderProgram
{
public:
    HGIGL_API
    ~HgiGLShaderProgram() override;

    HGIGL_API
    bool IsValid() const override;

    HGIGL_API
    std::string const& GetCompileErrors() override;

    HGIGL_API
    HgiShaderFunctionHandleVector const& GetShaderFunctions() const override;

    HGIGL_API
    size_t GetByteSizeOfResource() const override;

    HGIGL_API
    uint64_t GetRawResource() const override;

    /// Returns the gl resource id of the program.
    HGIGL_API
    uint32_t GetProgramId() const;

    /// Returns the gl resource for the uniform block of this shader program.
    /// This uniform block is used to store some per-shader values, such as
    /// indices or offsets into other buffers.
    /// See also Hgi::SetConstantValues.
    /// 'sizeHint' is used to store the byte size of the uniform buffer, but
    /// this fn does not actually allocate the data storage for the buffer.
    HGIGL_API
    uint32_t GetUniformBuffer(size_t sizeHint);

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLShaderProgram(HgiShaderProgramDesc const& desc);

private:
    HgiGLShaderProgram() = delete;
    HgiGLShaderProgram & operator=(const HgiGLShaderProgram&) = delete;
    HgiGLShaderProgram(const HgiGLShaderProgram&) = delete;

private:
    std::string _errors;
    uint32_t _programId;
    size_t _programByteSize;
    uint32_t _uniformBuffer;
    size_t _uboByteSize;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
