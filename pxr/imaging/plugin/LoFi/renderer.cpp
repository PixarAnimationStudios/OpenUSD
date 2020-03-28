//
// Copyright 2020 benmalartre
//
// Unlicensed 
//
#include "pxr/pxr.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/renderer.h"
#include "pxr/imaging/plugin/LoFi/mesh.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

#include <chrono>
#include <thread>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

static const char* VERTEX_SHADER = 
"#version 330\n" \
"uniform mat4 model;\n" \
"uniform mat4 view;\n" \
"uniform mat4 projection;\n" \
"\n" \
"in vec3 position;\n" \
"void main(){\n" \
"    vec3 p = vec3(view * model * vec4(position,1.0));\n" \
"    gl_Position = projection * vec4(p,1.0);\n" \
"}";

static const char* FRAGMENT_SHADER = 
"#version 330\n" \
"uniform vec3 color;\n" \
"out vec4 outColor;\n" \
"void main()\n" \
"{\n" \
"	outColor = vec4(1.0,1.0,1.0,1.0);\n" \
"}";

static float TRIANGLE_POINTS[9] = {
  -1.f, -1.f, 0.f,
   0.f,  1.f, 0.f,
   1.f, -1.f, 0.f
};

LoFiRenderer::LoFiRenderer()
    : _width(0)
    , _height(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    , _scene(nullptr)
    , _enableSceneColors(false)
{
  
  /*
  _vertex.Load("/Users/benmalartre/Documents/RnD/USD/pxr/imaging/plugin/LoFi/simple_vertex.glsl", SHADER_VERTEX);
  GLCheckError("Load Vertex Shader");
  _fragment.Load("/Users/benmalartre/Documents/RnD/USD/pxr/imaging/plugin/LoFi/simple_fragment.glsl", SHADER_FRAGMENT);
  GLCheckError("Load Fragment Shader");
  _program.Build("Simple", &_vertex, &_fragment);
  GLCheckError("LBuild Program");
  */
  _program.Build("Polymesh", VERTEX_SHADER, FRAGMENT_SHADER);


  size_t sz = 9 * sizeof(float);
  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao);

  // generate vertex buffer object
  glGenBuffers(1, &_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);

  // push buffer to GPU
  glBufferData(GL_ARRAY_BUFFER, sz, NULL,GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sz, &TRIANGLE_POINTS[0]);

  // attibute position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  // bind shader program
  glBindAttribLocation(_program.Get(), 0, "position");
  glLinkProgram(_program.Get());

  // unbind vertex array object
  glBindVertexArray(0);

}

LoFiRenderer::~LoFiRenderer()
{
}

void
LoFiRenderer::SetScene(LoFiScene* scene)
{
  _scene = scene;
}

void
LoFiRenderer::SetEnableSceneColors(bool enableSceneColors)
{
  _enableSceneColors = enableSceneColors;
}

void
LoFiRenderer::SetViewport(unsigned int width, unsigned int height)
{
  _width = width;
  _height = height;
}

void
LoFiRenderer::SetCamera(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projMatrix)
{
  _viewMatrix = viewMatrix;
  _projMatrix = projMatrix;
  _inverseViewMatrix = viewMatrix.GetInverse();
  _inverseProjMatrix = projMatrix.GetInverse();
}

void
LoFiRenderer::Render(void)
{

  _clearColor = GfVec4f(
    (float)rand()/(float)RAND_MAX,
    (float)rand()/(float)RAND_MAX,
    (float)rand()/(float)RAND_MAX,
    1.f
  );

  glClearColor(_clearColor[0],_clearColor[1],_clearColor[2],_clearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(_program.Get());
      
  GfMatrix4f identity(1);
  // model matrix
  glUniformMatrix4fv(
    glGetUniformLocation(_program.Get(), "model"),
    1,
    GL_FALSE,
    &identity[0][0]
  );
  
  // view matrix
  glUniformMatrix4fv(
    glGetUniformLocation(_program.Get(), "view"),
    1,
    GL_FALSE,
    &GfMatrix4f(_viewMatrix)[0][0]
  );

  // projection matrix
  glUniformMatrix4fv(
    glGetUniformLocation(_program.Get(), "projection"),
    1,
    GL_FALSE,
    &GfMatrix4f(_projMatrix)[0][0]
  );
        
  glBindVertexArray(_vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
  glUseProgram(0);

}


PXR_NAMESPACE_CLOSE_SCOPE
