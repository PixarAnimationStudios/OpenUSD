//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/plugin/LoFi/binding.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/plugin/LoFi/drawItem.h"
#include "pxr/imaging/plugin/LoFi/timer.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

LoFiRenderPass::LoFiRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection)
    : HdRenderPass(index, collection)
{
}

LoFiRenderPass::~LoFiRenderPass()
{
}

TfToken LoFiRenderPass::_GetShaderPath(char const * shader)
{
  static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
  const std::string path =
      PlugFindPluginResource(plugin, TfStringCatPaths("shaders", shader));
  TF_VERIFY(!path.empty(), "Could not find shader: %s\n", shader);
  return TfToken(path);
}

LoFiGLSLProgramSharedPtr 
LoFiRenderPass::_SetupGLSLProgram(const LoFiBinder* binder)
{
  // build shader from string
  LoFiGLSLProgram* program = new LoFiGLSLProgram();

  // get resource registry
  LoFiResourceRegistrySharedPtr const& resourceRegistry = 
    std::dynamic_pointer_cast<LoFiResourceRegistry>(
      GetRenderIndex()->GetResourceRegistry()
    );
  TF_VERIFY(resourceRegistry);

  bool hasGeometryShader = false;
  TfToken shaderPath;
  switch(binder->GetProgramType())
  {
    case LoFiProgramType::LOFI_PROGRAM_MESH:
      shaderPath = _GetShaderPath("mesh.glslfx");
      break;
    case LoFiProgramType::LOFI_PROGRAM_CURVE:
      shaderPath = _GetShaderPath("curve.glslfx");
      hasGeometryShader = true;
      break;
    case LoFiProgramType::LOFI_PROGRAM_POINT:
      shaderPath = _GetShaderPath("points.glslfx");
      break;
    case LoFiProgramType::LOFI_PROGRAM_CONTOUR:
      shaderPath = _GetShaderPath("contour.glslfx");
      hasGeometryShader = true;
      break;
  }

  LoFiShaderCodeSharedPtr shaderCode(new LoFiShaderCode(shaderPath));

  LoFiCodeGen codeGen(
    binder->GetProgramType(),
    binder->GetUniformBindings(), 
    binder->GetAttributeBindings(),
    shaderCode
  );
  codeGen.GenerateProgramCode(hasGeometryShader);

  if(!hasGeometryShader)
  {
    program->Build(
      binder->GetProgramName().GetText(), 
      codeGen.GetVertexShaderCode().c_str(), 
      codeGen.GetFragmentShaderCode().c_str());
  }
  else
  {
    program->Build(
      binder->GetProgramName().GetText(), 
      codeGen.GetVertexShaderCode().c_str(), 
      codeGen.GetGeometryShaderCode().c_str(), 
      codeGen.GetFragmentShaderCode().c_str());
  }

  HdInstance<LoFiGLSLProgramSharedPtr> instance =
  resourceRegistry->RegisterGLSLProgram(program->Hash());

  if(instance.IsFirstInstance())
  {
    if(TfDebug::IsEnabled(LOFI_RENDERER)) 
    {
      std::cerr << "[LoFi][Renderer] Set GLSL program in registry : " <<
      program->Hash() << std::endl;
    }
    
    instance.SetValue(LoFiGLSLProgramSharedPtr(program));      
  }
  else
  {
    if(TfDebug::IsEnabled(LOFI_RENDERER)) 
    {
      std::cerr << "[LoFi][Renderer] Get GLSL program from registry : " <<
      program->Hash() << std::endl;
    }
    
    delete program;  
  }
  return instance.GetValue();

}

void
LoFiRenderPass::_SetupDrawTarget(int width, int height)
{
  GfVec2i attachmentSize(width,height);
  GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(attachmentSize);

  // create initial attachments
  drawTarget->Bind();
  drawTarget->AddAttachment(
      "color", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
  drawTarget->AddAttachment(
      "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);
  drawTarget->Unbind();
}

void
LoFiRenderPass::_Execute( HdRenderPassStateSharedPtr const& renderPassState,
                          TfTokenVector const &renderTags)
{
  GfMatrix4d viewMatrix = renderPassState->GetWorldToViewMatrix();
  GfMatrix4d projMatrix = renderPassState->GetProjectionMatrix();
  GfMatrix4d cullMatrix = renderPassState->GetCullMatrix();
  GfVec4f viewport = renderPassState->GetViewport();
  HdRenderPass* renderPass = (HdRenderPass*)this;
  auto drawItems = GetRenderIndex()->GetDrawItems(GetRprimCollection(), renderTags);

  // first check draw item program map
  for(auto drawItem: drawItems)
  {
    const LoFiDrawItem* lofiDrawItem = 
      reinterpret_cast<const LoFiDrawItem*>(drawItem);

    if(!lofiDrawItem->GetVisible()) continue;

    const LoFiBinder* binder = lofiDrawItem->GetBinder();
    TfToken programName = binder->GetProgramName();
  
    auto programIt = _programs.find(programName);
    if (programIt == _programs.end())
    {
      LoFiGLSLProgramSharedPtr program;
      program = _SetupGLSLProgram(lofiDrawItem->GetBinder());
      _programs[programName] = program;
    }
    LoFiDrawItemPtrSet& drawItemSet = _programDrawItemsMap[programName];
    drawItemSet.insert(lofiDrawItem);
  }
  GfVec4f clearColor = GfVec4f(0.63f,0.63f,0.63f,1.f);

  glClearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  std::cout << "----------------- LOFI RENDER PASS EXECUTE -------------" << std::endl;
  for(auto programIt: _programs)
  {
    TfToken programName = programIt.first;
    LoFiGLSLProgramSharedPtr program = programIt.second;

    GLuint pgm = program->Get();
    glUseProgram(pgm);
      
    GfMatrix4f identity(1);
    GLuint viewportUniform = glGetUniformLocation(pgm, "viewport");
    GLuint modelUniform = glGetUniformLocation(pgm, "model");
    GLuint viewUniform = glGetUniformLocation(pgm, "view");
    GLuint projUniform = glGetUniformLocation(pgm, "projection");
    
    // viewport
    glUniform4fv(viewportUniform, 1,(const GLfloat*)&viewport[0]);

    // view matrix
    glUniformMatrix4fv(
      viewUniform,
      1,
      GL_FALSE,
      &GfMatrix4f(viewMatrix)[0][0]
    );

    // projection matrix
    glUniformMatrix4fv(
      projUniform,
      1,
      GL_FALSE,
      &GfMatrix4f(projMatrix)[0][0]
    );

    LoFiDrawItemPtrSet drawItemSet = _programDrawItemsMap[programName];
    for(auto drawItem: drawItemSet)
    {
      const LoFiBinder* binder = drawItem->GetBinder();

      const LoFiVertexArray* vertexArray = drawItem->GetVertexArray();

      if(drawItem->HasInstancer())
      {
        for(const auto& instanceXform: drawItem->GetInstancesXforms())
        {
          GfBBox3d instanceBBox = drawItem->GetBounds();
          instanceBBox.Transform(GfMatrix4d(instanceXform));

          // cull 
          if(GfFrustum::IntersectsViewVolume (instanceBBox, GfMatrix4d(cullMatrix)))
          {
            // model matrix
            glUniformMatrix4fv(
              modelUniform,
              1,
              GL_FALSE,
              &(GfMatrix4f(drawItem->GetMatrix()) * instanceXform)[0][0]
            );
            vertexArray->Draw();
          }
        }
      }
      else
      {
        // cull 
        if(GfFrustum::IntersectsViewVolume(drawItem->GetBounds(), cullMatrix))
        {
          ((LoFiDrawItem*)drawItem)->FindSilhouettes(viewMatrix.GetInverse());

          // model matrix
          glUniformMatrix4fv(
            modelUniform,
            1,
            GL_FALSE,
            &GfMatrix4f(drawItem->GetMatrix())[0][0]
          );
          vertexArray->Draw();
        
        }
        
      }
      
      vertexArray->Unbind();

    }
  }

  glUseProgram(0);
  glDisable(GL_POINT_SMOOTH);
}

PXR_NAMESPACE_CLOSE_SCOPE
