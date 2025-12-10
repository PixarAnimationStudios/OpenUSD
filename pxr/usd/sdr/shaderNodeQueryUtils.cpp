//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdr/shaderNodeQueryUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

VtDictionary
SdrShaderNodeQueryUtils::GroupQueryResults(
    const SdrShaderNodeQueryResult& result)
{
    // Return an empty VtDictionary when the query result has no keys.
    if (result.GetKeys().empty()) {
        return VtDictionary();
    }

    // Return a VtDictionary of potentially nested values
    std::vector<std::vector<std::string>> stringifiedValues
        = result.GetStringifiedValues();
    std::vector<SdrShaderNodePtrVec> shaderNodesByValues
        = result.GetShaderNodesByValues();
    VtDictionary dict;

    for (size_t i = 0; i < stringifiedValues.size(); ++i) {
        const std::vector<std::string>& keyPath = stringifiedValues[i];
        const SdrShaderNodePtrVec& shaderNodes = shaderNodesByValues[i];
        dict.SetValueAtPath(keyPath, VtValue(shaderNodes));
    }

    return dict;
}

PXR_NAMESPACE_CLOSE_SCOPE
