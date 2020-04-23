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

  TfToken shaderPath;
  switch(binder->GetProgramType())
  {
    case LoFiProgramType::LOFI_PROGRAM_MESH:
      shaderPath = _GetShaderPath("mesh.glslfx");
      break;
    case LoFiProgramType::LOFI_PROGRAM_CURVE:
      shaderPath = _GetShaderPath("curve.glslfx");
      break;
    case LoFiProgramType::LOFI_PROGRAM_POINT:
      shaderPath = _GetShaderPath("points.glslfx");
      break;
    case LoFiProgramType::LOFI_PROGRAM_INSTANCE:
      shaderPath = _GetShaderPath("instance.glslfx");
      break;
  }

  LoFiShaderCodeSharedPtr shaderCode(new LoFiShaderCode(shaderPath));

  LoFiCodeGen codeGen(
    binder->GetProgramType(),
    binder->GetUniformBindings(), 
    binder->GetAttributeBindings(),
    shaderCode
  );
  codeGen.GenerateProgramCode();

  const char* vertexCode = codeGen.GetVertexShaderCode().c_str();
  const char* fragmentCode = codeGen.GetFragmentShaderCode().c_str();

  program->Build(binder->GetProgramName().GetText(), vertexCode, fragmentCode);

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
  GfVec4f viewport = renderPassState->GetViewport();
  HdRenderPass* renderPass = (HdRenderPass*)this;
  auto drawItems = GetRenderIndex()->GetDrawItems(GetRprimCollection(), renderTags);

  // first check draw item program map
  for(auto drawItem: drawItems)
  {
    const LoFiDrawItem* lofiDrawItem = 
      reinterpret_cast<const LoFiDrawItem*>(drawItem);

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
      if(!drawItem->GetVisible()) continue;
      const LoFiBinder* binder = drawItem->GetBinder();
      // model matrix
      glUniformMatrix4fv(
        modelUniform,
        1,
        GL_FALSE,
        &GfMatrix4f(drawItem->GetMatrix())[0][0]
      );

      const LoFiVertexArray* vertexArray = drawItem->GetVertexArray();
      
      switch(binder->GetProgramType())
      {
        case LoFiProgramType::LOFI_PROGRAM_POINT:
          glPointSize(1);
          glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); 
          vertexArray->Bind();
          glDrawArrays(GL_POINTS, 0, vertexArray->GetNumElements());
          break;
        case LoFiProgramType::LOFI_PROGRAM_MESH:
          vertexArray->Bind();
          glDrawArrays(GL_TRIANGLES, 0, vertexArray->GetNumElements());
          break;
      }
      

      vertexArray->Unbind();

    }
  }

  glUseProgram(0);
  glDisable(GL_POINT_SMOOTH);
}

PXR_NAMESPACE_CLOSE_SCOPE
