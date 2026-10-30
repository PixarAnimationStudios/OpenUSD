//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/usd/sdr/discoveryPlugin.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static void wrapDiscoveryPluginContext(nanobind::module_ &m)
{
    typedef SdrDiscoveryPluginContext This;

    nb::class_<This> cls(m, "DiscoveryPluginContext",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbWeakPtr(cls);
    cls.def("GetShadingSystem", &This::GetShadingSystem)
        .def("GetSourceType", &This::GetSourceType)
        ;
}

void wrapDiscoveryPlugin(nanobind::module_ &m)
{
    typedef SdrDiscoveryPlugin This;

    nb::class_<This> cls(m, "DiscoveryPlugin",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbWeakPtr(cls);
    cls.def("DiscoverShaderNodes", &This::DiscoverShaderNodes)
        .def("GetSearchURIs", &This::GetSearchURIs)
        ;

    wrapDiscoveryPluginContext(m);
}
