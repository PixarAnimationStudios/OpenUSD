//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/external/boost/python/class.hpp"

#include "pxr/pxr.h"
#include "pxr/usd/ar/defaultResolver.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapDefaultResolver()
{
    using This = ArDefaultResolver;

    class_<This, bases<ArResolver>, boost::noncopyable>
        ("DefaultResolver", no_init)

        .def("SetDefaultSearchPath", &This::SetDefaultSearchPath,
             args("searchPath"))
        .staticmethod("SetDefaultSearchPath")
        ;
}
