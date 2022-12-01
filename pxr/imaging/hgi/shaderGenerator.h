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
