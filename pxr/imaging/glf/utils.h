//
// Copyright 2016 Pixar
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
#ifndef PXR_IMAGING_GLF_UTILS_H
#define PXR_IMAGING_GLF_UTILS_H

/// \file glf/utils.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/garch/gl.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Base image format
///
/// Returns the base image format for the given number of components
///
/// Supported number of components: 1, 2, 3, 4
GLenum GlfGetBaseFormat(int numComponents);

/// Number of elements.
///
/// Returns the number of elements (channels) in a given GL enum format.
///
/// Supported formats are : GL_DEPTH_COMPONENT, GL_COLOR_INDEX, GL_ALPHA, 
/// GL_RED, GL_LUMINANCE, GL_RG, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA
GLF_API 
int GlfGetNumElements(GLenum format);

/// Byte size of a GL type.
///
/// Returns the size in bytes of a given GL type.
///
/// Supported types are : GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, 
/// GL_SHORT, GL_FLOAT, GL_DOUBLE
GLF_API
int GlfGetElementSize(GLenum type);

/// Checks the valitidy of a GL framebuffer
///
/// True if the currently bound GL framebuffer is valid and can be bound
/// or returns the cause of the problem
GLF_API
bool GlfCheckGLFrameBufferStatus(GLuint target, std::string * reason);

/// Check if the format is compressed.
///
/// Supported GL compressed formats : GL_COMPRESSED_RGBA_BPTC_UNORM, 
/// GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
GLF_API
bool GlfIsCompressedFormat(GLenum format);

/// Calculate the byte size of compressed textures.
///
/// Supported GL compressed formats : GL_COMPRESSED_RGBA_BPTC_UNORM, 
/// GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
GLF_API
size_t GlfGetCompressedTextureSize(int width, int height, 
                                   GLenum format, GLenum type);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
