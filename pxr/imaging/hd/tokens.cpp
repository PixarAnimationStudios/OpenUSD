//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/token.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdTokens, HD_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdInstancerTokens, HD_INSTANCER_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdReprTokens, HD_REPR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdCullStyleTokens, HD_CULLSTYLE_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPerfTokens, HD_PERF_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdShaderTokens, HD_SHADER_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdRenderTagTokens, HD_RENDERTAG_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdRenderContextTokens, HD_RENDER_CONTEXT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdMaterialTagTokens, HD_MATERIALTAG_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdMaterialTerminalTokens, HD_MATERIAL_TERMINAL_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdModelDrawModeTokens, HD_MODEL_DRAWMODE_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdOptionTokens, HD_OPTION_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdLightTypeTokens, HD_LIGHT_TYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdLightFilterTypeTokens, HD_LIGHT_FILTER_TYPE_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdRprimTypeTokens, HD_RPRIMTYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdSprimTypeTokens, HD_SPRIMTYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdBprimTypeTokens, HD_BPRIMTYPE_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrimTypeTokens, HD_PRIMTYPE_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrimvarRoleTokens, HD_PRIMVAR_ROLE_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdAovTokens, HD_AOV_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdRenderSettingsTokens, HD_RENDER_SETTINGS_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdRenderSettingsPrimTokens,
                        HD_RENDER_SETTINGS_PRIM_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdAspectRatioConformPolicyTokens, 
                        HD_ASPECT_RATIO_CONFORM_POLICY);

TF_DEFINE_PUBLIC_TOKENS(HdResourceTypeTokens, HD_RESOURCE_TYPE_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdSceneIndexEmulationTokens, 
    HD_SCENE_INDEX_EMULATION_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdCollectionEmulationTokens, 
    HD_COLLECTION_EMULATION_TOKENS);

bool HdPrimTypeIsGprim(TfToken const& primType)
{
    return (primType == HdPrimTypeTokens->mesh ||
            primType == HdPrimTypeTokens->basisCurves ||
            primType == HdPrimTypeTokens->points ||
            primType == HdPrimTypeTokens->volume);
}

const TfTokenVector &HdLightPrimTypeTokens()
{
    return HdLightTypeTokens->allTokens;
}

bool HdPrimTypeIsLight(TfToken const& primType)
{
    const auto &lightTypes = HdLightPrimTypeTokens();
    return std::find(lightTypes.begin(), lightTypes.end(), primType) !=
        lightTypes.end();
}

bool HdPrimTypeSupportsGeomSubsets(const TfToken& primType) {
    static const TfTokenVector types = {
        HdPrimTypeTokens->mesh,
        HdPrimTypeTokens->basisCurves,
        // XXX: tetMesh not yet supported
    };
    return std::find(types.begin(), types.end(), primType) != types.end();
}

TfToken HdAovTokensMakePrimvar(TfToken const& primvar)
{
    return TfToken(
        HdAovTokens->primvars.GetString() +
        primvar.GetString());
}

TfToken HdAovTokensMakeLpe(TfToken const& lpe)
{
    return TfToken(
        HdAovTokens->lpe.GetString() +
        lpe.GetString());
}

TfToken HdAovTokensMakeShader(TfToken const& shader)
{
    return TfToken(
        HdAovTokens->shader.GetString() +
        shader.GetString());
}

PXR_NAMESPACE_CLOSE_SCOPE

