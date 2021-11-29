//
// Copyright 2021 Pixar
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

#include "hdPrman/offlineRenderParam.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/rixStrings.h"     // Strings
#include "pxr/base/tf/pathUtils.h"  // Extract extension from tf token
#include "RixShadingUtils.h"        // RixConstants

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_OfflineRenderParam::HdPrman_OfflineRenderParam()
{
    _CreateRiley();
}

static
void
_SetDefaultShutterCurve(HdPrmanCameraContext &context)
{
    static float shutterPoints[8] =
        { 0.0f, 0.0f,  // points before open time
          0.0f, 0.0f,

          0.0f, 1.0f,  // points after close time
          0.3f, 0.0f };
        
    context.SetShutterCurve(
        0.0f,  // open time
        0.0f,  // close time
        shutterPoints);
}

static
riley::RenderOutputType
_ToRenderOutputType(const TfToken &t)
{
    if (t == TfToken("color3f")) {
        return riley::RenderOutputType::k_Color;
    } else if (t == TfToken("float")) {
        return riley::RenderOutputType::k_Float;
    } else if (t == TfToken("int")) {
        return riley::RenderOutputType::k_Integer;
    } else {
        TF_RUNTIME_ERROR("Unimplemented renderVar dataType '%s'; "
                         "skipping", t.GetText());
        return riley::RenderOutputType::k_Integer;
    }
}

// Helper to convert a dictionary of Hydra settings to Riley params.
static
RtParamList
_ToRtParamList(VtDictionary const& dict)
{
    RtParamList params;

    for (auto const& entry: dict) {
        RtUString riName(entry.first.c_str());

        if (entry.second.IsHolding<int>()) {
            params.SetInteger(riName, entry.second.UncheckedGet<int>());
        } else if (entry.second.IsHolding<float>()) {
            params.SetFloat(riName, entry.second.UncheckedGet<float>());
        } else if (entry.second.IsHolding<std::string>()) {
            params.SetString(riName,
                RtUString(entry.second.UncheckedGet<std::string>().c_str()));
        } else if (entry.second.IsHolding<VtArray<int>>()) {
            auto const& array = entry.second.UncheckedGet<VtArray<int>>();
            params.SetIntegerArray(riName, &array[0], array.size());
        } else if (entry.second.IsHolding<VtArray<float>>()) {
            auto const& array = entry.second.UncheckedGet<VtArray<float>>();
            params.SetFloatArray(riName, &array[0], array.size());
        } else {
            TF_CODING_ERROR("Unimplemented setting %s of type %s\n",
                            entry.first.c_str(),
                            entry.second.GetTypeName().c_str());
        }
    }

    return params;
}

void
HdPrman_OfflineRenderParam::Begin(HdPrmanRenderDelegate * const renderDelegate)
{
    RtParamList &options = GetOptions();

    // Ri:Shutter needs to be set before any prims are synced for
    // motion blur to work.
    SetOptionsFromRenderSettings(renderDelegate, options);

    HdPrman_UpdateSearchPathsFromEnvironment(options);
    _riley->SetOptions(options);

    _CreateIntegrator(renderDelegate);

    const VtDictionary &renderSpec =
        renderDelegate->GetRenderSetting<VtDictionary>(
            HdPrmanRenderSettingsTokens->experimentalRenderSpec,
            VtDictionary());

    const VtArray<VtDictionary> &renderVarDicts =
        VtDictionaryGet<VtArray<VtDictionary>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderVars);

    for (const VtDictionary &renderVarDict : renderVarDicts) {
        const std::string &nameStr = VtDictionaryGet<std::string>(
            renderVarDict, HdPrmanExperimentalRenderSpecTokens->name);
        const RtUString name(nameStr.c_str());

        _AddRenderOutput(
            name,
            _ToRenderOutputType(
                VtDictionaryGet<TfToken>(
                    renderVarDict,
                    HdPrmanExperimentalRenderSpecTokens->type)),
            _ToRtParamList(
                VtDictionaryGet<VtDictionary>(
                    renderVarDict,
                    HdPrmanExperimentalRenderSpecTokens->params)));
    }

    _SetDefaultShutterCurve(GetCameraContext());

    GetCameraContext().Begin(AcquireRiley());

    // Resolution will be updated by
    // SetResolutionOfRenderTargets called by render pass.
    const riley::Extent outputFormat = { 512, 512, 1 };

    TfToken outputFilename;

    const VtArray<VtDictionary> & renderProducts =
        VtDictionaryGet<VtArray<VtDictionary>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderProducts);

    // TODO:
    // Deal with more than one render product.
    // Respect the renderVarIndices.
    if (!renderProducts.empty()) {
        outputFilename =
            VtDictionaryGet<TfToken>(
                renderProducts[0],
                HdPrmanExperimentalRenderSpecTokens->name);
    }

    _SetRenderTargetAndDisplay(outputFormat, outputFilename);

    _CreateFallbackMaterials();
    _CreateFallbackLight();
}

HdPrman_OfflineRenderParam::~HdPrman_OfflineRenderParam()
{
    _DestroyRiley();
}

void
HdPrman_OfflineRenderParam::_AddRenderOutput(
    RtUString name, 
    riley::RenderOutputType type,
    RtParamList const& params)
{
    HdPrmanRenderViewContext &ctx = GetRenderViewContext();

    riley::FilterSize const filterwidth = { 1.0f, 1.0f };
    ctx.renderOutputIds.push_back(
        _riley->CreateRenderOutput(riley::UserId::DefaultId(),
            name,
            type,
            name,
            RixStr.k_filter, 
            RixStr.k_box, 
            filterwidth, 
            1.0f, 
            params));
}

void
HdPrman_OfflineRenderParam::_SetRenderTargetAndDisplay(
    riley::Extent format,
    TfToken outputFilename)
{
    HdPrmanRenderViewContext &ctx = GetRenderViewContext();

    ctx.renderTargetId = _riley->CreateRenderTarget(
            riley::UserId::DefaultId(),
            {(uint32_t) ctx.renderOutputIds.size(), ctx.renderOutputIds.data()},
            format,
            RtUString("weighted"),
            1.0,
            RtParamList());

    // get output display driver type
    // TODO this is not a robust solution
    static const std::map<std::string,TfToken> extToDisplayDriver{
        { std::string("exr"),  TfToken("openexr") },
        { std::string("tif"),  TfToken("tiff") },
        { std::string("tiff"), TfToken("tiff") },
        { std::string("png"),  TfToken("png") }
    };

    std::string outputExt = TfGetExtension(outputFilename);
    TfToken dspyFormat = extToDisplayDriver.at(outputExt);
    _riley->CreateDisplay(
        riley::UserId::DefaultId(),
        ctx.renderTargetId,
        RtUString(outputFilename.GetText()),
        RtUString(dspyFormat.GetText()), 
        {(uint32_t)ctx.renderOutputIds.size(), ctx.renderOutputIds.data()},
        RtParamList());

    ctx.renderViewId = _riley->CreateRenderView(
        riley::UserId::DefaultId(), 
        ctx.renderTargetId,
        GetCameraContext().GetCameraId(),
        GetActiveIntegratorId(),
        {0, nullptr}, 
        {0, nullptr}, 
        RtParamList());
    
    _riley->SetDefaultDicingCamera(GetCameraContext().GetCameraId());
}

void
HdPrman_OfflineRenderParam::Render()
{
    std::cout << "   > Rendering" << std::endl;
    RtParamList renderOptions;
    static RtUString const US_RENDERMODE = RtUString("renderMode");
    static RtUString const US_BATCH = RtUString("batch");   
    renderOptions.SetString(US_RENDERMODE, US_BATCH);   

    HdPrmanRenderViewContext &ctx = GetRenderViewContext();

    const std::vector<riley::RenderViewId> renderViews = { ctx.renderViewId };

    _riley->Render(
        {static_cast<uint32_t>(renderViews.size()), renderViews.data()},
            renderOptions);
}

bool 
HdPrman_OfflineRenderParam::IsValid() const
{
    return _riley;
}


riley::IntegratorId
HdPrman_OfflineRenderParam::GetActiveIntegratorId()
{
    return GetIntegratorId();
}

riley::Riley *
HdPrman_OfflineRenderParam::AcquireRiley()
{
    return _riley;
}

PXR_NAMESPACE_CLOSE_SCOPE
