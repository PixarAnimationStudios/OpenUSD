//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/ndr/property.h"
#include "pxr/usd/sdf/types.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapProperty()
{
    typedef NdrProperty This;
    typedef NdrPropertyPtr ThisPtr;

    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<This, ThisPtr, boost::noncopyable>("Property", no_init)
        .def("__repr__", &This::GetInfoString)
        .def("GetName", &This::GetName, copyRefPolicy)
        .def("GetType", &This::GetType, copyRefPolicy)
        .def("GetDefaultValue", &This::GetDefaultValue, copyRefPolicy)
        .def("IsOutput", &This::IsOutput)
        .def("IsArray", &This::IsArray)
        .def("IsDynamicArray", &This::IsDynamicArray)
        .def("GetArraySize", &This::GetArraySize)
        .def("GetInfoString", &This::GetInfoString)
        .def("GetMetadata", &This::GetMetadata,
            return_value_policy<TfPyMapToDictionary>())
        .def("IsConnectable", &This::IsConnectable)
        .def("CanConnectTo", &This::CanConnectTo)
        .def("GetTypeAsSdfType", &This::GetTypeAsSdfType,
            return_value_policy<TfPyPairToTuple>())
        ;
}
