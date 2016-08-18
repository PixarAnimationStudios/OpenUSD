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
#ifndef TF_STACKTRACE_H
#define TF_STACKTRACE_H

#include "pxr/base/tf/api.h"

#include <cstdio>
#include <iosfwd>
#include <string>

class TfCallContext;

/// Gets both the C++ and the python stack and returns it as a string.
TF_API std::string TfGetStackTrace();

/// Prints both the C++ and the python stack to the \c file provided
TF_API
void TfPrintStackTrace(FILE *file, const std::string &reason);

/// Prints both the C++ and the python stack to the \a stream provided
TF_API
void TfPrintStackTrace(std::ostream &out, std::string const &reason);

/// Logs both the C++ and the python stack to a file in /var/tmp
/// A message is printed to stderr reporting that a stack trace
/// has been taken and what file it has been written to. If \c 
/// logtodb is true, then the stack trace will be added to the 
/// stack_trace database table.
TF_API
void TfLogStackTrace(const std::string &reason, bool logtodb=false);

/// Creates a nicely formatted message describing a crash and writes it to a
/// temporary file.
///
/// \p reason is a very short descriptive title for the error (ie, FATAL_ERROR)
/// \p message further describes the crash (ie, Dereferenced an invalid MfHandle)
/// \p additionalInfo is secondary, possibly multi-line, information that should
///    be included in the report.
/// \p callContext describes the location of the crash
/// \p logToDB controls whether the stack will be added to the stack_trace db table.
TF_API
void TfLogCrash(const std::string &reason,
    const std::string &message, const std::string &additionalInfo,
    TfCallContext const &context, bool logToDB);

/// Returns the application's launch time.
TF_API
time_t TfGetAppLaunchTime();

#endif
