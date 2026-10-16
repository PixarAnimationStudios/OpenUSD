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


PXR_NAMESPACE_USING_DIRECTIVE


#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

namespace {

static void
_PrintStackTrace(nanobind::handle obj, const std::string &reason)
{
    int fd = PyObject_AsFileDescriptor(obj.ptr());
    if (fd >= 0)
    {
        FILE * file = ArchFdOpen(fd, "w");
        if (!file) {
            PyErr_SetFromErrno(PyExc_OSError);
            throw nanobind::python_error();
        }
        TfPrintStackTrace(file, reason);
        fclose(file);
    }
    else {
        // Wrong type for obj
        TfPyThrowTypeError("Expected file object.");
    }
}

} // anonymous namespace 

void
wrapStackTrace(nanobind::module_& m)
{
    namespace nb = nanobind;
    m.def("GetStackTrace", TfGetStackTrace,
        "GetStackTrace()\n\n"
        "Return both the C++ and the python stack as a string.");
    
    m.def("PrintStackTrace", _PrintStackTrace,
        "PrintStackTrace(file, str)\n\n"
        "Prints both the C++ and the python stack to the file provided.");

    m.def("LogStackTrace", TfLogStackTrace,
        nb::arg("reason"), nb::arg("logToDb")=false);

    m.def("GetAppLaunchTime", TfGetAppLaunchTime,
        "GetAppLaunchTime() -> int \n\n"
        "Return the time (in seconds since the epoch) at which "
        "the application was started.");
}
