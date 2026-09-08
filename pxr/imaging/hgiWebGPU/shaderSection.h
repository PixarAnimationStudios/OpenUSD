//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGIWEBGPU_SHADERSECTION_H
#define PXR_IMAGING_HGIWEBGPU_SHADERSECTION_H

#include "pxr/imaging/hgi/shaderFunctionDesc.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiWebGPUShaderSection
///
/// Base class for WebGPU code sections. The generator holds these
///
class HgiWebGPUShaderSection : public HgiShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes = {},
        const std::string &storageQualifier = std::string(),
        const std::string &defaultValue = std::string(),
        const std::string &arraySize = std::string(),
        const std::string &blockInstanceIdentifier = std::string());

    HGIWEBGPU_API
    ~HgiWebGPUShaderSection() override;

    HGIWEBGPU_API
    virtual void WriteDeclaration(std::ostream &ss) const;
    HGIWEBGPU_API
    virtual void WriteParameter(std::ostream &ss) const;

    HGIWEBGPU_API
    virtual bool VisitGlobalIncludes(std::ostream &ss);
    HGIWEBGPU_API
    virtual bool VisitGlobalMacros(std::ostream &ss);
    HGIWEBGPU_API
    virtual bool VisitGlobalStructs(std::ostream &ss);
    HGIWEBGPU_API
    virtual bool VisitGlobalMemberDeclarations(std::ostream &ss);
    HGIWEBGPU_API
    virtual bool VisitGlobalFunctionDefinitions(std::ostream &ss);

protected:
    const std::string _storageQualifier;

private:
    HgiWebGPUShaderSection() = delete;
    HgiWebGPUShaderSection & operator=(const HgiWebGPUShaderSection&) = delete;
    HgiWebGPUShaderSection(const HgiWebGPUShaderSection&) = delete;

    const std::string _arraySize;
};

using HgiWebGPUShaderSectionPtrVector = std::vector<HgiWebGPUShaderSection*>;

using HgiWebGPUShaderSectionUniquePtrVector =
    std::vector<std::unique_ptr<HgiWebGPUShaderSection>>;

/// \class HgiWebGPUMemberShaderSection
///
/// Declares a member in global scope, for declaring instances of structs,
/// constant params etc - it's quite flexible in it's writing capabilities
///
class ARCH_EXPORT_TYPE HgiWebGPUMemberShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUMemberShaderSection(
        const std::string &identifier,
        const std::string &typeName,
        const HgiInterpolationType interpolation,
        const HgiSamplingType sampling,
        const HgiStorageType storage,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &storageQualifier = std::string(),
        const std::string &defaultValue = std::string(),
        const std::string &arraySize = std::string(),
        const std::string &blockInstanceIdentifier = std::string());

    HGIWEBGPU_API
    ~HgiWebGPUMemberShaderSection() override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

    HGIWEBGPU_API
    void WriteType(std::ostream& ss) const override;

    HGIWEBGPU_API
    void WriteInterpolation(std::ostream& ss) const;

    HGIWEBGPU_API
    void WriteSampling(std::ostream& ss) const;

    HGIWEBGPU_API
    void WriteStorage(std::ostream& ss) const;

private:
    HgiWebGPUMemberShaderSection() = delete;
    HgiWebGPUMemberShaderSection & operator=(
        const HgiWebGPUMemberShaderSection&) = delete;
    HgiWebGPUMemberShaderSection(const HgiWebGPUMemberShaderSection&) = delete;

    std::string _typeName;
    HgiInterpolationType _interpolation;
    HgiSamplingType _sampling;
    HgiStorageType _storage;
};

using HgiWebGPUMemberShaderSectionPtrVector =
    std::vector<HgiWebGPUMemberShaderSection*>;

/// \class HgiWebGPUKeywordShaderSection
///
/// Declares reserved shader inputs, and their cross language function
///
class ARCH_EXPORT_TYPE HgiWebGPUKeywordShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUKeywordShaderSection(
        const std::string &identifier,
        const std::string &type,
        const std::string &keyword);

    HGIWEBGPU_API
    ~HgiWebGPUKeywordShaderSection() override;

    HGIWEBGPU_API
    void WriteType(std::ostream &ss) const override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

private:
    HgiWebGPUKeywordShaderSection() = delete;
    HgiWebGPUKeywordShaderSection & operator=(
        const HgiWebGPUKeywordShaderSection&) = delete;
    HgiWebGPUKeywordShaderSection(const HgiWebGPUKeywordShaderSection&) = delete;

    const std::string _type;
    const std::string _keyword;
};

/// \class HgiWebGPUMacroShaderSection
///
/// A ShaderSection for defining macros.
/// Accepts raw strings and dumps it to the global scope under includes
///
class ARCH_EXPORT_TYPE HgiWebGPUMacroShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUMacroShaderSection(
        const std::string& macroDeclaration, const std::string& macroComment);

    HGIWEBGPU_API
    ~HgiWebGPUMacroShaderSection() override;

    HGIWEBGPU_API
    bool VisitGlobalMacros(std::ostream& ss) override;

private:
    HgiWebGPUMacroShaderSection() = delete;
    HgiWebGPUMacroShaderSection& operator=(
        const HgiWebGPUMacroShaderSection&) = delete;
    HgiWebGPUMacroShaderSection(const HgiWebGPUMacroShaderSection&) = delete;

    const std::string _macroComment;
};

/// \class HgiWebGPUFunctionDefShaderSection
///
/// A ShaderSection for injecting raw GLSL into the global function
/// definitions scope (after member declarations).
///
class ARCH_EXPORT_TYPE HgiWebGPUFunctionDefShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUFunctionDefShaderSection(const std::string& src);

    HGIWEBGPU_API
    ~HgiWebGPUFunctionDefShaderSection() override;

    HGIWEBGPU_API
    bool VisitGlobalFunctionDefinitions(std::ostream& ss) override;

private:
    HgiWebGPUFunctionDefShaderSection() = delete;
    HgiWebGPUFunctionDefShaderSection& operator=(
        const HgiWebGPUFunctionDefShaderSection&) = delete;
    HgiWebGPUFunctionDefShaderSection(
        const HgiWebGPUFunctionDefShaderSection&) = delete;
};

/// \class HgiWebGPUSamplerShaderSection
///
/// Creates a texture sampler shader
/// section that defines how textures are sampled
///
class ARCH_EXPORT_TYPE HgiWebGPUSamplerShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    static const uint32_t bindingSet;
    HGIWEBGPU_API
    explicit HgiWebGPUSamplerShaderSection(
        const std::string& textureSharedIdentifier,
        const uint32_t arrayOfSamplersSize, const bool isShadow = false,
        const HgiShaderSectionAttributeVector& attributes = {});

    HGIWEBGPU_API
    ~HgiWebGPUSamplerShaderSection() override;

    HGIWEBGPU_API
    bool IsArray() const { return _arrayOfSamplersSize > 0; }

    HGIWEBGPU_API
    void WriteType(std::ostream& ss) const override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream& ss) override;
    HGIWEBGPU_API
    bool VisitGlobalFunctionDefinitions(std::ostream& ss) override;

private:
    HgiWebGPUSamplerShaderSection() = delete;
    HgiWebGPUSamplerShaderSection& operator=(
        const HgiWebGPUSamplerShaderSection&) = delete;
    HgiWebGPUSamplerShaderSection(
        const HgiWebGPUSamplerShaderSection&) = delete;

    const uint32_t _arrayOfSamplersSize{};
    const bool _isShadow{};
    const std::string _textureSharedIdentifier;
    static const std::string _storageQualifier;
};

/// \class HgiWebGPUMemberShaderSection
///
/// Declares OpenGL textures, and their cross language function
///
class ARCH_EXPORT_TYPE HgiWebGPUTextureShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    static const uint32_t bindingSet;
    HGIWEBGPU_API
    explicit HgiWebGPUTextureShaderSection(const std::string& identifier,
        const HgiWebGPUSamplerShaderSection* samplerShaderSectionDependency,
        const uint32_t dimensions, const HgiFormat format,
        const HgiShaderTextureType textureType, const uint32_t arraySize,
        const bool writable, const HgiShaderSectionAttributeVector& attributes,
        const std::string& defaultValue = std::string());

    HGIWEBGPU_API
    ~HgiWebGPUTextureShaderSection() override;

    HGIWEBGPU_API
    void WriteType(std::ostream& ss) const override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream& ss) override;
    HGIWEBGPU_API
    bool VisitGlobalFunctionDefinitions(std::ostream& ss) override;

private:
    HgiWebGPUTextureShaderSection() = delete;
    HgiWebGPUTextureShaderSection& operator=(
        const HgiWebGPUTextureShaderSection&) = delete;
    HgiWebGPUTextureShaderSection(
        const HgiWebGPUTextureShaderSection&) = delete;

    void _WriteTextureType(std::ostream& ss) const;
    void _WriteSampledDataType(std::ostream& ss) const;

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
class ARCH_EXPORT_TYPE HgiWebGPUBufferShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    static const uint32_t bindingSet;
    static const uint32_t constantsBindingSet;

    HGIWEBGPU_API
    explicit HgiWebGPUBufferShaderSection(const std::string& identifier,
        const bool writable, const std::string& type,
        const HgiBindingType binding, const std::string arraySize,
        const HgiShaderSectionAttributeVector& attributes);

    HGIWEBGPU_API
    ~HgiWebGPUBufferShaderSection() override;

    HGIWEBGPU_API
    void WriteType(std::ostream& ss) const override;

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream& ss) override;

private:
    HgiWebGPUBufferShaderSection() = delete;
    HgiWebGPUBufferShaderSection& operator=(
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
class ARCH_EXPORT_TYPE HgiWebGPUInterstageBlockShaderSection final
    : public HgiWebGPUShaderSection
{
public:
    HGIWEBGPU_API
    explicit HgiWebGPUInterstageBlockShaderSection(
        const std::string& blockIdentifier,
        const std::string& blockInstanceIdentifier,
        const HgiShaderSectionAttributeVector& attributes,
        const std::string& qualifier, const std::string& arraySize,
        const HgiWebGPUMemberShaderSectionPtrVector& members);

    HGIWEBGPU_API
    bool VisitGlobalMemberDeclarations(std::ostream& ss) override;

private:
    HgiWebGPUInterstageBlockShaderSection() = delete;
    HgiWebGPUInterstageBlockShaderSection& operator=(
        const HgiWebGPUInterstageBlockShaderSection&) = delete;
    HgiWebGPUInterstageBlockShaderSection(
        const HgiWebGPUInterstageBlockShaderSection&) = delete;

    const std::string _qualifier;
    const HgiWebGPUMemberShaderSectionPtrVector _members;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
