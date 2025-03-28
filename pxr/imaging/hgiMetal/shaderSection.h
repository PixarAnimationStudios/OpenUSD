//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGIMETAL_SHADERSECTION_H
#define PXR_IMAGING_HGIMETAL_SHADERSECTION_H

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgiMetal/api.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HgiMetalShaderSection
///
/// A base for all metal shader section that provides metal hooks
///
class HgiMetalShaderSection: public HgiShaderSection
{
public:
    HGIMETAL_API
    ~HgiMetalShaderSection() override;

    HGIMETAL_API
    virtual bool VisitGlobalMacros(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitGlobalMemberDeclarations(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitScopeStructs(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitScopeMemberDeclarations(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitScopeFunctionDefinitions(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitScopeConstructorDeclarations(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitScopeConstructorInitialization(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitScopeConstructorInstantiation(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitEntryPointParameterDeclarations(std::ostream &ss);
    HGIMETAL_API
    virtual bool VisitEntryPointFunctionExecutions(
        std::ostream& ss,
        const std::string &scopeInstanceName);

    /// Write out the attribute and also the attribute index in case
    /// either exists
    HGIMETAL_API
    void WriteAttributesWithIndex(std::ostream& ss) const;

    using HgiShaderSection::HgiShaderSection;
};

using HgiMetalShaderSectionUniquePtr = 
    std::unique_ptr<HgiMetalShaderSection>;
using HgiMetalShaderSectionUniquePtrVector = 
    std::vector<HgiMetalShaderSectionUniquePtr>;
using HgiMetalShaderSectionPtrVector = 
    std::vector<HgiMetalShaderSection*>;

/// \class HgiMetalMacroShaderSection
///
/// A ShaderSection for defining macros.
/// Accepts raw strings and dumps it to the global scope under includes
///
class HgiMetalMacroShaderSection final: public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalMacroShaderSection(
        const std::string &macroDeclaration,
        const std::string &macroComment);

    HGIMETAL_API
    bool VisitGlobalMacros(std::ostream &ss) override;

private:
    HgiMetalMacroShaderSection() = delete;
    HgiMetalMacroShaderSection & operator=(
        const HgiMetalMacroShaderSection&) = delete;
    HgiMetalMacroShaderSection(
        const HgiMetalMacroShaderSection&) = delete;

    const std::string _macroComment;
};

/// \class MetalMemberShaderSection
///
/// Defines a member that will be defined within the scope
///
class HgiMetalMemberShaderSection final : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    HgiMetalMemberShaderSection(
        const std::string &identifier,
        const std::string &type,
        const std::string &qualifiers,
        const HgiShaderSectionAttributeVector &attributes = {},
        const std::string arraySize = std::string(),
        const std::string &blockInstanceIdentifier = std::string());

    HGIMETAL_API
    ~HgiMetalMemberShaderSection() override;

    HGIMETAL_API
    void WriteType(std::ostream &ss) const override;
    
    HGIMETAL_API
    void WriteParameter(std::ostream& ss) const override;

    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;

private:
    const std::string _type;
    const std::string _qualifiers;
};

/// \class HgiMetalSamplerShaderSection
///
/// Creates a texture sampler shader
/// section that defines how textures are sampled
///
class HgiMetalSamplerShaderSection final : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    HgiMetalSamplerShaderSection(
        const std::string &textureSharedIdentifier,
        const std::string &parentScopeIdentifier,
        const uint32_t arrayOfSamplersSize,
        const HgiShaderSectionAttributeVector &attributes = {});

    HGIMETAL_API
    void WriteType(std::ostream& ss) const override;
    HGIMETAL_API
    void WriteParameter(std::ostream& ss) const override;

    HGIMETAL_API
    bool VisitScopeConstructorDeclarations(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeConstructorInitialization(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeConstructorInstantiation(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;

private:
    HgiMetalSamplerShaderSection() = delete;
    HgiMetalSamplerShaderSection & operator=(
        const HgiMetalSamplerShaderSection&) = delete;
    HgiMetalSamplerShaderSection(const HgiMetalSamplerShaderSection&) = delete;

    const std::string _textureSharedIdentifier;
    const uint32_t _arrayOfSamplersSize;
    const std::string _parentScopeIdentifier;
};

/// \class HgiMetalTextureShaderSection
///
/// Declares the texture, the sampler and the
/// helper function for cross language sampling
///
class HgiMetalTextureShaderSection final
        : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    HgiMetalTextureShaderSection(
        const std::string &samplerSharedIdentifier,
        const std::string &parentScopeIdentifier,
        const HgiShaderSectionAttributeVector &attributes,
        const HgiMetalSamplerShaderSection *samplerShaderSectionDependency,
        uint32_t dimensions,
        HgiFormat format,
        bool textureArray,
        uint32_t arrayOfTexturesSize,
        bool shadow,
        bool writable,
        const std::string &defaultValue = std::string());

    HGIMETAL_API
    void WriteType(std::ostream& ss) const override;
    HGIMETAL_API
    void WriteParameter(std::ostream& ss) const override;

    HGIMETAL_API
    bool VisitScopeConstructorDeclarations(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeConstructorInitialization(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeConstructorInstantiation(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeFunctionDefinitions(std::ostream &ss) override;

private:
    HgiMetalTextureShaderSection() = delete;
    HgiMetalTextureShaderSection & operator=(
        const HgiMetalTextureShaderSection&) = delete;
    HgiMetalTextureShaderSection(const HgiMetalTextureShaderSection&) = delete;

    const std::string _samplerSharedIdentifier;
    const HgiMetalSamplerShaderSection* const _samplerShaderSectionDependency;
    const uint32_t _dimensionsVar;
    const HgiFormat _format;
    const bool _textureArray;
    const uint32_t _arrayOfTexturesSize;
    const bool _shadow;
    const bool _writable;
    std::string _baseType;
    std::string _returnType;
    const std::string _parentScopeIdentifier;
};

/// \class HgiMetalBufferShaderSection
///
/// Declares the buffer
///
class HgiMetalBufferShaderSection final
        : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    HgiMetalBufferShaderSection(
        const std::string &samplerSharedIdentifier,
        const std::string &parentScopeIdentifier,
        const std::string &type,
        const HgiBindingType binding,
        const bool writable,
        const HgiShaderSectionAttributeVector &attributes);

    // For a dummy padded binding point
    HGIMETAL_API
    HgiMetalBufferShaderSection(
        const std::string &samplerSharedIdentifier,
        const HgiShaderSectionAttributeVector &attributes);

    HGIMETAL_API
    void WriteType(std::ostream& ss) const override;
    HGIMETAL_API
    void WriteParameter(std::ostream& ss) const override;

    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeConstructorDeclarations(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeConstructorInitialization(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeConstructorInstantiation(std::ostream &ss) override;

private:
    HgiMetalBufferShaderSection() = delete;
    HgiMetalBufferShaderSection & operator=(
        const HgiMetalTextureShaderSection&) = delete;
    HgiMetalBufferShaderSection(const HgiMetalBufferShaderSection&) = delete;

    const std::string _type;
    const HgiBindingType _binding;
    const bool _writable;
    const bool _unused;
    const std::string _samplerSharedIdentifier;
    const std::string _parentScopeIdentifier;
};

/// \class HgiMetalStructTypeDeclarationShaderSection
///
/// Defines how to declare a struct type. Takes in members that it will include
///
class HgiMetalStructTypeDeclarationShaderSection final
    : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalStructTypeDeclarationShaderSection(
        const std::string &identifier,
        const HgiMetalShaderSectionPtrVector &members,
        const std::string &templateWrapper = std::string(),
        const std::string &templateWrapperParameters
                            = std::string());

    HGIMETAL_API
    void WriteType(std::ostream &ss) const override;
    HGIMETAL_API
    void WriteDeclaration(std::ostream &ss) const override;
    HGIMETAL_API
    void WriteParameter(std::ostream &ss) const override;
    
    void WriteTemplateWrapper(std::ostream &ss) const;

    //TODO make pointer
    const HgiMetalShaderSectionPtrVector& GetMembers() const;

private:
    HgiMetalStructTypeDeclarationShaderSection() = delete;
    HgiMetalStructTypeDeclarationShaderSection & operator=(
        const HgiMetalStructTypeDeclarationShaderSection&) = delete;
    HgiMetalStructTypeDeclarationShaderSection(
        const HgiMetalStructTypeDeclarationShaderSection&) = delete;

    const HgiMetalShaderSectionPtrVector _members;
    const std::string _templateWrapper;
    const std::string _templateWrapperParameters;
};

/// \class HgiMetalStructInstanceShaderSection
///
/// Allows writing of instances of struct type shader sections
///
class HgiMetalStructInstanceShaderSection : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalStructInstanceShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes,
        HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration,
        const std::string &defaultValue = std::string());

    HGIMETAL_API
    void WriteType(std::ostream &ss) const override;

    HGIMETAL_API
    const HgiMetalStructTypeDeclarationShaderSection*
    GetStructTypeDeclaration() const;

private:
    const HgiMetalStructTypeDeclarationShaderSection * const _structTypeDeclaration;
};

/// \class HgiMetalParameterInputShaderSection
///
/// An input struct to a shader stage
///
class HgiMetalParameterInputShaderSection final
        : public HgiMetalStructInstanceShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalParameterInputShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &addressSpace,
        const bool isPointer,
        HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration);

    HGIMETAL_API
    bool VisitEntryPointParameterDeclarations(std::ostream &ss) override;

    HGIMETAL_API
    bool VisitEntryPointFunctionExecutions(
        std::ostream& ss,
        const std::string &scopeInstanceName) override;

    HGIMETAL_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

    HGIMETAL_API
    void WriteParameter(std::ostream& ss) const override;

private:
    const std::string _addressSpace;
    const bool _isPointer;
};

/// \class HgiMetalArgumentBufferInputShaderSection
///
/// An argument buffer for all bindless buffer bindings to a shader stage
///
class HgiMetalArgumentBufferInputShaderSection final
        : public HgiMetalStructInstanceShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalArgumentBufferInputShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &addressSpace,
        const bool isPointer,
        HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration);

    HGIMETAL_API
    bool VisitEntryPointParameterDeclarations(std::ostream &ss) override;

    HGIMETAL_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

    HGIMETAL_API
    void WriteParameter(std::ostream& ss) const override;

private:
    const std::string _addressSpace;
    const bool _isPointer;
};

/// \class HgiMetalKeywordInputShaderSection
///
/// Defines and writes out special shader keyword inputs
///
class HgiMetalKeywordInputShaderSection final
    : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalKeywordInputShaderSection(
        const std::string &identifier,
        const std::string &type,
        const HgiShaderSectionAttributeVector &attributes);

    HGIMETAL_API
    void WriteType(std::ostream& ss) const override;

    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitEntryPointParameterDeclarations(std::ostream &ss)  override;
    HGIMETAL_API
    bool VisitEntryPointFunctionExecutions(
        std::ostream& ss,
        const std::string &scopeInstanceName) override;

private:
    HgiMetalKeywordInputShaderSection() = delete;
    HgiMetalKeywordInputShaderSection & operator=(
        const HgiMetalKeywordInputShaderSection&) = delete;
    HgiMetalKeywordInputShaderSection(const HgiMetalBufferShaderSection&) = delete;

    const std::string _type;
};

/// \class HgiMetalStageOutputShaderSection
///
/// Defines and writes out shader stage outputs
///
class HgiMetalStageOutputShaderSection final
    : public HgiMetalStructInstanceShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalStageOutputShaderSection(
        const std::string &identifier,
        HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration);

    HGIMETAL_API
    explicit HgiMetalStageOutputShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &addressSpace,
        const bool isPointer,
        HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration);

    HGIMETAL_API
    bool VisitEntryPointFunctionExecutions(
        std::ostream& ss,
        const std::string &scopeInstanceName) override;

    HGIMETAL_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;
};

/// \class HgiMetalInterstageBlockShaderSection
///
/// Defines and writes out an interstage interface block
///
class HgiMetalInterstageBlockShaderSection final
    : public HgiMetalShaderSection
{
public:
    HGIMETAL_API
    explicit HgiMetalInterstageBlockShaderSection(
        const std::string &blockIdentifier,
        const std::string &blockInstanceIdentifier,
        const HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration);

    HGIMETAL_API
    const HgiMetalStructTypeDeclarationShaderSection*
    GetStructTypeDeclaration() const;

    HGIMETAL_API
    bool VisitScopeStructs(std::ostream &ss) override;

    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;

private:
    HgiMetalInterstageBlockShaderSection() = delete;
    HgiMetalInterstageBlockShaderSection & operator=(
        const HgiMetalInterstageBlockShaderSection&) = delete;
    HgiMetalInterstageBlockShaderSection(const HgiMetalInterstageBlockShaderSection&) = delete;

private:
    const HgiMetalStructTypeDeclarationShaderSection * const _structTypeDeclaration;
};

using HgiMetalInterstageBlockShaderSectionPtrVector =
    std::vector<HgiMetalInterstageBlockShaderSection*>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif