//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGI_SHADERGENERATOR_H
#define PXR_IMAGING_HGI_SHADERGENERATOR_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiShaderFunctionDesc;

/// \class HgiShaderGenerator
///
/// Base class for shader function generation
/// Given a descriptor, converts glslfx domain language to concrete shader
/// languages. Can be extended with new types of code sections and specialized
/// for different APIs. It's main role is to make GLSLFX a write once language,
/// no matter the API
///
class HgiShaderGenerator
{
public:
    HGI_API
    virtual ~HgiShaderGenerator();

    // Execute shader generation.
    HGI_API
    void Execute();

    // Return generated shader source.
    HGI_API
    const char *GetGeneratedShaderCode() const;

protected:
    HGI_API
    explicit HgiShaderGenerator(const HgiShaderFunctionDesc &descriptor);

    HGI_API
    virtual void _Execute(std::ostream &ss) = 0;

    HGI_API
    const char *_GetShaderCodeDeclarations() const;

    HGI_API
    const char *_GetShaderCode() const;

    HGI_API
    HgiShaderStage _GetShaderStage() const;

private:
    const HgiShaderFunctionDesc &_descriptor;

    // This is used if the descriptor does not specify a string
    // to be used as the destination for generated output.
    std::string _localGeneratedShaderCode;

    HgiShaderGenerator() = delete;
    HgiShaderGenerator & operator=(const HgiShaderGenerator&) = delete;
    HgiShaderGenerator(const HgiShaderGenerator&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
