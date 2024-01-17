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
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/diagnostic.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace mx = MaterialX;


void TestShaderGen(
    const mx::FilePath& mtlxFilename, 
    const HdSt_MxShaderGenInfo& mxHdInfo)
{
    // Load Standard Libraries/setup SearchPaths (for mxDoc and mxShaderGen)
    mx::FilePathVec libraryFolders;
    mx::FileSearchPath searchPath = HdMtlxSearchPaths();
    mx::DocumentPtr stdLibraries = mx::createDocument();
    mx::loadLibraries(libraryFolders, searchPath, stdLibraries);

    // Read the mtlx file
    mx::DocumentPtr mxDoc = mx::createDocument();
    fprintf(stderr, "reading the mtlx file: \n - %s\n", mtlxFilename.asString().c_str());
    mx::readFromXmlFile(mxDoc, mtlxFilename, searchPath);
    mxDoc->importLibrary(stdLibraries);

    // Validate the document.
    std::string message;
    if (!mxDoc->validate(&message)) {
        std::cerr << "*** Validation warnings for " 
                  << mtlxFilename.getBaseName() << " ***" << std::endl;
        std::cerr << message;
    }

    // Generate the HdSt MaterialX Shader
    mx::ShaderPtr glslfx = HdSt_GenMaterialXShader(
        mxDoc, stdLibraries, searchPath, mxHdInfo, HgiTokens->OpenGL);
    std::cout << glslfx->getSourceCode(mx::Stage::PIXEL);
}

int main(int argc, char *argv[])
{
    HdSt_MxShaderGenInfo mxHdInfo;
    mx::FilePath mtlxFile = "standard_surface_default.mtlx";

    for (int i=0; i<argc; ++i) {
        const std::string arg(argv[i]);

        if (arg == "--filename") {
            mtlxFile = mx::FilePath(argv[++i]);
        }
        if (arg == "--textureMap") {
            const std::string textureMap(argv[++i]);
            auto const& npos = textureMap.find(":");
            if (npos != std::string::npos) {
                const std::string mx = textureMap.substr(0, npos);
                const std::string hd = textureMap.substr(npos + 1);
                mxHdInfo.textureMap[mx] = hd;
            }
            else {
                std::cerr << "textureMap input not formatted correctly.\n";
                std::cerr << "--textureMap <mxTextureNodeName>:<HdInptName>\n";
                return EXIT_FAILURE;
            }
        }
        if (arg == "--primvarMap") {
            const std::string primvarMap(argv[++i]);
            auto const& npos = primvarMap.find(":");
            if (npos != std::string::npos) {
                const std::string name = primvarMap.substr(0, npos);
                const std::string type = primvarMap.substr(npos + 1);
                mxHdInfo.primvarMap[name] = type;
            }
            else {
                std::cerr << "primvarMap input not formatted correctly.\n";
                std::cerr << "--primvarMap <PrimvarName>:<PrimvarType>\n";
                return EXIT_FAILURE;
            }
        }
        if (arg == "--materialTag") {
            mxHdInfo.materialTag = argv[++i];
        }
        if (arg == "--bindless") {
            mxHdInfo.bindlessTexturesEnabled = true;
        }
    }
    TestShaderGen(mtlxFile, mxHdInfo);
}
