
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

#include "pch.h"
#include "pxr/imaging/hgiDX/shaderGenerator.h"

#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

   struct sysVarInfo
   {
      std::string dataType;
      std::string strSemantics;
      DXGI_FORMAT format;
   };

   const std::map<std::string, sysVarInfo> glSysType2DXSysType{
      { "gl_Position", {"float4", "SV_Position", DXGI_FORMAT_R32_FLOAT}},
      { "gl_FragCoord", {"float4", "SV_Position", DXGI_FORMAT_R32G32B32A32_FLOAT}},

      // clip distance(s) are also significantly different between gl and DX:
      // in gl world it's an array of undefined size
      // in dx world there are just 2 values SV_ClipDistance0, SV_ClipDistance1
      // and it seems like each can store max 4 float each for a max of 8 in total
      //
      // what I could do now, is hope we do not need more than 4 values 
      // or it will result in some serious issues
      // I can also force max 4 / discard if we have more, and warn and see where that leads us
      // see "HgiDXCapabilities"
      { "gl_ClipDistance", {"float4", "SV_ClipDistance0", DXGI_FORMAT_R32_FLOAT}},

      //
      // There is a big issue with SV_PrimitiveID.
      // Experimentally, what happens is:
      // in the most common case when we have both gs and ps stages,
      // using the "SV_PrimitiveID" semantics will almost always result in a stage mismatch
      // saying SV_PrimitiveID uses different hardware registers between gs and ps.
      //
      // Apparently, there are some cases where it works, probably depending on some lucky 
      // data arranging, but there does not seem to be any safe solution to setting this value 
      // so that it works for all cases (e.g. if the definition of something before it changes the error comes back)
      //
      // Reading this article here: 
      // https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html#sv_primitiveid-in-the-pixel-shader
      // makes me think that maybe the correct solution is to:
      // keep it as a system value for gs, and for ps only when we do not have a gs stage 
      // and if we have both gs and ps, just change the semantic to something else so I can avoid the 
      // MS conflicting implementation

      //
      // Unfortunately, with the current way HDStorm builds the shaders, I do not have eanough context 
      // information to deal with the above, but I can implement a quick hack for the moment... 
      // (codegen.cpp - dx, force add gl_PrimitiveID with -1 interstage slot)
      { "gl_PrimitiveID", {"uint", "PRIMITIVEID", DXGI_FORMAT_R32_UINT}},
      //{ "gl_PrimitiveID", {"uint", "SV_PrimitiveID", DXGI_FORMAT_R32_UINT}},
      { "gl_FrontFacing", {"bool", "SV_IsFrontFace", DXGI_FORMAT_R8_UINT}},
      { "gl_FragColor", {"float4", "SV_Target", DXGI_FORMAT_R32G32B32A32_FLOAT}},
      { "gl_FragDepth", {"float", "SV_Depth", DXGI_FORMAT_R32_FLOAT}},
      { "gl_PointSize", {"float", "PSIZE", DXGI_FORMAT_R32_FLOAT}},
      { "gl_VertexID", {"uint", "SV_VertexID", DXGI_FORMAT_R32_UINT}},
      { "gl_InstanceID", {"uint", "SV_InstanceID", DXGI_FORMAT_R32_UINT}},
      { "hd_VertexID", {"uint", "SV_VertexID", DXGI_FORMAT_R32_UINT}},
      { "hd_InstanceID", {"uint", "SV_InstanceID", DXGI_FORMAT_R32_UINT}},
      { "gl_BaryCoordNoPerspNV", {"noperspective float3", "SV_Barycentrics", DXGI_FORMAT_R32G32B32_FLOAT}},
      { "hd_GlobalInvocationID", {"uint3", "SV_DispatchThreadID", DXGI_FORMAT_R32G32B32_UINT}},
      // next one is used for Tesselation Control / Hull Shader
      { "gl_InvocationID", {"uint", "SV_ControlPointID", DXGI_FORMAT_R32_UINT}},
   };

   //
   // taking what OpenGL generator does as example, and extrapolating by the fact that
   // DirectX does not support these at the moment, this is a decent way to handle such things:
   const std::map < std::string, std::string> paramsToDefineAsGlobalConstants{
      {"gl_BaseInstance", "static const uint gl_BaseInstance = 0;\n"},
      {"gl_BaseVertex", "static const uint gl_BaseVertex = 0;\n"},
      {"hd_BaseInstance", "static const uint hd_BaseInstance = 0;\n"},
      {"hd_BaseVertex", "static const uint hd_BaseVertex = 0;\n"},
   };

   const static std::set<std::string> systemProvidedParams{
      "SV_InstanceID",
      "SV_PrimitiveID",
      "SV_VertexID",
      "SV_OutputControlPointID",
      "SV_IsFrontFace",
      "SV_SampleIndex",
      "SV_InputCoverage",
   };

   const static std::set<std::string> systemDestinationParams{
      "SV_ClipDistance0",
      "SV_ClipDistance1",
      "SV_Position",
   };
}

HgiDXShaderGenerator::HgiDXShaderGenerator(Hgi const* hgi,
                                           const HgiShaderFunctionDesc& descriptor)
   : HgiShaderGenerator(descriptor)
   , _hgi(hgi)
   , _bufRegisterIdx(0)
{

   if (descriptor.shaderStage == HgiShaderStageCompute) {

      _csWorkSizeX = descriptor.computeDescriptor.localSize[0];
      _csWorkSizeY = descriptor.computeDescriptor.localSize[1];
      _csWorkSizeZ = descriptor.computeDescriptor.localSize[2];

      //
      // This is a strange check, but I am doing it like OGL does, just to be safe...
      if (_csWorkSizeX == 0 || _csWorkSizeY == 0 || _csWorkSizeZ == 0) {
         _csWorkSizeX = 1;
         _csWorkSizeY = 1;
         _csWorkSizeZ = 1;
      }
   }

   _WriteConstantParams(descriptor.constantParams);
   _WriteTextures(descriptor.textures);

   //
   // Start again with simple params parsing from the data we have
   // without any out->in matching or dealing with in params not provided by out.
   _ProcessStageInOut(_hgi, GetDescriptor(), true);
   _ProcessStageInOut(_hgi, GetDescriptor(), false);
   _WriteInOuts(descriptor.stageInputBlocks, descriptor.stageOutputBlocks);

   _WriteBuffers(descriptor.buffers);
}

void
HgiDXShaderGenerator::_Execute(std::ostream& ss)
{
   //
   // Definitions of hgi & DX specific stuff...
   ss << _GetMacroBlob() << "\n";
   ss << _GetPackedTypeDefinitions() << "\n";

   std::string strShaderCodeDecl = _GetShaderCodeDeclarations();

   //
   // I need to cleanup the hard-coded "packed type definitions", 
   // because not only are most of them not needed, but they are also impossible to fix to compile.
   std::string strStart = "// Alias hgi vec and matrix types to hd.";
   std::string strEnd = "// End alias hgi vec and matrix types to hd.";
   _CleanupText(strShaderCodeDecl, strStart, strEnd, nullptr);
   ss << strShaderCodeDecl;
 
   HgiDXShaderSectionUniquePtrVector* shaderSections = GetShaderSections();
   
   ss << "\n// //////// Global Includes ////////\n";
   for (const std::unique_ptr<HgiDXShaderSection>
        & shaderSection : *shaderSections) {
      shaderSection->VisitGlobalIncludes(ss);
   }

   ss << "\n// //////// Global Macros ////////\n";
   for (const std::unique_ptr<HgiDXShaderSection>
        & shaderSection : *shaderSections) {
      shaderSection->VisitGlobalMacros(ss);
   }

   ss << "\n// //////// Global Structs ////////\n";
   for (const std::unique_ptr<HgiDXShaderSection>
        & shaderSection : *shaderSections) {
      shaderSection->VisitGlobalStructs(ss);
   }

   ss << "\n// //////// Global Member Declarations ////////\n";
   for (const std::unique_ptr<HgiDXShaderSection>
        & shaderSection : *shaderSections) {
      shaderSection->VisitGlobalMemberDeclarations(ss);
   }

   ss << "\n// //////// Global Function Definitions ////////\n";
   for (const std::unique_ptr<HgiDXShaderSection>
        & shaderSection : *shaderSections) {
      shaderSection->VisitGlobalFunctionDefinitions(ss);
   }

   std::string shaderCode = _GetShaderCode();
   _CleanupGeneratedCode(ss, shaderCode);

   ss << "\n";
   ss << "\n// //////// DX scope trick to make global methods and parameters compatible to DX phylosophy\n";
   _WriteScopeStart(ss);


   // Write the previously (HdSt) generated shader code here
   ss << shaderCode;

   _WriteScopeEnd(ss);
}

template <class T>
bool _FindShaderSection(const HgiDXShaderSectionUniquePtrVector& pSections, const std::string& strName)
{
   static_assert(std::is_base_of<HgiShaderSection, T>::value, "T not derived from HgiShaderSection");

   bool bFound = false;

   //
   // Not very efficient, but...
   for (const std::unique_ptr<HgiDXShaderSection>& pShaderSection : pSections) {
      const T* pType = dynamic_cast<const T*>(pShaderSection.get());
      if (nullptr != pType)
      {
         if (pType->GetIdentifier() == strName)
         {
            bFound = true;
            break;
         }
      }
   }

   return bFound;
}

void 
HgiDXShaderGenerator::_WriteInOuts(const HgiShaderFunctionParamBlockDescVector& paramBlocksIn,
                                   const HgiShaderFunctionParamBlockDescVector& paramBlocksOut)
{
   //
   // I still need to define the blocks structs before the stage in/out definitions.
   HgiDXShaderSectionUniquePtrVector* pAllShaderSections = GetShaderSections();
   for (const HgiShaderFunctionParamBlockDesc& paramBlock : paramBlocksIn) {

      const std::string& paramName = paramBlock.blockName;

      //
      // I need this to avoid redefinition of structs for in/out parameters 
      // (e.g. the VertexData, PrimvarData in the gs shader).
      if (!_FindShaderSection<HgiDXParamsShaderSection>(*pAllShaderSections, paramName))
      {
         std::unique_ptr<HgiDXParamsShaderSection>paramsSSBlock = std::make_unique<HgiDXParamsShaderSection>(paramName);

         for (HgiShaderFunctionParamBlockDesc::Member member : paramBlock.members) {
            paramsSSBlock->AddParamInfo(member.type, member.name, "", "");
         }

         GetShaderSections()->push_back(std::move(paramsSSBlock));
      }
   }

   for (const HgiShaderFunctionParamBlockDesc& paramBlock : paramBlocksOut) {

      const std::string& paramName = paramBlock.blockName;

      //
      // I need this to avoid redefinition of structs for in/out parameters 
      // (e.g. the VertexData, PrimvarData in the gs shader).
      if (!_FindShaderSection<HgiDXParamsShaderSection>(*pAllShaderSections, paramName))
      {
         std::unique_ptr<HgiDXParamsShaderSection>paramsSSBlock = std::make_unique<HgiDXParamsShaderSection>(paramName);

         for (HgiShaderFunctionParamBlockDesc::Member member : paramBlock.members) {
            paramsSSBlock->AddParamInfo(member.type, member.name, "", "");
         }

         GetShaderSections()->push_back(std::move(paramsSSBlock));
      }
   }

   //
   // Now take care of my STAGE_IN, OUT definitions.
   std::unique_ptr<HgiDXParamsShaderSection> paramsSSIn = std::make_unique<HgiDXParamsShaderSection>("STAGE_IN");
   std::unique_ptr<HgiDXParamsShaderSection> paramsSSOut = std::make_unique<HgiDXParamsShaderSection>("STAGE_OUT");

   for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageIn)
   {
      paramsSSIn->AddParamInfo(spi.strShaderDataType, 
                               spi.strShaderDataName, 
                               spi.strArrSize,
                               spi.strSemanticName);
   }
   

   if (_sdi.stageOut.size() < 1) {
      _bNoStageOut = true;
   }

   for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageOut)
   {
      paramsSSOut->AddParamInfo(spi.strShaderDataType, 
                                spi.strShaderDataName, 
                                spi.strArrSize,
                                spi.strSemanticName);
   }

   GetShaderSections()->push_back(std::move(paramsSSIn));
   GetShaderSections()->push_back(std::move(paramsSSOut));
}

void
HgiDXShaderGenerator::_WriteBuffers(const HgiShaderFunctionBufferDescVector& buffers)
{
   //Extract buffer descriptors and add appropriate buffer sections.
   for (size_t i = 0; i < buffers.size(); i++) {

      const HgiShaderFunctionBufferDesc& bufferDescription = buffers[i];

      bool bConst = true;
      if ((bufferDescription.binding == HgiBindingType::HgiBindingTypeArray) ||
          (bufferDescription.binding == HgiBindingType::HgiBindingTypeUniformArray) ||
          (bufferDescription.binding == HgiBindingType::HgiBindingTypePointer)) {
          bConst = false;
      }

      DXShaderInfo::RootParamInfo rpi;
      rpi.strName = bufferDescription.nameInShader;
      rpi.strTypeName = bufferDescription.type;
      rpi.nArrSize = bufferDescription.arraySize;
      rpi.bTexture = false;
      rpi.bConst = bConst;
      rpi.bWritable = bufferDescription.writable;
      //rpi.nShaderRegister = _bufRegisterIdx;
      rpi.nShaderRegister = bufferDescription.bindIndex;

      //if(bufferDescription.arraySize > 1) // 0 or 1 is 1
      //   _bufRegisterIdx+=bufferDescription.arraySize;
      //else 
         _bufRegisterIdx++; 

      rpi.nRegisterSpace = 0;
      rpi.nSuggestedBindingIdx = bufferDescription.bindIndex;
      rpi.nBindingIdx = -1;
      rpi.nSamplerBindingIdx = -1;

      _rootParamInfo.push_back(rpi);

      GetShaderSections()->push_back(
         std::make_unique<HgiDXBufferShaderSection>(bufferDescription.nameInShader,
                                                    bufferDescription.type,
                                                    rpi.nShaderRegister,
                                                    rpi.nRegisterSpace,
                                                    bufferDescription.writable,
                                                    bConst));
   }
}

void
HgiDXShaderGenerator::_WriteConstantParams(const HgiShaderFunctionParamDescVector& parameters)
{
   if (parameters.empty()) {
      return;
   }

   //
   // I will put all the passed constant params into one CBV (const struct).

   //
   // First define the struct.
   std::string strConstParamsStructName = "ParamBuffer";
   std::string strConstParamsInstName = "ConstParams";

   std::unique_ptr<HgiDXParamsShaderSection> paramsBlock = std::make_unique<HgiDXParamsShaderSection>(strConstParamsStructName);
   for (const HgiShaderFunctionParamDesc& param : parameters) {
   
      std::string strName = param.nameInShader;
      if (!param.arraySize.empty())
      {
         if (param.arraySize == " ")
            TF_WARN("Variable size array here will probably not work. ");

         strName += "[";
         strName += param.arraySize;
         strName += "]";
      }

      paramsBlock->AddParamInfo(param.type, strName, "", "");
   }
   GetShaderSections()->push_back(std::move(paramsBlock));

   //
   // Now define the buffer.
   DXShaderInfo::RootParamInfo rpi;
   rpi.strName = strConstParamsInstName;
   rpi.strTypeName = strConstParamsStructName;
   rpi.nArrSize = 1;
   rpi.bTexture = false;
   rpi.bConst = true;
   rpi.nShaderRegister = _bufRegisterIdx++;
   rpi.nRegisterSpace = 0;

   //
   // We are basically inventing this buffer right here and a SuggestedBindingIdx of "0" is very likely to overlap another buffer
   // What I can do, is hard-code a value unlikely to overlap something else for this and also use it later when 
   // we bind the "const" data
   rpi.nSuggestedBindingIdx = -2; 
   rpi.nBindingIdx = -1;
   rpi.nSamplerBindingIdx = -1;

   _rootParamInfo.push_back(rpi);

   GetShaderSections()->push_back(
      std::make_unique<HgiDXBufferShaderSection>(strConstParamsInstName,
                                                 strConstParamsStructName,
                                                 rpi.nShaderRegister,
                                                 rpi.nRegisterSpace,
                                                 false,
                                                 true));
}

void
HgiDXShaderGenerator::_WriteTextures(const HgiShaderFunctionTextureDescVector& textures)
{
   size_t binding = 0;
   for (size_t i = 0; i < textures.size(); i++) {
      const HgiShaderFunctionTextureDesc& textureDescription = textures[i];

      DXShaderInfo::RootParamInfo rpi = {};
      rpi.strName = textureDescription.nameInShader;
      rpi.strTypeName = textureDescription.textureType; // this is obviously not useful 
      // textureDescription.format??
      rpi.nArrSize = textureDescription.arraySize;
      rpi.bTexture = true;
      rpi.bConst = false; // or would it also work with "const" - must test someday or read more about it
      rpi.bWritable = textureDescription.writable;
      //rpi.nShaderRegister = _bufRegisterIdx;
      rpi.nShaderRegister = textureDescription.bindIndex;

      _bufRegisterIdx++;

      rpi.nRegisterSpace = 1; // puttiong textures in space 1, to avoid index overlap with buffers
      rpi.nSuggestedBindingIdx = textureDescription.bindIndex;
      rpi.nBindingIdx = -1;
      rpi.nSamplerBindingIdx = -1;

      _rootParamInfo.push_back(rpi);

      //
      // new evolution: for each texture I will also create a sampler root parameter
      // but since I want to do that for each texture, there's no point in declaring anything here
      // I'll just do it at the other end.

      _rootParamInfo.push_back(rpi);

      HgiShaderSectionAttributeVector attrs = {
            HgiShaderSectionAttribute{"binding", std::to_string(binding)} };

      if (textureDescription.writable) {

         /* TODO: clarify DX needs with respect to this,
         probably we need the same: the format and need to translate it
         attrs.insert(attrs.begin(), HgiShaderSectionAttribute{
             HgiGLConversions::GetImageLayoutFormatQualifier(
                 textureDescription.format),
             "" });*/
      }

      GetShaderSections()->push_back(
         std::make_unique<HgiDXTextureShaderSection>(
            textureDescription.nameInShader,
            rpi.nShaderRegister,
            rpi.nRegisterSpace,
            textureDescription.dimensions,
            textureDescription.format,
            textureDescription.textureType,
            textureDescription.arraySize,
            textureDescription.writable,
            attrs));


      //
      // this is sync'd to the "program" code
      // for the moment I am making 2 assumtions:
      //    only input textures need a sampler
      //    and the writable ones are output
      //if (!rpi.bWritable)
      {
         GetShaderSections()->push_back(
            std::make_unique<HgiDXSamplerShaderSection>(textureDescription.nameInShader + "_Sampler", rpi.nShaderRegister, rpi.nRegisterSpace));
      }

      if (textureDescription.arraySize > 0) {
         binding += textureDescription.arraySize;
      }
      else {
         binding++;
      }
   }

   //
   // Going for a more normal solution with one sampler for each texture
   //if (textures.size() > 0)
   //{
   //   //
   //   // I'll add a unique, hard-coded global sampler for now
   //   // will have to review this later and find a better wat to deal with it

   //   //
   //   // TODO: also make the hard-coded name a variable somewhere
   //   GetShaderSections()->push_back(
   //      std::make_unique<HgiDXSamplerShaderSection>("g_Sampler", 0, 0));
   //}

}

HgiDXShaderSectionUniquePtrVector*
HgiDXShaderGenerator::GetShaderSections()
{
   return &_shaderSections;
}

void
HgiDXShaderGenerator::_GetSemanticName(bool bInParam,
                                       const HgiShaderStage& shaderStage,
                                       const std::string& varName,
                                       const std::string& inVarType,
                                       std::string& strShaderSemanticName,
                                       std::string& strPipelineInputSemanticName,
                                       std::string& outVarType,
                                       int& nPipelineInputIndex)
{
   //
   // First check for known system variables.
   auto it = glSysType2DXSysType.find(varName);
   if (it != glSysType2DXSysType.end())
   {
      outVarType = it->second.dataType;
      //
      // I also want to check for the "exception" (hack_PrimitiveID_part2).
      bool bNotHandled = true;
      if (varName == "gl_PrimitiveID")
      {
         if (_bSVPrimitiveIDAsSystem)
         {
            bNotHandled = false;
            strPipelineInputSemanticName = strShaderSemanticName = "SV_PrimitiveID";
            nPipelineInputIndex = 0;
         }
      }

      if (bNotHandled)
      {
         strPipelineInputSemanticName = strShaderSemanticName = it->second.strSemantics;
         nPipelineInputIndex = 0;
      }
   }
   else
   {
      outVarType = inVarType;

      //
      // I will hard-code this entirely for now, 
      // because I am convinced this is an easily solvable problem that will 
      // just burn some dev time. 
      // Besides, seeing here what we need to achieve will actually help with the final, proper implementation.
      std::string strTempName = varName;
      std::transform(strTempName.begin(), strTempName.end(), strTempName.begin(), ::toupper);

      strPipelineInputSemanticName = strShaderSemanticName = strTempName;
      nPipelineInputIndex = 0;

      //
      // Cut out the *_DC_ prefixes.
      size_t nStart = strTempName.find("_DC_");
      if (nStart != std::string::npos)
      {
         strTempName = strTempName.substr(nStart + 1);
         strPipelineInputSemanticName = strShaderSemanticName = strTempName;
      }

      //
      // If it ends with a number, split it out in the strPipelineInputSemanticName, nPipelineInputIndex vars.
      std::regex expr(R"([0-9]+$)");
      std::smatch m;
      if (std::regex_search(strTempName, m, expr))
      {
         int nPos = m.position(0);
         strPipelineInputSemanticName = strTempName.substr(0, nPos);
         nPipelineInputIndex = std::stoi(m[0]);
      }
      else if (strTempName == "INDATA")
      {
         strShaderSemanticName = strPipelineInputSemanticName = "VERT_DATA";
      }
      else if (strTempName == "OUTDATA")
      {
         strShaderSemanticName = strPipelineInputSemanticName = "VERT_DATA";
      }
      else if (strTempName == "INPRIMVARS")
      {
         strShaderSemanticName = strPipelineInputSemanticName = "PRIMVARS";
      }
      else if (strTempName == "OUTPRIMVARS")
      {
         strShaderSemanticName = strPipelineInputSemanticName = "PRIMVARS";
      }
      else
      {
         //
         // Output (color) of ps stage should be SV_Target.
         if ((!bInParam) &&
               (shaderStage == HgiShaderStageBits::HgiShaderStageFragment))
         {
            nStart = strTempName.find("COLOR");
            if (nStart != std::string::npos)
            {
               strPipelineInputSemanticName = strShaderSemanticName = "SV_Target";
            }
         }
      }
   }
}

const std::vector<DXShaderInfo::StageParamInfo>&
HgiDXShaderGenerator::GetStageInputInfo() const
{
   return _sdi.stageIn;
}

const std::vector<DXShaderInfo::RootParamInfo>& 
HgiDXShaderGenerator::GetStageRootParamInfo() const
{
   return _rootParamInfo;
}

struct less_than_spi
{
   inline bool operator() (const DXShaderInfo::StageParamInfo& spi1, const DXShaderInfo::StageParamInfo& spi2)
   {
      bool bAssignedSlot1 = spi1.nInterstageSlot > -1;
      bool bAssignedSlot2 = spi2.nInterstageSlot > -1;

      bool bSystemProvidedParam1 = systemProvidedParams.find(spi1.strSemanticName) != systemProvidedParams.end();
      bool bSystemProvidedParam2 = systemProvidedParams.find(spi2.strSemanticName) != systemProvidedParams.end();

      bool bSystemDestinationParam1 = systemDestinationParams.find(spi1.strSemanticName) != systemDestinationParams.end();
      bool bSystemDestinationParam2 = systemDestinationParams.find(spi2.strSemanticName) != systemDestinationParams.end();

      //
      // for some crazy reason in some cases I get a slot assigned to SV_Position, in others I do not
      // which breaks my sort assumtions and makes stages define parameters in different order
      // (SV_Position vs SV_ClipDistance0, see test "testUsdImagingDXBasicDrawing_rvt_window_3d_cam_lights_flat")
      if (bSystemProvidedParam1 || bSystemDestinationParam1)
         bAssignedSlot1 = false; // I hope this is enough to bring things to normal

      if (bSystemProvidedParam2 || bSystemDestinationParam2)
         bAssignedSlot2 = false; // I hope this is enough to bring things to normal

      //
      // The most important thing I want to do, is move system provided params at the end of the list 
      // to make sure I minimize the chance for the DX stages mismatch error which may appear if we 
      // introduce a system generated parameter in the middle of the list, 
      // before other parameters actually provided by the stage out before.
      if (bSystemProvidedParam1)
      {
         if (bSystemProvidedParam2)
         {
            if (bAssignedSlot1)
            {
               if (bAssignedSlot2)
                  return spi1.nInterstageSlot < spi2.nInterstageSlot;
               else
                  return true; // the param that has an assigned slot has priority
            }
            else
            {
               if (bAssignedSlot2)
                  return false; // the param that has an assigned slot has priority
               else
                  return spi1.strSemanticName > spi2.strSemanticName; // provide max stability
            }
         }
         else
            return false; // the param that is not system provided has priority
      }
      else
      {
         if (bSystemProvidedParam2)
            return true; // the param that is not system provided has priority
         else
         {
            if (bAssignedSlot1)
            {
               if (bAssignedSlot2)
                  return spi1.nInterstageSlot < spi2.nInterstageSlot;
               else
                  return true; // the param that has a slot assigned has priority
            }
            else
            {
               if (bAssignedSlot2)
                  return false; // the param that has a slot assigned has priority
               else
               {
                  if (bSystemDestinationParam1)
                  {
                     if (bSystemDestinationParam2)
                        return spi1.strSemanticName < spi2.strSemanticName; // this is very unlikely, but... provide max stability
                     else
                        return false; // params not needed by us but by the system have lowest priority
                  }
                  else
                  {
                     if (bSystemDestinationParam2)
                        return true; // params not needed by us but by the system have lowest priority
                     else
                        return spi1.strSemanticName < spi2.strSemanticName; // this is very unlikely, but... provide max stability
                  }
               }
            }
         }
      }
   }
};


void
HgiDXShaderGenerator::_ProcessStageInOut(const Hgi* pHgi,
                                         const HgiShaderFunctionDesc& stageDesc,
                                         bool bIn)
{
   std::vector<DXShaderInfo::StageParamInfo> stageInfo;
   std::set<std::string> semanticsSet;
   std::string emptyComment = "";

   const HgiShaderFunctionParamDescVector& params = bIn ? stageDesc.stageInputs : stageDesc.stageOutputs;
   int nOriginalPos = 0;

   if (bIn)
   {
      //
      // I will search for the primitiveID (hack_PrimitiveID_part1).
      for (const HgiShaderFunctionParamDesc& pd : params)
      {
         if (pd.nameInShader == "gl_PrimitiveID")
         {
            if (-1 == pd.interstageSlot)
            {
               //
               // This is a hacky way I am telling myself from codegen.cpp that there is no gs stage
               // In this case, I want to map "gl_PrimitiveID" to "SV_PrimitiveID" system generated param.
               _bSVPrimitiveIDAsSystem = true;
               break;
            }
         }
      }
   }

   //const std::string strSizeIgnore = "HD_NUM_PRIMITIVE_VERTS";

   for (const HgiShaderFunctionParamDesc& pd : params)
   {
      std::string strSemanticName;
      std::string strPipelineInputSemanticName;
      int nSemanticPipelineIdx;

      if (semanticsSet.find(pd.nameInShader) != semanticsSet.end())
      {
         if (pd.nameInShader != "gl_PrimitiveID")
         {
            TF_WARN("Duplicated stage in/out param found. This could be a serious error.");
         }
         continue;
      }
      semanticsSet.insert(pd.nameInShader);

      //
      // Check if this is a parameter we must transform to a constant.
      bool bAddAsParam = true;

      auto constParam = paramsToDefineAsGlobalConstants.find(pd.nameInShader);
      if(constParam != paramsToDefineAsGlobalConstants.end())
      {
         //
         // I will add a macro for this.
         std::unique_ptr<HgiDXMacroShaderSection> constParamSS = std::make_unique<HgiDXMacroShaderSection>(constParam->second, emptyComment);
         GetShaderSections()->push_back(std::move(constParamSS));
         
         bAddAsParam = false;
      }

      if (bAddAsParam)
      {
         std::string strVarType;
         HgiDXShaderGenerator::_GetSemanticName(bIn,
                                                stageDesc.shaderStage,
                                                pd.nameInShader,
                                                pd.type,
                                                strSemanticName,
                                                strPipelineInputSemanticName,
                                                strVarType,
                                                nSemanticPipelineIdx);

         DXShaderInfo::StageParamInfo spi;
         spi.strSemanticName = strSemanticName;
         spi.strSemanticPipelineName = strPipelineInputSemanticName;
         spi.nSemanticPipelineIndex = nSemanticPipelineIdx;

         //
         // I have a problem with the fact that sometimes, some in/out variables are multi-dimensional, but
         // for the geometry "gs" stage, they should all be multi-dimensional only inside the processing scope and not 
         // in the stage_in, stage_out declarations.
         // Before my last changes I would always ignore the provided "size" information 
         // and automatically dimension the gs stage variables to have the "proper" dimension 
         // based on the gs type (line, triangle, ...).
         // 
         // Now, I could use the provided size, but that is incorrect for the STAE_IN/ STAGE_OUT, when it comes from 
         // the GS stage as "HD_NUM_PRIMITIVE_VERTS", so I could simply ignore all sizes equal to "HD_NUM_PRIMITIVE_VERTS"
         //
         // This leaves me vulnerable to a few scenarios: 
         //    - HD_NUM_PRIMITIVE_VERTS changes name and again I'll write bad shaders
         //    - HD_NUM_PRIMITIVE_VERTS comes as a valid size for a different stage than gs and I'll incorrectly ignore it
         // 
         // Probably it would be best if we could safely differentiate between size of gs input vs real size of some 
         // really multi-dimensional things like the "HD_HAS_numClipPlanes" for the "renderPassState.clipPlanes"
         // Still I'll move forward with this approach for now, as it is the cheapest way forward.

         //
         // The other case when I observed a need for size was for the clip planes distances definition
         // which I just decided to declare as a float 4, and lock it at max 4 values from HgiDXCapabilities
         // so, now, this means I no longer have a known use case for a valid "size"
         // so, rather than complicating the code to exclude with the scalpel all exceptiosn = all known cases
         // I will revert to always ignoring the input size
         //spi.strArrSize = strSizeIgnore == pd.arraySize ? "" : pd.arraySize; // we'd need to xcplude the clip planes case also
         spi.strArrSize = "";

         spi.strShaderDataType = strVarType;
         spi.strShaderDataName = pd.nameInShader;
         spi.nSuggestedBindingIdx = pd.location;
         spi.nOriginalPosInList = nOriginalPos++;
         spi.nInterstageSlot = pd.interstageSlot;
         spi.Format = HgiDXConversions::ParamType2DXFormat(pd.type);

         stageInfo.push_back(std::move(spi));
      }
   }

   const HgiShaderFunctionParamBlockDescVector& paramBlocks = bIn ? stageDesc.stageInputBlocks : stageDesc.stageOutputBlocks;
   for (const HgiShaderFunctionParamBlockDesc& pd : paramBlocks)
   {
      std::string strSemanticName;
      std::string strPipelineInputSemanticName;
      int nSemanticPipelineIdx;

      if (semanticsSet.find(pd.instanceName) != semanticsSet.end())
      {
         TF_WARN("Duplicated stage in/out param found. This could be a serious error.");
      }
      semanticsSet.insert(pd.instanceName);

      std::string strVarType;
      HgiDXShaderGenerator::_GetSemanticName(bIn, 
                                             stageDesc.shaderStage, 
                                             pd.instanceName, 
                                             pd.blockName,
                                             strSemanticName, 
                                             strPipelineInputSemanticName, 
                                             strVarType,
                                             nSemanticPipelineIdx);

      DXShaderInfo::StageParamInfo spi;

      spi.strSemanticName = strSemanticName;
      spi.strSemanticPipelineName = strPipelineInputSemanticName;
      spi.nSemanticPipelineIndex = nSemanticPipelineIdx;
      spi.strShaderDataType = strVarType;
      spi.strShaderDataName = pd.instanceName;
      spi.nSuggestedBindingIdx = UINT_MAX;
      spi.nOriginalPosInList = nOriginalPos++;
      spi.nInterstageSlot = pd.interstageSlot;
      spi.Format = DXGI_FORMAT_UNKNOWN;

      stageInfo.push_back(std::move(spi));
   }

   std::sort(stageInfo.begin(), stageInfo.end(), less_than_spi());

   if(bIn)
      _sdi.stageIn = std::move(stageInfo);
   else
      _sdi.stageOut = std::move(stageInfo);
}

void 
HgiDXShaderGenerator::_CleanupText(std::string& shaderCode,
                                   std::string& textStart,
                                   std::string& textEnd,
                                   fcDealWithMatchStr* pFc)
{
   bool bFound = false;
   size_t nStart = shaderCode.find(textStart);
   if (std::string::npos != nStart)
   {
      size_t nEnd = shaderCode.find(textEnd, nStart);
      if (std::string::npos != nEnd)
      {
         bFound = true;
         nEnd += textEnd.size();

         if(nullptr != pFc)
            (*pFc)(shaderCode.substr(nStart, nEnd - nStart));

         std::string strCleanText;
         int nAllSize = shaderCode.size();
         strCleanText.reserve(nAllSize);
         strCleanText += shaderCode.substr(0, nStart);
         strCleanText += shaderCode.substr(nEnd, nAllSize - nEnd);
         shaderCode = strCleanText;
      }
   }
}

void 
HgiDXShaderGenerator::_CleanupText(std::string& text,
                                   const std::regex& expr, 
                                   const std::string& replaceWith, 
                                   fcDealWithMatch* pFc)
{
   std::string strEmpty = "";
   _CleanupText(text, expr, strEmpty, replaceWith, pFc);
}

void
HgiDXShaderGenerator::_CleanupText(std::string& text, 
                                   const std::regex& expr,
                                   const std::string& strAdditionalInMatch,
                                   const std::string& replaceWith,
                                   fcDealWithMatch* pFc)
{
   std::sregex_iterator iter(text.begin(), text.end(), expr);
   std::sregex_iterator end;
   if (iter != end)
   {
      std::string strCleanText;
      strCleanText.reserve(text.size());
      int nOldPos = 0;

      while (iter != end)
      {
         std::smatch match = *iter;
         std::string strMatch = match[0];
         if ((strAdditionalInMatch.size() == 0) ||
             (strMatch.find(strAdditionalInMatch) != std::string::npos))
         {
            int nPos = match.position(0);
            std::string strKeep = text.substr(nOldPos, nPos - nOldPos);
            strCleanText += strKeep;

            if (replaceWith.length())
               strCleanText += replaceWith;

            nOldPos = nPos + match.length();

            if (nullptr != pFc)
               (*pFc)(match);
         }

         iter++;
      }

      strCleanText += text.substr(nOldPos);
      text = strCleanText;
   }
}

void 
HgiDXShaderGenerator::_ExtractStructureFromScope(std::ostream& ss, std::string& shaderCode)
{
   //
   // Unfortunately to make things maximally difficult, sometimes the structure 
   // declaration will also contain a variable declaration (hopefully not more)
   // in which case I need to extract the structure declaration outside of the scope
   // but leave the variable declaration in the scope.

   std::regex exprFindStruct(R"(struct ([a-zA-Z_\n]*?)([ ]*?\{[\s\S]*?\})[ ]*([\S]*?);\n)");
   std::sregex_iterator iter(shaderCode.begin(), shaderCode.end(), exprFindStruct);
   std::sregex_iterator end;
   if (iter != end)
   {
      std::string strCleanText;
      strCleanText.reserve(shaderCode.size());
      int nOldPos = 0;

      while (iter != end)
      {
         std::smatch match = *iter;
         if (match.size() > 2)
         {
            //
            // Need to extract everything but the variable definition.
            ss << "struct " << match[1] << match[2] << ";\n";

            int nPos = match.position(0);
            std::string strKeep = shaderCode.substr(nOldPos, nPos - nOldPos);
            strCleanText += strKeep;

            nOldPos = nPos + match.length();

            //
            // If there is a variable definition...
            if (match.size() > 3)
            {
               varInfo vi;
               vi.strName = match[3];

               if (vi.strName.size() > 0)
               {
                  vi.strType = match[1];
                  _additionalScopeParams.push_back(vi);
               }
            }
         }


         iter++;
      }

      strCleanText += shaderCode.substr(nOldPos);
      shaderCode = strCleanText;
   }
}

void 
HgiDXShaderGenerator::_CleanupGeneratedCode(std::ostream& ss, std::string& shaderCode)
{
   try
   {
      std::string strEmpty = "";

      fcDealWithMatch fcExtractBeforeScope = [&ss](const std::smatch& match) {
         std::string strMatch = match.str();
         ss << strMatch.c_str() << "\n";
      };

      bool bFound = false;
      fcDealWithMatchStr fcExtractBeforeScope2 = [&ss, &bFound](const std::string& match) {
         ss << match << "\n\n";
         bFound = true;
      };

      //
      // Cleanup any definition of PI - decided to add it to the main defines area
      // and now we need to remove it if it appears all over the code
      // e.g.
      //     const float PI = 3.1415;
      std::regex strDefine(R"(.*float PI.*)");
      _CleanupText(shaderCode, strDefine, strEmpty);


      //
      // Replace mat4(1) with MAT4Init(1)
      // This is now done in translation from glslfx -> hlslfx
      //std::regex strMat4Ctor(R"(mat4\(1\))");
      //std::string strMat4DxCtor = "MAT4Init(1)";
      //_CleanupText(shaderCode, strMat4Ctor, strMat4DxCtor, nullptr);

      //
      // This does not happen except when "HDST_ENABLE_HGI_RESOURCE_GENERATION" is turned off
      // 
      // between "//////// Codegen Defines ////////"
      // and "//////// Codegen Decl ////////"
      // there is sometimes code can contain defines like "#define HD_INSTANCE_INDEX_WIDTH 1"
      // which may be used in a structure declaration, and if I move the structure definition 
      // above, it will no longer compile, so I have to first extract the "#define"(s)
      /*
      std::regex strDefine(R"(^#define .*\n)");
      _CleanupText(shaderCode, strDefine, strEmpty, &fcExtractBeforeScope);*/

      if (GetDescriptor().shaderStage == HgiShaderStageBits::HgiShaderStageGeometry)
      {
         //
         // post processing OSD code to remove all buffers, struct definitions, 
         // defines and redefines of things in the correct order is a nightmare
         // so, at least for a while I'll go ahead with something extra hacky
         // and very unstale based on a particular case observation:
         std::string strStartOSD = "// //////// OSD_CODE_START ////////";
         std::string strEndOSD = "// //////// OSD_CODE_END ////////";
         bFound = false;
         _CleanupText(shaderCode, strStartOSD, strEndOSD, &fcExtractBeforeScope2);

         if (!bFound)
            TF_WARN("Failed to find the OSD generated code. This may result in a shader build failure. Check codegen.cpp, '_GetOSDCommonShaderSource'");
         
         //
         // one particular codegen definition that does not compile for DirectX
         // _procGS << "  const vec3 coords[4] = vec3[](\n" ...
         std::regex expr10(R"(const vec3 coords\[4\] =[\s\S]*?\);)");
         std::string strReplaceWith = "const vec3 coords[4] = { vec3(0,0,1), vec3(1,0,0), vec3(0,1,0), vec3(1,0,0) };";
         _CleanupText(shaderCode, expr10, strReplaceWith, nullptr);

         expr10 = std::regex(R"(const vec3 coords\[3\] =[\s\S]*?\);)");
         strReplaceWith = std::string("const vec3 coords[3] = { vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };");
         _CleanupText(shaderCode, expr10, strReplaceWith, nullptr);


         //
         // Commenting out the gs layout info in the hlslfx
         // layout(triangles) in;
         // layout(triangle_strip, max_vertices = 3) out;
         std::regex expr3(R"(layout[\S \(]*\bmax_vertices\b[ =]*([0-9]*)[\S\) ;]*?\n)");
         _CleanupText(shaderCode, expr3, strEmpty, nullptr);
      }

      //
      // I want to search for structures defined inside my future "scope", 
      // something like this:
      // struct <name> ... { ... };
      // and output it now to ss, because I know I call this code here, before opening the scope.
      _ExtractStructureFromScope(ss, shaderCode);
   }
   catch (std::exception ex)
   {
      throw ex;
   }
}

int 
HgiDXShaderGenerator::_GetGeomShaderNumInValues()
{
   int nRet = -1; // I want the unhandled cases or errors to come out asap

   switch (GetDescriptor().geometryDescriptor.inPrimitiveType)
   {
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::Points:
      nRet = 1;
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::Lines:
      nRet = 2;
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::LinesAdjacency:
      nRet = 4;
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::Triangles:
      nRet = 3;
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::TrianglesAdjacency:
      nRet = 6;
      break;
   default:
      break;
   }

   return nRet;
}

std::string
HgiDXShaderGenerator::_GetGeomShaderInVarType()
{
   std::string strRet;

   switch (GetDescriptor().geometryDescriptor.inPrimitiveType)
   {
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::Points:
      strRet = "point";
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::Lines:
      strRet = "line";
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::LinesAdjacency:
      strRet = "lineadj";
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::Triangles:
      strRet = "triangle";
      break;
   case HgiShaderFunctionGeometryDesc::InPrimitiveType::TrianglesAdjacency:
      strRet = "triangleadj";
      break;
   default:
      break;
   }

   return strRet;
}

std::string 
HgiDXShaderGenerator::_GetGeomShaderOutVarType()
{
   std::string strRet;

   switch (GetDescriptor().geometryDescriptor.outPrimitiveType)
   {
   case HgiShaderFunctionGeometryDesc::OutPrimitiveType::Points:
      strRet = "PointStream<STAGE_OUT>";
      break;
   case HgiShaderFunctionGeometryDesc::OutPrimitiveType::LineStrip:
      strRet = "LineStream<STAGE_OUT>";
      break;
   case HgiShaderFunctionGeometryDesc::OutPrimitiveType::TriangleStrip:
      strRet = "TriangleStream<STAGE_OUT>";
      break;
   default:
      break;
   }

   return strRet;
}

void 
HgiDXShaderGenerator::_WriteScopeStart_OpenScope(std::ostream& ss)
{
   //
   // Open scope.
   ss << "struct Processing_Scope { \n";
}

void 
HgiDXShaderGenerator::_WriteScopeStart_ForwardDeclarations(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   //
   // for now, only the geometry stage needs this
   if (HgiShaderStageBits::HgiShaderStageGeometry == stage)
   {
      //
      // I want to forward declare the 2 methods that will allow me to generate 
      // whatever we want to generate...
      ss << "// Declare DirectX callbacks:\n";
      ss << "#define OutStream " << _GetGeomShaderOutVarType() << "\n";
      
      // These are functions inside a struct, they do not need to be pre-declared
      //ss << "void EmitVertex(inout OutStream ts); \n";
      //ss << "void EndPrimitive(inout OutStream ts);\n\n";
   }
}

void 
HgiDXShaderGenerator::_WriteScopeStart_DeclareInput(std::ostream& ss)
{
   //
   // Declare additional variables we extracted from scope.
   for (const varInfo& vi : _additionalScopeParams)
      ss << "   " << vi.strType << " " << vi.strName << ";\n";
   ss << "\n";
   
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   if (HgiShaderStageBits::HgiShaderStageGeometry == stage)
   {
      int nInValues = _GetGeomShaderNumInValues();

      //
      // Redeclare all input without semantics.
      for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageIn)
      {
         if (!spi.strArrSize.empty())
            ss << "   " << spi.strShaderDataType << " " << spi.strShaderDataName << "[" << nInValues << "][" << spi.strArrSize <<"];\n";
         else
            ss << "   " << spi.strShaderDataType << " " << spi.strShaderDataName << "[" << nInValues << "];\n";
      }

      //
      // And one hard-coded thing... so far only for geometry stage
      ss << "   uint gl_PrimitiveIDIn;\n";
   }
   else
   {
      //
      // Redeclare all input without semantics.
      for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageIn)
      {
         if (spi.strArrSize.empty())
            ss << "   " << spi.strShaderDataType << " " << spi.strShaderDataName << ";\n";
         else
            ss << "   " << spi.strShaderDataType << " " << spi.strShaderDataName << "[" << spi.strArrSize << "];\n";
      }
      ss << "\n";
   }
}

void 
HgiDXShaderGenerator::_WriteScopeStart_DeclareOutput(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   if (HgiShaderStageBits::HgiShaderStageCompute == stage)
   {
      //
      // so far only the compute stage does not need this
      // compute does not output anything
   }
   else
   {
      //
      // Redeclare all output without semantics.
      for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageOut)
      {
         if (spi.strArrSize.empty())
            ss << "   " << spi.strShaderDataType << " " << spi.strShaderDataName << ";\n";
         else
            ss << "   " << spi.strShaderDataType << " " << spi.strShaderDataName << "[" << spi.strArrSize << "];\n";
      }
      ss << "\n";
   }

   
}

void
HgiDXShaderGenerator::_WriteScopeStart(std::ostream& ss)
{
   _WriteScopeStart_OpenScope(ss);
   _WriteScopeStart_ForwardDeclarations(ss);
   _WriteScopeStart_DeclareInput(ss);
   _WriteScopeStart_DeclareOutput(ss);
   ss << "\n";

}


void 
HgiDXShaderGenerator::_WriteScopeEnd_ExtraMethods(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   if (HgiShaderStageBits::HgiShaderStageGeometry == stage)
   {
      //
      // The implementation of "EmitVertex" and "EndPrimitive" 
      // (should still be inside the "scope").
      ss << "void EmitVertex(inout OutStream ts) {\n";
      ss << "   STAGE_OUT OUT = (STAGE_OUT)0;\n";
      for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageOut)
      {
         if (spi.strArrSize.empty())
            ss << "   OUT." << spi.strShaderDataName << " = " << spi.strShaderDataName << ";\n";
         else
         {
            ss << "   for(int ii=0; ii<" << spi.strArrSize << "; ii++)\n";
            ss << "      OUT." << spi.strShaderDataName << "[ii] = " << spi.strShaderDataName << "[ii];\n";
         }
      }
      ss << "   ts.Append(OUT);\n";
      ss << "}\n\n";

      ss << "void EndPrimitive(inout OutStream ts) {\n";
      ss << "   ts.RestartStrip();\n";
      ss << "}\n\n";
   }
}

void 
HgiDXShaderGenerator::_WriteScopeEnd_CloseScope(std::ostream& ss)
{
   ss << "}; // end Processing_Scope\n";
}

void 
HgiDXShaderGenerator::_WriteScopeEnd_StartMainFc(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   if (HgiShaderStageBits::HgiShaderStageGeometry == stage)
   {
      int nInValues = _GetGeomShaderNumInValues();
      std::string strOutMaxVerts = GetDescriptor().geometryDescriptor.outMaxVertices;
      int nMaxPrimsOut = std::stoi(strOutMaxVerts);
      std::string strInType = _GetGeomShaderInVarType();
      
      ss << "\n[maxvertexcount(" << nMaxPrimsOut << ")]\n";

      // changed because of shader model 6 which complains about the main fc inside the hacky "scope"
      ss << "void mainDX (";
      ss << strInType << " STAGE_IN IN[" << nInValues << "], uint primitiveID : SV_PrimitiveID, inout OutStream ts) {\n";
   }
   else if (HgiShaderStageBits::HgiShaderStageCompute == stage)
   {
      ss << "\n[numthreads(" << _csWorkSizeX << ", " << _csWorkSizeY << ", " << _csWorkSizeZ << ")]\n";
      ss << "void mainDX (STAGE_IN IN) {\n";
   }
   else
   {
      //
      // Write main fc that deals with setting scope, call and get result out.

      if(_bNoStageOut)
         ss << "\n" << "void mainDX (STAGE_IN IN) {\n";
      else
         ss << "\n" << "STAGE_OUT mainDX (STAGE_IN IN) {\n";
   }

   //
   // Initialize the scope.
   ss << "   Processing_Scope procScope;\n";
}

void 
HgiDXShaderGenerator::_WriteScopeEnd_InitializeOutputVars(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;

   //
   // compute has no output
   if (HgiShaderStageBits::HgiShaderStageCompute != stage)
   {

      if (HgiShaderStageBits::HgiShaderStageGeometry == stage)
         ss << "   procScope.gl_PrimitiveIDIn = primitiveID;\n";
      
      //
      // Initialize output members with some default values to avoid dx error about uninitialized variables.
      for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageOut)
      {
         if (spi.strArrSize.empty())
            ss << "   procScope." << spi.strShaderDataName << " = (" << spi.strShaderDataType << ")0;\n";
         else
         {
            ss << "   for(int ii=0; ii<" << spi.strArrSize << "; ii++)\n";
            ss << "      procScope." << spi.strShaderDataName << "[ii] = (" << spi.strShaderDataType << ")0;\n";
         }
      }
   }
}

void 
HgiDXShaderGenerator::_WriteScopeEnd_SetInputVars(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;

   if (HgiShaderStageBits::HgiShaderStageGeometry == stage)
   {
      int nInValues = _GetGeomShaderNumInValues();

      ss << "\n\n";
      for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageIn)
      {
         for (int idx = 0; idx < nInValues; idx++)
         {
            if (!spi.strArrSize.empty())
            {
               ss << "   {\n"; // I want to to avoid confusing the compiler with different iterators sharing the same scope
               ss << "      for(int ii=0; ii<" << spi.strArrSize << "; ii++)\n";
               ss << "         procScope." << spi.strShaderDataName << "[" << idx << "][ii] = IN[" << idx << "]." << spi.strShaderDataName << "[ii];\n";
               ss << "   }\n";
            }
            else
               ss << "   procScope." << spi.strShaderDataName << "[" << idx << "] = IN[" << idx << "]." << spi.strShaderDataName << ";\n";
         }
      }
   }
   else
   {
      //
      // Set input members.
      for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageIn)
      {
         if (spi.strArrSize.empty())
            ss << "   procScope." << spi.strShaderDataName << " = IN." << spi.strShaderDataName << ";\n";
         else
         {
            ss << "   for(int kk=0; kk<" << spi.strArrSize << "; kk++)\n";
            ss << "      procScope." << spi.strShaderDataName << "[kk] = IN." << spi.strShaderDataName << "[kk];\n";
         }
      }
   }
}

void 
HgiDXShaderGenerator::_WriteScopeEnd_CallRealMain(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   if (HgiShaderStageBits::HgiShaderStageGeometry == stage)
      ss << "\n   procScope.main(ts);\n\n";
   else
      ss << "\n   procScope.main();\n\n";
}

void 
HgiDXShaderGenerator::_WriteScopeEnd_GetOutputVars(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   if ((HgiShaderStageBits::HgiShaderStageGeometry == stage) ||
       (HgiShaderStageBits::HgiShaderStageCompute == stage)) {
      //
      // nothing to do here
   }
   else
   {
      if (!_bNoStageOut) {
         ss << "   STAGE_OUT OUT;\n";
         for (const DXShaderInfo::StageParamInfo& spi : _sdi.stageOut) {
            if (spi.strArrSize.empty())
               ss << "   OUT." << spi.strShaderDataName << " = procScope." << spi.strShaderDataName << ";\n";
            else {
               ss << "   for(int jj=0; jj<" << spi.strArrSize << "; jj++)\n";
               ss << "      OUT." << spi.strShaderDataName << "[jj] = procScope." << spi.strShaderDataName << "[jj];\n";
            }
         }
      }
   }
}

void 
HgiDXShaderGenerator::_WriteScopeEnd_Finish(std::ostream& ss)
{
   const HgiShaderStage& stage = GetDescriptor().shaderStage;
   if ((HgiShaderStageBits::HgiShaderStageGeometry == stage) ||
       (HgiShaderStageBits::HgiShaderStageCompute == stage))
   {
      ss << "}\n\n";
   }
   else
   {
      if (!_bNoStageOut) 
         ss << "   return OUT;\n";

      ss << "}\n\n";
   }
}

void 
HgiDXShaderGenerator::_WriteScopeEnd(std::ostream& ss)
{
   _WriteScopeEnd_ExtraMethods(ss);
   _WriteScopeEnd_CloseScope(ss);
   _WriteScopeEnd_StartMainFc(ss);
   _WriteScopeEnd_InitializeOutputVars(ss);
   _WriteScopeEnd_SetInputVars(ss);
   _WriteScopeEnd_CallRealMain(ss);
   _WriteScopeEnd_GetOutputVars(ss);
   _WriteScopeEnd_Finish(ss);
}

const char*
HgiDXShaderGenerator::_GetMacroBlob()
{
   return
      "// Alias GLSL types to HLSL\n"
      "#define ivec2 int2\n"
      "#define vec2 float2\n"
      "#define ivec3 int3\n"
      "#define hd_ivec3 int3\n"
      "#define uvec3 uint3\n"
      "#define vec3 float3\n"
      "#define bvec3 bool3\n"
      "#define dvec3 double3\n"
      "#define ivec4 int4\n"
      "#define vec4 float4\n"
      "#define bvec4 bool4\n"
      "#define mat3 float3x3\n"
      "#define dmat3 double3x3\n"
      "#define mat4 float4x4\n"
      "\n"

      // Hgi stuff that makes no sense and is hopefully useless
      //"#define hgi_ivec3 int3\n"
      //"#define hgi_vec3 float3\n"
      //"#define hgi_dvec3 double3\n"
      //"#define hgi_mat3 float3x3\n"
      //"#define hgi_dmat3 double3x3\n"
      //"struct hgi_ivec3 { hgi_ivec3(int a, int b, int c):x(a),y(b),z(c);   int x, y, z; };\n"
      //"struct hgi_vec3  { float  x, y, z; };\n"
      //"struct hgi_dvec3 { double x, y, z; };\n"
      //"struct hgi_mat3  { float  m00, m01, m02,\n"
      //"                          m10, m11, m12,\n"
      //"                          m20, m21, m22; };\n"
      //"struct hgi_dmat3 { double m00, m01, m02,\n"
      //"                          m10, m11, m12,\n"
      //"                          m20, m21, m22; };\n";
      //"\n"

      //
      // There are some rare cases when they still use these hd_ types.
      // Found such a situation when using a point instancer
      "// HD to HLSL.\n"
      "#define hd_ivec3 int3\n"
      "#define hd_vec3 float3\n"
      "#define hd_dvec3 double3\n"
      "#define hd_mat3 float3x3\n"
      "#define hd_dmat3 double3x3\n"
      "\n"
      
      //"\n"
      // the ones below look like completely useless definitions
      //"ivec3 hd_ivec3_get(ivec3 v)    { return v; }\n"
      //"vec3  hd_vec3_get(vec3 v)      { return v; }\n"
      //"dvec3 hd_dvec3_get(dvec3 v)    { return v; }\n"
      //"mat3  hd_mat3_get(mat3 v)      { return v; }\n"
      //"dmat3 hd_dmat3_get(dmat3 v)    { return v; }\n"
      //"hd_ivec3 hd_ivec3_set(hd_ivec3 v) { return v; }\n"
      //"hd_vec3 hd_vec3_set(hd_vec3 v)    { return v; }\n"
      //"hd_dvec3 hd_dvec3_set(hd_dvec3 v) { return v; }\n"
      //"hd_mat3  hd_mat3_set(hd_mat3 v)   { return v; }\n"
      //"hd_dmat3 hd_dmat3_set(hd_dmat3 v) { return v; }\n"
      
      "#pragma pack_matrix( column_major )\n"
      "#define hd_ivec3_get\n"
      "#define hd_vec3_get\n"
      "#define hd_dvec3_get\n"
      "int hd_int_get(int v)          { return v; }\n"
      "int hd_int_get(ivec2 v)        { return v.x; }\n"
      "int hd_int_get(ivec3 v)        { return v.x; }\n"
      "int hd_int_get(ivec4 v)        { return v.x; }\n"
      // 
      // helper functions for 410 specification
      // applying a swizzle operator on int and float is not allowed in 410.
      //"int hd_int_get(int v)          { return v; }\n"
      //"int hd_int_get(ivec2 v)        { return v.x; }\n"
      //"int hd_int_get(ivec3 v)        { return v.x; }\n"
      //"int hd_int_get(ivec4 v)        { return v.x; }\n"
      // udim helper function
      "vec3 hd_sample_udim(vec2 v) {\n"
      "   vec2 vf = floor(v);\n"
      "   return vec3(v.x - vf.x, v.y - vf.y, clamp(vf.x, 0.0, 10.0) + 10.0 * vf.y);\n"
      "}\n"
      "#define REF(space,type) inout type\n"
      "#define FORWARD_DECL(func_decl) func_decl\n"
      // DX HGI specific definitions:
      "// DX HGI specific definitions:"
      "\n"
      "#define mix(x,y,z) lerp(x,y,z)\n"
      "#define dFdx(x) ddx(x)\n"
      "#define dFdy(x) -ddy(x)\n" // this I hope will deal with a difference in behavior between dx and gl (probably caused by inverted screen y)
                                  // TODO: test this on my classic (Atrium house) case, by reverting changes in mesh.hlslfx, "vec3 ComputeScreenSpaceNeye()"
      "#define bitfieldReverse reversebits\n"
      "#define intBitsToFloat asfloat\n"

      // I am running into some errors with Pi definitions.
      // The situation was that PI was defined twice:
      // simpleLighting -> static const float PI = 3.1415;
      // previewSurface.glslfx -> #define PI 3.1415
      // which when mixed in the same file results in crezy directX compile errors
      "#define PI 3.1415\n"

      // Seems some of our shaders use some atomic operations that are not available in hlsl 
      // until shader model 6.6 and even there they look very differently
      // So, for the moment I will define the atomics to regular operations and hope for the best
      // TODO: this needs to be better understood and fixed
      "#define ATOMIC_ADD(x, y) (x += y)\n"
      "#define ATOMIC_EXCHANGE(x, y) (x = y)\n"
      "#define ATOMIC_LOAD(x) x\n"
      "#define atomic_int int\n"

      // Moved this in codegen.cpp for all backends                            
      // but unfortunately I am cutting out their entire definition set due to too many things that 
      // do more harm than good, so I have to redefine it here again anyway
      "float4x4 MAT4Init(float x) {\n"
      "   return float4x4(x,0,0,0, 0,x,0,0, 0,0,x,0, 0,0,0,x); }\n\n"
      // Adding the matrix "inverse" definition here, because it is now used in several places
      "float4x4 inverse(float4x4 m) {\n"
      "    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];\n"
      "    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];\n"
      "    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];\n"
      "    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];\n"
      "\n"
      "    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;\n"
      "    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;\n"
      "    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;\n"
      "    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;\n"
      "\n"
      "    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;\n"
      "    float idet = 1.0f / det;\n"
      "\n"
      "    float4x4 ret;\n"
      "\n"
      "    ret[0][0] = t11 * idet;\n"
      "    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;\n"
      "    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;\n"
      "    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;\n"
      "\n"
      "    ret[1][0] = t12 * idet;\n"
      "    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;\n"
      "    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;\n"
      "    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;\n"
      "\n"
      "    ret[2][0] = t13 * idet;\n"
      "    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;\n"
      "    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;\n"
      "    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;\n"
      "\n"
      "    ret[3][0] = t14 * idet;\n"
      "    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;\n"
      "    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;\n"
      "    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;\n"
      "\n"
      "    return ret;\n"
      "}\n"
      // more missing functions from hlsl
      "bool3 lessThan(float3 v1, float3 v2) {\n"
      "   return bool3(v1.x < v2.x, v1.y < v2.y, v1.z < v2.z);\n"
      "}\n"
      "bool3 greaterThan(float3 v1, float3 v2) {\n"
      "   return bool3(v1.x > v2.x, v1.y > v2.y, v1.z > v2.z);\n"
      "}\n"
      "bool4 lessThan(float4 v1, float4 v2) {\n"
         "   return bool4(v1.x < v2.x, v1.y < v2.y, v1.z < v2.z, v1.w < v2.w);\n"
      "}\n"
      "bool4 greaterThan(float4 v1, float4 v2) {\n"
      "   return bool4(v1.x > v2.x, v1.y > v2.y, v1.z > v2.z, v1.w > v2.w);\n"
      "}\n"
      "bool3 equal(int3 v1, int3 v2) {\n"
      "   return bool3(v1.x == v2.x, v1.y == v2.y, v1.z == v2.z);\n"
      "}\n"
      "bool4 equal(int4 v1, int4 v2) {\n"
      "   return bool4(v1.x == v2.x, v1.y == v2.y, v1.z == v2.z, v1.w == v2.w);\n"
      "}\n"
      "bool3 not(bool3 v1) {\n"
      "   return bool3(!v1.x, !v1.y, !v1.z);\n"
      "}\n"
      "bool4 not(bool4 v1) {\n"
      "   return bool4(!v1.x, !v1.y, !v1.z, !v1.w);\n"
      "}\n"
      "float atan(float a, float b) { \n"
      "   return atan(a/b);\n" 
      "}\n"

      // I could define mod -> fmod, but documentation says they have different behavior for negative values, so...
      "float mod(float x, float y) {\n"
      "   return x - y * floor(x/y);\n"
      "}\n"
      ;
}

const char*
HgiDXShaderGenerator::_GetPackedTypeDefinitions()
{
   //
   // These seem like hgi specific stuff that need to be defined 
   // in order for the rest of the code to make sense.
   return
      // -------------------------------------------------------------------
      // Packed HdType implementation.

      "vec4 hd_vec4_2_10_10_10_get(int v) {\n"
      "   ivec4 unpacked = ivec4((v & 0x3ff) << 22, (v & 0xffc00) << 12,\n"
      "                          (v & 0x3ff00000) << 2, (v & 0xc0000000));\n"
      "   return vec4(unpacked) / 2147483647.0; }\n"
      "int hd_vec4_2_10_10_10_set(vec4 v) {\n"
      "   return ( (int(v.x * 511.0) & 0x3ff) |\n"
      "            ((int(v.y * 511.0) & 0x3ff) << 10) |\n"
      "            ((int(v.z * 511.0) & 0x3ff) << 20) |\n"
      "            ((int(v.w) & 0x1) << 30)); }\n"
      ;
}

PXR_NAMESPACE_CLOSE_SCOPE
