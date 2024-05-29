//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_SHADERFUNCTION_H
#define PXR_IMAGING_HGI_METAL_SHADERFUNCTION_H

#include "pxr/imaging/hgi/shaderFunction.h"

#include "pxr/imaging/hgiMetal/api.h"

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

class HgiMetal;

///
/// \class HgiMetalShaderFunction
///
/// Metal implementation of HgiShaderFunction
///
class HgiMetalShaderFunction final : public HgiShaderFunction
{
public:
    HGIMETAL_API
    ~HgiMetalShaderFunction() override;

    HGIMETAL_API
    bool IsValid() const override;

    HGIMETAL_API
    std::string const& GetCompileErrors() override;

    HGIMETAL_API
    size_t GetByteSizeOfResource() const override;

    HGIMETAL_API
    uint64_t GetRawResource() const override;

    /// Returns the metal resource id of the shader.
    HGIMETAL_API
    id<MTLFunction> GetShaderId() const;

protected:
    friend class HgiMetal;

    HGIMETAL_API
    HgiMetalShaderFunction(HgiMetal *hgi, HgiShaderFunctionDesc const& desc);

private:
    HgiMetalShaderFunction() = delete;
    HgiMetalShaderFunction & operator=(const HgiMetalShaderFunction&) = delete;
    HgiMetalShaderFunction(const HgiMetalShaderFunction&) = delete;

private:
    std::string _errors;

    id<MTLFunction> _shaderId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
