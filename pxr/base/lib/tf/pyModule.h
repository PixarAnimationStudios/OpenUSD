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
#ifdef TF_PYMODULE_H
#error This file should only be included once in any given source (.cpp) file.
#endif
#define TF_PYMODULE_H

#include "pxr/pxr.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/tf/api.h"

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/python/module.hpp>

// Helper macros for module files.  If you implement your wrappers for classes
// as functions named wrapClassName(), then you can create your module like
// this:
//
// TF_WRAP_MODULE(ModuleName) {
//    TF_WRAP(ClassName1);
//    TF_WRAP(ClassName2);
//    TF_WRAP(ClassName3);
// }
//

// Forward declare the function that will be provided that does the wrapping.
static void WrapModule();

PXR_NAMESPACE_OPEN_SCOPE

TF_API
void Tf_PyInitWrapModule(void (*wrapModule)(),
                         const char* packageModule,
                         const char* packageName,
                         const char* packageTag,
                         const char* packageTag2);

ARCH_EXPORT
void BOOST_PP_CAT(init_module_, MFB_PACKAGE_NAME)() {

    Tf_PyInitWrapModule(
        WrapModule,
        BOOST_PP_STRINGIZE(MFB_PACKAGE_MODULE),
        BOOST_PP_STRINGIZE(MFB_ALT_PACKAGE_NAME),
        "Wrap " BOOST_PP_STRINGIZE(MFB_ALT_PACKAGE_NAME),
        BOOST_PP_STRINGIZE(MFB_PACKAGE_NAME)
        );
}

PXR_NAMESPACE_CLOSE_SCOPE

// In presto, when we generate boost python bindings for a library
// named Foo, we generate the library contents in libFoo.so, we
// generate a python package in a directory named Foo that has
// __init__.py and _Foo.so, and we put all the python bindings in
// _Foo.so.  The __init__.py file imports _Foo and then publishes
// _Foo's symbols as its own.  Since the module with the bindings is
// named _Foo, the init routine must be named init_Foo.  This little
// block produces that function.
//
extern "C"
ARCH_EXPORT
void BOOST_PP_CAT(init_, MFB_PACKAGE_NAME)() {
    PXR_NAMESPACE_USING_DIRECTIVE
    boost::python::detail::init_module
        (BOOST_PP_STRINGIZE(BOOST_PP_CAT(_,MFB_PACKAGE_NAME)),
         BOOST_PP_CAT(&init_module_, MFB_PACKAGE_NAME));
}

// Boost wrapped 2x code follows a different pattern.  Both the
// library contents and the python bindings go into libfoo.so (2x uses
// lower case library names).  We still generate a package named foo
// but the __init__.py file in the package foo imports libfoo and
// publishes it's symbols as its own.  Since the module with the
// bindings is named libfoo, the init routine must be named initlibfoo.
// This little block produces that function.
//
// So there are two init routines in every library, but only the one
// that matches the name of the python module will be called by python
// when the module is imported.  So the total cost is a 1-line
// function that doesn't get called.
//
extern "C"
ARCH_EXPORT
void BOOST_PP_CAT(initlib, MFB_PACKAGE_NAME)() {
    PXR_NAMESPACE_USING_DIRECTIVE
    boost::python::detail::init_module
        (BOOST_PP_STRINGIZE(BOOST_PP_CAT(lib,MFB_PACKAGE_NAME)),
         BOOST_PP_CAT(&init_module_, MFB_PACKAGE_NAME));
}

#define TF_WRAP_MODULE static void WrapModule()

// Declares and calls the class wrapper for x
#define TF_WRAP(x) void wrap ## x (); wrap ## x ()
