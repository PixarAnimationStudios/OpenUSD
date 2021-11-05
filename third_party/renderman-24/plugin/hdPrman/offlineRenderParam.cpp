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
#include "hdPrman/rixStrings.h"     // Strings
#include "pxr/base/tf/pathUtils.h"  // Extract extension from tf token
#include "RixShadingUtils.h"        // RixConstants

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_OfflineRenderParam::HdPrman_OfflineRenderParam()
{
    _InitializePrman(); 
}

void
HdPrman_OfflineRenderParam::Initialize(
    RtParamList rileyOptions, 
    riley::ShadingNode integratorNode,
    riley::Extent outputFormat, TfToken outputFilename,
    std::vector<RenderOutput> const & renderOutputs)
{
    _options = rileyOptions;
    _activeIntegratorShadingNode = integratorNode;
    _SetRileyOptions(_options);
    _SetRileyIntegrator(_activeIntegratorShadingNode);
    for (auto const& ro : renderOutputs) {
        _AddRenderOutput(ro.name, ro.type, ro.params);
    }

    GetCameraContext().SetEnableMotionBlur(true);
    GetCameraContext().Begin(AcquireRiley());

    _SetRenderTargetAndDisplay(outputFormat, outputFilename);

    _CreateFallbackMaterials();
}

void
HdPrman_OfflineRenderParam::InitializeWithDefaults()
{
    int res[2] = {512,512};

    // Options
    {
        HdPrman_UpdateSearchPathsFromEnvironment(_options);

        float aspect = 10.0;
        _options.SetIntegerArray(RixStr.k_Ri_FormatResolution, res, 2);
        _options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, aspect);
        _SetRileyOptions(_options);
    }
    
    // Integrator
    riley::ShadingNode integratorNode;
    {
        static const RtUString us_PxrPathTracer("PxrPathTracer");
        static const RtUString us_PathTracer("PathTracer");
        riley::ShadingNode integratorNode {
            riley::ShadingNode::Type::k_Integrator,
            us_PxrPathTracer,
            us_PathTracer,
            RtParamList()
        };
        _SetRileyIntegrator(integratorNode);
    }

    // Displays & Display Channels
    {
        _AddRenderOutput(
            RtUString("Ci"), 
            riley::RenderOutputType::k_Color, 
            RtParamList());
    }

    GetCameraContext().SetEnableMotionBlur(true);
    GetCameraContext().Begin(AcquireRiley());

    // Output
    {
        riley::Extent const format = {uint32_t(res[0]), uint32_t(res[1]), 1};
        _SetRenderTargetAndDisplay(format, TfToken("default.exr"));
    }
    
    _CreateFallbackMaterials();

    // Default light
    {
        static const RtUString us_PxrDomeLight("PxrDomeLight");
        static const RtUString us_lightA("lightA");
        static const RtUString us_traceLightPaths("traceLightPaths");
        static const RtUString us_lightGroup("lightGroup");
        static const RtUString us_A("A");
        static const RtUString us_default("default");

        // Light shader
        riley::ShadingNode lightNode {
            riley::ShadingNode::Type::k_Light, // type
            us_PxrDomeLight, // name
            us_lightA, // handle
            RtParamList() 
        };
        lightNode.params.SetFloat(RixStr.k_intensity, 1.0f);
        lightNode.params.SetInteger(us_traceLightPaths, 1);
        lightNode.params.SetString(us_lightGroup, us_A);            

        // Light instance
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;
        riley::Transform xform = { 1, &matrix, &zerotime };
        RtParamList lightAttributes;
        lightAttributes.SetInteger(RixStr.k_visibility_camera, 0);
        lightAttributes.SetInteger(RixStr.k_visibility_indirect, 1);
        lightAttributes.SetInteger(RixStr.k_visibility_transmission, 1);
        lightAttributes.SetString(RixStr.k_grouping_membership, us_default);
        SetFallbackLight(lightNode, xform, lightAttributes);
    }
}

HdPrman_OfflineRenderParam::~HdPrman_OfflineRenderParam()
{
    _End();
}

void
HdPrman_OfflineRenderParam::_SetRileyOptions(RtParamList options)
{
    _riley->SetOptions(options);
}

void
HdPrman_OfflineRenderParam::_SetRileyIntegrator(riley::ShadingNode node)
{
    _integratorId = _riley->CreateIntegrator(riley::UserId::DefaultId(), node);
}

void
HdPrman_OfflineRenderParam::_AddRenderOutput(
    RtUString name, 
    riley::RenderOutputType type,
    RtParamList const& params)
{
    riley::FilterSize const filterwidth = { 1.0f, 1.0f };
    _renderOutputs.push_back(
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
HdPrman_OfflineRenderParam::SetResolutionOfRenderTargets(const GfVec2i &res)
{
    riley::Extent const format = {uint32_t(res[0]), uint32_t(res[1]), 1};

    _riley->ModifyRenderTarget(
        _rtid,
        nullptr,
        &format,
        nullptr,
        nullptr,
        nullptr);
}

void
HdPrman_OfflineRenderParam::_SetRenderTargetAndDisplay(
    riley::Extent format,
    TfToken outputFilename)
{
    _rtid = _riley->CreateRenderTarget(
            riley::UserId::DefaultId(),
            {(uint32_t) _renderOutputs.size(), _renderOutputs.data()},
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
        _rtid,
        RtUString(outputFilename.GetText()),
        RtUString(dspyFormat.GetText()), 
        {(uint32_t)_renderOutputs.size(), _renderOutputs.data()},
        RtParamList());

    riley::RenderViewId const renderView = _riley->CreateRenderView(
        riley::UserId::DefaultId(), 
        _rtid,
        GetCameraContext().GetCameraId(),
        _integratorId,
        {0, nullptr}, 
        {0, nullptr}, 
        RtParamList());
    _renderViews.push_back(renderView);
    
    _riley->SetDefaultDicingCamera(GetCameraContext().GetCameraId());
}

void 
HdPrman_OfflineRenderParam::SetFallbackLight(
    riley::ShadingNode node, 
    riley::Transform xform,
    RtParamList params)
{
    riley::CoordinateSystemList const k_NoCoordsys = { 0, nullptr };

    riley::LightShaderId lightShader = _riley->CreateLightShader(
        riley::UserId::DefaultId(), 
        {1, &node}, 
        {0, nullptr});
    
    _fallbackLightId = _riley->CreateLightInstance(
      riley::UserId::DefaultId(),
      riley::GeometryPrototypeId::InvalidId(), // no group
      riley::GeometryPrototypeId::InvalidId(), // no geo
      riley::MaterialId::InvalidId(), // no material
      lightShader,
      k_NoCoordsys,
      xform,
      params);
}

void
HdPrman_OfflineRenderParam::Render()
{
    std::cout << "   > Rendering" << std::endl;
    RtParamList renderOptions;
    static RtUString const US_RENDERMODE = RtUString("renderMode");
    static RtUString const US_BATCH = RtUString("batch");   
    renderOptions.SetString(US_RENDERMODE, US_BATCH);   

    _riley->Render(
        {static_cast<uint32_t>(_renderViews.size()), _renderViews.data()},
            renderOptions);
}

bool 
HdPrman_OfflineRenderParam::IsValid() const
{
    return _riley;
}


RtParamList&
HdPrman_OfflineRenderParam::GetOptions()
{
    return _options;
}

riley::IntegratorId
HdPrman_OfflineRenderParam::GetActiveIntegratorId()
{
    return _integratorId;
}

riley::ShadingNode &
HdPrman_OfflineRenderParam::GetActiveIntegratorShadingNode()
{
    return _activeIntegratorShadingNode;
}

HdPrmanCameraContext &
HdPrman_OfflineRenderParam::GetCameraContext()
{
    return _cameraContext;
}

void
HdPrman_OfflineRenderParam::_End()
{
    std::cout << "Destroy Prman" << std::endl;
    // Reset to initial state.
    if (_mgr) {
        if(_riley) {
            _mgr->DestroyRiley(_riley);
        }
        _mgr = nullptr;
    }

    _riley = nullptr;

    if (_rix) {
        RixXcpt* rix_xcpt = (RixXcpt*)_rix->GetRixInterface(k_RixXcpt);
        rix_xcpt->Unregister(&_xcpt);
    }

    if (_ri) {
        _ri->PRManEnd();
        _ri = nullptr;
    }
}

riley::Riley *
HdPrman_OfflineRenderParam::AcquireRiley()
{
    return _riley;
}

PXR_NAMESPACE_CLOSE_SCOPE
