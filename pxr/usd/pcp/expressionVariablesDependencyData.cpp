//
// Copyright 2023 Pixar
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
