//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/assetInfo.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static VtValue
_GetResolverInfo(const ArAssetInfo& info)
{
    return info.resolverInfo;
}

static void
_SetResolverInfo(ArAssetInfo& info, const VtValue& resolverInfo)
{
    info.resolverInfo = resolverInfo;
}

static size_t
_GetHash(const ArAssetInfo& info)
{
    return hash_value(info);
}

void
wrapAssetInfo()
{
    using This = ArAssetInfo;

    class_<This>("AssetInfo")
        .def(init<>())

        .def(self == self)
        .def(self != self)

        .def("__hash__", &_GetHash)

        .def_readwrite("version", &This::version)
        .def_readwrite("assetName", &This::assetName)

        // Using .def_readwrite for resolverInfo gives this error on access:
        // "No python class registered for C++ class VtValue"
        //
        // Using .add_property works as expected.
        .add_property("resolverInfo", &_GetResolverInfo, &_SetResolverInfo)
        ;
}
