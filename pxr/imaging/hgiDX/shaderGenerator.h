
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

#include "pxr/imaging/hgi/shaderGenerator.h"
#include "pxr/imaging/hgiDX/shaderSection.h"
#include "pxr/imaging/hgiDX/shaderInfo.h"
#include "pxr/imaging/hgiDX/api.h"

#include <regex>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

using HgiDXShaderSectionUniquePtrVector = std::vector<std::unique_ptr<HgiDXShaderSection>>;

/// \class HgiDXShaderGenerator
///
/// Takes in a descriptor and spits out HLSL code through it's execute function.
///
class HgiDXShaderGenerator final : public HgiShaderGenerator
{
public:
   HGIDX_API
   explicit HgiDXShaderGenerator( Hgi const* hgi, const HgiShaderFunctionDesc& descriptor);

   //This is not commonly consumed by the end user, but is available.
   HGIDX_API
   HgiDXShaderSectionUniquePtrVector* GetShaderSections();

   HGIDX_API
   const std::vector<DXShaderInfo::StageParamInfo>& GetStageInputInfo() const;

   HGIDX_API
   const std::vector<DXShaderInfo::RootParamInfo>& GetStageRootParamInfo() const;

protected:
   HGIDX_API
   void _Execute(std::ostream& ss) override;

private:
   HgiDXShaderGenerator() = delete;
   HgiDXShaderGenerator& operator=(const HgiDXShaderGenerator&) = delete;
   HgiDXShaderGenerator(const HgiDXShaderGenerator&) = delete;

   void _WriteConstantParams(const HgiShaderFunctionParamDescVector& parameters);

   void _WriteTextures(const HgiShaderFunctionTextureDescVector& textures);

   void _WriteBuffers(const HgiShaderFunctionBufferDescVector& buffers);

   void _WriteInOuts(const HgiShaderFunctionParamBlockDescVector& paramBlocksIn, 
                     const HgiShaderFunctionParamBlockDescVector& paramBlocksOut);

   static DXGI_FORMAT _ParamType2DXFormat(std::string strParamType);

   void _ProcessStageInOut(const Hgi* pHgi,
                           const HgiShaderFunctionDesc& stageDesc,
                           bool bIn);

   
   typedef std::function<void(const std::smatch& matsch)> fcDealWithMatch;
   typedef std::function<void(const std::string& match)> fcDealWithMatchStr;

   void _CleanupText(std::string& shaderCode,
                     std::string& textStart,
                     std::string& textEnd, 
                     fcDealWithMatchStr* pFc);

   void _CleanupText(std::string& text, 
                     const std::regex& expr,
                     const std::string& replaceWith,
                     fcDealWithMatch* pFc = nullptr);
   
   void _CleanupText(std::string& text,
                     const std::regex& expr,
                     const std::string& strAdditionalInMatch, // this is like a second check - easier to write than a more complex regex
                     const std::string& replaceWith,
                     fcDealWithMatch* pFc = nullptr);

   void _ExtractStructureFromScope(std::ostream& ss, std::string& shaderCode);

   void _CleanupGeneratedCode(std::ostream& ss, std::string& shaderCode);

   void _WriteScopeStart(std::ostream& ss);
   void _WriteScopeEnd(std::ostream& ss);

   void _WriteScopeStart_OpenScope(std::ostream& ss);
   void _WriteScopeStart_ForwardDeclarations(std::ostream& ss);
   void _WriteScopeStart_DeclareInput(std::ostream& ss);
   void _WriteScopeStart_DeclareOutput(std::ostream& ss);

   void _WriteScopeEnd_ExtraMethods(std::ostream& ss);
   void _WriteScopeEnd_CloseScope(std::ostream& ss);
   void _WriteScopeEnd_StartMainFc(std::ostream& ss);
   void _WriteScopeEnd_InitializeOutputVars(std::ostream& ss);
   void _WriteScopeEnd_SetInputVars(std::ostream& ss);
   void _WriteScopeEnd_CallRealMain(std::ostream& ss);
   void _WriteScopeEnd_GetOutputVars(std::ostream& ss);
   void _WriteScopeEnd_Finish(std::ostream& ss);


   void _GetSemanticName(bool bInParam,
                         const HgiShaderStage& shaderStage,
                         std::string varName,
                         std::string& strShaderSemanticName,
                         std::string& strPipelineInputSemanticName,
                         int& nPipelineInputIndex);

   int _GetGeomShaderNumInValues();
   std::string _GetGeomShaderInVarType();
   std::string _GetGeomShaderOutVarType();
   
   static const char* _GetMacroBlob();
   static const char* _GetPackedTypeDefinitions();



   struct varInfo{
      std::string strType;
      std::string strName;
   };

private:
   HgiDXShaderSectionUniquePtrVector _shaderSections;
   Hgi const* _hgi;

   uint32_t _bufRegisterIdx;

   int _csWorkSizeX = 0;
   int _csWorkSizeY = 0;
   int _csWorkSizeZ = 0;

   bool _bSVPrimitiveIDAsSystem = false;
   DXShaderInfo::StageDXInfo _sdi;
   std::vector<varInfo> _additionalScopeParams;
   std::vector<DXShaderInfo::RootParamInfo> _rootParamInfo;
};

PXR_NAMESPACE_CLOSE_SCOPE
