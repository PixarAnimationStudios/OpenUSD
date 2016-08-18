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

#ifdef TF_EXECUTEATSTARTUP_H
#error This file should only be included once in any given source (.cpp) file.
#endif

#define TF_EXECUTEATSTARTUP_H

/// \file tf/executeAtStartup.h
/// \ingroup group_tf_Initialization
/// Construct for executing arbitrary code at program start time.

/// Construct for executing arbitrary code at program start time.
///
/// It is often convenient to populate data registries by writing code
/// that automatically executes at program start time, without being
/// directly invoked. This is useful because it allows the mere linking
/// in of libraries to advertise their facilities.  To aid in this
/// pattern, the following macro allows code to be executed
/// automatically at program start time.
///
/// Here is an example of use:
///
/// \code
/// #include "pxr/base/tf/executeAtStartup.h"
///
/// class MyWidget {
///     ...
/// };
///
/// TF_EXECUTE_AT_STARTUP()
/// {
///     cout << "Debugging msg: executing MyWidget stuff...\n";
///     UicRegisterWidget<MyWidget>("some info", "more stuf");
/// }
///
/// MyWidget::MyWidget()
/// {
///    ...
/// }
/// \endcode
///
/// When the program begins, the code within the body of the \c
/// TF_EXECUTE_AT_STARTUP() construct will be executed.  For code that
/// is linked into the program, program start time is (hopefully!)
/// sometime before main.  Program start time for a dynamically loaded
/// DSO however is when the DSO is actually opened by the running
/// program.
///
/// The \c TF_EXECUTE_AT_STARTUP() macro can be used once per .cpp
/// file.  Attempts to use this macro twice in the same .cpp file
/// result in a compile error.  Do \e NOT use this macro in a header
/// file.
///
/// \c TF_EXECUTE_AT_STARTUP() is meant to look like a function
/// definition, except that no return type is specified.  Any code that
/// is legal to put in a function can occur in the body (i.e. between
/// the braces).
///
/// \note
/// This facility should only be used for anonymous
/// execution, i.e. for cases when, outside the given file, no one is
/// necessarily aware that the code exists.  If there is a way to directly
/// invoke the startup-code, and this facility is being used merely to
/// have it automatically happen before main(), you should not be using
/// this construct.  See \ref page_tf_Initialization for further discussion.
///
/// \deprecated This macro has been deprecated.  Almost all need for this
/// functionality has been subsumed by \c TfRegistryManager; most typically, if
/// you were using this function to call \c TF_ADD_ENUM_NAME(), see \c TfEnum
/// for the proper way to do this. If you think you still need to use this
/// function, please ask for some consultation first, and then use it only if
/// no other solution meets your needs.
///
/// \ingroup group_tf_Initialization
/// \hideinitializer
#define TF_EXECUTE_AT_STARTUP()                                         \
    static void Tf_ExecuteAtStartupFunction(int);                       \
    static int Tf_ExecuteAtStartupObject =                              \
        (Tf_ExecuteAtStartupFunction(Tf_ExecuteAtStartupObject), 0);    \
    static void Tf_ExecuteAtStartupFunction(int)

