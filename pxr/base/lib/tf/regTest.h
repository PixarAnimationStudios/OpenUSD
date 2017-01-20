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
#ifndef TF_REGTEST_H
#define TF_REGTEST_H

/// \file tf/regTest.h
/// \ingroup group_tf_Internal
/// Support for simple regression tests.

#include "pxr/pxr.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfRegTest
/// \ingroup group_tf_Internal
///
/// \c TfRegTest is a singleton class, which is used to register functions
/// with either type \c bool \c (*)(int, char*[]), or functions returning type
/// \c bool and taking no arguments.
///
/// Here is how \c TfRegTest is used to create tests in separate files, which
/// are then compiled into a single standalone executable (and not part of any
/// library):
///
/// // file: main.cpp
/// \include test/main.cpp
///
/// // file: hammer.cpp
/// \code
/// #include "pxr/base/tf/regTest.h"
///
/// static bool
/// Test_PtHammer()
/// {
///     bool success;
///     ...
///     return success;
/// }
/// TF_ADD_REGTEST(PtHammer);
/// \endcode
///
/// \code
/// // file: drill.cpp
/// static bool
/// Test_PtDrill(int argc, char *argv[])
/// {
///     bool success;
///     ...
///     return success;
/// }
/// TF_ADD_REGTEST(PtDrill);
/// \endcode
///
/// When \c main.cpp, \c drill.cpp and \c hammer.cpp are compiled into
/// an executable, then a test can be run by invoking the executable
/// with a first argument of either \p PtHammer or \p PtDrill.
/// Since \p PtHammer is a function without arguments, supplying additional
/// arguments is an error; but \p PtDrill takes arguments, so additional
/// command-line arguments specified are passed to the function.
/// (Most library test functions shouldn't need any arguments.)
///
class TfRegTest {
public:
    /// Run a single regression test function, returning 0 if the function
    /// succeeded and 1 otherwise.
    ///
    /// This function is intended to be called as follows:
    /// \code
    ///     int main(int argc, char *argv[]) {
    ///         return TfRegTest::Main(argc, argv);
    ///     }
    /// \endcode
    ///
    /// The first argument is the name of the test to be run.  If the
    /// registered test function run takes no arguments, then no arguments
    /// other than the test name should be supplied.  Otherwise, the \c Main()
    /// passes \c argc-1 and \c argv+1 to the test function, and the test
    /// function is responsible for argument checking.
    static int Main(int argc, char *argv[]) {
        return GetInstance()._Main(argc, argv);
    }

    static TfRegTest& GetInstance();

    /// Type of a function with no arguments.
    typedef bool (*RegFunc)();

    /// Type of a function with arguments.
    ///
    /// When \c Main(argc,argv) is requested to run a function of type
    /// \c RegFuncWithArgs, it invokes the function with arguments \c argc-1
    /// and \c argv+1.
    typedef bool (*RegFuncWithArgs)(int argc, char *argv[]);

    bool Register(const char* name, RegFunc);
    bool Register(const char* name, RegFuncWithArgs);

private:
    int _Main(int argc, char *argv[]);

    void _PrintTestNames();

    typedef TfHashMap<std::string, RegFunc, TfHash> _Hash;
    typedef TfHashMap<std::string, RegFuncWithArgs, TfHash> _HashWithArgs;
    _Hash _functionTable;
    _HashWithArgs _functionTableWithArgs;
};

/// Adds the function Test_\p name, under name \p name, as a runnable
/// regression test. Test_\p name must be of type \c RegFunc or
/// \c RegFuncWithArgs.
///
/// \ingroup group_tf_Internal
/// \hideinitializer
#define TF_ADD_REGTEST(name)    \
    bool Tf_RegTst##name = TfRegTest::GetInstance().Register(#name, Test_##name)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
