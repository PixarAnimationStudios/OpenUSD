//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_STACK_TRACE_H
#define PXR_BASE_TF_STACK_TRACE_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <cstdio>
#include <ctime>
#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
