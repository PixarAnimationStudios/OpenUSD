//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "query.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/noncopyable.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapQuery()
{
    using This = InvertibleRigsExample_Query;

    class_<This, noncopyable>("Query", no_init)
        .def(init<const UsdStageRefPtr &>())
        .def("FindSwitchAvars", &This::FindSwitchAvars)
    ;
}
