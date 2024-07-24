//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dynamicFileFormatDependencyData.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/vt/value.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void 
wrapDynamicFileFormatDependencyData()
{
    using This = PcpDynamicFileFormatDependencyData;
    class_<This>("DynamicFileFormatDependencyData", no_init)     
        .def("GetRelevantFieldNames",
             make_function(&This::GetRelevantFieldNames,
                           return_value_policy< TfPySequenceToList >()))
        .def("GetRelevantAttributeNames",
             make_function(&This::GetRelevantAttributeNames,
                           return_value_policy< TfPySequenceToList >()))
        .def("CanFieldChangeAffectFileFormatArguments", 
             &This::CanFieldChangeAffectFileFormatArguments)
        .def("CanAttributeDefaultValueChangeAffectFileFormatArguments", 
             &This::CanAttributeDefaultValueChangeAffectFileFormatArguments)
        .def("IsEmpty", &This::IsEmpty)
        ;
}
