//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stageCache.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdCommon(nb::module_ &m)
{
    m.def("Describe", (std::string (*)(const UsdObject &)) UsdDescribe);
    m.def("Describe", (std::string (*)(const UsdStageWeakPtr &)) UsdDescribe);
    m.def("Describe", (std::string (*)(const UsdStageCache &)) UsdDescribe);

    TfNbWrapEnum<UsdListPosition>(m);
    TfNbWrapEnum<UsdLoadPolicy>(m);
    nb::enum_<UsdSchemaKind> schemaKindEnum(m, "SchemaKind");
    schemaKindEnum
        .value("Invalid", UsdSchemaKind::Invalid)
        .value("AbstractBase", UsdSchemaKind::AbstractBase)
        .value("AbstractTyped", UsdSchemaKind::AbstractTyped)
        .value("ConcreteTyped", UsdSchemaKind::ConcreteTyped)
        .value("NonAppliedAPI", UsdSchemaKind::NonAppliedAPI)
        .value("SingleApplyAPI", UsdSchemaKind::SingleApplyAPI)
        .value("MultipleApplyAPI", UsdSchemaKind::MultipleApplyAPI)
    ;
}
