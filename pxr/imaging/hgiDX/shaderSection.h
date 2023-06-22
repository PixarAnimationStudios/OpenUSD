
//
// Copyright 2023 Pixar
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

#pragma once

#include "pxr/imaging/hgi/shaderFunctionDesc.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgiDX/api.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiDXShaderSection
///
/// Base class for DX code sections. The generator holds these
///
class HgiDXShaderSection : public HgiShaderSection
{
public:
   HGIDX_API
   explicit HgiDXShaderSection(const std::string& identifier,
                               const HgiShaderSectionAttributeVector& attributes = {},
                               const std::string& defaultValue = std::string(),
                               const std::string& arraySize = std::string(),
                               const std::string& blockInstanceIdentifier = std::string());

   HGIDX_API
   ~HgiDXShaderSection() override;

   HGIDX_API
   virtual bool VisitGlobalIncludes(std::ostream& ss);
   HGIDX_API
   virtual bool VisitGlobalMacros(std::ostream& ss);
   HGIDX_API
   virtual bool VisitGlobalStructs(std::ostream& ss);
   HGIDX_API
   virtual bool VisitGlobalMemberDeclarations(std::ostream& ss);
   HGIDX_API
   virtual bool VisitGlobalFunctionDefinitions(std::ostream& ss);

private:
   HgiDXShaderSection() = delete;
   HgiDXShaderSection& operator=( const HgiDXShaderSection&) = delete;
   HgiDXShaderSection(const HgiDXShaderSection&) = delete;
};


/// <summary>
/// This is for the stage input & output parameters
/// </summary>
class HgiDXParamsShaderSection final : public HgiDXShaderSection
{
public:
   HgiDXParamsShaderSection(const std::string& name);

   struct ParamInfo
   {
      std::string Type;
      std::string Name;
      std::string Semantic;
   };

   void AddParamInfo(const std::string& type, const std::string& name, const std::string& semantic);

   virtual bool VisitGlobalStructs(std::ostream& ss) override;

private:
   std::vector<ParamInfo> _info;
};


/// <summary>
/// Macros - these are just a dumb piece of text
/// </summary>
class HgiDXMacroShaderSection final : public HgiDXShaderSection
{
public:
   HGIDX_API
   explicit HgiDXMacroShaderSection(const std::string& macroDeclaration,
                                    const std::string& macroComment);

   HGIDX_API
   ~HgiDXMacroShaderSection() override;

   HGIDX_API
   bool VisitGlobalMacros(std::ostream& ss) override;

private:
   HgiDXMacroShaderSection() = delete;
   HgiDXMacroShaderSection& operator=( const HgiDXMacroShaderSection&) = delete;
   HgiDXMacroShaderSection(const HgiDXMacroShaderSection&) = delete;

   const std::string _macroComment;
};


/// <summary>
/// global DX buffers
/// </summary>
class HgiDXBufferShaderSection final : public HgiDXShaderSection
{
public:
   HGIDX_API
   explicit HgiDXBufferShaderSection(const std::string& identifier, // buffer name
                                     const std::string& type,
                                     const std::string& arraySize,
                                     const uint32_t registerIndex,
                                     const uint32_t spaceIndex,
                                     bool bWritable);

   HGIDX_API
   ~HgiDXBufferShaderSection() override;

   HGIDX_API
   bool VisitGlobalMemberDeclarations(std::ostream& ss) override;

private:
   HgiDXBufferShaderSection() = delete;
   HgiDXBufferShaderSection& operator=(const HgiDXBufferShaderSection&) = delete;
   HgiDXBufferShaderSection(const HgiDXBufferShaderSection&) = delete;

   const std::string _type;
   const uint32_t _registerIdx;
   const uint32_t _spaceIdx;
   bool _bWritable;
};


/// \class HgiDXMemberShaderSection
///
/// Declares a member in global scope, for declaring instances of structs, constant
/// params etc - it's quite flexible in it's writing capabilities
///
class HgiDXMemberShaderSection final : public HgiDXShaderSection
{
public:
   HGIDX_API
   explicit HgiDXMemberShaderSection(  const std::string& identifier,
                                       const std::string& typeName,
                                       const HgiShaderSectionAttributeVector& attributes,
                                       const std::string& storageQualifier,
                                       const std::string& defaultValue,
                                       const std::string& arraySize);

   HGIDX_API
   ~HgiDXMemberShaderSection() override;

   HGIDX_API
   bool VisitGlobalMemberDeclarations(std::ostream& ss) override;

   HGIDX_API
   void WriteType(std::ostream& ss) const override;

private:
   HgiDXMemberShaderSection() = delete;
   HgiDXMemberShaderSection& operator=( const HgiDXMemberShaderSection&) = delete;
   HgiDXMemberShaderSection(const HgiDXMemberShaderSection&) = delete;

   std::string _typeName;
};

/// \class HgiDXConstantShaderSection
///
/// For writing out "costant" blocks, defines them in the global member declarations.
///
class HgiDXConstantShaderSection final : public HgiDXShaderSection
{
public:
   HGIDX_API
   explicit HgiDXConstantShaderSection(const std::string& identifier,
                                       const HgiShaderFunctionParamDescVector& parameters);

   HGIDX_API
   ~HgiDXConstantShaderSection() override;

   HGIDX_API
   bool VisitGlobalMemberDeclarations(std::ostream& ss) override;

private:
   const HgiShaderFunctionParamDescVector _parameters;
};


/// \class HgiDXTextureShaderSection
///
/// Declares DX textures, and their cross language function
///
class HgiDXTextureShaderSection final : public HgiDXShaderSection
{
public:
   HGIDX_API
   explicit HgiDXTextureShaderSection( const std::string& identifier,
                                       const uint32_t layoutIndex,
                                       const uint32_t dimensions,
                                       const HgiFormat format,
                                       const HgiShaderTextureType textureType,
                                       const uint32_t arraySize,
                                       const bool writable,
                                       const HgiShaderSectionAttributeVector& attributes,
                                       const std::string& defaultValue = std::string());

   HGIDX_API
   ~HgiDXTextureShaderSection() override;

   HGIDX_API
   void WriteType(std::ostream& ss) const override;

   HGIDX_API
   bool VisitGlobalMemberDeclarations(std::ostream& ss) override;
   HGIDX_API
   bool VisitGlobalFunctionDefinitions(std::ostream& ss) override;

private:
   HgiDXTextureShaderSection() = delete;
   HgiDXTextureShaderSection& operator=( const HgiDXTextureShaderSection&) = delete;
   HgiDXTextureShaderSection(const HgiDXTextureShaderSection&) = delete;

   void _WriteSamplerType(std::ostream& ss) const;
   void _WriteSampledDataType(std::ostream& ss) const;

   const uint32_t _dimensions;
   const HgiFormat _format;
   const HgiShaderTextureType _textureType;
   const uint32_t _arraySize;
   const bool _writable;
   static const std::string _storageQualifier;
};

/// \class HgiDXKeywordShaderSection
///
/// Declares reserved DX shader inputs, and their cross language function
///
class HgiDXKeywordShaderSection final : public HgiDXShaderSection
{
public:
   HGIDX_API
   explicit HgiDXKeywordShaderSection( const std::string& identifier,
                                       const std::string& type,
                                       const std::string& keyword);

   HGIDX_API
   ~HgiDXKeywordShaderSection() override;

   HGIDX_API
   void WriteType(std::ostream& ss) const override;

   HGIDX_API
   bool VisitGlobalMemberDeclarations(std::ostream& ss) override;

private:
   HgiDXKeywordShaderSection() = delete;
   HgiDXKeywordShaderSection& operator=( const HgiDXKeywordShaderSection&) = delete;
   HgiDXKeywordShaderSection(const HgiDXKeywordShaderSection&) = delete;

   const std::string _type;
   const std::string _keyword;
};

PXR_NAMESPACE_CLOSE_SCOPE

