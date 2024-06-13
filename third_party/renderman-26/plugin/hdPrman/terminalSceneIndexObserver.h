//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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