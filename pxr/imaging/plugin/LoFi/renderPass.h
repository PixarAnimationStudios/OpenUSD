//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/shader.h"

PXR_NAMESPACE_OPEN_SCOPE

static const char *VERTEX_SHADER_120[1] = {
  "#version 120                                             \n" 
  "uniform mat4 model;                                      \n"
  "uniform mat4 view;                                       \n"
  "uniform mat4 projection;                                 \n"
  "                                                         \n"
  "attribute vec3 position;                                 \n"
  "attribute vec3 normal;                                   \n"
  "attribute vec3 color;                                    \n"
  "varying vec3 vertex_normal;                              \n"
  "varying vec3 vertex_color;                               \n"
  "void main(){                                             \n"
  "    vertex_normal = (model * vec4(normal, 0.0)).xyz;      \n"
  "    vertex_color = color;                                \n"
  "    vec3 p = vec3(view * model * vec4(position,1.0));    \n"
  "    gl_Position = projection * vec4(p,1.0);              \n"
  "}"
};

static const char *FRAGMENT_SHADER_120[1] = {
  "#version 120                                             \n"
  "varying vec3 vertex_normal;                              \n"
  "varying vec3 vertex_color;                               \n"
  "void main()                                              \n"
  "{                                                        \n"
  " vec3 color = vertex_normal * vertex_color;              \n"
  "	gl_FragColor = vec4(vertex_color,1.0);                         \n"
  "}"
};

static const char *VERTEX_SHADER_330[1] = {
  "#version 330 core                                        \n" 
  "uniform mat4 model;                                      \n"
  "uniform mat4 view;                                       \n"
  "uniform mat4 projection;                                 \n"
  "                                                         \n"
  "in vec3 position;                                        \n"
  "in vec3 color;                                           \n"
  "out vec3 vertex_color;                                   \n"
  "void main(){                                             \n"
  "    vertex_color = color;                                \n"
  "    vec3 p = vec3(view * model * vec4(position,1.0));    \n"
  "    gl_Position = projection * vec4(p,1.0);              \n"
  "}"
};

static const char *FRAGMENT_SHADER_330[1] = {
  "#version 330 core                                        \n"
  "in vec3 vertex_color;                                    \n"
  "out vec4 outColor;                                       \n"
  "void main()                                              \n"
  "{                                                        \n"
  "	outColor = vec4(vertex_color,1.0);                      \n"
  "}"
};

static const int NUM_TEST_POINTS = 4;

static float TEST_POINTS[NUM_TEST_POINTS * 3] = {
  -100.f, -100.f, 0.f,
  -100.f,  100.f, 0.f,
   100.f,  100.f, 0.f,
   100.f, -100.f, 0.f
};

static float TEST_COLORS[NUM_TEST_POINTS * 3] = {
  1.f, 0.f, 0.f,
  0.f, 1.f, 0.f,
  0.f, 0.f, 1.f,
  1.f, 0.f, 1.f
};

static const int NUM_TEST_INDICES = 6;

static int TEST_INDICES[NUM_TEST_INDICES] = {
  0, 1, 2,
  2, 3, 0
};

/// \class LoFiRenderPass
///
class LoFiRenderPass final : public HdRenderPass 
{
public:
    /// Renderpass constructor.
    ///   \param index The render index containing scene data to render.
    ///   \param collection The initial rprim collection for this renderpass.
    LoFiRenderPass( HdRenderIndex *index,
                    HdRprimCollection const &collection);

    /// Renderpass destructor.
    virtual ~LoFiRenderPass();

protected:

    /// Setup simple GLSL program
    void _SetupSimpleGLSLProgram();
    void _SetupSimpleVertexArray();

    /// Setup the framebuffer with color and depth attachments
    ///   \param width The width of the framebuffer
    ///   \param height The height of the framebuffer
    void _SetupDrawTarget(int width, int height);

    /// Draw the scene with the bound renderpass state.
    ///   \param renderPassState Input parameters (including viewer parameters)
    ///                          for this renderpass.
    ///   \param renderTags Which rendertags should be drawn this pass.
    void _Execute(HdRenderPassStateSharedPtr const& renderPassState,
                  TfTokenVector const &renderTags) override;

private:
  //LoFiRenderer*             _renderer;
  GlfDrawTargetRefPtr       _drawTarget;

  // Our simple glsl program;
  LoFiGLSLProgramSharedPtr       _program;

  // Test buffers
  GLuint               _vao;
  GLuint               _vbo;
  GLuint               _cbo;
  GLuint               _ebo;

};

PXR_NAMESPACE_CLOSE_SCOPE
