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

#include "hdPrman/offlineContext.h"
#include "hdPrman/rixStrings.h"     // Strings
#include "pxr/base/tf/pathUtils.h"  // Extract extension from tf token
#include "RixShadingUtils.h"        // RixConstants

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_OfflineContext::HdPrman_OfflineContext()
{
    _InitializePrman(); 
}

void
HdPrman_OfflineContext::Initialize(
    RtParamList rileyOptions, 
    riley::ShadingNode integratorNode,
    RtUString cameraName, 
    riley::ShadingNode cameraNode, 
    riley::Transform cameraXform, RtParamList cameraParams,
    riley::Extent outputFormat, TfToken outputFilename,
    std::vector<riley::ShadingNode> const & fallbackMaterialNodes,
    std::vector<riley::ShadingNode> const & fallbackVolumeNodes,
    std::vector<RenderOutput> const & renderOutputs)
{
    _SetRileyOptions(rileyOptions);
    _SetRileyIntegrator(integratorNode);
    _SetCamera(cameraName, cameraNode, cameraXform, cameraParams);
    for (auto const& ro : renderOutputs) {
        _AddRenderOutput(ro.name, ro.type, ro.params);
    }
    _SetRenderTargetAndDisplay(outputFormat, outputFilename);
    _SetFallbackMaterial(fallbackMaterialNodes);
    _SetFallbackVolumeMaterial(fallbackVolumeNodes);
}

void
HdPrman_OfflineContext::InitializeWithDefaults()
{
    int res[2] = {512,512};

    // Options
    {
        RtParamList options;
        HdPrman_UpdateSearchPathsFromEnvironment(options);
        
        float aspect = 1.0;
        options.SetIntegerArray(RixStr.k_Ri_FormatResolution, res, 2);
        options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, aspect);
        _SetRileyOptions(options);
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

     // Camera
    {
        static const RtUString us_main_cam("main_cam");
        riley::ShadingNode cameraNode;
        RtUString cameraName;

        std::string cameraProjection("PxrPerspective");
        bool isOrthographic = false;
        float shutterCurve[10] = {0, 0, 0, 0, 0, 0, 0, 1, 0.3, 0};
        cameraName = us_main_cam;
        riley::Transform cameraXform;
        RtParamList cameraParams;
        RtParamList projParams;

        // Shutter curve (this is relative to the Shutter interval above).
        cameraParams.SetFloat(RixStr.k_shutterOpenTime, shutterCurve[0]);
        cameraParams.SetFloat(RixStr.k_shutterCloseTime, shutterCurve[1]);
        cameraParams.SetFloatArray(RixStr.k_shutteropening, shutterCurve+2,8);

        // Projection
        projParams.SetFloat(RixStr.k_fov, 60.0f);
        cameraNode = riley::ShadingNode {
            riley::ShadingNode::Type::k_Projection,
            RtUString(cameraProjection.c_str()),
            RtUString("main_cam_projection"),
            projParams
        };

        // Transform
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;

        // Orthographic camera:
        // XXX In HdPrman RenderPass we apply orthographic
        // projection as a scale onto the viewMatrix. This
        // is because we currently cannot update Renderman's
        // `ScreenWindow` once it is running.
        if (isOrthographic) {
            matrix.Scale(10,10,10);
        }

        // Translate camera back a bit
        matrix.Translate(0.f, 0.f, -10.0f);
        cameraXform = { 1, &matrix, &zerotime };
        _SetCamera(cameraName, cameraNode, cameraXform, cameraParams);
    }

    // Displays & Display Channels
    {
        _AddRenderOutput(
            RtUString("Ci"), 
            riley::RenderOutputType::k_Color, 
            RtParamList());
    }

    // Output
    {
        riley::Extent const format = {uint32_t(res[0]), uint32_t(res[1]), 1};
        _SetRenderTargetAndDisplay(format, TfToken("default.exr"));
    }
    
    // Default material
    {
        static const RtUString us_PxrPrimvar("PxrPrimvar");
        static const RtUString us_PxrSurface("PxrSurface");
        static const RtUString us_simpleTestSurface("simpleTestSurface");
        static const RtUString us_pv_color("pv_color");
        static const RtUString us_pv_color_resultRGB("pv_color:resultRGB");
        static const RtUString us_varname("varname");
        static const RtUString us_displayColor("displayColor");
        static const RtUString us_defaultColor("defaultColor");
        static const RtUString us_diffuseColor("diffuseColor");
        static const RtUString us_specularEdgeColor("specularEdgeColor");
        static const RtUString us_specularFaceColor("specularFaceColor");
        static const RtUString us_specularModelType("specularModelType");

        std::vector<riley::ShadingNode> materialNodes;
        riley::ShadingNode pxrPrimvar_node;
        pxrPrimvar_node.type = riley::ShadingNode::Type::k_Pattern;
        pxrPrimvar_node.name = us_PxrPrimvar;
        pxrPrimvar_node.handle = us_pv_color;
        pxrPrimvar_node.params.SetString(us_varname, us_displayColor);
        // Note: this 0.5 gray is to match UsdImaging's fallback.
        pxrPrimvar_node.params.SetColor(
            us_defaultColor,
            RtColorRGB(0.5, 0.5, 0.5));
        pxrPrimvar_node.params.SetString(RixStr.k_type, RixStr.k_color);
        materialNodes.push_back(pxrPrimvar_node);

        riley::ShadingNode pxrSurface_node;
        pxrSurface_node.type = riley::ShadingNode::Type::k_Bxdf;
        pxrSurface_node.name = us_PxrSurface;
        pxrSurface_node.handle = us_simpleTestSurface;
        pxrSurface_node.params.SetColorReference(
            us_diffuseColor,
            us_pv_color_resultRGB);
        pxrSurface_node.params.SetInteger(us_specularModelType, 1);
        pxrSurface_node.params.SetColor(us_specularFaceColor,RtColorRGB(0.04f));
        pxrSurface_node.params.SetColor(us_specularEdgeColor, RtColorRGB(1.0f));
        materialNodes.push_back(pxrSurface_node);
        _SetFallbackMaterial(materialNodes);
    }

    // Volume default material
    {
        static const RtUString us_PxrVolume("PxrVolume");
        static const RtUString us_simpleVolume("simpleVolume");
        static const RtUString us_density("density");
        static const RtUString us_densityFloatPrimVar("densityFloatPrimVar");
        std::vector<riley::ShadingNode> volumeMaterialNodes;
        riley::ShadingNode pxrVolume_node;
        pxrVolume_node.type = riley::ShadingNode::Type::k_Bxdf;
        pxrVolume_node.name = us_PxrVolume;
        pxrVolume_node.handle = us_simpleVolume;
        pxrVolume_node.params.SetString(us_densityFloatPrimVar, us_density);
        volumeMaterialNodes.push_back(pxrVolume_node);
        _SetFallbackVolumeMaterial(volumeMaterialNodes);
    }

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

HdPrman_OfflineContext::~HdPrman_OfflineContext()
{
    _End();
}

void
HdPrman_OfflineContext::_SetRileyOptions(RtParamList options)
{
    riley->SetOptions(options);
}

void
HdPrman_OfflineContext::_SetRileyIntegrator(riley::ShadingNode node)
{
    _integratorId = riley->CreateIntegrator(riley::UserId::DefaultId(), node);
}

void
HdPrman_OfflineContext::_SetCamera(
    RtUString name, 
    riley::ShadingNode node, 
    riley::Transform xform, 
    RtParamList params)
{
    cameraId = riley->CreateCamera(riley::UserId::DefaultId(),
        name, node, xform, params);
}

void
HdPrman_OfflineContext::_AddRenderOutput(
    RtUString name, 
    riley::RenderOutputType type,
    RtParamList const& params)
{
    riley::FilterSize const filterwidth = { 1.0f, 1.0f };
    _renderOutputs.push_back(
        riley->CreateRenderOutput(riley::UserId::DefaultId(),
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
HdPrman_OfflineContext::_SetRenderTargetAndDisplay(
    riley::Extent format,
    TfToken outputFilename)
{
    _rtid = riley->CreateRenderTarget(
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
    riley->CreateDisplay(
        riley::UserId::DefaultId(),
        _rtid,
        RtUString(outputFilename.GetText()),
        RtUString(dspyFormat.GetText()), 
        {(uint32_t)_renderOutputs.size(), _renderOutputs.data()},
        RtParamList());

    riley::RenderViewId const renderView = riley->CreateRenderView(
        riley::UserId::DefaultId(), 
        _rtid,
        cameraId, 
        _integratorId,
        {0, nullptr}, 
        {0, nullptr}, 
        RtParamList());
    _renderViews.push_back(renderView);
    
    riley->SetDefaultDicingCamera(cameraId);
}

void 
HdPrman_OfflineContext::SetFallbackLight(
    riley::ShadingNode node, 
    riley::Transform xform,
    RtParamList params)
{
    riley::CoordinateSystemList const k_NoCoordsys = { 0, nullptr };

    riley::LightShaderId lightShader = riley->CreateLightShader(
        riley::UserId::DefaultId(), 
        {1, &node}, 
        {0, nullptr});
    
    _fallbackLightId = riley->CreateLightInstance(
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
HdPrman_OfflineContext::_SetFallbackMaterial(
    std::vector<riley::ShadingNode> const & materialNodes)
{
    fallbackMaterial = riley->CreateMaterial(riley::UserId::DefaultId(),
        {static_cast<uint32_t>(materialNodes.size()), &materialNodes[0]},
        RtParamList());
}

void 
HdPrman_OfflineContext::_SetFallbackVolumeMaterial(
    std::vector<riley::ShadingNode> const & materialNodes)
{
    fallbackVolumeMaterial = riley->CreateMaterial(riley::UserId::DefaultId(),
        {static_cast<uint32_t>(materialNodes.size()), &materialNodes[0]},
        RtParamList());
}

void
HdPrman_OfflineContext::Render()
{
    std::cout << "   > Rendering" << std::endl;
    RtParamList renderOptions;
    static RtUString const US_RENDERMODE = RtUString("renderMode");
    static RtUString const US_BATCH = RtUString("batch");   
    renderOptions.SetString(US_RENDERMODE, US_BATCH);   

    riley->Render(
        {static_cast<uint32_t>(_renderViews.size()), _renderViews.data()},
            renderOptions);
}

bool 
HdPrman_OfflineContext::IsValid() const
{
    return (riley != nullptr);
}

void
HdPrman_OfflineContext::_End()
{
    std::cout << "Destroy Prman" << std::endl;
    // Reset to initial state.
    if (mgr) {
        if(riley) {
            mgr->DestroyRiley(riley);
        }
        mgr = nullptr;
    }

    riley = nullptr;

    if (rix) {
        RixXcpt* rix_xcpt = (RixXcpt*)rix->GetRixInterface(k_RixXcpt);
        rix_xcpt->Unregister(&xcpt);
    }

    if (ri) {
        ri->PRManEnd();
        ri = nullptr;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
