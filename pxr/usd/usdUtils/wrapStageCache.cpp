//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/stageCache.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapStageCache(nanobind::module_ &m)
{
    nb::class_<UsdUtilsStageCache>(m, "StageCache")
        .def(nb::init<>())

        .def_static("Get", &UsdUtilsStageCache::Get,
             nb::rv_policy::reference)

        .def_static(
            "GetSessionLayerForVariantSelections",
            (SdfLayerRefPtr (*)(
                const TfToken& name,
                const std::vector<std::pair<std::string, std::string>>&))
                &UsdUtilsStageCache::GetSessionLayerForVariantSelections,
            nb::arg("modelName"), nb::arg("variantSelections"))
        .def_static(
            "GetSessionLayerForVariantSelections",
            (SdfLayerRefPtr (*)(
                const SdfPath& primPath,
                const std::vector<std::pair<std::string, std::string>>&))
                &UsdUtilsStageCache::GetSessionLayerForVariantSelections,
            nb::arg("primPath"), nb::arg("variantSelections"))
        ;
}
