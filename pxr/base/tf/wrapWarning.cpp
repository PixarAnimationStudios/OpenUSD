//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/pyCallContext.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/stringUtils.h"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void
_Warn(string const &msg, string const& moduleName, string const& functionName,
      string const& fileName, int lineNo)
{
    TfDiagnosticMgr::
        WarningHelper(Tf_PythonCallContext(fileName, moduleName,
                                           functionName, lineNo),
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

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

void wrapWarning(nanobind::module_& m) {
    m.def("_Warn", &_Warn);

    typedef TfWarning This;

    nanobind::class_<This, TfDiagnosticBase>(m, "Warning")
        .def("__repr__", TfWarning__repr__)
        ;
}
