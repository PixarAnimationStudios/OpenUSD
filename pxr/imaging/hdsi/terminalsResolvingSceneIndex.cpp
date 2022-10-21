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
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const std::map<TfToken, TfToken>& terminalRemappings)
{
    return TfCreateRefPtr(new HdsiTerminalsResolvingSceneIndex(
        inputSceneIndex, terminalRemappings));
}

static void
_RenameTerminal(
    HdMaterialNetworkInterface* interface,
    const TfToken& oldName,
    const TfToken& newName)
{
    interface->SetTerminalConnection(
        newName, interface->GetTerminalConnection(oldName).second);
    interface->DeleteTerminal(oldName);
}

// static
void
HdsiTerminalsResolvingSceneIndex::ResolveTerminals(
    HdMaterialNetworkInterface* interface,
    const std::map<TfToken, TfToken>& terminalRemappings)
{
    if (!interface) {
        return;
    }

    for (const TfToken& terminalName : interface->GetTerminalNames()) {
        auto iter = terminalRemappings.find(terminalName);
        if (iter != terminalRemappings.end()) {
            const TfToken& oldName = iter->first;
            const TfToken& newName = iter->second;
            _RenameTerminal(interface, oldName, newName);
        }
    }
}

HdsiTerminalsResolvingSceneIndex::FilteringFnc
HdsiTerminalsResolvingSceneIndex::_GetFilteringFunction() const
{
    return [this](HdMaterialNetworkInterface* interface) {
        ResolveTerminals(interface, this->_terminalRemappings);
    };
}

HdsiTerminalsResolvingSceneIndex::HdsiTerminalsResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const std::map<TfToken, TfToken>& terminalRemappings)
    : HdMaterialFilteringSceneIndexBase(inputSceneIndex)
    , _terminalRemappings(terminalRemappings)
{
}

HdsiTerminalsResolvingSceneIndex::~HdsiTerminalsResolvingSceneIndex() = default;

PXR_NAMESPACE_CLOSE_SCOPE
