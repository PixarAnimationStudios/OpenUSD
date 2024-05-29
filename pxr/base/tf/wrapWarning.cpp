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
