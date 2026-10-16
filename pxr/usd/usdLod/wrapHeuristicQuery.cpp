//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/heuristicQuery.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapHeuristicQuery(nb::module_ &m)
{
    typedef UsdLodHeuristicQuery This;

    nb::class_<This>(m, "HeuristicQuery")
        .def_prop_rw(
            "lodDomain",
            +[](const This& self) { return self.lodDomain.GetString(); },
            +[](This& self,
                const std::string& value) { self.lodDomain = TfToken(value); })
        ;
}
