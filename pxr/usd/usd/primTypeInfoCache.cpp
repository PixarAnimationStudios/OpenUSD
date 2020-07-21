//
// Copyright 2020 Pixar
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
#include "pxr/pxr.h"
#include "pxr/usd/usd/primTypeInfoCache.h"

PXR_NAMESPACE_OPEN_SCOPE

void 
Usd_PrimTypeInfoCache::ComputeInvalidPrimTypeToFallbackMap(
    const VtDictionary &fallbackPrimTypesDict,
    TfHashMap<TfToken, TfToken, TfHash> *typeToFallbackTypeMap)
{
    // The dictionary is expected to map prim type name strings each to a 
    // VtTokenArray containing the ordered list of fallback types to use if
    // the given type name is not valid.
    for (const auto &valuePair : fallbackPrimTypesDict) {
        // if the type has a valid schema, we don't need a fallback so 
        // just skip it.
        const TfToken typeName(valuePair.first);
        if (FindOrCreatePrimTypeInfo(TypeId(typeName))->GetSchemaType()) {
            continue;
        }
        if (!valuePair.second.IsHolding<VtTokenArray>()) {
            TF_WARN("Value for key '%s' in fallbackPrimTypes metadata "
                    "dictionary is not a VtTokenArray.",
                    typeName.GetText());
            continue;
        }
        const VtTokenArray &fallbackNames = 
            valuePair.second.UncheckedGet<VtTokenArray>();

        // Go through the list of fallbacks for the invalid type and choose
        // the first one that produces a valid schema type. 
        for (const TfToken &fallbackName : fallbackNames) {
            if (FindOrCreatePrimTypeInfo(
                    TypeId(fallbackName))->GetSchemaType()) {
                // Map the invalid type to the valid fallback type.
                typeToFallbackTypeMap->insert(
                    std::make_pair(typeName, fallbackName));
                break;
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

