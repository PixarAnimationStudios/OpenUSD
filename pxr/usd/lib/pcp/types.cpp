//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/pcp/types.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(PcpArcTypeRoot, "root");
    TF_ADD_ENUM_NAME(PcpArcTypeLocalInherit, "local inherit");
    TF_ADD_ENUM_NAME(PcpArcTypeGlobalInherit, "global inherit");
    TF_ADD_ENUM_NAME(PcpArcTypeRelocate, "relocate");
    TF_ADD_ENUM_NAME(PcpArcTypeVariant, "variant");
    TF_ADD_ENUM_NAME(PcpArcTypeReference, "reference");
    TF_ADD_ENUM_NAME(PcpArcTypePayload, "payload");
    TF_ADD_ENUM_NAME(PcpArcTypeLocalSpecializes, "local specializes");
    TF_ADD_ENUM_NAME(PcpArcTypeGlobalSpecializes, "global specializes");

    TF_ADD_ENUM_NAME(PcpRangeTypeRoot, "root");
    TF_ADD_ENUM_NAME(PcpRangeTypeLocalInherit, "local inherit");
    TF_ADD_ENUM_NAME(PcpRangeTypeGlobalInherit, "global inherit");
    TF_ADD_ENUM_NAME(PcpRangeTypeVariant, "variant");
    TF_ADD_ENUM_NAME(PcpRangeTypeReference, "reference");
    TF_ADD_ENUM_NAME(PcpRangeTypePayload, "payload");
    TF_ADD_ENUM_NAME(PcpRangeTypeLocalSpecializes, "local specializes");
    TF_ADD_ENUM_NAME(PcpRangeTypeGlobalSpecializes, "global specializes");
    TF_ADD_ENUM_NAME(PcpRangeTypeAll, "all");
    TF_ADD_ENUM_NAME(PcpRangeTypeAllInherits, "all inherits");
    TF_ADD_ENUM_NAME(PcpRangeTypeWeakerThanRoot, "weaker than root");
    TF_ADD_ENUM_NAME(PcpRangeTypeStrongerThanPayload, "stronger than payload");
    TF_ADD_ENUM_NAME(PcpRangeTypeInvalid, "invalid");
}
