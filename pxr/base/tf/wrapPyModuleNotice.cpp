//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyModuleNotice.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

TF_INSTANTIATE_NOTICE_WRAPPER(TfPyModuleWasLoaded, TfNotice);

} // anonymous namespace 

void wrapPyModuleNotice() {

    TfPyNoticeWrapper<TfPyModuleWasLoaded, TfNotice>::Wrap("PyModuleWasLoaded")
        .def("name", make_function(&TfPyModuleWasLoaded::GetName,
                                   return_value_policy<return_by_value>()))
        ;
}
