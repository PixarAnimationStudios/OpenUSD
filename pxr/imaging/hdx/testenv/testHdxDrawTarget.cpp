//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/base/gf/frustum.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/material.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hdx/drawTargetTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/usd/sdr/registry.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (testCollection)
);

int main(int argc, char *argv[])
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};

    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> index(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    TF_VERIFY(index);
    std::unique_ptr<Hdx_UnitTestDelegate> delegate =
        std::make_unique<Hdx_UnitTestDelegate>(index.get());
    HdEngine engine;

    // --------------------------------------------------------------------

    // AOVs
    SdfPath colorBuffer("/aovColor");
    {
        HdRenderBufferDescriptor desc;
        desc.dimensions = GfVec3i(512, 512, 1);
        desc.format = HdFormatUNorm8Vec4;
        desc.multiSampled = false;
        delegate->AddRenderBuffer(colorBuffer, desc);
    }
    SdfPath depthBuffer("/aovDepth");
    {
        HdRenderBufferDescriptor desc;
        desc.dimensions = GfVec3i(512, 512, 1);
        desc.format = HdFormatFloat32;
        desc.multiSampled = false;
        delegate->AddRenderBuffer(depthBuffer, desc);
    }

    // prep render task and drawtarget task
    SdfPath drawTargetTask("/drawTargetTask");
    SdfPath simpleLightTask("/simpleLightTask");
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    delegate->AddSimpleLightTask(simpleLightTask);
    delegate->AddDrawTargetTask(drawTargetTask);
    delegate->AddRenderSetupTask(renderSetupTask);
    delegate->AddRenderTask(renderTask);
    HdTaskSharedPtrVector tasks;
    tasks.push_back(index->GetTask(simpleLightTask));
    tasks.push_back(index->GetTask(drawTargetTask));
    tasks.push_back(index->GetTask(renderSetupTask));
    tasks.push_back(index->GetTask(renderTask));

    // prep lights
    GlfSimpleLight light1;
    light1.SetDiffuse(GfVec4f(0.8, 0.6, 0.7, 1.0));
    light1.SetPosition(GfVec4f(1,-0.5,1,0));
    delegate->AddLight(SdfPath("/light1"), light1);

    // draw target
    SdfPath drawTargetId("/drawTarget");
    delegate->AddDrawTarget(drawTargetId);

    // add a surface shader to use the drawtarget as a texture
    SdfPath materialId("/material");
    std::string const source(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"textures\" : { \n"
            "    \"texColor\": { \n"
            "        \"description\": \"DrawTarget Texture\" \n"
            "    } \n"
            "}, \n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"testHdxDrawTarget.Surface\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl testHdxDrawTarget.Surface \n\n"

        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    vec2 uv = mod(Peye.xy*0.3, vec2(1));                               \n"
        "    return vec4(FallbackLighting(Peye.xyz, Neye, HdGet_texColor(uv)), 1);\n"
        "}\n"
    );

    SdfPath drawTargetAttachmentId = drawTargetId.AppendProperty(TfToken("color"));

    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrSurfaceNode = 
        shaderReg.GetShaderNodeFromSourceCode(
            source, 
            HioGlslfxTokens->glslfx,
            SdrTokenMap()); // metadata

    TfToken const& terminalType = HdMaterialTerminalTokens->surface;

    // Adding basic material
    HdMaterialNetworkMap material1;
    HdMaterialNetwork& network1 = material1.map[terminalType];
    HdMaterialNode terminal1;
    terminal1.path = materialId.AppendPath(SdfPath("Shader"));
    terminal1.identifier = sdrSurfaceNode->GetIdentifier();
    terminal1.parameters[TfToken("texColor")] = VtValue(GfVec3f(1));

    // Insert texture node (for sampling from attachment)
    HdMaterialNode textureNode;
    textureNode.path = drawTargetAttachmentId;
    textureNode.identifier = TfToken("UsdUVTexture");
    textureNode.parameters[TfToken("fallback")] = VtValue(GfVec3f(1));

    // A texture associated with a render buffer can be used by setting
    // the file parameter to an SdfPath (instead of SdfAssetPath) that
    // contains the prim path of the render buffer.
    //
    // We point to the render buffer that serves as color attachment of
    // the draw target.
    textureNode.parameters[TfToken("file")] = 
        VtValue(drawTargetAttachmentId);
    textureNode.parameters[TfToken("wrapS")] =
        VtValue(TfToken("repeat"));
    textureNode.parameters[TfToken("wrapT")] =
        VtValue(TfToken("repeat"));
    textureNode.parameters[TfToken("minFilter")] =
        VtValue(TfToken("linear"));
    textureNode.parameters[TfToken("magFilter")] =
        VtValue(TfToken("linear"));

    // Insert connection between texture node and terminal
    HdMaterialRelationship rel;
    rel.inputId = textureNode.path;
    rel.inputName = TfToken("rgb");
    rel.outputId = terminal1.path;
    rel.outputName = TfToken("texColor");
    network1.relationships.push_back(std::move(rel));

    // Insert texture node
    network1.nodes.push_back(std::move(textureNode));

    // Insert terminal
    material1.terminals.push_back(terminal1.path);
    network1.nodes.push_back(std::move(terminal1)); // must be last in vector
    delegate->AddMaterialResource(
        materialId,
        VtValue(material1));

    // prep material bind
    delegate->BindMaterial(SdfPath("/grid"), materialId);

    // prep scene
    delegate->AddGrid(SdfPath("/grid"),
                     GfMatrix4d(10,0,0,0, 0,10,0,0, 0,0,10,0, 0,0,0,1));
    delegate->AddCube(SdfPath("/cube"),
                     GfMatrix4d( 1,0,0,0, 0,1,0,0,  0,0,1,0, -3,0,5,1));
    delegate->AddTet(SdfPath("/tet"),
                    GfMatrix4d( 2,0,0,0, 0,2,0,0,  0,0,2,0,  3,0,5,1));
    delegate->SetRefineLevel(SdfPath("/cube"), 4);
    delegate->SetRefineLevel(SdfPath("/tet"), 3);

    // camera
    GfFrustum frustum;
    frustum.SetNearFar(GfRange1d(0.1, 1000.0));
    frustum.SetPosition(GfVec3d(0, -5, 10));
    frustum.SetRotation(GfRotation(GfVec3d(1, 0, 0), 45));
    delegate->SetCamera(frustum.ComputeViewMatrix(),
                       frustum.ComputeProjectionMatrix());

    // draw target camera
    SdfPath dtCamera("/dtCamera");
    delegate->AddCamera(dtCamera);
    delegate->SetCamera(dtCamera,
                       frustum.ComputeViewMatrix(),
                       frustum.ComputeProjectionMatrix());

    // set renderTask
    delegate->SetTaskParam(
        renderTask, HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, 
            HdReprSelector(HdReprTokens->refined))));

    // set render setup param
    {
        VtValue vParam = delegate->GetTaskParam(renderSetupTask, HdTokens->params);
        HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
        param.enableLighting = true;
        param.framing = CameraUtilFraming(GfRect2i(GfVec2i(0,0), 512, 512));
        param.depthFunc = HdCmpFuncLess;

        HdRenderPassAovBinding colorBinding;
        colorBinding.aovName = TfToken("color");
        colorBinding.renderBufferId = colorBuffer;
        colorBinding.clearValue = VtValue(
            GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));

        HdRenderPassAovBinding depthBinding;
        depthBinding.aovName = TfToken("depth");
        depthBinding.renderBufferId = depthBuffer;
        depthBinding.clearValue = VtValue(1.0f);
        
        param.aovBindings = { colorBinding, depthBinding };
        delegate->SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));
    }

    // set '/tet' to drawtarget includes
    {
        HdRprimCollection dtCol(_tokens->testCollection,
            HdReprSelector(HdReprTokens->refined), 
            SdfPath("/tet"));
        delegate->SetDrawTarget(drawTargetId, HdTokens->camera, VtValue(dtCamera));
        delegate->SetDrawTarget(drawTargetId, HdTokens->collection, VtValue(dtCol));
    }

    // --------------------------------------------------------------------
    // draw.

    engine.Execute(index.get(), &tasks);

    delegate->WriteRenderBufferToFile(colorBuffer, "color1.png");

    std::cout << "OK" << std::endl;
}
