//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/hd/renderPassState.h"
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
  _SetupSimpleGLSLProgram();
  _SetupSimpleVertexArray();
}

LoFiRenderPass::~LoFiRenderPass()
{
  std::cout << "### [LOFI] DELETE RENDER PASS..." << std::endl;
}

void 
LoFiRenderPass::_SetupSimpleGLSLProgram()
{
  // build shader from string
  LoFiGLSLProgram* program = new LoFiGLSLProgram();

  // get resource registry
  LoFiResourceRegistrySharedPtr const& resourceRegistry = 
    std::dynamic_pointer_cast<LoFiResourceRegistry>(
      GetRenderIndex()->GetResourceRegistry()
    );
  TF_VERIFY(resourceRegistry);

  if(LOFI_GL_VERSION >= 330)
  {
    program->Build("Simple330", VERTEX_SHADER_330, FRAGMENT_SHADER_330);
  }
  else
  {
    program->Build("Simple120", VERTEX_SHADER_120, FRAGMENT_SHADER_120);
  }
  std::cout << "BUILD SIMPLE SHADER PROGRAM..." << std::endl;
  HdInstance<LoFiGLSLProgramSharedPtr> instance =
  resourceRegistry->RegisterGLSLProgram(program->Hash());
  std::cout << "REGISTER IN REGISTRY.." << std::endl;
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
  _program = instance.GetValue();
  std::cout << "BUILD SIMPLE SHADER PROGRAM DONE.." << std::endl;
}

void
LoFiRenderPass::_SetupSimpleVertexArray()
{
  std::cout << "SETUP SIMPLE VERTEX ARRAY.." << std::endl;
  size_t szp = NUM_TEST_POINTS * 3 * sizeof(float);
  size_t szi = NUM_TEST_INDICES * sizeof(int);

#ifdef __APPLE__
  if(LOFI_GL_VERSION >= 330)
  {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
  }
  else
  {
    glGenVertexArraysAPPLE(1, &_vao);
    glBindVertexArrayAPPLE(_vao);
  }
#else
  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao);
#endif


  // specify position attribute
  glGenBuffers(1, &_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER, szp, &TEST_POINTS[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(0);

  // specify color attribute
  glGenBuffers(1, &_cbo);
  glBindBuffer(GL_ARRAY_BUFFER, _cbo);
  glBufferData(GL_ARRAY_BUFFER, szp, &TEST_COLORS[0], GL_STATIC_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(1);


  /*

  // generate vertex buffer object
  glGenBuffers(1, &_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);

  // push buffer to GPU
  glEnableClientState(GL_VERTEX_ARRAY);
  glBufferData(GL_ARRAY_BUFFER, 2*szp, NULL,GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, szp, &TEST_POINTS[0]);
  glBufferSubData(GL_ARRAY_BUFFER, szp, szp, &TEST_COLORS[0]);
  */
  // generate element buffer object
  glGenBuffers(1, &_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
  
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, szi, NULL,GL_STATIC_DRAW);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, szi, &TEST_INDICES[0]);

  // bind shader program
  glUseProgram(_program->Get());
  glBindAttribLocation(_program->Get(), 0, "position");
  glBindAttribLocation(_program->Get(), 1, "normal");
  glBindAttribLocation(_program->Get(), 2, "color");
  glLinkProgram(_program->Get());

  // unbind vertex array object
#ifdef __APPLE__
  if(LOFI_GL_VERSION >= 330)glBindVertexArray(0);
  else glBindVertexArrayAPPLE(0);
#else
  glBindVertexArray(0);
#endif
  
  glUseProgram(0);
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
  uint64_t T = ns();
  /*
  _renderer->SetCamera(
      renderPassState->GetWorldToViewMatrix(), 
      renderPassState->GetProjectionMatrix()
  );
  _renderer->Render(_renderIndex, _collection);
  */
  GfMatrix4d viewMatrix = renderPassState->GetWorldToViewMatrix();
  GfMatrix4d projMatrix = renderPassState->GetProjectionMatrix();
  HdRenderPass* renderPass = (HdRenderPass*)this;
  auto drawItems = GetRenderIndex()->GetDrawItems(GetRprimCollection(), renderTags);

/*
  std::cout << "[LoFi][Renderer] Draw Items -> Num = " << drawItems.size() << std::endl;

  for(auto drawItem: drawItems)
  {
    const LoFiDrawItem* lofiDrawItem = 
      reinterpret_cast<const LoFiDrawItem*>(drawItem);
    std::cout << "BOUNDS : " << lofiDrawItem->GetBounds() << std::endl;
    std::cout << "VERTEX ARRAY : " << lofiDrawItem->GetVertexArray()->Get() << std::endl;
    std::cout << "MATRIX : " << lofiDrawItem->GetMatrix() << std::endl;
  }
  */

  GfVec4f clearColor = GfVec4f(0.63f,0.63f,0.63f,1.f);

  glClearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(_program->Get());
      
  GfMatrix4f identity(1);
  GLuint modelUniform = glGetUniformLocation(_program->Get(), "model");
  GLuint viewUniform = glGetUniformLocation(_program->Get(), "view");
  GLuint projUniform = glGetUniformLocation(_program->Get(), "projection");

  // model matrix
  glUniformMatrix4fv(
    modelUniform,
    1,
    GL_FALSE,
    &identity[0][0]
  );
  
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

  
  static GfVec3f red(1.f, 0.f, 0.f);
  static GfVec3f green(0.f, 1.f, 0.f);

  // draw red points
  glEnable(GL_POINT_SMOOTH);
  glPointSize(2);
  uint32_t numDrawItems = 0;
  for(auto drawItem: drawItems)
  {
    const LoFiDrawItem* lofiDrawItem = 
      reinterpret_cast<const LoFiDrawItem*>(drawItem);

    const LoFiVertexArray* vertexArray = lofiDrawItem->GetVertexArray();
    vertexArray->Bind();

    // model matrix
    glUniformMatrix4fv(
      modelUniform,
      1,
      GL_FALSE,
      &GfMatrix4f(drawItem->GetMatrix())[0][0]
    );

    glDrawArrays(GL_TRIANGLES, 0, vertexArray->GetNumElements());

    vertexArray->Unbind();
      /*
    std::cout << "BOUNDS : " << lofiDrawItem->GetBounds() << std::endl;
    std::cout << "VERTEX ARRAY : " << lofiDrawItem->GetVertexArray()->Get() << std::endl;
    std::cout << "MATRIX : " << lofiDrawItem->GetMatrix() << std::endl;
    */
   numDrawItems++;
  }
  uint64_t elapsed = ns() - T;
  std::cout << "### [LOFI] Render took " << (elapsed * 1e-9) << " seconds to complete!" << std::endl;
  std::cout << "### [LOFI] WE DEAL WITH " << numDrawItems << " DRAW ITEMS..." <<std::endl; 
/*

  // draw red points
  glEnable(GL_POINT_SMOOTH);
  glPointSize(2);
  glBindVertexArrayAPPLE(_vao);
  
  glUniform3fv(
    glGetUniformLocation(_program->Get(),"color"),
    1,
    &green[0]
  );

  glDrawElements(
    GL_TRIANGLES, 
    NUM_TEST_INDICES, 
    GL_UNSIGNED_INT, 
    (const void*)0
  );

  glUniform3fv(
    glGetUniformLocation(_program->Get(),"color"),
    1,
    &red[0]
  );
  glDrawArrays(GL_POINTS, 0, NUM_TEST_POINTS);

  glBindVertexArrayAPPLE(0);
*/
  
  glUseProgram(0);
  glDisable(GL_POINT_SMOOTH);
}

PXR_NAMESPACE_CLOSE_SCOPE
