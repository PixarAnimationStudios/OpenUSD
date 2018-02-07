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

#include "pxr/pxr.h"
#include "pxr/base/tf/stackTrace.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/vsnprintf.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/scopeDescriptionPrivate.h"
#include "pxr/base/tf/stringUtils.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyUtils.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <cstdio>
#include <iostream>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

void
TfPrintStackTrace(FILE *file, const string &reason)
{
    std::ostringstream oss;

    TfPrintStackTrace(oss, reason);

    if (file == NULL)
        file = stderr;

    fprintf(file, "%s", oss.str().c_str());
    fflush(file);
}

void
TfPrintStackTrace(std::ostream &out, const string &reason)
{
    ArchPrintStackTrace(out, reason);
#ifdef PXR_PYTHON_SUPPORT_ENABLED 
    vector<string> trace = TfPyGetTraceback();
    TF_REVERSE_FOR_ALL(line, trace)
        out << *line;
    out << "=============================================================\n";
#endif // PXR_PYTHON_SUPPORT_ENABLED
}

string
TfGetStackTrace()
{
    std::ostringstream oss;
    TfPrintStackTrace(oss, string());
    return oss.str();
}

// Helper function for TfLogStackTrace and TfLogCrash. This creates a
// temporary file and returns the file descriptor. It also optionally
// returns the file name.
static int
_MakeStackFile(std::string *fileName)
{
    string tmpFile;
    int fd = ArchMakeTmpFile(ArchStringPrintf("st_%s",
            ArchGetProgramNameForErrors()), &tmpFile);

    if (fileName) {
        *fileName = tmpFile;
    }
    return fd;
}

void
TfLogStackTrace(const std::string &reason, bool logtodb)
{
    string tmpFile;
    int fd = _MakeStackFile(&tmpFile);

    if (fd != -1) {
        FILE* fout = ArchFdOpen(fd, "w");
        fprintf(stderr, "Writing stack for %s to %s because of %s.\n",
            ArchGetProgramNameForErrors(),
            tmpFile.c_str(), reason.c_str());
        TfPrintStackTrace(fout, reason);
        fclose(fout);

        // Attempt to add it to the db
        if (logtodb && ArchGetFatalStackLogging()) {
            ArchLogSessionInfo(tmpFile.c_str());
        }
    }
    else {
        // we couldn't open the tmp file, so write the stack trace to stderr
        fprintf(stderr, "Error writing to stack trace file. "
            "Printing stack to stderr\n");
        TfPrintStackTrace(stderr, reason);
    }
}

void
TfLogCrash(
    const std::string &reason,
    const std::string &message,
    const std::string &additionalInfo,
    TfCallContext const &context,
    bool logtodb)
{
    // Create a nicely formatted message describing the crash
    std::string fullMessage = TfStringPrintf(
        "%s crashed. %s: %s\n"
        "in %s at line %zu of %s\n",
        ArchGetProgramNameForErrors(), reason.c_str(), message.c_str(),
        context.GetFunction(), context.GetLine(), context.GetFile());

    if (!additionalInfo.empty()) {
        fullMessage += additionalInfo + "\n";
    }

    Tf_ScopeDescriptionStackReportLock descStackReport;
    ArchLogPostMortem(
        nullptr, fullMessage.c_str(), descStackReport.GetMessage());
}

time_t
TfGetAppLaunchTime()
{
    time_t launchTime = ArchGetAppLaunchTime();
    if (launchTime == 0)
        TF_RUNTIME_ERROR("Could not determine application launch time.");
    return launchTime;
}

PXR_NAMESPACE_CLOSE_SCOPE
