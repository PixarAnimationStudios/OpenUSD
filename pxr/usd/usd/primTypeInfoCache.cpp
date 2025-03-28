//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

