//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_SHADERPROGRAM_H
#define PXR_IMAGING_HGI_METAL_SHADERPROGRAM_H

#include "pxr/imaging/hgi/shaderProgram.h"

#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"

#include <vector>

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE


///
/// \class HgiMetalShaderProgram
///
/// Metal implementation of HgiShaderProgram
///
class HgiMetalShaderProgram final : public HgiShaderProgram {
public:
    HGIMETAL_API
    ~HgiMetalShaderProgram() override;

    HGIMETAL_API
    bool IsValid() const override;

    HGIMETAL_API
    std::string const& GetCompileErrors() override;

    HGIMETAL_API
    HgiShaderFunctionHandleVector const& GetShaderFunctions() const override;

    HGIMETAL_API
    size_t GetByteSizeOfResource() const override;

    HGIMETAL_API
    uint64_t GetRawResource() const override;

    HGIMETAL_API
    id<MTLFunction> GetVertexFunction() const {
        return _vertexFunction;
    }

    HGIMETAL_API
    id<MTLFunction> GetFragmentFunction() const {
        return _fragmentFunction;
    }

    HGIMETAL_API
    id<MTLFunction> GetComputeFunction() const {
        return _computeFunction;
    }

    HGIMETAL_API
    id<MTLFunction> GetPostTessVertexFunction() const {
        return _postTessVertexFunction;
    }
    
    HGIMETAL_API
    id<MTLFunction> GetPostTessControlFunction() const {
        return _postTessControlFunction;
    }

protected:
    friend class HgiMetal;

    HGIMETAL_API
    HgiMetalShaderProgram(HgiShaderProgramDesc const& desc);

private:
    HgiMetalShaderProgram() = delete;
    HgiMetalShaderProgram & operator=(const HgiMetalShaderProgram&) = delete;
    HgiMetalShaderProgram(const HgiMetalShaderProgram&) = delete;

private:
    std::string _errors;
    
    id<MTLFunction> _vertexFunction;
    id<MTLFunction> _fragmentFunction;
    id<MTLFunction> _computeFunction;
    id<MTLFunction> _postTessVertexFunction;
    id<MTLFunction> _postTessControlFunction;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
