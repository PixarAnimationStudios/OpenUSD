
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

#include "pxr/imaging/hgiDX/api.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiDXShaderCompiler
///
/// DirectX implementation of HgiDXShaderCompiler
/// This is meant to be easily reusable code that deals with things like compile options, shader model, etc
/// 
class HgiDXShaderCompiler final
{
public:

   enum CompileTarget {
      kUnknown,
      kVS,
      kPS,
      kGS,
      kCS,
      kDS, // TessellationEval
      kHS, // TessellationControl
   };


   /// <summary>
   /// <strShaderSource> shader source code
   /// <strCompileTarget> e.g. vs_5_1, or vs_6_0
   /// <returns> either a ComPtr<ID3DBlob> or a ComPtr<IDxcBlob> depending on the shader model used
   /// </summary>
   HGIDX_API
   static ComPtr<IUnknown> Compile( const std::string& strShaderSource,
                                    CompileTarget ct,
                                    std::string& errors);

   static bool UsingShaderModel6();

   static LPCSTR GetTargetName(CompileTarget ct);
   static LPCWSTR GetTargetNameW(CompileTarget ct);

   static LPCSTR GetVSTargetName();
   static LPCSTR GetPSTargetName();
   static LPCSTR GetGSTargetName();
   static LPCSTR GetCSTargetName();
   static LPCSTR GetDSTargetName();
   static LPCSTR GetHSTargetName();

   static LPCWSTR GetVSTargetNameW();
   static LPCWSTR GetPSTargetNameW();
   static LPCWSTR GetGSTargetNameW();
   static LPCWSTR GetCSTargetNameW();
   static LPCWSTR GetDSTargetNameW();
   static LPCWSTR GetHSTargetNameW();

};


PXR_NAMESPACE_CLOSE_SCOPE
