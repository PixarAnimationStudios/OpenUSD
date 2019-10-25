//
// Copyright 2019 Pixar
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
#include "pxr/usd/pcp/dynamicFileFormatInterface.h"
#include "pxr/usd/pcp/dynamicFileFormatDependencyData.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

PcpDynamicFileFormatDependencyData::PcpDynamicFileFormatDependencyData(
    const PcpDynamicFileFormatDependencyData &other)
{
    // Have to copy the contents of the unique pointer if it's non-null.
    if (other._data) {
        _data.reset(new _Data(*other._data));
    }
}

void 
PcpDynamicFileFormatDependencyData::AddDependencyContext(
    const PcpDynamicFileFormatInterface *dynamicFileFormat,
    VtValue &&dependencyContextData,
    TfToken::Set &&composedFieldNames)
{
    // Create the data now if it was empty before this call..
    if (!_data) {
        _data.reset(new _Data());
    }

    // Add file format and context data to the list and update the list of
    // relevant fields.
    _data->dependencyContexts.emplace_back(
        dynamicFileFormat, std::move(dependencyContextData));
    _data->_AddRelevantFieldNames(std::move(composedFieldNames));
}

void 
PcpDynamicFileFormatDependencyData::AppendDependencyData(
    PcpDynamicFileFormatDependencyData &&dependencyData) 
{
    if (!dependencyData._data) {
        return;
    }
    // If we have our own data we need to append, otherwise we can just take
    // the other dependency data wholesale.
    if (_data) {
        // Take each context from the other data and add it to ours.
        for (_Data::_ContextData &contextData : 
                dependencyData._data->dependencyContexts) {
            _data->dependencyContexts.emplace_back(std::move(contextData));
        }
        // Add the other data's relevants fields to ours as well.
        _data->_AddRelevantFieldNames(
            std::move(dependencyData._data->relevantFieldNames));
    } else {
        Swap(dependencyData);
    }
}

bool 
PcpDynamicFileFormatDependencyData::CanFieldChangeAffectFileFormatArguments(
    const TfToken &fieldName, 
    const VtValue& oldValue, 
    const VtValue& newValue) const
{
    if (!_data) {
        return false;
    }

    // Early out if this particular field wasn't composed for this dependency.
    if (_data->relevantFieldNames.count(fieldName) == 0) {
        return false;
    }

    // Check each dependency context.
    for (const _Data::_ContextData &contextData : _data->dependencyContexts) {
        // We better not have logged a dependency for a file format that doesn't
        // support dynamic arguments.
        if (!TF_VERIFY(contextData.first)) {
            continue;
        }
        
        // Return true if any context's file format can be affect by this 
        // field change.
        if (contextData.first->CanFieldChangeAffectFileFormatArguments(
                fieldName, oldValue, newValue, contextData.second)) {
            return true;
        }
    }
    return false;
}

const TfToken::Set &
PcpDynamicFileFormatDependencyData::GetRelevantFieldNames() const
{
    static const TfToken::Set empty;
    return _data ? _data->relevantFieldNames : empty;
}

void 
PcpDynamicFileFormatDependencyData::_Data::_AddRelevantFieldNames(
    TfToken::Set &&fieldNames)
{
    // Avoid copying if our current relevant fields list is empty.
    if (relevantFieldNames.empty()) {
        relevantFieldNames.swap(fieldNames);
    } else {
        relevantFieldNames.insert(fieldNames.begin(), fieldNames.end());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
