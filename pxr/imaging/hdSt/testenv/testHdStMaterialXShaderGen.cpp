//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/diagnostic.h"

#include <MaterialXGenShader/Util.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace mx = MaterialX;

// Helpers to determine the Material Tag
// Note that similar helpers live in HdStMaterialXFilter

static bool 
_IsDifferentFrom(mx::InputPtr const& mxInput, float value)
{
    if (!mxInput) {
        return false;
    }

    if (mxInput->hasValue()) {
        const mx::ValuePtr val = mxInput->getValue();
        if (val->isA<float>()) {
            return val->asA<float>() != value;
        }
        return true;
    }

    return mxInput->hasNodeName() || mxInput->hasNodeGraphString() ||
           mxInput->hasOutputString() || mxInput->hasInterfaceName();
}

static TfToken const&
_GetUsdPreviewSurfaceMaterialTag(mx::NodePtr const &terminalNode)
{
    // See https://openusd.org/release/spec_usdpreviewsurface.html
    // and implementation in MaterialX libraries/bxdf/usd_preview_surface.mtlx

    // Non-zero opacityThreshold (or connected) triggers masked mode:
    if (_IsDifferentFrom(terminalNode->getInput("opacityThreshold"), 0.0f)) {
        return HdStMaterialTagTokens->masked;
    }

    // Opacity less than 1.0 (or connected) triggers transparent mode:
    if (_IsDifferentFrom(terminalNode->getInput("opacity"), 1.0f)) {
        return HdStMaterialTagTokens->translucent;
    }

    return HdStMaterialTagTokens->defaultMaterialTag;
}

static TfToken
_GetMaterialTag(mx::DocumentPtr const& mxDoc)
{
    // Find renderable elements in the Mtlx Document.
    // Note this code also lives in HdSt_GenMaterialXShader()
    std::vector<mx::TypedElementPtr> renderableElements;
    mx::findRenderableElements(mxDoc, renderableElements);

    // Should have exactly one renderable element (material).
    if (renderableElements.size() != 1) {
        TF_CODING_ERROR("Generated MaterialX Document does not "
                        "have 1 material");
        return HdStMaterialTagTokens->defaultMaterialTag;
    }

    // Extract out the Surface Shader Node for the Material Node
    mx::TypedElementPtr renderableElem = renderableElements.at(0);
    mx::NodePtr node = renderableElem->asA<mx::Node>();
    if (node && node->getType() == mx::MATERIAL_TYPE_STRING) {
        // Use auto so can compile against MaterialX 1.38.0 or 1.38.1
        auto mxShaderNodes =
            mx::getShaderNodes(node, mx::SURFACE_SHADER_TYPE_STRING);
        if (!mxShaderNodes.empty()) {
            renderableElem = *mxShaderNodes.begin();
        }
    }

    // The custom code to handle masked mode prevents MaterialX from 
    // correctly deducing transparency with mx::isTransparentSurface()
    node = renderableElem->asA<mx::Node>();
    if (node && node->getCategory() == "UsdPreviewSurface") {
        return _GetUsdPreviewSurfaceMaterialTag(node);
    }

    // XXX: Once other material tests are added (eg. glTf) similar helper 
    // helper functions will need to be added to get the correct MaterialTag
    if (mx::isTransparentSurface(renderableElem)) {
        return HdStMaterialTagTokens->translucent;
    }
    return HdStMaterialTagTokens->defaultMaterialTag;
}

void TestShaderGen(
    const mx::FilePath& mtlxFilename, 
    HdSt_MxShaderGenInfo* mxHdInfo)
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

    mxHdInfo->materialTag = _GetMaterialTag(mxDoc);

    // Generate the HdSt MaterialX Shader
    mx::ShaderPtr glslfx = HdSt_GenMaterialXShader(
        mxDoc, stdLibraries, searchPaths, *mxHdInfo, HgiTokens->OpenGL);
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
    TestShaderGen(mtlxFile, &mxHdInfo);
}
