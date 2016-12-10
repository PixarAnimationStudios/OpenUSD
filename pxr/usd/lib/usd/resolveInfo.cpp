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
#include "pxr/usd/usd/resolveInfo.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdResolveInfoSourceNone,
                     "No value");
    TF_ADD_ENUM_NAME(UsdResolveInfoSourceFallback,
                     "Built-in fallback value");
    TF_ADD_ENUM_NAME(UsdResolveInfoSourceDefault,
                     "Attribute default value");
    TF_ADD_ENUM_NAME(UsdResolveInfoSourceTimeSamples,
                     "Attribute time samples");
    TF_ADD_ENUM_NAME(UsdResolveInfoSourceValueClips,
                     "Value clips");
    TF_ADD_ENUM_NAME(UsdResolveInfoSourceIsTimeDependent,
                     "Source may vary over time");
}
