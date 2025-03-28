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

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/enum.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdCommon()
{
    def("Describe", (std::string (*)(const UsdObject &)) UsdDescribe);
    def("Describe", (std::string (*)(const UsdStageWeakPtr &)) UsdDescribe);
    def("Describe", (std::string (*)(const UsdStageCache &)) UsdDescribe);

    TfPyWrapEnum<UsdListPosition>();
    TfPyWrapEnum<UsdLoadPolicy>();
    enum_<UsdSchemaKind>("SchemaKind")
        .value("Invalid", UsdSchemaKind::Invalid)
        .value("AbstractBase", UsdSchemaKind::AbstractBase)
        .value("AbstractTyped", UsdSchemaKind::AbstractTyped)
        .value("ConcreteTyped", UsdSchemaKind::ConcreteTyped)
        .value("NonAppliedAPI", UsdSchemaKind::NonAppliedAPI)
        .value("SingleApplyAPI", UsdSchemaKind::SingleApplyAPI)
        .value("MultipleApplyAPI", UsdSchemaKind::MultipleApplyAPI)
    ;
}
