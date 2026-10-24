//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/defaultResolver.h"

PXR_NAMESPACE_USING_DIRECTIVE

#include "pxr/base/tf/pyNanobindBridge.h"

void
wrapDefaultResolver(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = ArDefaultResolver;

    nb::class_<This, ArResolver>(m, "DefaultResolver")
        .def_static("SetDefaultSearchPath", &This::SetDefaultSearchPath,
                    nb::arg("searchPath"))
        ;
}
