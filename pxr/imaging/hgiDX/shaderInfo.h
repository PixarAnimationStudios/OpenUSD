
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

class DXShaderInfo
{
public:

   struct StageParamInfo
   {
      std::string strSemanticName;
      std::string strShaderDataName;
      std::string strShaderDataType;

      std::string strSemanticPipelineName;
      UINT nSemanticPipelineIndex;
      
      UINT nSuggestedBindingIdx;

      UINT nOriginalPosInList;
      int nInterstageSlot; // this is -1 when not set

      DXGI_FORMAT Format;
   };

   struct RootParamInfo
   {
      std::string strTypeName;
      std::string strName;
      bool bConst;
      bool bWritable;
      UINT nShaderRegister;
      UINT nRegisterSpace;
      UINT nSuggestedBindingIdx;
      UINT nBindingIdx;
   };

   struct StageDXInfo
   {
      std::vector<StageParamInfo> stageIn;
      std::vector<StageParamInfo> stageOut;
   };
};