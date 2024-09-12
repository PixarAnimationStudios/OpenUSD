//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"

#include "pxr/external/boost/python/def.hpp"
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapTf_TestPyAnnotatedBoolResult()
{
    def("_TestAnnotatedBoolResult", &_TestAnnotatedBoolResult);

    Tf_TestAnnotatedBoolResult::Wrap<Tf_TestAnnotatedBoolResult>(
        "Tf_TestAnnotatedBoolResult", "annotation");
}
