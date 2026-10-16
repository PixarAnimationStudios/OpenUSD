//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticBase.h"

PXR_NAMESPACE_USING_DIRECTIVE

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

void wrapDiagnosticBase(nanobind::module_& m)
{
    using This = TfDiagnosticBase;

    nanobind::class_<This>(m, "_DiagnosticBase")
        .def_prop_ro("sourceFileName", &This::GetSourceFileName,
                     "The source file name that the error was posted from.")

        .def_prop_ro("sourceLineNumber", &This::GetSourceLineNumber,
                     "The source line number that the error was posted from.")

        .def_prop_ro("commentary", &This::GetCommentary,
                     "The commentary string describing this error.")

        .def_prop_ro("sourceFunction", &This::GetSourceFunction,
                     "The source function that the error was posted from.")

        .def_prop_ro("diagnosticCode", &This::GetDiagnosticCode,
                     "The diagnostic code posted.")

        .def_prop_ro("diagnosticCodeString", &This::GetDiagnosticCodeAsString,
                     "The error code posted for this error, as a string.")
        ;
}
