//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/renderPassShaderKey.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // Glslfx files to include
    ((renderPassGlslfx,              "renderPass.glslfx"))
    ((selectionGlslfx,               "selection.glslfx"))

    // render pass mixins
    ((renderPassCamera,              "RenderPass.Camera"))
    ((renderPassCameraFS,            "RenderPass.CameraFS"))
    ((renderPassApplyClipPlanes,     "RenderPass.ApplyClipPlanes"))
    ((renderPassApplyColorOverrides, "RenderPass.ApplyColorOverrides"))
    ((renderPassRenderOutput,        "RenderPass.RenderOutput"))
    ((renderPassNoSelection,         "RenderPass.NoSelection"))
    ((renderPassNoColorOverrides,    "RenderPass.NoColorOverrides"))

    // selection mixins
    ((selectionDecodeUtils,          "Selection.DecodeUtils"))
    ((selectionComputeColor,         "Selection.ComputeColor"))

    // Render AOV outputs mixins
    ((renderPassRenderColor,         "RenderPass.RenderColor"))
    ((renderPassRenderColorNoOp,     "RenderPass.RenderColorNoOp"))

    ((renderPassRenderId,            "RenderPass.RenderId"))
    ((renderPassRenderIdNoOp,        "RenderPass.RenderIdNoOp"))

    ((renderPassRenderNeye,          "RenderPass.RenderNeye"))
    ((renderPassRenderNeyeNoOp,      "RenderPass.RenderNeyeNoOp"))
);

static
bool
_AovHasIdSemantic(TfToken const & name)
{
    // For now id render only means primId or instanceId.
    return name == HdAovTokens->primId ||
           name == HdAovTokens->instanceId;
}

HdSt_RenderPassShaderKey::HdSt_RenderPassShaderKey(
    const HdRenderPassAovBindingVector& aovBindings)
    : glslfx(_tokens->renderPassGlslfx)
{
    bool renderColor = false;
    bool renderId = false;
    bool renderNeye = false;

    for (size_t i = 0; i < aovBindings.size(); ++i) {
        if (!renderColor && aovBindings[i].aovName == HdAovTokens->color) {
            renderColor = true;
        }
        if (!renderId && _AovHasIdSemantic(aovBindings[i].aovName)) {
            renderId = true;
        }
        if (!renderNeye && aovBindings[i].aovName == HdAovTokens->Neye) {
            renderNeye = true;
        }
    }

    VS[0] = _tokens->renderPassCamera;
    VS[1] = _tokens->renderPassApplyClipPlanes;
    VS[2] = TfToken();

    PTCS[0] = _tokens->renderPassCamera;
    PTCS[1] = TfToken();

    PTVS[0] = _tokens->renderPassCamera;
    PTVS[1] = _tokens->renderPassApplyClipPlanes;
    PTVS[2] = TfToken();

    TCS[0] = _tokens->renderPassCamera;
    TCS[1] = TfToken();

    TES[0] = _tokens->renderPassCamera;
    TES[1] = _tokens->renderPassApplyClipPlanes;
    TES[2] = TfToken();

    GS[0] = _tokens->renderPassCamera;
    GS[1] = _tokens->renderPassApplyClipPlanes;
    GS[2] = TfToken();

    uint8_t fsIndex = 0;
    FS[fsIndex++] = _tokens->renderPassCamera;
    FS[fsIndex++] = _tokens->renderPassCameraFS;

    if (renderColor) {
        FS[fsIndex++] = _tokens->selectionDecodeUtils;
        FS[fsIndex++] = _tokens->selectionComputeColor;
        FS[fsIndex++] = _tokens->renderPassApplyColorOverrides;
        FS[fsIndex++] = _tokens->renderPassRenderColor;
    } else {
        FS[fsIndex++] = _tokens->renderPassNoSelection;
        FS[fsIndex++] = _tokens->renderPassNoColorOverrides;
        FS[fsIndex++] = _tokens->renderPassRenderColorNoOp;
    }

    if (renderId) {
        FS[fsIndex++] = _tokens->renderPassRenderId;
    } else {
        FS[fsIndex++] = _tokens->renderPassRenderIdNoOp;
    }

    if (renderNeye) {
        FS[fsIndex++] = _tokens->renderPassRenderNeye;
    } else {
        FS[fsIndex++] = _tokens->renderPassRenderNeyeNoOp;
    }

    FS[fsIndex++] = _tokens->renderPassRenderOutput;
    FS[fsIndex++] = TfToken();
}

HdSt_RenderPassShaderKey::~HdSt_RenderPassShaderKey() = default;

std::string
HdSt_RenderPassShaderKey::GetGlslfxString() const
{
    std::stringstream ss;

    ss << "-- glslfx version 0.1\n";

    if (!GetGlslfxFilename().IsEmpty()) {
        ss << "#import $TOOLS/hdSt/shaders/" 
           << GetGlslfxFilename().GetText() << "\n";
    }
    // Additionally need one more import.
    ss << "#import $TOOLS/hdx/shaders/" << _tokens->selectionGlslfx << "\n";

    ss << "-- configuration\n"
       << "{\"techniques\": {\"default\": {\n";

    bool firstStage = true;
    ss << _JoinTokens("vertexShader",      GetVS(),  &firstStage);
    ss << _JoinTokens("tessControlShader", GetTCS(), &firstStage);
    ss << _JoinTokens("tessEvalShader",    GetTES(), &firstStage);
    ss << _JoinTokens("postTessControlShader",  GetPTCS(), &firstStage);
    ss << _JoinTokens("postTessVertexShader",   GetPTVS(), &firstStage);
    ss << _JoinTokens("geometryShader",    GetGS(),  &firstStage);
    ss << _JoinTokens("fragmentShader",    GetFS(),  &firstStage);
    ss << "}}}\n";

    return ss.str();
}


PXR_NAMESPACE_CLOSE_SCOPE