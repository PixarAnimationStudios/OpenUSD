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
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_TERMINAL_SCENE_INDEX_OBSERVER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_TERMINAL_SCENE_INDEX_OBSERVER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdPrman_RenderParamSharedPtr = std::shared_ptr<class HdPrman_RenderParam>;

/// Observes and processes notices from the terminal scene index (which is
/// currently managed by the render index during emulation).
///
class HdPrman_TerminalSceneIndexObserver : public HdSceneIndexObserver
{
public:
    HdPrman_TerminalSceneIndexObserver(
        const HdPrman_RenderParamSharedPtr &renderParam,
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    ~HdPrman_TerminalSceneIndexObserver() override;

    // ------------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------------
    
    // Process change notices that were aggregated since the last Update call.
    // This method mimics the intent of "Sync" in Hydra 1.0
    // 
    void Update();
 
 protected:

    // ------------------------------------------------------------------------
    // Satisfying HdSceneIndexObserver
    // ------------------------------------------------------------------------

    void PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries) override;

    void PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries) override;

    void PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries) override;

    void PrimsRenamed(
        const HdSceneIndexBase &sender,
        const RenamedPrimEntries &entries) override;

private:

    // Need a handle to renderParam since it manages the riley instance and
    // the render thread.
    std::shared_ptr<class HdPrman_RenderParam> _renderParam;
    HdSceneIndexBaseRefPtr const _terminalSi;
    bool _initialized;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_TERMINAL_SCENE_INDEX_OBSERVER_H