//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DL_H
#define PXR_BASE_TF_DL_H

/// \file tf/dl.h
/// \ingroup group_tf_SystemsExt
/// Interface for opening code libraries.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/arch/library.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \addtogroup group_tf_SystemsExt
///@{

/// Open a dynamic library and notify \c lib/tf that a new module has been
/// loaded.
///
/// This is a wrapper around ArchLibraryOpen() in the sense that this function
/// calls \c ArchLibraryOpen(\p filename, \p flag) but it will additionally
/// load script bindings if scripting is initialized and loading is requested.
/// 
/// If \p error is not \c NULL it will be set to a system reported error
/// if opening the library failed, otherwise it will be cleared.
///
/// If you set TF_DLOPEN in the TF_DEBUG environment variable then debug
/// output will be reported on each invocation of this function.
///
/// This returns an opaque handle to the opened library or \c NULL on
/// failure.
TF_API
void* TfDlopen(const std::string &filename,
               int flag, 
               std::string *error = NULL,
               bool loadScriptBindings = true);

/// Close a dynamic library.
TF_API
int TfDlclose(void* handle);

/// \private
TF_API
bool Tf_DlOpenIsActive();
/// \private
TF_API
bool Tf_DlCloseIsActive();

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
