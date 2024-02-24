
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

//
// This class (together with the other 2 files in this project) 
// is meant to be a tool that reads ".glslfx" files 
// and attempts to "translate" them to what I called ".hlslfx".
// What this means in practice, is copy the ".glslfx" and make minor changes 
// here and there to conform to HLSL requirements and to account for 
// some glsl -> HLSL differences. 
// The most interesting (and difficult to manage) example of such a difference
// is the matrix multiplication, because in HLSL "mat1 * mat2" 
// does not perform a mathematical multiplication as glsl does but rather 
// it multiplies matices members 1:1, e.g. mat[i][j] = mat1[i][j] * mat2[i][j].


#include <windows.h>

#include <array>
#include <set>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <Wincrypt.h>
#include <atlbase.h>

#include "shadersDefines.h"


using namespace std;
using namespace shadersAnalysisDefines;

void getAllLibsInFolder(const std::string& strFolder, std::vector<std::string>& libs);
bool buildAndCompareHash(const std::string& strShaderLibFolder, const std::string& strPrefix, const std::string& strGLSourceFile);
void buildDXShaderLibVersion(const std::string& strShaderLibFolder, const std::string& strGLSourceFile, const std::string& strFullOutFileName);
bool fileExists(const std::string& strFileName);

void compareGenerated_vs_Manual(const std::string& strFullSourceFolderPath,
                                const std::string& strReferencesDir,
                                const std::string& strLib,
                                const std::string& strPrefix);

//
// For debug purposes, to help focus on a particular issue 
std::set<std::string> filesSubsetTestTemp = {
   //"basisCurves.glslfx",
};

bool bOptimizeNotExecuteWhenSourceUnchanged = true;

int main(int argc, char* argv[])
{
   std::cout << "GL -> DX Shaders generation:" << std::endl;

   //::MessageBoxA(nullptr, "running", "running", MB_OK);

   //
   // this is an intermediary stage towards fully automating glslfx reuse for DirectX

   //
   // what I want to do for now is read GL shaders libraries (glslfx) and do several things:

   //
   // 1. generate a DX version of the library via some code transformations (regex changes).
   // 2. compare to the manually modified libraries, see how close I can get to the needed state automatically.
   // 3. compare the MD5 of the current shaders vs the "known" versions so I get a warning when shaders changed and may need manual review
   // 4. obtain a summary of code / files I cannot convert automatically for further consideration

   std::string strRootInstallDir = "";
   std::string strReferencesDir = "";

   //
   // very basic input parsing here
   for (int i = 1; i != argc; ++i) {
      if (strcmp(argv[i], "-installDir") == 0) {
         if (i < argc - 1)
            strRootInstallDir = argv[++i];
      }
      else if (strcmp(argv[i], "-refDir") == 0) {
         if (i < argc - 1)
            strReferencesDir = argv[++i];
      }
   }

   if (strRootInstallDir.length() < 1)
   {
      std::cout << "Missing install folder parameter. Cannot continue" << std::endl;
      return -1;
   }

   std::cout << "Using install dir: " << strRootInstallDir << ", and references dir: " << strReferencesDir << std::endl;

   for (const auto& folder : shadersLibsFolders)
   {
      const std::string& strFolder = folder.second;
      cout << endl << folder.first << ": " << endl;

      //
      // find all "*.glslfx" files
      std::vector<std::string> allShadersLibsInFolder;
      std::string strFullSourceFolderPath = strRootInstallDir + strFolder;
      getAllLibsInFolder(strFullSourceFolderPath, allShadersLibsInFolder);

      //
      // for each source glslfx:
      for (const std::string& strLibFile : allShadersLibsInFolder)
      {
         cout << "\t" << strLibFile << endl;
         if (filesSubsetTestTemp.size() > 0)
         {
            if (filesSubsetTestTemp.find(strLibFile) == filesSubsetTestTemp.end())
            {
               cout << "\t\tfiltered out." << endl;
               continue;
            }
         }

         //
         // build the output file name
         std::string strOutFileName;
         int nExtPos = strLibFile.find(strGlLibExt);
         if (nExtPos != std::string::npos)
            strOutFileName = strLibFile.substr(0, nExtPos);
         else
            strOutFileName = strLibFile;
         strOutFileName += strHlLibExt;

         std::string strFullOutFileName = strFullSourceFolderPath + "\\" + strOutFileName;

         bool bSourceUnchanged = buildAndCompareHash(strFullSourceFolderPath, folder.first, strLibFile);

         if (bOptimizeNotExecuteWhenSourceUnchanged)
         {
            //
            // I'll check that the destination filename exists.
            // 
            // If it does not, I'll continue
            //
            // If it does, I'll do some more checks to maybe avoid re-doing the translation
            if (fileExists(strFullOutFileName) && bSourceUnchanged)
            {
               cout << "\t\t Destination file exists and source is unchanged. Skipping re-generation." << endl;
               continue;
            }
         }

         buildDXShaderLibVersion(strFullSourceFolderPath, strLibFile, strFullOutFileName);
         compareGenerated_vs_Manual(strFullSourceFolderPath, strReferencesDir, strLibFile, folder.first);
      }
   }

   return 0;
}

void
getAllLibsInFolder(const std::string& strFolder, std::vector<std::string>& libs)
{
   WIN32_FIND_DATAA fd;
   int nExtLen = strGlLibExt.length();

   std::string strSearchPat = strFolder + "\\*" + strGlLibExt;

   HANDLE hFind = FindFirstFileA(strSearchPat.c_str(), &fd);
   BOOL bFound = (hFind != INVALID_HANDLE_VALUE);
   while (bFound)
   {
      std::string strFileName = fd.cFileName;
      libs.push_back(strFileName);
      bFound = FindNextFileA(hFind, &fd);
   }

   int nErr = GetLastError();
   if (ERROR_NO_MORE_FILES != nErr)
      cout << "Unexpected error finding files" << nErr << endl;

   FindClose(hFind);
}

std::string
guidToString(GUID guid) {
   std::array<char, 40> output;
   snprintf(output.data(), output.size(), "{%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
   return std::string(output.data());
}

std::string
getHash(const std::string& strFullFilePath)
{
   std::string ret;

   std::ifstream fileIn(strFullFilePath.c_str(), std::ios::in);
   std::stringstream buffer;
   buffer << fileIn.rdbuf();
   std::string strCode = buffer.str();
   fileIn.close();

   int nCodeLength = strCode.length();
   if (nCodeLength > 0)
   {
      HCRYPTPROV hProv = 0;
      if (CryptAcquireContext(&hProv,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT))
      {
         HCRYPTHASH hHash = 0;
         if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
         {
            if (CryptHashData(hHash, (const BYTE*)strCode.c_str(), nCodeLength, 0))
            {
               DWORD cbHash = MD5LEN;

               GUID hashGuid = GUID_NULL;
               CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)&hashGuid, &cbHash, 0);
               ret = guidToString(hashGuid);
            }

            CryptDestroyHash(hHash);
         }

         CryptReleaseContext(hProv, 0);
      }
   }
   else
      cout << "\t\tFailed to open file: " << strFullFilePath << endl;

   return ret;
}

bool
buildAndCompareHash(const std::string& strShaderLibFolder, const std::string& strPrefix, const std::string& strGLSourceFile)
{
   bool bEqual = false;
   std::string strFullPath = strShaderLibFolder + "\\" + strGLSourceFile;
   std::string strCurrHash = getHash(strFullPath);

   if (strCurrHash.length() > 0)
   {
      std::string strLibToFind = strPrefix + "/" + strGLSourceFile;

      std::map<std::string, std::string>::const_iterator it = knownHashes.find(strLibToFind);
      if (it == knownHashes.end())
         cout << "\t\tWarning: " << strGLSourceFile << " hash not present.Current hash : " << strCurrHash << endl;
      else
      {
         if (0 == it->second.compare(strCurrHash))
         {
            bEqual = true;
            cout << "\t\tFile identical to baseline: " << strGLSourceFile << " Hash: " << strCurrHash << endl;
         }
         else
            cout << "\t\tWarning: " << strGLSourceFile << " differs from baseline. Current hash : " << strCurrHash << ", Previous hash : " << it->second << endl;
      }
   }

   return bEqual;
}



bool executeCodeTranslation(const std::string& strSourceFileName, std::string& strCode);

void
buildDXShaderLibVersion(const std::string& strShaderLibFolder, const std::string& strGLSourceFile, const std::string& strFullOutFileName)
{
   std::string strFullPath = strShaderLibFolder + "\\" + strGLSourceFile;

   std::ifstream fileIn(strFullPath.c_str(), std::ios::in);
   std::stringstream buffer;
   buffer << fileIn.rdbuf();
   std::string strCode = buffer.str();
   fileIn.close();

   if (strCode.length() > 0)
   {
      cout << "\t\tTranslating: " << strFullPath << endl;

      bool bSomethingChanged = executeCodeTranslation(strGLSourceFile, strCode);

      if (bSomethingChanged)
      {
         //
         // output the final result in an "output" folder
         // for now how about keeping this the same as the input folder

         cout << "\t\tWriting translation: " << strFullOutFileName << endl;
         std::ofstream fileOut(strFullOutFileName.c_str(), std::ios::out);
         fileOut << strCode;
         fileOut.flush();
         fileOut.close();

      }
      else
         cout << "   No changes needed for this file." << endl;
   }

}

void
compareGenerated_vs_Manual(const std::string& strFullSourceFolderPath,
                           const std::string& strReferencesDir,
                           const std::string& strLib,
                           const std::string& strPrefix)
{
   size_t nPos = strLib.find(strGlLibExt);
   if (nPos != std::string::npos)
   {
      std::string strHlslfxFileName = strLib.substr(0, nPos) + strHlLibExt;
      std::string strPathGenerated = strFullSourceFolderPath + +"\\" + strHlslfxFileName;
      std::string strPathReference = strReferencesDir + "\\" + strPrefix + "\\" + strHlslfxFileName;

      //
      // I might not have references for all generated files for one reason or another
      // so look for that first
      if (!fileExists(strPathReference))
         cout << "\t\tReference file not found, skipping compare..." << endl;
      else
      {
         //
         // now I want to do the simplest compare...

         //
         // but since I have a hash computing fc ready, I'll just use that
         std::string strHashGenerated = getHash(strPathGenerated);
         std::string strHashReference = getHash(strPathReference);

         if (strHashGenerated.compare(strHashReference) == 0)
            cout << "\t\t+ OK: Generated file is identical to reference." << endl;
         else
            cout << "\t\t- WARNING: Generated file is different from reference." << endl;
      }
   }
   else
      cout << "\t\tError while trying to compare files..." << endl;
}

bool fileExists(const std::string& strFileName)
{
   bool bRet = true;
   //
   // kind of backwards, but good enough for now.
   if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(strFileName.c_str()) && GetLastError() == ERROR_FILE_NOT_FOUND)
      bRet = false;
   return bRet;
}