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
#ifndef TF_ENVSETTING_H
#define TF_ENVSETTING_H

/// \file tf/envSetting.h
/// Environment setting variable.
///
/// A \c TfEnvSetting<T> is used to access an environment variable that
/// controls program execution according to the value set in the environment.
/// Currently, the legal types for T are bool, int, and string.
///
/// The TfEnvSetting facility is used to enable new features in the code that
/// are still in "experimental" mode, and warn the user and/or QA that they
/// are pushing the edge of the envelope by setting a non-standard value for
/// these variables.  Accordingly, the \c TfEnvSetting construct should be
/// used as sparingly as possible in code.
///
/// In contrast, a variable that allows the user to customize program
/// execution but is not an in-development code path should simply use
/// TfGetenv() to access the variable.  An example would be supplying a
/// variable to override a default font or fontsize, for users who don't like
/// the default program choice (and when there is no other way to set the
/// preference).
///
/// Here is how to use the TfEnvSetting facility.
///
/// 1. First, define your variable in a single .cpp file:
///
/// \code
/// #include "pxr/base/tf/envSetting.h"
///
/// TF_DEFINE_ENV_SETTING(TDS_FILE_VERSION, 12,
///                       "Default file format to use");
/// \endcode
///
/// The first argument is the name of your variable; it is also the name for
/// the variable you can set in your shell to set the value at runtime.  The
/// second argument is the default value.  To create a bool variable, pass
/// either true or false.  To create a string variable, pass an explicit
/// string(), i.e.
///
/// \code
/// TF_DEFINE_ENV_SETTING(TDS_FILE_SUFFIX, string(".tid"),
///                       "Default file-name suffix");
/// \endcode
///
/// 2.  If you need to access this variable outside the .cpp file that defines
/// the variable, put the following in a common header file:
///
/// \code
/// extern TfEnvSetting<int> TDS_FILE_VERSION;
/// extern TfEnvSetting<string> TDS_FILE_SUFFIX;
/// \endcode
///
/// 3. At runtime, access your variable using TfGetEnvSetting().  For example:
///
/// \code
/// int version = TfGetEnvSetting(TDS_FILE_VERSION);
/// string const& suffix =TfGetEnvSetting(TDS_FILE_SUFFIX);
/// \endcode
///
/// You can also access a variable's value from Python:
///
/// \code{.py}
/// from import import Tf
/// suffix = Tf.GetEnvSetting("TDS_FILE_SUFFIX")
/// \endcode
///
/// \c Tf.GetEnvSetting() returns the value for the TfEnvSetting variable, or
/// None if no such variable is defined in the currently loaded C++ code.
///
/// If a user's environment has a value for a TfEnvSetting variable that
/// differs from the default, when the program starts or the module defining
/// the TfEnvSetting variable is loaded, a warning messages is printed.
///
/// From Python you can also access all currently defined TfEnvSetting
/// variables:
///
/// \code{.py}
/// settingsDict = Tf.GetEnvSettingsDictionary()
/// \endcode
///
/// The keys of the returned dictionary are the names of the environment
/// variables; the values are tuples (defaultValue, description) corresponding
/// to the second and third arguments in the TF_DEFINE_ENV_SETTING() macro.
///
/// Additionally, at program startup time (or when lib/tf is first loaded),
/// the environment variable PIXAR_TF_ENV_SETTING_FILE is examined.  If this
/// variable indicates a file that can be read, then the file is parsed, and
/// should contain lines of the form key=value.  For each line read, the
/// environment variable key is set to value.  For example:
///
/// \code{.sh}
/// $ setenv PIXAR_TF_ENV_SETTING_FILE /usr/anim/<UNIT>/admin/env-settings
///
/// $ cat /usr/anim/<UNIT>/admin/env-settings
/// TDS_DEF_VERSION=30
/// TDS_BLAH=
/// TDS_LONG_STRING=i am some long string with spaces
/// \endcode
///
/// Blank lines in the file and lines where the first character is '#' are
/// ignored.  If the file itself cannot be read, no error is printed; however,
/// if the file is malformed, errors are printed to stderr.

#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/registryManager.h"

#include <atomic>
#include <string>

// POD, statically initialized.
//
// We store the atomic_value separately and refer to it via pointer because we
// cannot use aggregate-initialization on a struct holding an atomic, but we
// can value-initialize a single std::atomic.
template <class T>
struct TfEnvSetting
{
    std::atomic<T*> *_value;
    T const _default;
    char const * const _name;
    char const * const _description;
};

// Specialize for string, default is stored as char const * (pointing to a
// literal).
template <>
struct TfEnvSetting<std::string>
{
    std::atomic<std::string*> *_value;
    char const * const _default;
    char const * const _name;
    char const * const _description;
};

template <class T>
void Tf_InitializeEnvSetting(TfEnvSetting<T> *);

/// Returns the value of the specified env setting, registered using
/// \c TF_DEFINE_ENV_SETTING.
template <class T>
inline T const &
TfGetEnvSetting(TfEnvSetting<T>& setting) {
    extern void Tf_InitEnvSettings();
    Tf_InitEnvSettings();

    T *val = setting._value->load();
    if (ARCH_UNLIKELY(not val)) {
        Tf_InitializeEnvSetting(&setting);
        val = setting._value->load();
    }
    return *val;
}

// Ensure that we only allow bool, int, and string, and map char * and char
// array to string.

bool Tf_ChooseEnvSettingType(bool);
int Tf_ChooseEnvSettingType(int);
std::string Tf_ChooseEnvSettingType(char const *);

/// Define an env setting named \p envVar with default value \p defValue and a
/// descriptive string \p description.
/// \hideinitializer
#define TF_DEFINE_ENV_SETTING(envVar, defValue, description)                  \
    std::atomic< decltype(Tf_ChooseEnvSettingType(defValue))*>                \
        envVar##_value = {nullptr};                                           \
    TfEnvSetting<decltype(Tf_ChooseEnvSettingType(defValue))> envVar = {      \
        &envVar##_value, defValue, #envVar, description };                    \
    TF_REGISTRY_FUNCTION_WITH_TAG(Tf_EnvSettingRegistry, envVar) {            \
        (void)TfGetEnvSetting(envVar);                                        \
    }

#endif // TF_ENVSETTING_H
