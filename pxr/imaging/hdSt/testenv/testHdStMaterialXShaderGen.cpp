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

#include <MaterialXGenShader/Util.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace mx = MaterialX;

bool IsDifferentFrom(mx::InputPtr const& input, float value) {
    if (!input) {
        return false;
    }

    if (input->hasValue()) {
        auto val = input->getValue();
        if (val->isA<float>()) {
            return val->asA<float>() != value;
        }
        return true;
    }

    return input->hasNodeName() || input->hasNodeGraphString() ||
           input->hasOutputString() || input->hasInterfaceName();
}

bool IsTranslucentDocument(mx::DocumentPtr const& mxDoc)
{
    // Find renderable elements in the Mtlx Document.
    std::vector<mx::TypedElementPtr> renderableElements;
    mx::findRenderableElements(mxDoc, renderableElements);

    // Should have exactly one renderable element (material).
    if (renderableElements.size() != 1) {
        TF_CODING_ERROR("Generated MaterialX Document does not "
                        "have 1 material");
        return false;
    }

    // Extract out the Surface Shader Node for the Material Node
    mx::TypedElementPtr renderableElem = renderableElements.at(0);
    mx::NodePtr node = renderableElem->asA<mx::Node>();
    if (node && node->getType() == mx::MATERIAL_TYPE_STRING) {
        // Use auto so can compile against MaterialX 1.38.0 or 1.38.1
        auto mxShaderNodes = mx::getShaderNodes(node, mx::SURFACE_SHADER_TYPE_STRING);
        if (!mxShaderNodes.empty()) {
            renderableElem = *mxShaderNodes.begin();
        }
    }

    auto renderableNode = renderableElem->asA<mx::Node>();
    if (renderableNode && renderableNode->getCategory() == "UsdPreviewSurface")
    {
        // The custom code to handle masked mode prevents MaterialX from auto-deducing
        // transparency correctly:
        return IsDifferentFrom(renderableNode->getInput("opacity"), 1.0f) ||
               IsDifferentFrom(renderableNode->getInput("opacityThreshold"), 0.0f);

        // Note that if we start introducing glTf tests, they will also incorrectly
        // be detected as transparent due to the presence of the alpha_mode input
        // which allows masking.
    }
    return mx::isTransparentSurface(renderableElements.at(0));
}

void TestShaderGen(
    const mx::FilePath& mtlxFilename, 
    HdSt_MxShaderGenInfo& mxHdInfo)
{
    // Get Standard Libraries and SearchPaths (for mxDoc and mxShaderGen)
    const mx::DocumentPtr& stdLibraries = HdMtlxStdLibraries();
    const mx::FileSearchPath& searchPaths = HdMtlxSearchPaths();

    // Read the mtlx file
    mx::DocumentPtr mxDoc = mx::createDocument();
    fprintf(stderr, "reading the mtlx file: \n - %s\n", mtlxFilename.asString().c_str());

    try {
        mx::readFromXmlFile(mxDoc, mtlxFilename, searchPaths);
    } catch (const mx::Exception& e) {
        TF_WARN("mx::readFromXmlFile threw an exception: %s", e.what());
        return;
    }

    mxDoc->importLibrary(stdLibraries);

    // Validate the document.
    std::string message;
    if (!mxDoc->validate(&message)) {
        std::cerr << "*** Validation warnings for " 
                  << mtlxFilename.getBaseName() << " ***" << std::endl;
        std::cerr << message;
    }

    mxHdInfo.materialTag = IsTranslucentDocument(mxDoc) ?
                           "translucent" :
                           "defaultMaterialTag";

    // Generate the HdSt MaterialX Shader
    mx::ShaderPtr glslfx = HdSt_GenMaterialXShader(
        mxDoc, stdLibraries, searchPaths, mxHdInfo, HgiTokens->OpenGL);
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
        if (arg == "--bindless") {
            mxHdInfo.bindlessTexturesEnabled = true;
        }
    }
    TestShaderGen(mtlxFile, mxHdInfo);
}
