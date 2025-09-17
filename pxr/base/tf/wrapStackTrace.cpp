//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static void
_PrintStackTrace(object &obj, const std::string &reason)
{
    int fd = PyObject_AsFileDescriptor(obj.ptr());
    if (fd >= 0)
    {
        FILE * file = expect_non_null(ArchFdOpen(fd, "w"));
        if (file)
        {
            TfPrintStackTrace(file, reason);
            fclose(file);
        }
    }
    else {
        // Wrong type for obj
        TfPyThrowTypeError("Expected file object.");
    }
}

} // anonymous namespace 

void
wrapStackTrace()
{
    def("GetStackTrace", TfGetStackTrace,
        "GetStackTrace()\n\n"
        "Return both the C++ and the python stack as a string.");
    
    def("PrintStackTrace", _PrintStackTrace,
        "PrintStackTrace(file, str)\n\n"
        "Prints both the C++ and the python stack to the file provided.");

    def("LogStackTrace", TfLogStackTrace,
        (arg("reason"), arg("logToDb")=false));

    def("GetAppLaunchTime", TfGetAppLaunchTime,
        "GetAppLaunchTime() -> int \n\n"
        "Return the time (in seconds since the epoch) at which "
        "the application was started.");
}
