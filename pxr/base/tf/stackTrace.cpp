//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
        "in %s at line %zu of %s",
        ArchGetProgramNameForErrors(), reason.c_str(), message.c_str(),
        context.GetFunction(), context.GetLine(), context.GetFile());

    if (!additionalInfo.empty()) {
        fullMessage += "\n" + additionalInfo;
    }

    Tf_ScopeDescriptionStackReportLock descStackReport;
    ArchLogFatalProcessState(
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
