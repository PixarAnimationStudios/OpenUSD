//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticBase.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapDiagnosticBase()
{
    using This = TfDiagnosticBase;

    class_<This>("_DiagnosticBase", no_init)
        .add_property("sourceFileName",
                      make_function(&This::GetSourceFileName,
                                    return_value_policy<return_by_value>()),
                      "The source file name that the error was posted from.")

        .add_property("sourceLineNumber", &This::GetSourceLineNumber,
                      "The source line number that the error was posted from.")

        .add_property("commentary",
                      make_function(&This::GetCommentary,
                                    return_value_policy<return_by_value>()),
                      "The commentary string describing this error.")

        .add_property("sourceFunction",
                      make_function(&This::GetSourceFunction,
                                    return_value_policy<return_by_value>()),
                      "The source function that the error was posted from.")

        .add_property("diagnosticCode", &This::GetDiagnosticCode,
                      "The diagnostic code posted.")

        .add_property("diagnosticCodeString",
                      make_function(&This::GetDiagnosticCodeAsString,
                                    return_value_policy<return_by_value>()),
                      "The error code posted for this error, as a string.")
        ;

}
