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

#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/pyCallContext.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/scope.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void
_Warn(string const &msg, string const& moduleName, string const& functionName,
      string const& fileName, int lineNo)
{
    TfDiagnosticMgr::
        WarningHelper(Tf_PythonCallContext(fileName.c_str(), moduleName.c_str(),
                                           functionName.c_str(), lineNo),
                      TF_DIAGNOSTIC_WARNING_TYPE,
                      TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_WARNING_TYPE)).
                      c_str()).
        Post(msg);
}

static string
TfWarning__repr__(TfWarning const &self)
{
    string ret = TfStringPrintf("Warning in '%s' at line %zu in file %s : '%s'",
             self.GetSourceFunction().c_str(),
             self.GetSourceLineNumber(),
             self.GetSourceFileName().c_str(),
             self.GetCommentary().c_str());

    return ret;
}

} // anonymous namespace 

void wrapWarning() {
    def("_Warn", &_Warn);

    typedef TfWarning This;

    scope warningScope =
        class_<This, bases<TfDiagnosticBase> >("Warning", no_init)

        .def("__repr__", TfWarning__repr__)
        ;
}
