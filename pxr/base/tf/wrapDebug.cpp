//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/debug.h"

// XXX: This include is a hack to avoid build errors due to
// incompatible macro definitions in pyport.h on macOS.
#include <locale>


PXR_NAMESPACE_USING_DIRECTIVE


#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace {

static void
_SetOutputFile(nanobind::handle const &file)
{
    int filefd = PyObject_AsFileDescriptor(file.ptr());
    if (filefd == ArchFileNo(stdout)) {
        TfDebug::SetOutputFile(stdout);
    }
    else if (filefd == ArchFileNo(stderr)) {
        TfDebug::SetOutputFile(stderr);
    }
    else {
        // reports an error indicating correct usage, either stdout or stderr
        TfDebug::SetOutputFile(NULL);
    }
}

} // anonymous namespace 

void wrapDebug(nanobind::module_& m)
{
    typedef TfDebug This;
    namespace nb = nanobind;

    nb::class_<This>(m, "Debug")
        .def_static("SetDebugSymbolsByName", &This::SetDebugSymbolsByName,
             nb::arg("pattern"), nb::arg("value"))
        .def_static("SetDebugSymbolsByName",
             [](const std::string &pattern, int value) {
                 return This::SetDebugSymbolsByName(pattern, bool(value));
             },
             nb::arg("pattern"), nb::arg("value"))

        .def_static("IsDebugSymbolNameEnabled", &This::IsDebugSymbolNameEnabled)

        .def_static("GetDebugSymbolDescriptions", &This::GetDebugSymbolDescriptions)

        .def_static("GetDebugSymbolNames", &This::GetDebugSymbolNames)

        .def_static("GetDebugSymbolDescription", &This::GetDebugSymbolDescription)

        .def_static("SetOutputFile", _SetOutputFile)

        ;
}
