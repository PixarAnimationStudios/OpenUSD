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

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/scope.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void
_Status(string const &msg, string const& moduleName, string const& functionName,
        string const& fileName, int lineNo)
{
    TfDiagnosticMgr::
        StatusHelper(Tf_PythonCallContext(fileName.c_str(), moduleName.c_str(),
                                          functionName.c_str(), lineNo),
                     TF_DIAGNOSTIC_STATUS_TYPE,
                     TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_STATUS_TYPE)).
                     c_str()).
        Post(msg);
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

void wrapStatus() {
    def("_Status", &_Status);

    typedef TfStatus This;

    // Can't call this scope Status because Tf.Status() is a function def'd
    // above.
    scope statusScope =
        class_<This, bases<TfDiagnosticBase> >("StatusObject", no_init)

        .def("__repr__", TfStatus__repr__)
        ;
}
