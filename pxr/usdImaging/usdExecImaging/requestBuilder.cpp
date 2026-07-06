//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/requestBuilder.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

void
UsdExecImaging_RequestBuilder::AddValueKey(
    const UsdPrim &providerPrim,
    const TfToken &computationName)
{
    _valueKeys.emplace_back(providerPrim, computationName);

    _AddValueKey({providerPrim.GetPath(), computationName});
}

void
UsdExecImaging_RequestBuilder::AddValueKey(
    const UsdAttribute &providerAttribute)
{
    _valueKeys.emplace_back(
        providerAttribute,
        ExecBuiltinComputations->computeValue);

    _AddValueKey({
        providerAttribute.GetPath(),
        ExecBuiltinComputations->computeValue});
}

void
UsdExecImaging_RequestBuilder::SetAdaptedPrim(
    const UsdPrim &prim,
    const UsdExecImagingPrimAdapterInterface &primAdapter)
{
    _adaptedPrimPath = prim.GetPath();
    _primAdapter = &primAdapter;
    const bool emplaced = _valueKeyMap.primToAdapterMap.emplace(
        _adaptedPrimPath, _primAdapter).second;
    TF_VERIFY(emplaced);
}

std::vector<ExecUsdValueKey>
UsdExecImaging_RequestBuilder::TakeValueKeys()
{
    return std::move(_valueKeys);
}

UsdExecImaging_ValueKeyMap
UsdExecImaging_RequestBuilder::TakeValueKeyMap()
{
    return std::move(_valueKeyMap);
}

void
UsdExecImaging_RequestBuilder::_AddValueKey(UsdExecImagingValueKey valueKey)
{
    // This method should be called after the ExecUsdValueKey has been inserted.
    if (!TF_VERIFY(!_valueKeys.empty())) {
        return;
    }

    const int valueKeyIndex = static_cast<int>(_valueKeys.size()) - 1;
    _valueKeyMap.valueKeyToIndexMap[valueKey] = valueKeyIndex;
    _valueKeyMap.indexToValueKeyInfo.push_back({
        std::move(valueKey),
        _adaptedPrimPath,
        _primAdapter
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
