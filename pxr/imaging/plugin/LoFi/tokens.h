//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_TOKENS_H
#define PXR_IMAGING_PLUGIN_LOFI_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

// registry tokens
#define LOFI_REGISTRY_TOKENS        \
    (elementBuffer)                 \
    (glslProgram)                   \
    (glslShader)                    \
    (vertexArray)                   \
    (vertexBuffer)                  \

// opengl types tokens
#define LOFI_GL_TOKENS              \
    ((_float, "float"))             \
    ((_int, "int"))                 \
    (ivec2)                         \
    (ivec3)                         \
    (ivec4)                         \
    (vec2)                          \
    (vec3)                          \
    (vec4)                          \
    (mat3)                          \
    (mat4)                          \
    (texture1D)                     \
    (texture2D)                     \
    (texture3D)                     \
    (isamplerBuffer)                \
    (samplerBuffer)                 \

// opengl buffers tokens
#define  LOFI_BUFFER_TOKENS         \
    (position)                      \
    (normal)                        \
    (tangent)                       \
    (color)                         \
    (uv)                            \
    (width)                         \
    (id)                            \
    (scale)                         \
    (shape_position)                \
    (shape_normal)                  \
    (shape_color)                   \
    (shape_uv)                      \

#define LOFI_STAGE_TOKENS           \
    (vertex)                        \
    (geometry)                      \
    (fragment)                      \

// opengl uniforms tokens
#define LOFI_UNIFORM_TOKENS         \
    (model)                         \
    (view)                          \
    (invview)                       \
    (projection)                    \
    (tex0)                          \
    (tex1)                          \
    (tex2)                          \
    (tex3)                          \
    (tex4)                          \
    (tex5)                          \
    (tex6)                          \
    (tex7)                          \
    (nearplane)                     \
    (farplane)                      \

// shader tokens
#define LOFI_SHADER_TOKENS          \
    (common)                        \
    (vertex)                        \
    (geometry)                      \
    (fragment)                      \

TF_DECLARE_PUBLIC_TOKENS(LoFiRegistryTokens, LOFI_API, LOFI_REGISTRY_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(LoFiGLTokens, LOFI_API, LOFI_GL_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(LoFiBufferTokens, LOFI_API, LOFI_BUFFER_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(LoFiStageTokens, LOFI_API, LOFI_STAGE_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(LoFiUniformTokens, LOFI_API, LOFI_UNIFORM_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(LoFiShaderTokens, LOFI_API, LOFI_SHADER_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_TOKENS_H
