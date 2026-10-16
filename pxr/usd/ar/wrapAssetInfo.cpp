//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/assetInfo.h"

PXR_NAMESPACE_USING_DIRECTIVE

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

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/vt/valueFromPython.h"

void
wrapAssetInfo(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = ArAssetInfo;

    nb::class_<This>(m, "AssetInfo")
        .def(nb::init<>())

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("__hash__", &_GetHash)

        .def_rw("version", &This::version)
        .def_rw("assetName", &This::assetName)
        .def_prop_rw("resolverInfo", &_GetResolverInfo, &_SetResolverInfo)
        ;
}
