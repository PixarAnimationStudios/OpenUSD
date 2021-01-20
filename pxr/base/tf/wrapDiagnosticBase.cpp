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
