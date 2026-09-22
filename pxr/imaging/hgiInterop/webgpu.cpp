//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiInterop/webgpu.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"


#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

static void
_ConvertHgiTextureToOpenGL(
    Hgi* hgi,
    HgiTextureHandle const &src,
    std::vector<uint8_t> &texels,
    uint32_t* glDest)
{
    // XXX we want to use EXT_external_objects and GL_EXT_semaphore to share
    // memory between openGL and Vulkan.
    // See examples: Nvidia gl_vk_simple_interop and Khronos: open_gl_interop
    // But for now we do a CPU readback of the GPU texels and upload to GPU.

    HgiTextureDesc const& texDesc = src->GetDescriptor();
    const size_t byteSize = src->GetByteSizeOfResource();

    HgiTextureGpuToCpuOp readBackOp;
    readBackOp.cpuDestinationBuffer = texels.data();
    readBackOp.destinationBufferByteSize = byteSize;
    readBackOp.destinationByteOffset = 0;
    readBackOp.gpuSourceTexture = src;
    readBackOp.mipLevel = 0;
    readBackOp.sourceTexelOffset = GfVec3i(0);

    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(readBackOp);
    hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    if (*glDest == 0) {
        glGenTextures(1, glDest);
        glBindTexture(GL_TEXTURE_RECTANGLE, *glDest);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glBindTexture(GL_TEXTURE_RECTANGLE, *glDest);
    }

    const int32_t width = texDesc.dimensions[0];
    const int32_t height = texDesc.dimensions[1];

    if (texDesc.format == HgiFormatFloat32Vec4) {
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                     GL_FLOAT, texels.data());
    } else if (texDesc.format == HgiFormatFloat16Vec4) {
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
                     GL_HALF_FLOAT, texels.data());
    } else if (texDesc.format == HgiFormatUNorm8Vec4) {
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, texels.data());
    } else if (texDesc.format == HgiFormatFloat32) {
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32F, width, height, 0, GL_RED,
                     GL_FLOAT, texels.data());
    } else {
        TF_WARN("Unsupported texture format for HgiTexture-GL interop");
    }

    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
}

namespace {
    struct Vertex {
        float position[2];
        float uv[2];
    };
}

static bool
_ProcessGLErrors(bool silent = false)
{
    bool foundError = false;
    GLenum error;
    // Protect from doing infinite looping when glGetError
    // is called from an invalid context.
    int watchDogCount = 0;
    while ((watchDogCount++ < 256) &&
           ((error = glGetError()) != GL_NO_ERROR)) {
        foundError = true;
        const GLubyte *errorString = gluErrorString(error);

        std::ostringstream errorMessage;
        if (!errorString) {
            errorMessage << "GL error code: 0x" << std::hex << error
                         << std::dec;
        } else {
            errorMessage << "GL error: " << errorString;
        }

        if (!silent) {
            TF_WARN("%s", errorMessage.str().c_str());
        }
    }
    return foundError;
}

static GLuint
_compileShader(
        GLchar const* const shaderSource, GLenum shaderType)
{
    GLint status;

    // Determine if GLSL version 140 is supported by this context.
    //  We'll use this info to generate a GLSL shader source string
    //  with the proper version preprocessor string prepended
    float  glLanguageVersion;

    sscanf((char *)glGetString(GL_SHADING_LANGUAGE_VERSION), "%f",
           &glLanguageVersion);
    GLchar const * const versionTemplate = "#version %d\n";

    // GL_SHADING_LANGUAGE_VERSION returns the version standard version form
    //  with decimals, but the GLSL version preprocessor directive simply
    //  uses integers (thus 1.10 should 110 and 1.40 should be 140, etc.)
    //  We multiply the floating point number by 100 to get a proper
    //  number for the GLSL preprocessor directive
    GLuint version = 100 * glLanguageVersion;

    // Prepend our vertex shader source string with the supported GLSL version
    // so the shader will work on ES, Legacy, and OpenGL 3.2 Core Profile
    // contexts
    GLchar versionString[sizeof(versionTemplate) + 8];
    snprintf(versionString, sizeof(versionString), versionTemplate, version);

    GLuint s = glCreateShader(shaderType);
    GLchar const* const sourceArray[2] = { versionString, shaderSource };
    glShaderSource(s, 2, sourceArray, NULL);
    glCompileShader(s);

    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint maxLength = 0;

        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        GLchar *errorLog = (GLchar*)malloc(maxLength);
        glGetShaderInfoLog(s, maxLength, &maxLength, errorLog);

        TF_CODING_ERROR("Failed to compile Metal/GL interop shader %s",
                        errorLog);
        free(errorLog);
    }

    return s;
}

static void
_OutputShaderLog(GLuint program)
{
    GLint maxLength = 2048;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
    if (maxLength)
    {
        // The maxLength includes the NULL character
        GLchar *errorLog = (GLchar*)malloc(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, errorLog);

        TF_FATAL_CODING_ERROR(
                "Failed to link GL program for Metal/GL interop:\n%s", errorLog);
        free(errorLog);
    }
}

void
HgiInteropWebGPU::_CreateShaderContext(
        int32_t vertexSource,
        int32_t fragmentSource,
        ShaderContext &shader)
{
    GLuint program;

    program = glCreateProgram();
    glAttachShader(program, fragmentSource);
    glAttachShader(program, vertexSource);
    glLinkProgram(program);

    _OutputShaderLog(program);

    glUseProgram(program);

    // Set up the vertex structure description
    shader.posAttrib = glGetAttribLocation(program, "inPosition");
    shader.texAttrib = glGetAttribLocation(program, "inTexCoord");

    shader.samplerColorLoc = glGetUniformLocation(program, "interopTexture");
    shader.samplerDepthLoc = glGetUniformLocation(program, "depthTexture");
    shader.blitTexSizeUniform = glGetUniformLocation(program, "texSize");

    shader.vao = 0;
    glGenVertexArrays(1, &shader.vao);
    if (shader.vao) {
        glBindVertexArray(shader.vao);
    }

    glGenBuffers(1, &shader.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, shader.vbo);

    if (shader.vao) {
        glEnableVertexAttribArray(shader.posAttrib);
        glVertexAttribPointer(shader.posAttrib,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (void*)(offsetof(Vertex, position)));
        glEnableVertexAttribArray(shader.texAttrib);
        glVertexAttribPointer(shader.texAttrib,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (void*)(offsetof(Vertex, uv)));
    }

    Vertex v[12] = {
            { {-1, -1}, {0, 1} },
            { { 1, -1}, {1, 1} },
            { {-1,  1}, {0, 0} },

            { {-1, 1}, {0, 0} },
            { {1, -1}, {1, 1} },
            { {1,  1}, {1, 0} }
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    shader.program = program;

    glBindVertexArray(0);
    glUseProgram(0);
}

HgiInteropWebGPU::HgiInteropWebGPU(Hgi* hgi)
        : _hgi(nullptr)
{
    _hgi = static_cast<Hgi*>(hgi);

    GarchGLApiLoad();

    _ProcessGLErrors(true);
    _CaptureOpenGlState();

    // Load our common vertex shader. This is used by both the fragment shaders
    // below
    static GLchar const* const vertexShader =
            "#if __VERSION__ >= 140\n"
            "in vec2 inPosition;\n"
            "in vec2 inTexCoord;\n"
            "out vec2 texCoord;\n"
            "#else\n"
            "attribute vec2 inPosition;\n"
            "attribute vec2 inTexCoord;\n"
            "varying vec2 texCoord;\n"
            "#endif\n"
            "\n"
            "void main()\n"
            "{\n"
            "    texCoord = inTexCoord;\n"
            "    gl_Position = vec4(inPosition, 0.0, 1.0);\n"
            "}\n";

    GLuint vs = _compileShader(vertexShader, GL_VERTEX_SHADER);

    static GLchar const* const fragmentShaderColor =
            "#if __VERSION__ >= 140\n"
            "in vec2         texCoord;\n"
            "out vec4        fragColor;\n"
            "#else\n"
            "varying vec2    texCoord;\n"
            "#endif\n"
            "\n"
            // A GL_TEXTURE_RECTANGLE
            "uniform sampler2DRect interopTexture;\n"
            "\n"
            // The dimensions of the source texture. The sampler coordinates for
            // a GL_TEXTURE_RECTANGLE are in pixels,
            // rather than the usual normalised 0..1 range.
            "uniform vec2 texSize;\n"
            "\n"
            "void main(void)\n"
            "{\n"
            "    vec2 uv = vec2(texCoord.x, 1.0 - texCoord.y) * texSize;\n"
            "#if __VERSION__ >= 140\n"
            "    fragColor = texture(interopTexture, uv.st);\n"
            "#else\n"
            "    gl_FragColor = texture2DRect(interopTexture, uv.st);\n"
            "#endif\n"
            "}\n";

    static GLchar const* const fragmentShaderColorDepth =
            "#if __VERSION__ >= 140\n"
            "in vec2         texCoord;\n"
            "out vec4        fragColor;\n"
            "#else\n"
            "varying vec2    texCoord;\n"
            "#endif\n"
            "\n"
            // A GL_TEXTURE_RECTANGLE
            "uniform sampler2DRect interopTexture;\n"
            "uniform sampler2DRect depthTexture;\n"
            "\n"
            // The dimensions of the source texture. The sampler coordinates for
            // a GL_TEXTURE_RECTANGLE are in pixels,
            // rather than the usual normalised 0..1 range.
            "uniform vec2 texSize;\n"
            "\n"
            "void main(void)\n"
            "{\n"
            "    vec2 uv = vec2(texCoord.x, 1.0 - texCoord.y) * texSize;\n"
            "#if __VERSION__ >= 140\n"
            "    vec4 encodedDepth = texture(depthTexture, uv.st);\n"
            "#else\n"
            "    vec4 encodedDepth = texture2DRect(depthTexture, uv.st);\n"
            "#endif\n"
            "    float depth = dot(encodedDepth,\n"
            "        vec4(1.0, 1 / 255.0, 1 / 65025.0, 1 / 16581375.0) );\n"
            "#if __VERSION__ >= 140\n"
            "    fragColor = texture(interopTexture, uv.st);\n"
            "#else\n"
            "    gl_FragColor = texture2DRect(interopTexture, uv.st);\n"
            "#endif\n"
            "    gl_FragDepth = depth;\n"
            "}\n";

    GLuint fsColor = _compileShader(fragmentShaderColor, GL_FRAGMENT_SHADER);
    GLuint fsColorDepth =
            _compileShader(fragmentShaderColorDepth, GL_FRAGMENT_SHADER);

    _CreateShaderContext(
            vs, fsColor, _shaderProgramContext[ShaderContextColor]);
    _CreateShaderContext(
            vs, fsColorDepth, _shaderProgramContext[ShaderContextColorDepth]);

    // Release the local instance of the fragment shader. The shader program
    // maintains a reference.
    glDeleteShader(vs);
    glDeleteShader(fsColor);
    glDeleteShader(fsColorDepth);

    _RestoreOpenGlState();

    _glColorTexture = 0;
    _glDepthTexture = 0;
}

HgiInteropWebGPU::~HgiInteropWebGPU()
{
    _FreeTransientTextureCacheRefs();
}

void
HgiInteropWebGPU::_FreeTransientTextureCacheRefs()
{
    if (_glColorTexture) {
        glDeleteTextures(1, &_glColorTexture);
        _glColorTexture = 0;
    }
    if (_glDepthTexture) {
        glDeleteTextures(1, &_glDepthTexture);
        _glDepthTexture = 0;
    }

}

void
HgiInteropWebGPU::_CaptureOpenGlState()
{
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_restoreDrawFbo);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &_restoreVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &_restoreVbo);
    glGetBooleanv(GL_DEPTH_TEST, (GLboolean*)&_restoreDepthTest);
    glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&_restoreDepthWriteMask);
    glGetBooleanv(GL_STENCIL_WRITEMASK, (GLboolean*)&_restoreStencilWriteMask);
    glGetBooleanv(GL_CULL_FACE, (GLboolean*)&_restoreCullFace);
    glGetIntegerv(GL_FRONT_FACE, &_restoreFrontFace);
    glGetIntegerv(GL_DEPTH_FUNC, &_restoreDepthFunc);
    glGetIntegerv(GL_VIEWPORT, _restoreViewport);
    glGetBooleanv(GL_BLEND, (GLboolean*)&_restoreblendEnabled);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &_restoreColorOp);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &_restoreAlphaOp);
    glGetIntegerv(GL_BLEND_SRC_RGB, &_restoreColorSrcFnOp);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &_restoreAlphaSrcFnOp);
    glGetIntegerv(GL_BLEND_DST_RGB, &_restoreColorDstFnOp);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &_restoreAlphaDstFnOp);
    glGetBooleanv(
            GL_SAMPLE_ALPHA_TO_COVERAGE,
            (GLboolean*)&_restoreAlphaToCoverage);
    glGetIntegerv(GL_POLYGON_MODE, &_restorePolygonMode);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &_restoreActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE, &_restoreTexture[0]);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE, &_restoreTexture[1]);

    if (!_restoreVao && _restoreVbo) {
        for (int i = 0; i < 2; i++) {
            VertexAttribState &state(_restoreVertexAttribState[i]);

            glGetVertexAttribiv(
                    i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &state.enabled);
            glGetVertexAttribiv(
                    i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &state.size);
            glGetVertexAttribiv(
                    i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &state.type);
            glGetVertexAttribiv(
                    i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &state.normalized);
            glGetVertexAttribiv(
                    i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &state.stride);
            glGetVertexAttribiv(
                    i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &state.bufferBinding);

            glGetVertexAttribPointerv(
                    i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &state.pointer);
        }
    }

    glGetIntegerv(GL_CURRENT_PROGRAM, &_restoreProgram);
}

void
HgiInteropWebGPU::_RestoreOpenGlState()
{
    if (_restoreAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    glBlendFuncSeparate(_restoreColorSrcFnOp, _restoreColorDstFnOp,
                        _restoreAlphaSrcFnOp, _restoreAlphaDstFnOp);
    glBlendEquationSeparate(_restoreColorOp, _restoreAlphaOp);

    if (_restoreblendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    glViewport(_restoreViewport[0], _restoreViewport[1],
               _restoreViewport[2], _restoreViewport[3]);
    glDepthFunc(_restoreDepthFunc);
    glDepthMask(_restoreDepthWriteMask);
    glStencilMask(_restoreStencilWriteMask);

    if (_restoreCullFace) {
        glEnable(GL_CULL_FACE);
    }
    else {
        glDisable(GL_CULL_FACE);
    }
    glFrontFace(_restoreFrontFace);

    if (_restoreDepthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glPolygonMode(GL_FRONT_AND_BACK, _restorePolygonMode);

    if (_restoreVao) {
        glBindVertexArray(_restoreVao);
    }
    glBindBuffer(GL_ARRAY_BUFFER, _restoreVbo);

    if (!_restoreVao && _restoreVbo) {
        for (int i = 0; i < 2; i++) {
            VertexAttribState &state(_restoreVertexAttribState[i]);
            if (state.enabled) {
                glVertexAttribPointer(state.bufferBinding, state.size,
                                      state.type, state.normalized, state.stride, state.pointer);
                glEnableVertexAttribArray(state.bufferBinding);
            } else {
                glDisableVertexAttribArray(state.bufferBinding);
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE, _restoreTexture[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, _restoreTexture[1]);

    glActiveTexture(_restoreActiveTexture);

    glUseProgram(_restoreProgram);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _restoreDrawFbo);
}

void
HgiInteropWebGPU::_BlitToOpenGL(VtValue const &framebuffer,
                               GfVec4i const &compRegion,
                               int shaderIndex,
                                HgiTextureDesc const& texDesc)
{
    // Clear GL error state
    _ProcessGLErrors(true);

    _CaptureOpenGlState();

    if (!framebuffer.IsEmpty()) {
        if (framebuffer.IsHolding<uint32_t>()) {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                              framebuffer.UncheckedGet<uint32_t>());
        } else {
            TF_CODING_ERROR(
                    "dstFramebuffer must hold uint32_t when targeting OpenGL");
        }
    }

    // XXX: This doesn't support "optional" depth. Enabling depth writes without
    // a depth aov to xfer would mean that the bound depth buffer is overwritten
    // with the fullscreen tri's depth (i.e., the near plane).
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(/*srcColor*/GL_ONE,
            /*dstColor*/GL_ONE_MINUS_SRC_ALPHA,
            /*srcAlpha*/GL_ONE,
            /*dstAlpha*/GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

    ShaderContext &shader = _shaderProgramContext[shaderIndex];
    glUseProgram(shader.program);

    // Set up the vertex structure description
    if (shader.vao) {
        glBindVertexArray(shader.vao);
    }
    else {
        glBindBuffer(GL_ARRAY_BUFFER, shader.vbo);

        glEnableVertexAttribArray(shader.posAttrib);
        glVertexAttribPointer(
                shader.posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                (void*)(offsetof(Vertex, position)));
        glEnableVertexAttribArray(shader.texAttrib);
        glVertexAttribPointer(
                shader.texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                (void*)(offsetof(Vertex, uv)));
    }

    GLint unit = 0;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE, _glColorTexture);
    glUniform1i(shader.samplerColorLoc, unit++);

    if (shader.samplerDepthLoc != -1) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_RECTANGLE, _glDepthTexture);
        glUniform1i(shader.samplerDepthLoc, unit);
    }

    glUniform2f(shader.blitTexSizeUniform,
                texDesc.dimensions[0],
                texDesc.dimensions[1]);

    // Region of the framebuffer over which to composite.
    glViewport(compRegion[0], compRegion[1], compRegion[2], compRegion[3]);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    _RestoreOpenGlState();
    glFlush();
}

void
HgiInteropWebGPU::CompositeToInterop(
        HgiTextureHandle const &color,
        HgiTextureHandle const &depth,
        VtValue const &framebuffer,
        GfVec4i const &compRegion)
{
    if (!ARCH_UNLIKELY(color)) {
        TF_CODING_ERROR("No valid color texture provided");
        return;
    }

    _FreeTransientTextureCacheRefs();
    int glShaderIndex = -1;
    // Convert textures from HgiTexture to GL
    if (_colorTarget.size() != color->GetByteSizeOfResource()) {
        _colorTarget.resize(color->GetByteSizeOfResource());
    }
    _ConvertHgiTextureToOpenGL(_hgi, color, _colorTarget, &_glColorTexture);

    if (depth) {
        if (_depthTarget.size() != depth->GetByteSizeOfResource()) {
            _depthTarget.resize(depth->GetByteSizeOfResource());
        }
        _ConvertHgiTextureToOpenGL(_hgi, depth, _depthTarget, &_glDepthTexture);
    }

    if (!ARCH_UNLIKELY(_glColorTexture)) {
        TF_CODING_ERROR("A valid color texture handle is required.\n");
        return;
    }

    glBindTexture(GL_TEXTURE_RECTANGLE, _glColorTexture);
    glBindTexture(GL_TEXTURE_RECTANGLE, _glDepthTexture);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    if (depth && color) {
        glShaderIndex = ShaderContextColorDepth;
    }
    else if (color) {
        glShaderIndex = ShaderContextColor;
    }

    if (glShaderIndex != -1) {
        _BlitToOpenGL(framebuffer, compRegion, glShaderIndex,
                      color->GetDescriptor());

        _ProcessGLErrors();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

