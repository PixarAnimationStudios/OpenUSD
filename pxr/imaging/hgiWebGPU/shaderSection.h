//
// Copyright 2022 Pixar
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

#ifndef PXR_IMAGING_HGIWEBGPU_SHADERSECTION_H
#define PXR_IMAGING_HGIWEBGPU_SHADERSECTION_H

#include "pxr/imaging/hgi/shaderFunctionDesc.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiWebGPUShaderSection
///
/// Base class for WebGPU code sections. The generator holds these
///
class HgiWebGPUShaderSection : public HgiBaseGLShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUShaderSection(
            const std::string &identifier,
            const HgiShaderSectionAttributeVector &attributes = {},
            const std::string &storageQualifier = std::string(),
            const std::string &defaultValue = std::string(),
            const std::string &arraySize = std::string(),
            const std::string &blockInstanceIdentifier = std::string())
            : HgiBaseGLShaderSection(identifier, attributes, storageQualifier,
                                     defaultValue, arraySize, blockInstanceIdentifier) {}
};

using HgiWebGPUShaderSectionPtrVector = 
    std::vector<HgiWebGPUShaderSection*>;

/// \class HgiWebGPUMacroShaderSection
///
/// A ShaderSection for defining macros.
/// Accepts raw strings and dumps it to the global scope under includes
///
class HgiWebGPUMacroShaderSection final: public HgiWebGPUShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUMacroShaderSection(
        const std::string &macroDeclaration,
        const std::string &macroComment);

    HGIWEBGPU_API
    ~HgiWebGPUMacroShaderSection() override;

    HGIWEBGPU_API
    bool VisitGlobalMacros(std::ostream &ss) override;

private:
    HgiWebGPUMacroShaderSection() = delete;
    HgiWebGPUMacroShaderSection & operator=(
        const HgiWebGPUMacroShaderSection&) = delete;
    HgiWebGPUMacroShaderSection(const HgiWebGPUMacroShaderSection&) = delete;

    const std::string _macroComment;
};

/// \class HgiWebGPUSamplerShaderSection
///
/// Creates a texture sampler shader
/// section that defines how textures are sampled
///
class HgiWebGPUSamplerShaderSection final : public HgiWebGPUShaderSection
{
public:
    static const uint32_t bindingSet;
    HGIWEBGPU_API
    explicit HgiWebGPUSamplerShaderSection(
            const std::string &textureSharedIdentifier,
            const uint32_t arrayOfSamplersSize,
            const HgiShaderSectionAttributeVector &attributes = {});

    HGIWEBGPU_API
    ~HgiWebGPUSamplerShaderSection() override;

    HGIWEBGPU_API
    void WriteType(std::ostream &ss) const override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;
    HGIWEBGPU_API
    bool VisitGlobalFunctionDefinitions(std::ostream &ss) override;

private:
    HgiWebGPUSamplerShaderSection() = delete;
    HgiWebGPUSamplerShaderSection & operator=(
            const HgiWebGPUSamplerShaderSection&) = delete;
    HgiWebGPUSamplerShaderSection(const HgiWebGPUSamplerShaderSection&) = delete;

    const std::string _textureSharedIdentifier;
    static const std::string _storageQualifier;
};

/// \class HgiWebGPUMemberShaderSection
///
/// Declares OpenGL textures, and their cross language function
///
class HgiWebGPUTextureShaderSection final: public HgiWebGPUShaderSection
{
public:
    static const uint32_t bindingSet;
    HGIWEBGPU_API
    explicit HgiWebGPUTextureShaderSection(
        const std::string &identifier,
        const HgiWebGPUSamplerShaderSection *samplerShaderSectionDependency,
        const uint32_t dimensions,
        const HgiFormat format,
        const HgiShaderTextureType textureType,
        const uint32_t arraySize,
        const bool writable,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &defaultValue = std::string());

    HGIWEBGPU_API
    ~HgiWebGPUTextureShaderSection() override;

    HGIWEBGPU_API
    void WriteType(std::ostream &ss) const override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;
    HGIWEBGPU_API
    bool VisitGlobalFunctionDefinitions(std::ostream &ss) override;

private:
    HgiWebGPUTextureShaderSection() = delete;
    HgiWebGPUTextureShaderSection & operator=(
        const HgiWebGPUTextureShaderSection&) = delete;
    HgiWebGPUTextureShaderSection(const HgiWebGPUTextureShaderSection&) = delete;

    void _WriteTextureType(std::ostream &ss) const;
    void _WriteSampledDataType(std::ostream &ss) const;

    const std::string _samplerSharedIdentifier;
    const uint32_t _dimensions;
    const HgiFormat _format;
    const HgiShaderTextureType _textureType;
    const uint32_t _arraySize;
    const bool _writable;
    const HgiWebGPUSamplerShaderSection* const _samplerShaderSectionDependency;
    static const std::string _storageQualifier;
};

/// \class HgiWebGPUBufferShaderSection
///
/// Declares WebGPU buffers, and their cross language function
///
class HgiWebGPUBufferShaderSection final: public HgiWebGPUShaderSection
{
public:
    static const uint32_t bindingSet;

    HGIWEBGPU_API
    explicit HgiWebGPUBufferShaderSection(
        const std::string &identifier,
        const bool writable,
        const std::string &type,
        const HgiBindingType binding,
        const std::string arraySize,
        const HgiShaderSectionAttributeVector &attributes);

    HGIWEBGPU_API
    ~HgiWebGPUBufferShaderSection() override;

    HGIWEBGPU_API
    void WriteType(std::ostream &ss) const override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

private:
    HgiWebGPUBufferShaderSection() = delete;
    HgiWebGPUBufferShaderSection & operator=(
        const HgiWebGPUBufferShaderSection&) = delete;
    HgiWebGPUBufferShaderSection(const HgiWebGPUBufferShaderSection&) = delete;

    const std::string _type;
    const HgiBindingType _binding;
    const std::string _arraySize;
};

/// \class HgiWebGPUInterstageBlockShaderSection
///
/// Defines and writes out an interstage interface block
///
class HgiWebGPUInterstageBlockShaderSection final: public HgiWebGPUShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUInterstageBlockShaderSection(
        const std::string &blockIdentifier,
        const std::string &blockInstanceIdentifier,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &qualifier,
        const std::string &arraySize,
        const HgiBaseGLShaderSectionPtrVector &members);

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

private:
    HgiWebGPUInterstageBlockShaderSection() = delete;
    HgiWebGPUInterstageBlockShaderSection & operator=(
        const HgiWebGPUInterstageBlockShaderSection&) = delete;
    HgiWebGPUInterstageBlockShaderSection(const HgiWebGPUInterstageBlockShaderSection&) = delete;

    const std::string _qualifier;
    const HgiBaseGLShaderSectionPtrVector _members;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
