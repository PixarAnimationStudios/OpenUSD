//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyObjWrapper.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static TfPyObjWrapper
_RoundTripWrapperTest(TfPyObjWrapper const &wrapper)
{
    return wrapper;
}

static TfPyObjWrapper
_RoundTripWrapperCallTest(TfPyObjWrapper const &wrapper)
{
    return wrapper();
}

static TfPyObjWrapper
_RoundTripWrapperIndexTest(TfPyObjWrapper const &wrapper, int index)
{
    return nanobind::object(wrapper[index]);
}

} // anonymous namespace 

void wrapPyObjWrapper(nanobind::module_ &m)
{
    m.def("_RoundTripWrapperTest", _RoundTripWrapperTest);
    m.def("_RoundTripWrapperCallTest", _RoundTripWrapperCallTest);
    m.def("_RoundTripWrapperIndexTest", _RoundTripWrapperIndexTest);
}
