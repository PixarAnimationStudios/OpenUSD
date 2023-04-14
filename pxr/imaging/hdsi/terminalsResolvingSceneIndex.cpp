//
// Copyright 2022 Pixar
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

#include "pxr/imaging/hdsi/terminalsResolvingSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

// static
HdsiTerminalsResolvingSceneIndexRefPtr
HdsiTerminalsResolvingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const TfTokenVector &contextNames)
{
    return TfCreateRefPtr(
        new HdsiTerminalsResolvingSceneIndex(inputSceneIndex, contextNames));
}

static TfToken
_WithoutPrefixOrEmpty(
    const TfToken &sToken,
    const TfToken &prefixToken)
{
    const std::string &s = sToken.GetString();
    const std::string &prefix = prefixToken.GetString();

    if (TfStringStartsWith(s, prefix)) {
        return TfToken(s.substr(prefix.size()));
    } else {
        return TfToken();
    }
}

static void
_RenameTerminal(
    HdMaterialNetworkInterface* const interface,
    const TfToken& oldName,
    const TfToken& newName)
{
    interface->SetTerminalConnection(
        newName, interface->GetTerminalConnection(oldName).second);
}

static
void
_ResolveTerminals(
    HdMaterialNetworkInterface* const interface,
    const TfTokenVector& reversedContextPrefixes)
{
    if (!interface) {
        return;
    }

    for (const TfToken& prefix : reversedContextPrefixes) {
        for (const TfToken& terminalName : interface->GetTerminalNames()) {
            const TfToken newName = _WithoutPrefixOrEmpty(terminalName, prefix);
            if (!newName.IsEmpty()) {
                _RenameTerminal(interface, terminalName, newName);
            }
        }
    }
}

HdsiTerminalsResolvingSceneIndex::FilteringFnc
HdsiTerminalsResolvingSceneIndex::_GetFilteringFunction() const
{
    return [this](HdMaterialNetworkInterface* const interface) {
        _ResolveTerminals(interface, this->_reversedContextPrefixes);
    };
}

HdsiTerminalsResolvingSceneIndex::HdsiTerminalsResolvingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const TfTokenVector &contextNames)
  : HdMaterialFilteringSceneIndexBase(inputSceneIndex)
{
    _reversedContextPrefixes.reserve(contextNames.size());

    for (auto it = contextNames.rbegin(); it != contextNames.rend(); ++it) {
        _reversedContextPrefixes.push_back(
            TfToken(it->GetString() + ":"));
    }
}

HdsiTerminalsResolvingSceneIndex::~HdsiTerminalsResolvingSceneIndex() = default;

PXR_NAMESPACE_CLOSE_SCOPE
