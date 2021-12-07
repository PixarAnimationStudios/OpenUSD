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

static
HdPrmanRenderViewDesc
_ComputeRenderViewDesc(
    const VtDictionary &renderSpec,
    const riley::CameraId cameraId,
    const riley::IntegratorId integratorId,
    const GfVec2i &resolution)
{
    HdPrmanRenderViewDesc renderViewDesc;

    renderViewDesc.cameraId = cameraId;
    renderViewDesc.integratorId = integratorId;
    renderViewDesc.resolution = resolution;

    const VtArray<VtDictionary> &renderVars =
        VtDictionaryGet<VtArray<VtDictionary>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderVars);
    
    for (const VtDictionary &renderVar : renderVars) {
        const std::string &nameStr =
            VtDictionaryGet<std::string>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->name);
        const RtUString name(nameStr.c_str());

        HdPrmanRenderViewDesc::RenderOutputDesc renderOutputDesc;
        renderOutputDesc.name = name;
        renderOutputDesc.type = _ToRenderOutputType(
            VtDictionaryGet<TfToken>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->type));
        renderOutputDesc.sourceName = name;
        renderOutputDesc.filterName = RixStr.k_filter;
        renderOutputDesc.params = _ToRtParamList(
            VtDictionaryGet<VtDictionary>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->params,
                VtDefault = VtDictionary()));
        renderViewDesc.renderOutputDescs.push_back(renderOutputDesc);
    }
    
    const VtArray<VtDictionary> & renderProducts =
        VtDictionaryGet<VtArray<VtDictionary>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderProducts);

    for (const VtDictionary &renderProduct : renderProducts) {
        HdPrmanRenderViewDesc::DisplayDesc displayDesc;

        const TfToken &name = 
            VtDictionaryGet<TfToken>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->name);

        displayDesc.name = RtUString(name.GetText());
        
        // get output display driver type
        // TODO this is not a robust solution
        static const std::map<std::string,TfToken> extToDisplayDriver{
            { std::string("exr"),  TfToken("openexr") },
            { std::string("tif"),  TfToken("tiff") },
            { std::string("tiff"), TfToken("tiff") },
            { std::string("png"),  TfToken("png") }
        };
        
        const std::string outputExt = TfGetExtension(name.GetString());
        const TfToken dspyFormat = extToDisplayDriver.at(outputExt);
        displayDesc.driver = RtUString(dspyFormat.GetText());

        displayDesc.params = _ToRtParamList(
            VtDictionaryGet<VtDictionary>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->params,
                VtDefault = VtDictionary()));

        const VtIntArray &renderVarIndices =
            VtDictionaryGet<VtIntArray>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->renderVarIndices);
        for (const int renderVarIndex : renderVarIndices) {
            displayDesc.renderOutputIndices.push_back(renderVarIndex);
        }
        renderViewDesc.displayDescs.push_back(displayDesc);
    }

    return renderViewDesc;
}

HdPrman_OfflineRenderParam::~HdPrman_OfflineRenderParam()
{
    _DestroyRiley();
}

void
HdPrman_OfflineRenderParam::CreateRenderView(
    HdPrmanRenderDelegate * const renderDelegate)
{
    const VtDictionary &renderSpec =
        renderDelegate->GetRenderSetting<VtDictionary>(
            HdPrmanRenderSettingsTokens->experimentalRenderSpec,
            VtDictionary());

    const HdPrmanRenderViewDesc renderViewDesc =
        _ComputeRenderViewDesc(
            renderSpec,
            GetCameraContext().GetCameraId(),
            GetActiveIntegratorId(),
            GfVec2i(512, 512));

    GetRenderViewContext().CreateRenderView(
        renderViewDesc, AcquireRiley());
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

    _SetDefaultShutterCurve(GetCameraContext());

    GetCameraContext().Begin(AcquireRiley());

    CreateRenderView(renderDelegate);

    _CreateFallbackMaterials();
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

    const riley::RenderViewId renderViews[] = { ctx.GetRenderViewId() };

    _riley->Render(
        {static_cast<uint32_t>(TfArraySize(renderViews)), renderViews},
        RtParamList());
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
