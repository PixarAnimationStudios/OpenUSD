//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_UTILS_H
#define PXR_IMAGING_GLF_UTILS_H

/// \file glf/utils.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/hio/types.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

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

/// HioFormat
///
/// Returns the HioFormat for the given GL format and GL type
///
/// Supported formats are : GL_DEPTH_COMPONENT, GL_COLOR_INDEX, GL_ALPHA, 
/// GL_RED, GL_LUMINANCE, GL_RG, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA
///
/// Supported types are : GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, 
/// GL_SHORT, GL_FLOAT, GL_DOUBLE
GLF_API
HioFormat GlfGetHioFormat(GLenum glFormat, GLenum glType, bool isSRGB);


/// Checks the valitidy of a GL framebuffer
///
/// True if the currently bound GL framebuffer is valid and can be bound
/// or returns the cause of the problem
GLF_API
bool GlfCheckGLFrameBufferStatus(GLuint target, std::string * reason);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
