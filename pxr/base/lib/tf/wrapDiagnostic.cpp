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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyEnum.h"

#include <boost/python/def.hpp>

using namespace std;
using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

// The menv2x python code invokes these methods.  The presto replacements
// tend to raise exceptions which is not quite the same behavior.  So we
// wrap these macros in python callable functions.
//
void wrapped_TF_DIAGNOSTIC_NONFATAL_ERROR(const string& msg)
{
    TF_DIAGNOSTIC_NONFATAL_ERROR(msg);
}

void wrapped_TF_DIAGNOSTIC_WARNING(const string& msg)
{
    TF_DIAGNOSTIC_WARNING(msg);
}

void wrapDiagnostic()
{
    TfPyWrapEnum<TfDiagnosticType>();

    def("TF_DIAGNOSTIC_NONFATAL_ERROR", &wrapped_TF_DIAGNOSTIC_NONFATAL_ERROR);
    def("TF_DIAGNOSTIC_WARNING", &wrapped_TF_DIAGNOSTIC_WARNING);

    def("TfInstallTerminateAndCrashHandlers",
        TfInstallTerminateAndCrashHandlers);

}

PXR_NAMESPACE_CLOSE_SCOPE
