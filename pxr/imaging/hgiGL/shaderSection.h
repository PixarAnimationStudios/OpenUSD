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

#ifndef PXR_IMAGING_HGIGL_SHADERSECTION_H
#define PXR_IMAGING_HGIGL_SHADERSECTION_H

#include "pxr/imaging/hgi/shaderFunctionDesc.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgiGL/api.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiGLShaderSection
///
/// Base class for OpenGL code sections. The generator holds these
///
class HgiGLShaderSection : public HgiShaderSection
{
public:
    HGIGL_API
    explicit HgiGLShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes = {},
        const std::string &storageQualifier = std::string(),
        const std::string &defaultValue = std::string());

    HGIGL_API
    ~HgiGLShaderSection() override;

    HGIGL_API
    void WriteDeclaration(std::ostream &ss) const override;
    HGIGL_API
    void WriteParameter(std::ostream &ss) const override;

    HGIGL_API
    virtual bool VisitGlobalIncludes(std::ostream &ss);
    HGIGL_API
    virtual bool VisitGlobalMacros(std::ostream &ss);
    HGIGL_API
    virtual bool VisitGlobalStructs(std::ostream &ss);
    HGIGL_API
    virtual bool VisitGlobalMemberDeclarations(std::ostream &ss);
    HGIGL_API
    virtual bool VisitGlobalFunctionDefinitions(std::ostream &ss);

private:
    HgiGLShaderSection() = delete;
    HgiGLShaderSection & operator=(const HgiGLShaderSection&) = delete;
    HgiGLShaderSection(const HgiGLShaderSection&) = delete;

    const std::string _storageQualifier;
};

/// \class HgiGLMacroShaderSection
///
/// A ShaderSection for defining macros.
/// Accepts raw strings and dumps it to the global scope under includes
///
class HgiGLMacroShaderSection final: public HgiGLShaderSection
{
public:
    HGIGL_API
    explicit HgiGLMacroShaderSection(
        const std::string &macroDeclaration,
        const std::string &macroComment);

    HGIGL_API
    ~HgiGLMacroShaderSection() override;

    HGIGL_API
    bool VisitGlobalMacros(std::ostream &ss) override;

private:
    HgiGLMacroShaderSection() = delete;
    HgiGLMacroShaderSection & operator=(
        const HgiGLMacroShaderSection&) = delete;
    HgiGLMacroShaderSection(const HgiGLMacroShaderSection&) = delete;

    const std::string _macroComment;
};

/// \class HgiGLMemberShaderSection
///
/// Declares a member in global scope, for declaring instances of structs, constant
/// params etc - it's quite flexible in it's writing capabilities
///
class HgiGLMemberShaderSection final: public HgiGLShaderSection
{
public:
    HGIGL_API
    explicit HgiGLMemberShaderSection(
        const std::string &identifier,
        const std::string &typeName,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &storageQualifier = std::string(),
        const std::string &defaultValue = std::string());

    HGIGL_API
    ~HgiGLMemberShaderSection() override;

    HGIGL_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

    HGIGL_API
    void WriteType(std::ostream& ss) const override;

private:
    HgiGLMemberShaderSection() = delete;
    HgiGLMemberShaderSection & operator=(
        const HgiGLMemberShaderSection&) = delete;
    HgiGLMemberShaderSection(const HgiGLMemberShaderSection&) = delete;

    std::string _typeName;
};

/// \class HgiGLBlockShaderSection
///
/// For writing out uniform blocks, defines them in the global member declerations.
///
class HgiGLBlockShaderSection final: public HgiGLShaderSection
{
public:
    HGIGL_API
    explicit HgiGLBlockShaderSection(
            const std::string &identifier,
            const HgiShaderFunctionParamDescVector &parameters,
            const uint32_t bindingNo=0);

    HGIGL_API
    ~HgiGLBlockShaderSection() override;

    HGIGL_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;

private:
    const HgiShaderFunctionParamDescVector _parameters;
    const uint32_t _bindingNo;
};

/// \class HgiGLMemberShaderSection
///
/// Declares OpenGL textures, and their cross language function
///
class HgiGLTextureShaderSection final: public HgiGLShaderSection
{
public:
    HGIGL_API
    explicit HgiGLTextureShaderSection(
        const std::string &identifier,
        const uint32_t layoutIndex,
        const uint32_t dimensions,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &defaultValue = std::string());

    HGIGL_API
    ~HgiGLTextureShaderSection() override;

    HGIGL_API
    void WriteType(std::ostream &ss) const override;

    HGIGL_API
    bool VisitGlobalMemberDeclarations(std::ostream &ss) override;
    HGIGL_API
    bool VisitGlobalFunctionDefinitions(std::ostream &ss) override;

private:
    HgiGLTextureShaderSection() = delete;
    HgiGLTextureShaderSection & operator=(
        const HgiGLTextureShaderSection&) = delete;
    HgiGLTextureShaderSection(const HgiGLTextureShaderSection&) = delete;

    const uint32_t _dimensions;
    static const std::string _storageQualifier;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
