//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/types.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(PcpArcTypeRoot, "root");
    TF_ADD_ENUM_NAME(PcpArcTypeInherit, "inherit");
    TF_ADD_ENUM_NAME(PcpArcTypeRelocate, "relocate");
    TF_ADD_ENUM_NAME(PcpArcTypeVariant, "variant");
    TF_ADD_ENUM_NAME(PcpArcTypeReference, "reference");
    TF_ADD_ENUM_NAME(PcpArcTypePayload, "payload");
    TF_ADD_ENUM_NAME(PcpArcTypeSpecialize, "specialize");

    TF_ADD_ENUM_NAME(PcpRangeTypeRoot, "root");
    TF_ADD_ENUM_NAME(PcpRangeTypeInherit, "inherit");
    TF_ADD_ENUM_NAME(PcpRangeTypeVariant, "variant");
    TF_ADD_ENUM_NAME(PcpRangeTypeReference, "reference");
    TF_ADD_ENUM_NAME(PcpRangeTypePayload, "payload");
    TF_ADD_ENUM_NAME(PcpRangeTypeSpecialize, "specialize");
    TF_ADD_ENUM_NAME(PcpRangeTypeAll, "all");
    TF_ADD_ENUM_NAME(PcpRangeTypeWeakerThanRoot, "weaker than root");
    TF_ADD_ENUM_NAME(PcpRangeTypeStrongerThanPayload, "stronger than payload");
    TF_ADD_ENUM_NAME(PcpRangeTypeInvalid, "invalid");
}

PXR_NAMESPACE_CLOSE_SCOPE
