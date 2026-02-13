//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdr/shaderNodeQuery.h"
#include "pxr/usd/sdr/registry.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

SdrShaderNodeQuery&
SdrShaderNodeQuery::SelectDistinct(const TfToken& key)
{
    _selectKeys.push_back(key);
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::SelectDistinct(const std::vector<TfToken>& keys)
{
    _selectKeys.insert(_selectKeys.end(), keys.begin(), keys.end());
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::NodeValueIs(
    const TfToken& key,
    const VtValue& value)
{
    _hasValues.emplace_back(key, value);
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::NodeValueIsIn(
    const TfToken& key,
    const std::vector<VtValue>& values)
{
    _hasOneOfValues.emplace_back(key, values);
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::NodeHasValueFor(const TfToken& key)
{
    // Empty VtValue is semantically equivalent to nonexistence
    // of a metadata item.
    _lacksValues.emplace_back(key, VtValue());
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::NodeValueIsNot(
    const TfToken& key,
    const VtValue& value)
{
    _lacksValues.emplace_back(key, value);
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::NodeValueIsNotIn(
    const TfToken& key,
    const std::vector<VtValue>& values)
{
    _lacksAllOfValues.emplace_back(key, values);
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::NodeHasNoValueFor(const TfToken& key)
{
    // Empty VtValue is semantically equivalent to nonexistence
    // of a metadata item.
    _hasValues.emplace_back(key, VtValue());
    return *this;
}

SdrShaderNodeQuery&
SdrShaderNodeQuery::CustomFilter(FilterFn fn)
{
    _customFilters.push_back(fn);
    return *this;
}

SdrShaderNodeQueryResult
SdrShaderNodeQuery::Run()
{
    static SdrRegistry& reg = SdrRegistry::GetInstance();
    return reg.RunQuery(*this);
}

std::vector<std::vector<std::string>>
SdrShaderNodeQueryResult::GetStringifiedValues() const
{
    std::vector<std::vector<std::string>> result;
    for (const std::vector<VtValue>& row : _values) {
        std::vector<std::string> rowStrings;
        for (const VtValue& value : row) {
                rowStrings.push_back(TfStringify(value));
        }
        result.push_back(rowStrings);
    }
    return result;
}

std::vector<SdrShaderNodePtrVec>
SdrShaderNodeQueryResult::GetShaderNodesByValues() const
{
    if (_values.empty()) {
        return {};
    }

    return _nodes;
}

SdrShaderNodePtrVec
SdrShaderNodeQueryResult::GetAllShaderNodes() const
{
    SdrShaderNodePtrVec result;
    for (const SdrShaderNodePtrVec& inner : _nodes) {
        result.insert(result.end(), inner.begin(), inner.end());
    }

    std::sort(result.begin(), result.end(),
        [](SdrShaderNodeConstPtr a, SdrShaderNodeConstPtr b) {
                return a->GetIdentifier() < b->GetIdentifier() ||
                       a->GetSourceType() < b->GetSourceType();
        });
    return result;
}

bool
SdrShaderNodeQueryResult::_IsValid() const
{
    const size_t num_keys = _keys.size();
    for (const std::vector<VtValue>& valueRow : _values) {
        if (num_keys != valueRow.size()) {
            return false;
        }
    }

    return GetShaderNodesByValues().size() == _values.size();
}

PXR_NAMESPACE_CLOSE_SCOPE
