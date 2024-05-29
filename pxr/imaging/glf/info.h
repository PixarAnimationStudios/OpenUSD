//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_INFO_H
#define PXR_IMAGING_GLF_INFO_H

/// \file glf/info.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// Tests for GL extension support.
///
/// Returns \c true if each extension name listed in \a extensions
/// is supported by the current GL context.
GLF_API
bool GlfHasExtensions(std::string const & extensions);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
