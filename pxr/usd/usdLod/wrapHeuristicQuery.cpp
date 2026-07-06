//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/heuristicQuery.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapHeuristicQuery()
{
    typedef UsdLodHeuristicQuery This;

    class_<This>("HeuristicQuery", no_init)

        .add_property(
            "lodDomain",
            +[](const This& self) { return self.lodDomain.GetString(); },
            +[](This& self,
                const std::string& value) { self.lodDomain = TfToken(value); })

        ;
}
