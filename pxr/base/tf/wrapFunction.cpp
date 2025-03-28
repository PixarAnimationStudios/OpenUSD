//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include "pxr/external/boost/python/object.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

void wrapFunction() {
    TfPyFunctionFromPython<void ()>();
    TfPyFunctionFromPython<bool ()>();
    TfPyFunctionFromPython<int ()>();
    TfPyFunctionFromPython<long ()>();
    TfPyFunctionFromPython<double ()>();
    TfPyFunctionFromPython<std::string ()>();
    TfPyFunctionFromPython<pxr_boost::python::object ()>();
    TfPyFunctionFromPython<TfPyObjWrapper ()>();
}
