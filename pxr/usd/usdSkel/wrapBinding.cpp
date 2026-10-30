//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/binding.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"

#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

VtArray<UsdSkelSkinningQuery>
_ConvertSkinningQueries(const nb::sequence& skinningQueries)
{
    const size_t numQueries = nb::len(skinningQueries);
    VtArray<UsdSkelSkinningQuery> skinningQueriesArray(numQueries);
    for (size_t i = 0; i < numQueries; ++i) {
        skinningQueriesArray[i] =
            nb::cast<const UsdSkelSkinningQuery&>(skinningQueries[i]);
    }
    return skinningQueriesArray;
}

} // namespace

void wrapUsdSkelBinding(nb::module_ &m)
{
    using This = UsdSkelBinding;

    nb::class_<This>(m, "Binding")
        .def(nb::init<>())

        .def("__init__", [](This* self, const UsdSkelSkeleton& skel,
                            const nb::sequence& skinningQueries) {
            new (self) This(skel, _ConvertSkinningQueries(skinningQueries));
        })

        .def("GetSkeleton", &This::GetSkeleton, nb::rv_policy::copy)

        .def("GetSkinningTargets", [](const This& self) {
            return TfNbSequenceToList(self.GetSkinningTargets());
        })
        ;

}
