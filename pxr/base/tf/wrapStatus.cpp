//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/status.h"
#include "pxr/base/tf/pyCallContext.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/stringUtils.h"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void
_Status(string const &msg, string const& moduleName, string const& functionName,
        string const& fileName, int lineNo)
{
    TfDiagnosticMgr::
        StatusHelper(Tf_PythonCallContext(fileName, moduleName,
                                          functionName, lineNo),
                     TF_DIAGNOSTIC_STATUS_TYPE,
                     TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_STATUS_TYPE)).
                     c_str()).
        Post(msg);
}

static void
_QuietStatus(string const& msg)
{
    TfDiagnosticMgr::StatusHelper(
        TfCallContext{},
        TF_DIAGNOSTIC_STATUS_TYPE,
        TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_STATUS_TYPE)).c_str()).Post(msg);
}

static string
TfStatus__repr__(TfStatus const &self)
{
    string ret = TfStringPrintf("Status in '%s' at line %zu in file %s : '%s'",
             self.GetSourceFunction().c_str(),
             self.GetSourceLineNumber(),
             self.GetSourceFileName().c_str(),
             self.GetCommentary().c_str());

    return ret;
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

void wrapStatus(nanobind::module_& m) {
    m.def("_Status", &_Status);
    m.def("_Status", &_QuietStatus);

    typedef TfStatus This;

    // Can't call this scope Status because Tf.Status() is a function def'd
    // above.
    nanobind::class_<This, TfDiagnosticBase>(m, "StatusObject")
        .def("__repr__", TfStatus__repr__)
        ;
}
