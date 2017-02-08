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
/*
 * This header is not meant to be included in a .h file.
 * Complain if we see this header twice through.
 */

#ifdef TF_EXECUTEATUNLOAD_H
#error This file should only be included once in any given source (.cpp) file.
#endif

#define TF_EXECUTEATUNLOAD_H

/// \file tf/executeAtUnload.h
/// \ingroup group_tf_Initialization
/// Construct for executing arbitrary code on TfDlClose.

#include "pxr/pxr.h"
#include "pxr/base/tf/dl.h"

PXR_NAMESPACE_OPEN_SCOPE

// All this structure does is execute a function at destruction time,
// if destruction occurs in the call-chain of TfDlclose().
class Tf_DestructorObject {
public:
    Tf_DestructorObject(void (*functionPtr)())
        : _functionPtr(functionPtr) {
    }

    ~Tf_DestructorObject() {
        if (Tf_DlCloseIsActive())
            (*_functionPtr)();
    }

private:
    void (*_functionPtr)();
};

/// Construct for executing arbitrary code on \c TfDlClose.
///
/// It is sometimes important for dynamically loaded modules (i.e. plugins) to
/// execute "clean up" code upon unload.  This macro allows one to specify
/// code to be executed each time a dynamically loaded module is unloaded.  In
/// particular, if the module is reloaded and then again unloaded, the
/// clean-up code is again run.
///
/// Note that "unload time" differs from "exit time".  The clean-up code is
/// not run when the program exits, because modules are not unloaded on exit.
///
/// Finally, this facility assumes that the dynamically loaded module was
/// unloaded by a call to \c TfDlclose(), rather than directly by a call to \c
/// dlclose().
///
/// Here is an example of use:
///
/// \code
/// #include "pxr/base/tf/executeAtUnload.h"
/// #include "pxr/base/tf/instantiateSingleton.h"
///
/// class FileCache {
///     ...
/// };
/// TF_INSTANTIATE_SINGLETON(FileCache);
///
/// TF_EXECUTE_AT_UNLOAD()
/// {
///     TfSingleton<FileCache>::DeleteInstance();
/// }
/// \endcode
///
/// If a singleton intance of \c FileCache was ever created, then at module
/// unload time, it will be destroyed.
///
/// The \c TF_EXECUTE_AT_UNLOAD() macro can be used once per .cpp file.
/// Attempts to use this macro twice in the same .cpp file result in a compile
/// error.  Do \e NOT use this macro in a header file.
///
/// \c TF_EXECUTE_AT_UNLOAD() is meant to look like a function definition,
/// except that no return type is specified.  Any code that is legal to put in
/// a function can occur in the body (i.e. between the braces).
///
/// \ingroup group_tf_Initialization
/// \hideinitializer
#define TF_EXECUTE_AT_UNLOAD()                                          \
    static void Tf__LocalUnloadFunction();                              \
    static Tf_DestructorObject                                          \
        tf__localUnloadFunctionObject(Tf__LocalUnloadFunction);         \
    static void Tf__LocalUnloadFunction()

PXR_NAMESPACE_CLOSE_SCOPE
