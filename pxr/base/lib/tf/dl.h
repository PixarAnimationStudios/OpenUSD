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
#ifndef TF_DL_H
#define TF_DL_H

/// \file tf/dl.h
/// \ingroup group_tf_SystemsExt
/// Interface for opening code libraries.
#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/api.h"

#include <string>

/// \addtogroup group_tf_SystemsExt
///@{

/// Call \c dlopen() and notify \c lib/tf that a new module has been loaded.
///
/// This is a wrapper around dlopen(), in the sense that this function simply
/// calls \c dlopen(\p name, \p flag).  It will additionally load script
/// bindings if scripting is initialized and loading is requested.
/// 
/// If \p error is not \c NULL, it will be set to the return value of a
/// \c dlerror() call after the \c dlopen(), or cleared if that value is
/// \c NULL. It is not reliable to get the error string by calling
/// \c dlerror() after \c TfDlopen().
///
/// If you set the environment variable \c TF_DLOPEN_DEBUG then debug output
/// will be sent to \c stdout on each invocation of this function.
void* TfDlopen(const std::string &filename,
               int flag, 
               std::string *error = NULL,
               bool loadScriptBindings = true);


/*!
 * \brief Call ArchOpenLibrary() and notify lib/tf that a new module has been loaded.
 * \ingroup group_tf_SystemsExt
 *
 * This is a wrapper around ArchOpenLibrary(), in the sense that this function
 * simply calls ArchOpenLibrary(\p name, \p flag).  It will additionally load
 * script bindings if scripting is initialized and loading is requested.
 * 
 * If \p error is not NULL, it will be set to the return value of
 * a ArchLibraryError() call after the dlopen(), or cleared if that value is NULL.
 * It is not reliable to get the error string by calling ArchLibraryError()
 * after \c TfDlopen().
 *
 * If you set the environment variable TF_DLOPEN_DEBUG then debug output
 * will be sent to stdout on each invocation of this function.
 */
TF_API
void* TfDlopen(const std::string &filename, int flag, 
               std::string *error = NULL, bool loadScriptBindings = true);

/*!
 * \brief Call dlclose().
 * \ingroup group_tf_SystemsExt
 */
TF_API
int TfDlclose(void* handle);

/// \private
bool Tf_DlOpenIsActive();
/// \private
bool Tf_DlCloseIsActive();

///@}

#endif
