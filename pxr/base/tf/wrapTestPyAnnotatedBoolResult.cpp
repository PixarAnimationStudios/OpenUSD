//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct Tf_TestAnnotatedBoolResult : TfPyAnnotatedBoolResult<std::string> {
    Tf_TestAnnotatedBoolResult(bool value, const std::string& annotation)
        : TfPyAnnotatedBoolResult<std::string>(value, annotation) { }
};

static Tf_TestAnnotatedBoolResult
_TestAnnotatedBoolResult(
    bool value,
    const std::string& annotation)
{
    return Tf_TestAnnotatedBoolResult(value, annotation);
}

} // anonymous namespace 

void wrapTf_TestPyAnnotatedBoolResult(nanobind::module_& m)
{
    m.def("_TestAnnotatedBoolResult", &_TestAnnotatedBoolResult);

    Tf_TestAnnotatedBoolResult::WrapNb<Tf_TestAnnotatedBoolResult>(
        m, "Tf_TestAnnotatedBoolResult", "annotation");
}
