//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/base/tf/pySingleton.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

void wrapRegistry(nanobind::module_ &m)
{
    typedef KindRegistry This;

    nanobind::class_<This> cls(
        m, "Registry", nanobind::is_weak_referenceable(),
        nanobind::dynamic_attr());
    TfNbSingleton(cls);
    cls
        .def_static("HasKind", &This::HasKind)
        .def_static("GetAllKinds", &This::GetAllKinds)
        .def_static("GetBaseKind", &This::GetBaseKind)
        .def_static("IsA", &This::IsA)
        .def_static("IsModel", &This::IsModel)
        .def_static("IsGroup", &This::IsGroup)
        .def_static("IsAssembly", &This::IsAssembly)
        .def_static("IsComponent", &This::IsComponent)
        .def_static("IsSubComponent", &This::IsSubComponent)
        ;
}
