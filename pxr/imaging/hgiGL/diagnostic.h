//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_DIAGNOSTIC_H
#define PXR_IMAGING_HGI_GL_DIAGNOSTIC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/base/arch/functionLite.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// Posts diagnostic errors for all GL errors in the current context.
/// This macro tags the diagnostic errors with the name of the calling
/// function.
#define HGIGL_POST_PENDING_GL_ERRORS() \
        HgiGLPostPendingGLErrors(__ARCH_PRETTY_FUNCTION__)

/// Returns true if GL debug is enabled
HGIGL_API
bool HgiGLDebugEnabled();

/// Posts diagnostic errors for all GL errors in the current context.
HGIGL_API
void HgiGLPostPendingGLErrors(std::string const & where = std::string());

/// Setup OpenGL 4 debug facilities
HGIGL_API
void HgiGLSetupGL4Debug();

HGIGL_API
bool HgiGLMeetsMinimumRequirements();

/// Calls glObjectLabel making sure the label is not too long.
HGIGL_API
void HgiGLObjectLabel(uint32_t identifier,
                      uint32_t name,
                      const std::string &label);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
