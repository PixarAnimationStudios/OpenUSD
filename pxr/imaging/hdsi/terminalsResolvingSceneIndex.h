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
#ifndef PXR_IMAGING_HDSI_TERMINALS_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_TERMINALS_RESOLVING_SCENE_INDEX_H

#include "pxr/imaging/hd/materialFilteringSceneIndexBase.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiTerminalsResolvingSceneIndex);

/// SceneIndex that "resolves" terminals by renaming them from their context
/// specific names (i.e. "ri:surface") to the HdMaterialTerminalToken
/// ("surface").  The mapping is provided by a `terminalRemappings` parameter
/// that is passed to the constructor.
class HdsiTerminalsResolvingSceneIndex final
    : public HdMaterialFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiTerminalsResolvingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const std::map<TfToken, TfToken>& terminalRemappings);

    // This static function is provided to allow some
    // backwards-compatibility....
    HDSI_API
    static void ResolveTerminals(
        HdMaterialNetworkInterface* interface,
        const std::map<TfToken, TfToken>& terminalRemappings);

protected: // HdMaterialFilteringSceneIndexBase overrides
    FilteringFnc _GetFilteringFunction() const override;

protected:
    HdsiTerminalsResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const std::map<TfToken, TfToken>& terminalRemappings);
    ~HdsiTerminalsResolvingSceneIndex() override;

private:
    const std::map<TfToken, TfToken> _terminalRemappings;
};
PXR_NAMESPACE_CLOSE_SCOPE
#endif
