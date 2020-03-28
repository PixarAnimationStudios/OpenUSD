//
// Copyright 2020 benmalartre
//
// Unlicensed 
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_RENDERER_H
#define PXR_IMAGING_PLUGIN_LOFI_RENDERER_H

#include "pxr/pxr.h"

#include "pxr/imaging/plugin/LoFi/shader.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/matrix4d.h"

#include <random>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiScene;

/// \class LoFiRenderer
///
/// LoFiRenderer implements a low-fidelity opengl 3.3 renderer
///
class LoFiRenderer final {
public:
    /// Renderer constructor.
    LoFiRenderer();

    /// Renderer destructor.
    ~LoFiRenderer();

    /// Set the LoFi scene that this renderer will use
    ///   \param scene The LoFi scene to use.
    void SetScene(LoFiScene* scene);

    /// Specify a new viewport size for the sample/color buffer.
    ///   \param width The new viewport width.
    ///   \param height The new viewport height.
    void SetViewport(unsigned int width, unsigned int height);

    /// Set the camera to use for rendering.
    ///   \param viewMatrix The camera's world-to-view matrix.
    ///   \param projMatrix The camera's view-to-NDC projection matrix.
    void SetCamera(const GfMatrix4d& viewMatrix, const GfMatrix4d& projMatrix);

    /// Sets whether to use scene colors while rendering.
    ///   \param enableSceneColors Whether drawing should sample color, or draw
    ///                            everything as white.
    void SetEnableSceneColors(bool enableSceneColors);

    /// Set the clear color to use 
    void GetClearColor(const GfVec4f& clearValue);

    /// Rendering entrypoint
    void Render(void);
    

private:

    // The width of the viewport we're rendering into.
    unsigned int _width;
    // The height of the viewport we're rendering into.
    unsigned int _height;

    // View matrix: world space to camera space.
    GfMatrix4d _viewMatrix;
    // Projection matrix: camera space to NDC space.
    GfMatrix4d _projMatrix;
    // The inverse view matrix: camera space to world space.
    GfMatrix4d _inverseViewMatrix;
    // The inverse projection matrix: NDC space to camera space.
    GfMatrix4d _inverseProjMatrix;

    // Clear color
    GfVec4f _clearColor;

    // Our handle to the lofi scene.
    LoFiScene* _scene;

    // Our simple glsl program;
    GLSLProgram         _program;
    GLSLShader          _vertex;
    GLSLShader          _fragment;

    GLuint               _vao;
    GLuint               _vbo;
  
    // Should we enable scene colors?
    bool _enableSceneColors;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_RENDERER_H
