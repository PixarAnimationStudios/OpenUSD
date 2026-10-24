//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/trace/aggregateTree.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"


PXR_NAMESPACE_USING_DIRECTIVE


void wrapAggregateTree(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = TraceAggregateTree;

    nb::class_<This> cls(m, "AggregateTree",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbRefAndWeakPtr(cls);
    cls.def(TfNbMakePyConstructor(&This::New))
        .def_prop_ro("root", &This::GetRoot)
    ;
}
