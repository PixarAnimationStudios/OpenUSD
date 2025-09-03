//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esfUsd/sceneAdapter.h"

#include "pxr/exec/esfUsd/attribute.h"
#include "pxr/exec/esfUsd/object.h"
#include "pxr/exec/esfUsd/prim.h"
#include "pxr/exec/esfUsd/property.h"
#include "pxr/exec/esfUsd/stage.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

EsfStage
EsfUsdSceneAdapter::AdaptStage(const UsdStageConstRefPtr &stage)
{
    if (!stage) {
        TF_CODING_ERROR(
            "EsfUsdSceneAdapter cannot adapt a null stage pointer");
    }
    return {std::in_place_type<EsfUsd_Stage>, stage};
}

EsfStage
EsfUsdSceneAdapter::AdaptStage(UsdStageConstRefPtr &&stage)
{
    if (!stage) {
        TF_CODING_ERROR(
            "EsfUsdSceneAdapter cannot adapt a null stage pointer");
    }
    return {std::in_place_type<EsfUsd_Stage>, std::move(stage)};
}

EsfObject
EsfUsdSceneAdapter::AdaptObject(const UsdObject &object)
{
    return {std::in_place_type<EsfUsd_Object>, object};
}

EsfObject
EsfUsdSceneAdapter::AdaptObject(UsdObject &&object)
{
    return {std::in_place_type<EsfUsd_Object>, std::move(object)};
}

EsfPrim
EsfUsdSceneAdapter::AdaptPrim(const UsdPrim &prim)
{
    return {std::in_place_type<EsfUsd_Prim>, prim};
}

EsfPrim
EsfUsdSceneAdapter::AdaptPrim(UsdPrim &&prim)
{
    return {std::in_place_type<EsfUsd_Prim>, std::move(prim)};
}

EsfProperty
EsfUsdSceneAdapter::AdaptProperty(const UsdProperty &property)
{
    return {std::in_place_type<EsfUsd_Property>, property};
}

EsfProperty
EsfUsdSceneAdapter::AdaptProperty(UsdProperty &&property)
{
    return {std::in_place_type<EsfUsd_Property>, std::move(property)};
}

EsfAttribute
EsfUsdSceneAdapter::AdaptAttribute(const UsdAttribute &attribute)
{
    return {std::in_place_type<EsfUsd_Attribute>, attribute};
}

EsfAttribute
EsfUsdSceneAdapter::AdaptAttribute(UsdAttribute &&attribute)
{
    return {std::in_place_type<EsfUsd_Attribute>, std::move(attribute)};
}

PXR_NAMESPACE_CLOSE_SCOPE
