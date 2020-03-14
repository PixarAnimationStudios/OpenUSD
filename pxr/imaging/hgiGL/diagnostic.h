//
// Copyright 2019 Pixar
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

/// Posts diagnostic errors for all GL errors in the current context.
HGIGL_API
void HgiGLPostPendingGLErrors(std::string const & where = std::string());

/// Setup OpenGL 4 debug facilities
HGIGL_API
void HgiGLSetupGL4Debug();

HGIGL_API
bool HgiGLMeetsMinimumRequirements();


PXR_NAMESPACE_CLOSE_SCOPE

#endif
