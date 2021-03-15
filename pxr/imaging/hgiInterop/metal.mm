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

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiInterop/metal.h"

#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/hgi.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

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
HgiInteropMetal::_CreateShaderContext(
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
        { {-1, -1}, {0, 0} },
        { { 1, -1}, {1, 0} },
        { {-1,  1}, {0, 1} },
        
        { {-1, 1}, {0, 1} },
        { {1, -1}, {1, 0} },
        { {1,  1}, {1, 1} },
        
        // Second set have flipped v coord
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

HgiInteropMetal::HgiInteropMetal(Hgi* hgi)
    : _hgiMetal(nullptr)
{
    _hgiMetal = static_cast<HgiMetal*>(hgi);
    _device = _hgiMetal->GetPrimaryDevice();

    NSError *error = NULL;
    
    GarchGLApiLoad();
    
    _currentOpenGLContext = [NSOpenGLContext currentContext];
    
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
    
    error = NULL;

    // Load all the default shader files
    NSString *shaderSource =
        @"#include <metal_stdlib>\n"
        "#include <simd/simd.h>\n"
        "\n"
        "using namespace metal;\n"
        "\n"
        // Depth buffer copy function
        "kernel void copyDepth(depth2d<float, access::read> texIn,\n"
        "                      texture2d<float, access::write> texOut,\n"
        "                      uint2 gid [[thread_position_in_grid]])\n"
        "{\n"
        "    if(gid.x >= texOut.get_width() || gid.y >= texOut.get_height())\n"
        "        return;\n"
        "    float depth = texIn.read(gid);\n"
        // This works around an issue with AMD Vega incorrectly resolving
        // depth buffers when there's an empty render target performing the
        // load/resolve
        "    float maxDepth = 1.0f - FLT_EPSILON;\n"
        "    depth = depth == 0.0f?maxDepth:min(maxDepth, depth);\n"
        "    float4 encodedDepth =\n"
        "        float4(1.0f, 255.0f, 65025.0f, 16581375.0f) * depth;\n"
        "    encodedDepth = fract(encodedDepth);\n"
        "    encodedDepth -= encodedDepth.yzww *\n"
        "        float4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);\n"
        "    texOut.write(encodedDepth, gid);\n"
        "}\n"
        "\n"
        "kernel void copyColour(\n"
        "    texture2d<float, access::read> texIn,\n"
        "    texture2d<float, access::write> texOut,\n"
        "    uint2 gid [[thread_position_in_grid]])\n"
        "{\n"
        "    if(gid.x >= texOut.get_width() || gid.y >= texOut.get_height())\n"
        "        return;\n"
        "    texOut.write(texIn.read(gid), gid);\n"
        "}\n";

    MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
    options.fastMathEnabled = YES;
        
    _defaultLibrary = [_device newLibraryWithSource:shaderSource
                                            options:options
                                              error:&error];
    
    if (!_defaultLibrary) {
        NSString *errStr = [error localizedDescription];
        TF_FATAL_CODING_ERROR(
            "Failed to create interop pipeline state: %s",
            [errStr UTF8String]);
    }
    
    // Load the fragment program into the library
    _computeDepthCopyProgram =
        [_defaultLibrary newFunctionWithName:@"copyDepth"];
    _computeColorCopyProgram =
        [_defaultLibrary newFunctionWithName:@"copyColour"];
    
    [options release];
    options = nil;

    
    MTLAutoreleasedComputePipelineReflection* reflData = 0;
    
    MTLComputePipelineDescriptor *computePipelineStateDescriptor =
        [[MTLComputePipelineDescriptor alloc] init];
    
    computePipelineStateDescriptor.computeFunction = _computeDepthCopyProgram;
    HGIMETAL_DEBUG_LABEL(computePipelineStateDescriptor, "Interop depth blit");
    
    // Create a new Compute pipeline state object
    _computePipelineStateDepth = [_device
        newComputePipelineStateWithDescriptor:computePipelineStateDescriptor
                                      options:MTLPipelineOptionNone
                                   reflection:reflData
                                        error:&error];

    if (!_computePipelineStateDepth) {
        NSString *errStr = [error localizedDescription];
        TF_FATAL_CODING_ERROR(
            "Failed to create compute pipeline state, error %s",
            [errStr UTF8String]);
    }
        
    computePipelineStateDescriptor.computeFunction = _computeColorCopyProgram;
    HGIMETAL_DEBUG_LABEL(computePipelineStateDescriptor, "Interop color blit");
    
    // Create a new Compute pipeline state object
    _computePipelineStateColor = [_device
        newComputePipelineStateWithDescriptor:computePipelineStateDescriptor
                                      options:MTLPipelineOptionNone
                                   reflection:reflData
                                        error:&error];
    [computePipelineStateDescriptor release];

    if (!_computePipelineStateColor) {
        NSString *errStr = [error localizedDescription];
        TF_FATAL_CODING_ERROR(
            "Failed to create compute pipeline state, error %s",
            [errStr UTF8String]);
    }

    CVReturn cvret;

    // Create the texture caches
    cvret = CVMetalTextureCacheCreate(
        kCFAllocatorDefault, nil, _device, nil, &_cvmtlTextureCache);
    if (cvret != kCVReturnSuccess) {
        TF_FATAL_CODING_ERROR(
            "Failed to create a Metal texture cache for Metal/GL interop");
    }

    _cvmtlColorTexture = nil;
    _cvmtlDepthTexture = nil;
    
    _mtlAliasedColorTexture = nil;
    _mtlAliasedDepthRegularFloatTexture = nil;
    
    CGLContextObj glctx = [_currentOpenGLContext CGLContextObj];
    CGLPixelFormatObj glPixelFormat =
        [[[NSOpenGLContext currentContext] pixelFormat] CGLPixelFormatObj];
    cvret = CVOpenGLTextureCacheCreate(
        kCFAllocatorDefault, nil, (__bridge CGLContextObj _Nonnull)(glctx),
        glPixelFormat, nil, &_cvglTextureCache);
    if (cvret != kCVReturnSuccess) {
        TF_FATAL_CODING_ERROR(
            "Failed to create an OpenGL texture cache for Metal/GL interop");
    }
    
    _pixelBuffer = nil;
    _depthBuffer = nil;
    _cvglColorTexture = nil;
    _cvglDepthTexture = nil;
    _glColorTexture = 0;
    _glDepthTexture = 0;
    
    _SetAttachmentSize(256, 256);
}

HgiInteropMetal::~HgiInteropMetal()
{
	_FreeTransientTextureCacheRefs();

    if (_cvglTextureCache) {
        CFRelease(_cvglTextureCache);
        _cvglTextureCache = nil;
    }
    if (_cvmtlTextureCache) {
        CFRelease(_cvmtlTextureCache);
        _cvmtlTextureCache = nil;
    }
}

void
HgiInteropMetal::_FreeTransientTextureCacheRefs()
{
    if (_glColorTexture) {
        glDeleteTextures(1, &_glColorTexture);
        _glColorTexture = 0;
    }
    if (_glDepthTexture) {
        glDeleteTextures(1, &_glDepthTexture);
        _glDepthTexture = 0;
    }
    
    if (_mtlAliasedColorTexture) {
        [_mtlAliasedColorTexture release];
        _mtlAliasedColorTexture = nil;
    }
    if (_mtlAliasedDepthRegularFloatTexture) {
        [_mtlAliasedDepthRegularFloatTexture release];
        _mtlAliasedDepthRegularFloatTexture = nil;
    }

    _cvmtlColorTexture = nil;
    _cvmtlDepthTexture = nil;

    _cvglColorTexture = nil;
    _cvglDepthTexture = nil;

    if (_pixelBuffer) {
        CFRelease(_pixelBuffer);
        _pixelBuffer = nil;
    }
    if (_depthBuffer) {
        CFRelease(_depthBuffer);
        _depthBuffer = nil;
    }
}

void
HgiInteropMetal::_ValidateGLContext()
{
    if (_currentOpenGLContext != [NSOpenGLContext currentContext]) {
        TF_FATAL_CODING_ERROR(
            "Current OpenGL context does not match that when HgiInteropMetal "
            "was created");
    }
}

void
HgiInteropMetal::_SetAttachmentSize(int width, int height)
{
    if (_mtlAliasedColorTexture != nil) {
        if (_mtlAliasedColorTexture.width == width &&
            _mtlAliasedColorTexture.height == height) {
            return;
        }
    }
    
    _ValidateGLContext();

    NSDictionary* cvBufferProperties = @{
        (__bridge NSString*)kCVPixelBufferOpenGLCompatibilityKey : @(TRUE),
        (__bridge NSString*)kCVPixelBufferMetalCompatibilityKey : @(TRUE),
    };
    
    _FreeTransientTextureCacheRefs();
    
    CVReturn cvret;

    // Create the IOSurface backed pixel buffers to hold the color and depth
    // data in OpenGL
    CVPixelBufferCreate(
        kCFAllocatorDefault,
        width,
        height,
        kCVPixelFormatType_32BGRA,
        (__bridge CFDictionaryRef)cvBufferProperties,
        &_pixelBuffer);
    
    CVPixelBufferCreate(
        kCFAllocatorDefault,
        width,
        height,
        kCVPixelFormatType_32BGRA,
        (__bridge CFDictionaryRef)cvBufferProperties,
        &_depthBuffer);
    
    // Create the OpenGL texture for the color buffer
    cvret = CVOpenGLTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault,
        _cvglTextureCache,
        _pixelBuffer,
        nil,
        &_cvglColorTexture);
    if (cvret != kCVReturnSuccess) {
        TF_FATAL_CODING_ERROR(
            "Failed to create the shared OpenGL color texture "
            "for Metal/GL interop");
    }
    _glColorTexture = CVOpenGLTextureGetName(_cvglColorTexture);
    
    // Create the OpenGL texture for the depth buffer
    cvret = CVOpenGLTextureCacheCreateTextureFromImage(
       kCFAllocatorDefault,
       _cvglTextureCache,
       _depthBuffer,
       nil,
       &_cvglDepthTexture);
    if (cvret != kCVReturnSuccess) {
        TF_FATAL_CODING_ERROR(
            "Failed to create the shared OpenGL depth texture "
            "for Metal/GL interop");
    }
    _glDepthTexture = CVOpenGLTextureGetName(_cvglDepthTexture);
    
    // Create the metal texture for the color buffer
    NSDictionary* metalTextureProperties = @{
        (__bridge NSString*)kCVMetalTextureCacheMaximumTextureAgeKey : @0,
    };
    cvret = CVMetalTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault,
        _cvmtlTextureCache,
        _pixelBuffer,
        (__bridge CFDictionaryRef)metalTextureProperties,
        MTLPixelFormatBGRA8Unorm,
        width,
        height,
        0,
        &_cvmtlColorTexture);
    if (cvret != kCVReturnSuccess) {
        TF_FATAL_CODING_ERROR(
            "Failed to create the shared Metal color texture "
            "for Metal/GL interop");
    }
    _mtlAliasedColorTexture = CVMetalTextureGetTexture(_cvmtlColorTexture);
    
    // Create the Metal texture for the depth buffer
    cvret = CVMetalTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault,
        _cvmtlTextureCache,
        _depthBuffer,
        (__bridge CFDictionaryRef)metalTextureProperties,
        MTLPixelFormatBGRA8Unorm,
        width,
        height,
        0,
        &_cvmtlDepthTexture);
    if (cvret != kCVReturnSuccess) {
        TF_FATAL_CODING_ERROR(
            "Failed to create the shared Metal depth texture "
            "for Metal/GL interop");
    }
    _mtlAliasedDepthRegularFloatTexture =
        CVMetalTextureGetTexture(_cvmtlDepthTexture);

    MTLTextureDescriptor *depthTexDescriptor =
        [MTLTextureDescriptor
         texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float
         width:width
         height:height
         mipmapped:false];
    depthTexDescriptor.usage =
        MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;
    depthTexDescriptor.resourceOptions =
        MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModePrivate;
    
    // Flush the caches
    CVOpenGLTextureCacheFlush(_cvglTextureCache, 0);
    CVMetalTextureCacheFlush(_cvmtlTextureCache, 0);
}

void
HgiInteropMetal::_CaptureOpenGlState()
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
HgiInteropMetal::_RestoreOpenGlState()
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
HgiInteropMetal::_BlitToOpenGL(VtValue const &framebuffer,
                               GfVec4i const &compRegion,
                               bool flipY, int shaderIndex)
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
                        /*dstAlpha*/GL_ONE);
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
                _mtlAliasedColorTexture.width,
                _mtlAliasedColorTexture.height);
    
    // Region of the framebuffer over which to composite.
    glViewport(compRegion[0], compRegion[1], compRegion[2], compRegion[3]);

    if (flipY) {
        glDrawArrays(GL_TRIANGLES, 6, 12);
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    _RestoreOpenGlState();
    glFlush();
}

void
HgiInteropMetal::CompositeToInterop(
    HgiTextureHandle const &color,
    HgiTextureHandle const &depth,
    VtValue const &framebuffer,
    GfVec4i const &compRegion)
{
    if (!ARCH_UNLIKELY(color)) {
        TF_CODING_ERROR("No valid color texture provided");
        return;
    }

    _ValidateGLContext();

    // XXX We need to flip all renderers (Embree, Prman, ...) for now as we
    // assume they all output gl coords. That may not always be the case if
    // Storm renders with Metal directly.
    constexpr bool flipImage = true;

    const int width = 
        color ? color->GetDescriptor().dimensions[0] :
        depth ? depth->GetDescriptor().dimensions[0] :
        256;

    const int height = 
        color ? color->GetDescriptor().dimensions[1] :
        depth ? depth->GetDescriptor().dimensions[1] :
        256;

    _SetAttachmentSize(width, height);

    id<MTLTexture> colorTexture = nil;
    id<MTLTexture> depthTexture = nil;
    if (color) {
        colorTexture = id<MTLTexture>(color->GetRawResource());
    }
    if (depth) {
        depthTexture = id<MTLTexture>(depth->GetRawResource());
    }

    id<MTLCommandBuffer> commandBuffer = _hgiMetal->GetPrimaryCommandBuffer();
    
    id<MTLComputeCommandEncoder> computeEncoder;
    
    if (_hgiMetal->GetCapabilities().concurrentDispatchSupported) {
        computeEncoder = [commandBuffer
         computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
    }
    else {
        computeEncoder = [commandBuffer computeCommandEncoder];
    }

    int glShaderIndex = -1;
    //
    // Depth
    //
    if (depthTexture) {
        NSUInteger exeWidth = [_computePipelineStateDepth threadExecutionWidth];
        NSUInteger maxThreadsPerThreadgroup =
            [_computePipelineStateDepth maxTotalThreadsPerThreadgroup];

        MTLSize threadgroupCount = MTLSizeMake(exeWidth,
            maxThreadsPerThreadgroup / exeWidth, 1);
        MTLSize threadsPerGrid   = MTLSizeMake(
            (_mtlAliasedDepthRegularFloatTexture.width +
                (threadgroupCount.width - 1)) / threadgroupCount.width,
            (_mtlAliasedDepthRegularFloatTexture.height +
                (threadgroupCount.height - 1)) / threadgroupCount.height,
            1);

        [computeEncoder setComputePipelineState:_computePipelineStateDepth];
        [computeEncoder setTexture:depthTexture atIndex:0];
        [computeEncoder setTexture:_mtlAliasedDepthRegularFloatTexture
                           atIndex:1];
        
        [computeEncoder dispatchThreadgroups:threadsPerGrid
                       threadsPerThreadgroup:threadgroupCount];
    }
    
    //
    // Color
    //
    if (colorTexture) {
        NSUInteger exeWidth = [_computePipelineStateColor threadExecutionWidth];
        NSUInteger maxThreadsPerThreadgroup =
            [_computePipelineStateColor maxTotalThreadsPerThreadgroup];

        MTLSize threadgroupCount = MTLSizeMake(exeWidth,
            maxThreadsPerThreadgroup / exeWidth, 1);
        MTLSize threadsPerGrid   = MTLSizeMake(
            (_mtlAliasedColorTexture.width +
                (threadgroupCount.width - 1)) / threadgroupCount.width,
            (_mtlAliasedColorTexture.height +
                (threadgroupCount.height - 1)) / threadgroupCount.height,
            1);

        [computeEncoder setComputePipelineState:_computePipelineStateColor];
        [computeEncoder setTexture:colorTexture atIndex:0];
        [computeEncoder setTexture:_mtlAliasedColorTexture atIndex:1];
        
        [computeEncoder dispatchThreadgroups:threadsPerGrid
                       threadsPerThreadgroup:threadgroupCount];
    }

    [computeEncoder endEncoding];
    
    if (depthTexture && colorTexture) {
        glShaderIndex = ShaderContextColorDepth;
    }
    else if (colorTexture) {
        glShaderIndex = ShaderContextColor;
    }

    // We wait until the work is scheduled for execution so that future OpenGL
    // calls are guaranteed to happen after the Metal work encoded above
    _hgiMetal->CommitPrimaryCommandBuffer(
        HgiMetal::CommitCommandBuffer_WaitUntilScheduled);

    if (glShaderIndex != -1) {
        _BlitToOpenGL(framebuffer, compRegion, flipImage, glShaderIndex);

        _ProcessGLErrors();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

