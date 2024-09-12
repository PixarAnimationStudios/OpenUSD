//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyFunction.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"

#include "pxr/usd/usdUtils/userProcessingFunc.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUserProcessingFunc()
{
    TfPyFunctionFromPython<UsdUtilsProcessingFunc>();
    
    typedef UsdUtilsDependencyInfo This;

    class_<This>("DependencyInfo", init<>())
        .def(init<const This&>())
        .def(init<const std::string &>())
        .def(init<const std::string &, const std::vector<std::string>>())
        .add_property("assetPath", make_function(&This::GetAssetPath,
            return_value_policy<return_by_value>()))
        .add_property("dependencies", make_function(&This::GetDependencies,
            return_value_policy<TfPySequenceToList>()))
    ;
}
