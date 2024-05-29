//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/expressionVariablesDependencyData.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/stl.h"

#include <iterator>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class PcpExpressionVariablesDependencyData::_Data
{
public:
    using LayerStackToExpressionVarsMap = std::unordered_map<
        PcpLayerStackPtr, std::unordered_set<std::string>, TfHash>;
    LayerStackToExpressionVarsMap layerStackToExpressionVars;
};

PcpExpressionVariablesDependencyData::PcpExpressionVariablesDependencyData()
    = default;

PcpExpressionVariablesDependencyData::~PcpExpressionVariablesDependencyData()
    = default;

PcpExpressionVariablesDependencyData::PcpExpressionVariablesDependencyData(
    PcpExpressionVariablesDependencyData&&) = default;

PcpExpressionVariablesDependencyData&
PcpExpressionVariablesDependencyData::operator=(
    PcpExpressionVariablesDependencyData&&) = default;

bool
PcpExpressionVariablesDependencyData::IsEmpty() const
{
    return !static_cast<bool>(_data);
}

void
PcpExpressionVariablesDependencyData::AppendDependencyData(
    PcpExpressionVariablesDependencyData&& other)
{
    if (!other._data) {
        return;
    }

    _Data& otherData = other._GetWritableData();
    for (auto& otherEntry : otherData.layerStackToExpressionVars) {
        AddDependencies(otherEntry.first, std::move(otherEntry.second));
    }
}

void 
PcpExpressionVariablesDependencyData::AddDependencies(
    const PcpLayerStackPtr& layerStack,
    std::unordered_set<std::string>&& exprVarDependencies)
{
    if (exprVarDependencies.empty()) {
        return;
    }

    _Data& data = _GetWritableData();

    std::unordered_set<std::string>& storedDeps = 
        data.layerStackToExpressionVars[layerStack];

    if (storedDeps.empty()) {
        storedDeps = std::move(exprVarDependencies);
    }
    else {
        storedDeps.insert(
            std::make_move_iterator(exprVarDependencies.begin()),
            std::make_move_iterator(exprVarDependencies.end()));
    }
}

const std::unordered_set<std::string>*
PcpExpressionVariablesDependencyData::GetDependenciesForLayerStack(
    const PcpLayerStackPtr& layerStack) const
{
    const _Data* data = _GetData();
    return data ? 
        TfMapLookupPtr(data->layerStackToExpressionVars, layerStack) : nullptr;
}

void
PcpExpressionVariablesDependencyData::_ForEachDependency(
    const _ForEachFunctionRef& fn) const
{
    const _Data* data = _GetData();
    if (!data) {
        return;
    }

    for (const auto& entry : data->layerStackToExpressionVars) {
        fn(entry.first, entry.second);
    }
}

const PcpExpressionVariablesDependencyData::_Data*
PcpExpressionVariablesDependencyData::_GetData() const
{
    return _data.get();
}

PcpExpressionVariablesDependencyData::_Data&
PcpExpressionVariablesDependencyData::_GetWritableData()
{
    if (!_data) {
        _data = std::make_unique<_Data>();
    }
    return *_data;
}

PXR_NAMESPACE_CLOSE_SCOPE
