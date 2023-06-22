
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

#include <string>
#include <regex>
#include <functional>
#include <iostream>
#include <set>
#include "shadersDefines.h"

using namespace shadersAnalysisDefines;


bool fix_glslfx_2_hlslfx(std::string& strCode);
bool fix_matInit(std::string& strCode);
bool fix_vectorsInit(std::string& strCode);
bool fix_constVarDecl(std::string& strCode);
bool fix_atan(std::string& strCode);
bool fix_computeShaders(const std::string& strFileName, std::string& strCode);
bool fix_geometryShaders(const std::string& strFileName, std::string& strCode);
bool fix_matMultiplication(std::string& strCode);
bool fix_forward_decl(std::string& strCode);
bool fix_verySpecialCases(const std::string& strFileName, std::string& strCode);

bool executeCodeTranslation(const std::string& strSourceFileName, std::string& strCode)
{
   bool bSomethingChanged = false;
   bSomethingChanged = fix_glslfx_2_hlslfx(strCode) || bSomethingChanged;
   bSomethingChanged = fix_forward_decl(strCode) || bSomethingChanged;
   bSomethingChanged = fix_matInit(strCode) || bSomethingChanged;
   bSomethingChanged = fix_vectorsInit(strCode) || bSomethingChanged;
   bSomethingChanged = fix_constVarDecl(strCode) || bSomethingChanged;
   bSomethingChanged = fix_atan(strCode) || bSomethingChanged;

   bSomethingChanged = fix_computeShaders(strSourceFileName, strCode) || bSomethingChanged;
   bSomethingChanged = fix_geometryShaders(strSourceFileName, strCode) || bSomethingChanged;
   bSomethingChanged = fix_matMultiplication(strCode) || bSomethingChanged;
   bSomethingChanged = fix_verySpecialCases(strSourceFileName, strCode) || bSomethingChanged;

   return bSomethingChanged;
}



typedef std::function<void(std::string& newText, const std::smatch& match)> fcDealWithMatch;
bool replaceText( std::string& text,
                  const std::regex& expr,
                  const std::string& replaceWith,
                  fcDealWithMatch* pFc = nullptr)
{
   bool bRet = false;

   std::sregex_iterator iter(text.begin(), text.end(), expr);
   std::sregex_iterator end;
   if (iter != end)
   {
      std::string strNewText;
      strNewText.reserve(text.size());
      int nOldPos = 0;
      bRet = true;

      while (iter != end)
      {
         std::smatch match = *iter;
         std::string strMatch = match[0];
         
         int nPos = match.position(0);
         std::string strKeep = text.substr(nOldPos, nPos - nOldPos);
         strNewText += strKeep;

         if (replaceWith.length())
            strNewText += replaceWith;

         nOldPos = nPos + match.length();

         if (nullptr != pFc)
            (*pFc)(strNewText, match);

         iter++;
      }

      strNewText += text.substr(nOldPos);
      text = strNewText;
   }

   return bRet;
}

// replace all references inside the glslfx files with references to hlslfx
bool fix_glslfx_2_hlslfx(std::string& strCode)
{
   //
   // the difficulty here is that I do not want to modify the first line of each shader
   // because for some reason the usd code feels the need to parse that 
   // and needs to find a ".glslfx" string in there
   // so let's write a lot more code here just for that:
   // return replaceText(strCode, std::regex(strGlLibExt), strHlLibExt);

   bool bRet = false;
   std::regex strFind(R"(([-. ]{3})*glslfx)");
   std::string strFix = "";

   fcDealWithMatch fcChangeRef = [](std::string& strNewCode, const std::smatch& match) {
      std::string strPrefix = match[1].str();

      if (strPrefix.size() > 0)
      {
         //
         // leave it alone
         strNewCode += match[0];
      }
      else
         strNewCode += "hlslfx";
   };

   bRet = replaceText(strCode, strFind, strFix, &fcChangeRef) || bRet;

   return bRet;
}

bool fix_matInit(std::string& strCode)
{
   bool bRet = false;
   std::regex strMat4Ctor(R"(MAT4\(([0-9.]*)\))");
   std::string strMat4DxCtor = "MAT4Init(";

   fcDealWithMatch fcAddParam = [&bRet](std::string& strNewCode, const std::smatch& match) {
      std::string strMatch = match[1].str();
      strNewCode += strMatch;
      strNewCode += ")";

      bRet = true;
   };

   replaceText(strCode, strMat4Ctor, strMat4DxCtor, &fcAddParam);

   return bRet;
}

std::map<std::string, int> vecCtorsToChange = {
   {"vec2", 2},
   {"vec3", 3},
   {"vec4", 4},
};

bool fix_vectorsInit(std::string& strCode)
{
   bool bRet = false;

   //
   // I want to change:
   // vec2(a) -> vec2(a,a)
   // vec3(a) -> vec3(a,a,a)
   // ...

   int nTimesAdd = 0;

   fcDealWithMatch fcAddParam = [&bRet, &nTimesAdd](std::string& strNewCode, const std::smatch& match) {
      
      if (match.size() > 2)
      {
         std::string strMatch = match[2].str();

         strNewCode += strMatch;
         for (int i = 1; i < nTimesAdd; i++)
         {
            strNewCode += ",";
            strNewCode += strMatch;
         }

         strNewCode += ")";
      }
      else
         std::cout << "Unexpected match in vector fix fc" << match.str() << std::endl;

      bRet = true;
   };

   for (const auto& it : vecCtorsToChange)
   {
      std::string strRegexVecCtor = "(?!.*(.xy))" + it.first + "\\(([0-9.a-zA-Z_]*)\\)";
      std::string strVecDxCtor = it.first + "(";
      nTimesAdd = it.second;

      replaceText(strCode, std::regex(strRegexVecCtor), strVecDxCtor, &fcAddParam);
   }

   return bRet;
}

bool fix_atan(std::string& strCode)
{
   bool bRet = false;
   
   //
   // I want to change:
   // atan(a, b) -> atan (a/b)
   std::regex strAtanExpr(R"(atan[ ]*\(([0-9a-zA-Z_. ]*),([0-9a-zA-Z_. ]*)\))");

   
   fcDealWithMatch fcAddParam = [&bRet](std::string& strNewCode, const std::smatch& match) {
      std::string strMatch1 = match[1].str();
      std::string strMatch2 = match[2].str();
      strNewCode += "atan(";
      strNewCode += strMatch1;
      strNewCode += "/";
      strNewCode += strMatch2;
      strNewCode += ")";

      bRet = true;
   };

   replaceText(strCode, strAtanExpr, "", &fcAddParam);

   return bRet;
}

std::map<std::string, std::string> forwardDeclsToChange = {
   {"FORWARD_DECL\\(float GetPointRasterSize\\(int\\)\\);", "FORWARD_DECL(float GetPointRasterSize(int id));"},
   {"FORWARD_DECL\\(void ProcessPointId\\(int\\)\\);", "FORWARD_DECL(void ProcessPointId(int id));"},
   {"FORWARD_DECL\\(bool IsPointSelected\\(int\\)\\);", "FORWARD_DECL(bool IsPointSelected(int id));"},
};


bool fix_forward_decl(std::string& strCode)
{
   //
   // I will take a very simple route for this, for now:
   // rather than build a regex, I'll search for the few cases I know cause issues
   // especially since I think this change belongs in the glslfx directly, should not be done during translation
   bool bSomethingChanged = false;

   for (const auto& it : forwardDeclsToChange)
      bSomethingChanged = replaceText(strCode, std::regex(it.first), it.second) || bSomethingChanged;

   return bSomethingChanged;
}

std::string constVarDecl[] = {
   "const float edgePickRadius", 
   "const float edgePickParametricRadius"
};

bool fix_constVarDecl(std::string& strCode)
{
   //
   // I will take a very simple route for this, for now:
   // rather than build a regex, I'll search for the few cases I know cause issues
   // especially since I think this change belongs in the glslfx directly, should not be done during translation

   //
   // I want to change:
   // const float edgePickRadius -> static const float edgePickRadius
   bool bSomethingChanged = false;

   for (const std::string& decl : constVarDecl)
   {
      std::string strReplace = "static " + decl;
      bSomethingChanged = replaceText(strCode, std::regex(decl), strReplace) || bSomethingChanged;
   }

   return bSomethingChanged;
}


std::set<std::string> strShadersToCheckForCompute = {
   "compute.glslfx",
};

std::string computeShadersVarsToChange[] = {
   "vertexOffset",
   "adjacencyOffset",
   "elementOffset",
   "topologyOffset",
   "pParamOffset",
   "indexOffset",
   "normalsOffset",
   "pointsOffset",
   "primvarOffset",
   "indexEnd",
   "primIndexEnd",
   "pointIndexStart",
   "pointIndexEnd",
   "sizesBase",
   "offsetsBase",
   "indicesBase",
   "weightsBase",
   "srcStride",
   "normalsStride",
   "dstStride",
   "primvarStride",
   "indexStride",
   "pParamStride",
   "pointsStride",
   "srcBase",
   "dstBase",
   //"drawRangeNDC",
   //"drawCommandNumUints",
   //"cullMatrix",
};

bool fix_computeShaders(const std::string& strFileName, std::string& strCode)
{
   //
   // I want to optimize a bit and not look in more places than I should
   // because all this regex searching takes too long already and besides there is a big risk of meeting 
   // identically named variables in other files

   bool bRet = false;

   if (strShadersToCheckForCompute.find(strFileName) != strShadersToCheckForCompute.end())
   {
      for (const std::string& varName : computeShadersVarsToChange)
      {
         std::string strReplace = "ConstParams." + varName;
         bRet = replaceText(strCode, std::regex(varName), strReplace) || bRet;
      }
   }

   return bRet;
}



bool fix_emit(std::string& strCode);
bool fix_endPrimitive(std::string& strCode);
bool fix_main(std::string& strCode);

bool fix_geometryShaders(const std::string& strFileName, std::string& strCode)
{
   bool bRet = false;

   if (0 == strFileName.compare("mesh.glslfx"))
   {
      bRet = fix_emit(strCode) || bRet;
      bRet = fix_endPrimitive(strCode) || bRet;
      bRet = fix_main(strCode) || bRet;
   }

   return bRet;
}

bool fix_emit(std::string& strCode)
{
   bool bRet = false;
   std::regex strFind(R"((void)? ?emit\(([a-zA-Z 0-9,]+)\))");
   std::string strFix = "";

   fcDealWithMatch fcEmit = [](std::string& strNewCode, const std::smatch& match) {
      std::string str1 = match[1].str(); // void or no void
      std::string str2 = match[2].str(); // normal content

      if (0 == str1.compare("void"))
      {
         //
         // this is a fc def
         strNewCode += "void emit(";
         strNewCode += str2;
         strNewCode += ", inout OutStream ts)";
      }
      else
      {
         //
         // this is a fc call
         strNewCode += " emit(";
         strNewCode += str2;
         strNewCode += ", ts)";
      }
   };

   bRet = replaceText(strCode, strFind, strFix, &fcEmit) || bRet;

   return bRet;
}
bool fix_endPrimitive(std::string& strCode)
{
   bool bRet = false;
   std::regex strFind1(R"(EndPrimitive\(\);)");
   std::string strFix1 = "EndPrimitive(ts);";

   std::regex strFind2(R"(EmitVertex\(\);)");
   std::string strFix2 = "EmitVertex(ts);";

   bRet = replaceText(strCode, strFind1, strFix1) || bRet;
   bRet = replaceText(strCode, strFind2, strFix2) || bRet;

   return bRet;
}
bool fix_main(std::string& strCode)
{
   bool bRet = false;
   std::regex strFind(R"(void main\(void\)(\n? ?\{[0-9a-zA-Z\s\n_=.,;\(\)\/\-\?\[\]:\+\|<>']*\}\n))");
   std::string strFix = "";

   fcDealWithMatch fcMain = [](std::string& strNewCode, const std::smatch& match) {
      std::string strFcContent = match[1];

      if (std::string::npos != strFcContent.find("emit("))
      {
         strNewCode += "void main (inout OutStream ts)";
         strNewCode += match[1];
      }
      else
      {
         //
         // leave it alone
         strNewCode += match[0];
      }
   };

   bRet = replaceText(strCode, strFind, strFix, &fcMain) || bRet;

   return bRet;
}



bool fix_matMultiplication_AB_vec4(std::string& strCode);
bool fix_matMultiplication_AB_eq(std::string& strCode);
bool fix_matMultiplication_ABC(std::string& strCode);

bool fix_matMultiplication(std::string& strCode)
{
   bool bRet = false;
   bRet = fix_matMultiplication_AB_vec4(strCode) || bRet;
   bRet = fix_matMultiplication_AB_eq(strCode) || bRet;
   bRet = fix_matMultiplication_ABC(strCode) || bRet;

   return bRet;
}

std::string matMulWhitelist[] = {
   "transform",
   "transformInv",
   "GetWorldToViewMatrix()",
   "GetWorldToViewInverseMatrix()",
   "GetProjectionMatrix()",
   "GetRotationMatrix",
   "GetInstanceTransform",
   "HdGet_instancerTransform",
   "projectionMatrix",
   "wvpMatrix",
   "wvMatrix",
   "Peye",
   "invT",
};

std::string matMulBlacklist[] = {
   "transpose",
   ".rgb",
   "worldSpaceWidth",
   "screenWidth",
};

bool fix_matMultiplication_AB_vec4(std::string& strCode) 
{
   //
   // in this case, I will try to change:
   // = vec4(a * b) -> = mul(a, b)
   // this case is special because I want to remove the "vec4" because it makes no sense
   // = transpose(a * b) -> = transpose(mul(a, b))
   // ...
   
   bool bRet = false;
   std::regex exprMatMulAB_TrInv(R"(((transpose)|(inverse))\(([0-9a-zA-Z()_.]+) *\n? *\* ?\n? *([0-9a-zA-Z()_.]+)\))");
   std::regex exprMatMulAB_Norm(R"((normalize\()\(([0-9a-zA-Z()_.]+) *\n? *\* ?\n? *([0-9a-zA-Z()_.,]+)(\).xyz\);))");
   std::regex exprMatMulAB_Vec4(R"(vec4\(([0-9a-zA-Z()_.]+)( *\n?\* ?\n? *)([0-9a-zA-Z()_,. ]+)\))");
   //vec4\(([0-9a-zA-Z()_.]+) ?\* ?([0-9a-zA-Z()_,. ]+)\)
   //vec4\(([0-9a-zA-Z()_., ]+)( +\n?\* ?\n? *)([0-9a-zA-Z()_,. ]+)\)

   fcDealWithMatch fcMulMatTrInv = [&bRet](std::string& strNewCode, const std::smatch& match) {
      //std::string strMatchStart = match[1].str();
      std::string strMatch1 = match[1].str();
      std::string strMatch2 = match[4].str();
      std::string strMatch3 = match[5].str();

      strNewCode += strMatch1;
      strNewCode += "(mul(";
      strNewCode += strMatch2;
      strNewCode += ", ";
      strNewCode += strMatch3;
      strNewCode += "))";

      bRet = true;
   };

   // TODO: maybe I can unify things a bit more
   // ex of this in simpleLighting.glslfx
   fcDealWithMatch fcMulMatNorm = [&bRet](std::string& strNewCode, const std::smatch& match) {
      //std::string strMatchStart = match[1].str();
      std::string strMatch1 = match[1].str();
      std::string strMatch2 = match[2].str();
      std::string strMatch3 = match[3].str();
      std::string strMatch4 = match[4].str();

      strNewCode += strMatch1;
      strNewCode += "mul(";
      strNewCode += strMatch2;
      strNewCode += ", ";
      strNewCode += strMatch3;
      strNewCode += strMatch4;

      bRet = true;
   };

   fcDealWithMatch fcMulMatVec4 = [&bRet](std::string& strNewCode, const std::smatch& match) {
      //std::string strMatchStart = match[1].str();
      std::string strMatch1 = match[1].str();
      std::string strMatch2 = match[3].str();

      strNewCode += "mul(";
      strNewCode += strMatch1;
      strNewCode += ", ";
      strNewCode += strMatch2;
      strNewCode += ")";

      bRet = true;
   };

   replaceText(strCode, exprMatMulAB_TrInv, "", &fcMulMatTrInv);
   replaceText(strCode, exprMatMulAB_Norm, "", &fcMulMatNorm);
   replaceText(strCode, exprMatMulAB_Vec4, "", &fcMulMatVec4);

   return bRet;
}
bool fix_matMultiplication_AB_eq(std::string& strCode)
{
   //
   // in this case, I will try to change:
   // = a * b -> = mul(a, b)
   // but since this obviously would match far too many cases, 
   // I will do a set of additional empyrical checks to try and only change the legitimate cases

   bool bRet = false;
   std::regex exprMatMulAB(R"(((= *)|(return *))\n? *([0-9a-zA-Z\(\)_,.]+) *\n?\t* *\* *\n?\t* *([0-9a-zA-Z\(\)_ ,]+)\;)");
   //= *\n? *([0-9a-zA-Z()_,.]+) *\n?\t* *\* *\n?\t* *([0-9a-zA-Z()_ ,]+)\;
   
   fcDealWithMatch fcMulMat = [&bRet](std::string& strNewCode, const std::smatch& match) {
      std::string strMatchStart = match[1].str();
      std::string strMatch1 = match[4].str();
      std::string strMatch2 = match[5].str();

      bool bReplace = false;

      for (const std::string& keyword : matMulWhitelist) {
         if ((std::string::npos != strMatch1.find(keyword)) ||
             (std::string::npos != strMatch2.find(keyword))) {
            bReplace = true;
            break;
         }
      }

      if (bReplace)
      {
         for (const std::string& keyword : matMulBlacklist) {
            if ((std::string::npos != strMatch1.find(keyword)) ||
                (std::string::npos != strMatch2.find(keyword))) {
               bReplace = false;
               break;
            }
         }
      }


      if (bReplace)
      {
         strNewCode += strMatchStart;
         strNewCode += "mul(";
         strNewCode += strMatch1;
         strNewCode += ", ";
         strNewCode += strMatch2;
         strNewCode += ");";
      }
      else
      {
         //
         // leave it as it was
         strNewCode += match[0].str();
      }

      bRet = true;
   };

   replaceText(strCode, exprMatMulAB, "", &fcMulMat);

   return bRet;
}
bool fix_matMultiplication_ABC(std::string& strCode)
{
   //
   // in this case, I will try to change:
   // a * b * c -> = mul(a, mul(b, c))
   // ofc, there is danger of modifying other multiplications which should not be modified, 
   // but that may actuallynot be such a big issue, 
   // because apparently the hslsl mul works for ints & floats also

   bool bRet = false;
   std::regex exprMatMulAB1(R"(\(([0-9a-zA-Z_.()]+) ?\* ?([0-9a-zA-Z_.]+) ?\* ?([0-9a-zA-Z_.(),]+)\))");

   fcDealWithMatch fcMulMat1 = [&bRet](std::string& strNewCode, const std::smatch& match) {
      //std::string strMatchStart = match[1].str();
      std::string strMatch1 = match[1].str();
      std::string strMatch2 = match[2].str();
      std::string strMatch3 = match[3].str();

      strNewCode += "(mul(";
      strNewCode += strMatch1;
      strNewCode += ", mul(";
      strNewCode += strMatch2;
      strNewCode += ", ";
      strNewCode += strMatch3;
      strNewCode += ")))";

      bRet = true;
   };

   replaceText(strCode, exprMatMulAB1, "", &fcMulMat1);

   //
   // next I want to search again for all the 3x multiplications that are on the right side of an "=" sign
   std::regex exprMatMulAB2(R"(= ?([0-9a-zA-Z_.]+?) ?\* ?([0-9a-zA-Z_.]+?) ?\* ?([0-9a-zA-Z_.]+?)\;)");

   fcDealWithMatch fcMulMat2 = [&bRet](std::string& strNewCode, const std::smatch& match) {
      //std::string strMatchStart = match[1].str();
      std::string strMatch1 = match[1].str();
      std::string strMatch2 = match[2].str();
      std::string strMatch3 = match[3].str();

      bool bReplace = true;
      for (const std::string& keyword : matMulBlacklist) {
         if ((std::string::npos != strMatch1.find(keyword)) ||
             (std::string::npos != strMatch2.find(keyword)) ||
             (std::string::npos != strMatch3.find(keyword))) {
            bReplace = false;
            break;
         }
      }
      
      if (bReplace)
      {
         strNewCode += "= mul(";
         strNewCode += strMatch1;
         strNewCode += ", mul(";
         strNewCode += strMatch2;
         strNewCode += ", ";
         strNewCode += strMatch3;
         strNewCode += "));";
      }
      else
      {
         //
         // leave it as it was
         strNewCode += match[0].str();
      }
      
      bRet = true;
   };

   replaceText(strCode, exprMatMulAB2, "", &fcMulMat2);

   return bRet;
}



// terminals.glslfx
bool fix_vec3_def_complex(std::string& strCode);

// frustumCull.glslfx
bool fix_mat4_cullMat(std::string& strCode);
bool fix_drawRangeNDC(std::string& strCode);
bool fix_drawCommandNumUints(std::string& strCode);
bool fix_ivec3_mistake(std::string& strCode);

// mesh.glslfx, meshNormal.glslfx, basisCurves.hlslfx
bool fix_layouts(std::string& strCode);
bool fix_special_mul_ms(std::string& strCode);

// instancing.glslfx
bool fix_big_mat_mul(std::string& strCode);
bool fix_special_mul(std::string& strCode);


bool fix_verySpecialCases(const std::string& strFileName, std::string& strCode)
{
   bool bRet = false;
   
   if (0 == strFileName.compare("terminals.glslfx"))
   {
      bRet = fix_vec3_def_complex(strCode) || bRet;
   }
   else if (0 == strFileName.compare("frustumCull.glslfx"))
   {
      bRet = fix_mat4_cullMat(strCode) || bRet;
      bRet = fix_drawRangeNDC(strCode) || bRet;
      bRet = fix_drawCommandNumUints(strCode) || bRet;
      bRet = fix_ivec3_mistake(strCode) || bRet;
   }
   else if (0 == strFileName.compare("mesh.glslfx"))
   {
      bRet = fix_layouts(strCode) || bRet;
      bRet = fix_special_mul_ms(strCode) || bRet;
   }
   else if (0 == strFileName.compare("basisCurves.glslfx"))
   {
      bRet = fix_special_mul_ms(strCode) || bRet;
   }
   else if (0 == strFileName.compare("meshNormal.glslfx"))
   {
      bRet = fix_special_mul_ms(strCode) || bRet;
   }
   else if (0 == strFileName.compare("instancing.glslfx"))
   {
      bRet = fix_big_mat_mul(strCode) || bRet;
      bRet = fix_special_mul(strCode) || bRet;
   }

   return bRet;
}

bool fix_vec3_def_complex(std::string& strCode)
{
   std::regex strFind(R"(result.color = vec3\(pow\(HdGet_scalarOverride\(\), 2.2\)\);)");
   std::string strFix = "float valCol = pow(HdGet_scalarOverride(), 2.2);\n"
                    "    result.color = vec3(valCol, valCol, valCol);";
   return replaceText(strCode, strFind, strFix);
}

bool fix_mat4_cullMat(std::string& strCode)
{
   std::regex strFind(R"(MAT4\(cullMatrix\);)");
   std::string strFix = "ConstParams.cullMatrix;";
   return replaceText(strCode, strFind, strFix);
}
bool fix_drawRangeNDC(std::string& strCode)
{
   std::regex strFind(R"(toClip, localMin, localMax, drawRangeNDC)");
   std::string strFix = "toClip, localMin, localMax, ConstParams.drawRangeNDC";
   return replaceText(strCode, strFind, strFix);
}
bool fix_drawCommandNumUints(std::string& strCode)
{
   std::regex strFind(R"(drawCommandNumUints \+ instanceCountOffset)");
   std::string strFix = "ConstParams.drawCommandNumUints + instanceCountOffset";
   return replaceText(strCode, strFind, strFix);
}
bool fix_ivec3_mistake(std::string& strCode)
{
   bool bRet = false;
   std::regex strFind1(R"(ivec3\(clip0,clip0,clip0\))");
   std::string strFix1 = "ivec3(clip0)";
   bRet = replaceText(strCode, strFind1, strFix1) || bRet;

   std::regex strFind2(R"(ivec3\(clip1,clip1,clip1\))");
   std::string strFix2 = "ivec3(clip1)";
   bRet = replaceText(strCode, strFind2, strFix2) || bRet;

   return bRet;
}

bool fix_layouts(std::string& strCode)
{
   bool bRet = false;
   std::regex strFind1(R"(-- layout Mesh.Fragment[\n\[ "a-z,A-Z0-9\]]*(\["vec3", "Neye"\])\n* *\])");
   std::regex strFind2(R"((-- layout Mesh.Geometry.[\n\[ "a-z,A-Z0-9\]_]*)(\["out", "vec4", "gsPatchCoord"\]))");
   std::string strFix = "";

   fcDealWithMatch fcLayout_1 = [](std::string& strNewCode, const std::smatch& match) {
      std::string strExisting = match[0].str();
      
      strNewCode += strExisting;
      strNewCode += ",\n"
         "    [\"in\", \"uint\", \"gl_PrimitiveID\"]";
   };

   fcDealWithMatch fcLayout_2 = [](std::string& strNewCode, const std::smatch& match) {
      std::string strExisting = match[1].str();

      strNewCode += strExisting;
      strNewCode += "[\"out\", \"uint\", \"gl_PrimitiveID\"],\n"
         "    [\"out\", \"vec4\", \"gsPatchCoord\"],\n"
         "    [\"out\", \"vec4\", \"gl_Position\"]";
   };

   bRet = replaceText(strCode, strFind1, strFix, &fcLayout_1) || bRet;
   bRet = replaceText(strCode, strFind2, strFix, &fcLayout_2) || bRet;


   return bRet;
}

bool fix_special_mul_ms(std::string& strCode)
{
   bool bRet = false;

   std::regex strFind(R"(vec4(\(transpose\(mul\(transformInv, GetWorldToViewInverseMatrix\(\)\)\)) \*\n *(vec4\([a-zA-Z]+,0\)\)))");
   std::string strFix = "";

   fcDealWithMatch fcfixmul = [](std::string& strNewCode, const std::smatch& match) {
      std::string str1 = match[1].str();
      std::string str2 = match[2].str();

      strNewCode += "mul";
      strNewCode += str1;
      strNewCode += ", ";
      strNewCode += str2;
   };

   std::regex strFind2(R"(vec4(\(GetWorldToViewMatrix\(\)) \* transform \*\n *(vec4\([a-zA-Z_()]+, [0-9.]+\)\)))");
   fcDealWithMatch fcfixmul2 = [](std::string& strNewCode, const std::smatch& match) {
      std::string str1 = match[1].str();
      std::string str2 = match[2].str();

      strNewCode += "mul";
      strNewCode += str1;
      strNewCode += ", mul(transform, ";
      strNewCode += str2;
      strNewCode += ")";
   };
   
   bRet = replaceText(strCode, strFind, strFix, &fcfixmul) || bRet;
   bRet = replaceText(strCode, strFind2, strFix, &fcfixmul2) || bRet;

   return bRet;
}

bool fix_big_mat_mul(std::string& strCode)
{
   bool bRet = false;
   std::regex strFind(R"(= MAT4\(([a-z0-9,. \n]*)\) \* m;)");
   std::string strFix = "";

   fcDealWithMatch fcbigmatmul = [](std::string& strNewCode, const std::smatch& match) {
      std::string strMatContent = match[1].str();

      strNewCode += "= mul(MAT4(";
      strNewCode += strMatContent;
      strNewCode += "), m);";
   };

   bRet = replaceText(strCode, strFind, strFix, &fcbigmatmul) || bRet;


   return bRet;
}
bool fix_special_mul(std::string& strCode)
{
   std::regex strFind(R"(HdGetInstance_instanceTransform\(level, MAT4Init\(1\)\) \* m)");
   std::string strFix = "mul(HdGetInstance_instanceTransform(level, MAT4Init(1)), m)";
   return replaceText(strCode, strFind, strFix);
}