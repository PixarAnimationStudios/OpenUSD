//
// Copyright 2020 Pixar
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
#include "pxr/pxr.h"

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/base/gf/frustum.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/material.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hdx/drawTargetTask.h"
#include "pxr/imaging/hdx/drawTargetResolveTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

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
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;
    GlfContextCaps::InitInstance();

    // prep draw target
    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(GfVec2i(512, 512));
    drawTarget->Bind();
    drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                              GL_DEPTH24_STENCIL8);
    drawTarget->Unbind();

    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat clearDepth[1] = { 1.0f };

    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> index(HdRenderIndex::New(&renderDelegate));
    TF_VERIFY(index);
    std::unique_ptr<Hdx_UnitTestDelegate> delegate(
                                         new Hdx_UnitTestDelegate(index.get()));
    HdEngine engine;

    // --------------------------------------------------------------------

    // prep render task and drawtarget task
    SdfPath drawTargetTask("/drawTargetTask");
    SdfPath simpleLightTask("/simpleLightTask");
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath drawTargetResolveTask("/drawTargetResolveTask");
    delegate->AddSimpleLightTask(simpleLightTask);
    delegate->AddDrawTargetTask(drawTargetTask);
    delegate->AddDrawTargetResolveTask(drawTargetResolveTask);
    delegate->AddRenderSetupTask(renderSetupTask);
    delegate->AddRenderTask(renderTask);
    HdTaskSharedPtrVector tasks;
    tasks.push_back(index->GetTask(simpleLightTask));
    tasks.push_back(index->GetTask(drawTargetTask));
    tasks.push_back(index->GetTask(drawTargetResolveTask));
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
            NdrTokenMap()); // metadata

    TfToken const& terminalType = HdMaterialTerminalTokens->surface;

    // Adding basic material
    HdMaterialNetworkMap material1;
    HdMaterialNetwork& network1 = material1.map[terminalType];
    HdMaterialNode terminal1;
    terminal1.path = materialId.AppendPath(SdfPath("/Shader"));
    terminal1.identifier = sdrSurfaceNode->GetIdentifier();
    terminal1.parameters[TfToken("texColor")] = VtValue(GfVec3f(1));

    // Insert texture node (for sampling from attachment)
    HdMaterialNode textureNode;
    textureNode.path = drawTargetAttachmentId;
    textureNode.identifier = TfToken("UsdUVTexture");
    textureNode.parameters[TfToken("fallback")] = VtValue(GfVec3f(1));

    // For the file path, HdSt doesn't really care what it is since it is going
    // to do a lookup of the prim via GetTextureResource on the sceneDelegate.
    // The file path cannot be empty though, because if it is empty HdSt will
    // use the fallback value of the texture node.
    textureNode.parameters[TfToken("file")] = 
        VtValue(drawTargetAttachmentId.GetString());

    // Insert connection between texture node and terminal
    HdMaterialRelationship rel;
    rel.inputId = textureNode.path;
    rel.inputName = TfToken("rgba");
    rel.outputId = terminal1.path;
    rel.outputName = TfToken("texColor");
    network1.relationships.emplace_back(std::move(rel));

    // Insert texture node
    network1.nodes.emplace_back(std::move(textureNode));

    // Insert terminal
    material1.terminals.push_back(terminal1.path);
    network1.nodes.emplace_back(std::move(terminal1)); // must be last in vector
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
    drawTarget->Bind();
    glViewport(0, 0, 512, 512);
    glEnable(GL_DEPTH_TEST);
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    engine.Execute(index.get(), &tasks);

    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color1.png");

    std::cout << "OK" << std::endl;
}
