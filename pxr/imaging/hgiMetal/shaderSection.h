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
        const HgiShaderSectionAttributeVector &attributes = {});

    HGIMETAL_API
    ~HgiMetalMemberShaderSection() override;

    HGIMETAL_API
    void WriteType(std::ostream &ss) const override;

    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;

private:
    const std::string _type;
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
        const HgiShaderSectionAttributeVector &attributes = {});

    HGIMETAL_API
    void WriteType(std::ostream& ss) const override;

    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;

private:
    HgiMetalSamplerShaderSection() = delete;
    HgiMetalSamplerShaderSection & operator=(
        const HgiMetalSamplerShaderSection&) = delete;
    HgiMetalSamplerShaderSection(const HgiMetalSamplerShaderSection&) = delete;
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
        const HgiShaderSectionAttributeVector &attributes,
        const HgiMetalSamplerShaderSection *samplerShaderSectionDependency,
        const std::string &defaultValue = std::string(),
        uint32_t dimension = 2);

    HGIMETAL_API
    void WriteType(std::ostream& ss) const override;

    HGIMETAL_API
    bool VisitScopeMemberDeclarations(std::ostream &ss) override;
    HGIMETAL_API
    bool VisitScopeFunctionDefinitions(std::ostream &ss) override;

private:
    HgiMetalTextureShaderSection() = delete;
    HgiMetalTextureShaderSection & operator=(
        const HgiMetalTextureShaderSection&) = delete;
    HgiMetalTextureShaderSection(const HgiMetalTextureShaderSection&) = delete;

    const HgiMetalSamplerShaderSection* const _samplerShaderSectionDependency;
    const uint32_t _dimensionsVar;
    const std::string _samplerSharedIdentifier;
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
        const HgiMetalShaderSectionPtrVector &members);

    HGIMETAL_API
    void WriteType(std::ostream &ss) const override;
    HGIMETAL_API
    void WriteDeclaration(std::ostream &ss) const override;
    HGIMETAL_API
    void WriteParameter(std::ostream &ss) const override;

    //TODO make pointer
    const HgiMetalShaderSectionPtrVector& GetMembers() const;

private:
    HgiMetalStructTypeDeclarationShaderSection() = delete;
    HgiMetalStructTypeDeclarationShaderSection & operator=(
        const HgiMetalStructTypeDeclarationShaderSection&) = delete;
    HgiMetalStructTypeDeclarationShaderSection(
        const HgiMetalStructTypeDeclarationShaderSection&) = delete;

    const HgiMetalShaderSectionPtrVector _members;    
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


/// \class HgiMetalArgumentBufferInputShaderSection
///
/// An input struct to a shader stage
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
