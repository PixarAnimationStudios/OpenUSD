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

#ifndef PXR_IMAGING_HGIVULKAN_SHADERSECTION_H
#define PXR_IMAGING_HGIVULKAN_SHADERSECTION_H

#include "pxr/imaging/hgi/shaderFunctionDesc.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgiVulkan/api.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiVulkanShaderSection
///
/// Base class for Vulkan code sections. The generator holds these
///
class HgiVulkanShaderSection : public HgiShaderSection
{
public:
    HGIVULKAN_API
    explicit HgiVulkanShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes = {},
        const std::string &storageQualifier = std::string(),
        const std::string &defaultValue = std::string());

    HGIVULKAN_API
    ~HgiVulkanShaderSection() override;

    HGIVULKAN_API
    void WriteDeclaration(std::ostream &ss) const override;
    HGIVULKAN_API
    void WriteParameter(std::ostream &ss) const override;

    HGIVULKAN_API
    virtual bool VisitGlobalIncludes(std::ostream &ss);
    HGIVULKAN_API
    virtual bool VisitGlobalMacros(std::ostream &ss);
    HGIVULKAN_API
    virtual bool VisitGlobalStructs(std::ostream &ss);
    HGIVULKAN_API
    virtual bool VisitGlobalMemberDeclarations(std::ostream &ss);
    HGIVULKAN_API
    virtual bool VisitGlobalFunctionDefinitions(std::ostream &ss);

protected:
    const std::string _storageQualifier;

private:
    HgiVulkanShaderSection() = delete;
    HgiVulkanShaderSection & operator=(const HgiVulkanShaderSection&) = delete;
    HgiVulkanShaderSection(const HgiVulkanShaderSection&) = delete;
};

/// \class HgiVulkanMacroShaderSection
///
/// A ShaderSection for defining macros.
/// Accepts raw strings and dumps it to the global scope under includes
///
class HgiVulkanMacroShaderSection final: public HgiVulkanShaderSection
{
public:
    HGIVULKAN_API
    explicit HgiVulkanMacroShaderSection(
        const std::string &macroDeclaration,
        const std::string &macroComment);

    HGIVULKAN_API
    ~HgiVulkanMacroShaderSection() override;

    HGIVULKAN_API
    bool VisitGlobalMacros(std::ostream &ss) override;

private:
    HgiVulkanMacroShaderSection() = delete;
    HgiVulkanMacroShaderSection & operator=(
        const HgiVulkanMacroShaderSection&) = delete;
    HgiVulkanMacroShaderSection(const HgiVulkanMacroShaderSection&) = delete;

    const std::string _macroComment;
};

/// \class HgiVulkanMemberShaderSection
///
/// Declares a member in global scope, for declaring instances of structs, constant
/// params etc - it's quite flexible in it's writing capabilities
///
class HgiVulkanMemberShaderSection final: public HgiVulkanShaderSection
{
public:
    HGIVULKAN_API
    explicit HgiVulkanMemberShaderSection(
        const std::string &identifier,
        const std::string &typeName,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &storageQualifier,
        const std::string &defaultValue = std::string());

    HGIVULKAN_API
    ~HgiVulkanMemberShaderSection() override;

    HGIVULKAN_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

    HGIVULKAN_API
    void WriteType(std::ostream& ss) const override;

private:
    HgiVulkanMemberShaderSection() = delete;
    HgiVulkanMemberShaderSection & operator=(
        const HgiVulkanMemberShaderSection&) = delete;
    HgiVulkanMemberShaderSection(const HgiVulkanMemberShaderSection&) = delete;

    std::string _typeName;
};

/// \class HgiVulkanBlockShaderSection
///
/// For writing out uniform blocks, defines them in the global member declerations.
///
class HgiVulkanBlockShaderSection final: public HgiVulkanShaderSection
{
public:
    HGIVULKAN_API
    explicit HgiVulkanBlockShaderSection(
            const std::string &identifier,
            const HgiShaderFunctionParamDescVector &parameters);

    HGIVULKAN_API
    ~HgiVulkanBlockShaderSection() override;

    HGIVULKAN_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

private:
    const HgiShaderFunctionParamDescVector _parameters;
};

/// \class HgiVulkanMemberShaderSection
///
/// Declares OpenGL textures, and their cross language function
///
class HgiVulkanTextureShaderSection final: public HgiVulkanShaderSection
{
public:
    HGIVULKAN_API
    explicit HgiVulkanTextureShaderSection(
        const std::string &identifier,
        const uint32_t layoutIndex,
        const uint32_t dimensions,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &defaultValue = std::string());

    HGIVULKAN_API
    ~HgiVulkanTextureShaderSection() override;

    HGIVULKAN_API
    void WriteType(std::ostream &ss) const override;

    HGIVULKAN_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;
    HGIVULKAN_API
    bool VisitGlobalFunctionDefinitions(std::ostream &ss) override;

private:
    HgiVulkanTextureShaderSection() = delete;
    HgiVulkanTextureShaderSection & operator=(
        const HgiVulkanTextureShaderSection&) = delete;
    HgiVulkanTextureShaderSection(const HgiVulkanTextureShaderSection&) = delete;

    const uint32_t _dimensions;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
