//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_Museum.h"

#include "pxr/base/tf/pyEnum.h"

#include <boost/python.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

using This = TsTest_Museum;


void wrapTsTest_Museum()
{
    // First the class object, so we can create a scope for it...
    class_<This> classObj("TsTest_Museum", no_init);
    scope classScope(classObj);

    // ...then the nested type wrappings, which require the scope...
    TfPyWrapEnum<This::DataId>();

    // ...then the defs, which must occur after the nested type wrappings.
    classObj

        .def("GetData", &This::GetData)

        ;
}
