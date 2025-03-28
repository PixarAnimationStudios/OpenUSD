//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/binding.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"

#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"

#include "pxr/external/boost/python.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


namespace {


UsdSkelBinding*
_New(const UsdSkelSkeleton& skel, const pxr_boost::python::list& skinningQueries)
{
    const size_t numQueries = len(skinningQueries);
    VtArray<UsdSkelSkinningQuery> skinningQueriesArray(numQueries);
    for (size_t i = 0; i < numQueries; ++i) {
        skinningQueriesArray[i] =
            extract<const UsdSkelSkinningQuery&>(skinningQueries[i]);
    }
    return new UsdSkelBinding(skel, skinningQueriesArray);
}


} // namespace


void wrapUsdSkelBinding()
{
    using This = UsdSkelBinding;

    class_<This>("Binding", init<>())

        .def("__init__", make_constructor(&_New))

        .def("GetSkeleton", &This::GetSkeleton,
             return_value_policy<return_by_value>())

        .def("GetSkinningTargets", &This::GetSkinningTargets,
             return_value_policy<TfPySequenceToList>())
        ;
}
