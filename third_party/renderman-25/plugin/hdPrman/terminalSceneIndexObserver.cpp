//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/terminalSceneIndexObserver.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_TerminalSceneIndexObserver::HdPrman_TerminalSceneIndexObserver(
    HdPrman_RenderParamSharedPtr const &renderParam,
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
: _renderParam(renderParam)
, _terminalSi(inputSceneIndex)
, _initialized(false)
{
    if (!renderParam) {
        TF_CODING_ERROR("Invalid render param provided.");
    }

    if (!inputSceneIndex) {
        TF_CODING_ERROR("Invalid input (terminal) scene index provided.\n");
    } else {
        inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(this));
    }
}

HdPrman_TerminalSceneIndexObserver::~HdPrman_TerminalSceneIndexObserver()
    = default;

// ------------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------------

void
HdPrman_TerminalSceneIndexObserver::Update()
{
    HD_TRACE_FUNCTION();

    if (!_initialized) {
        TF_DEBUG(HDPRMAN_TERMINAL_SCENE_INDEX_OBSERVER).Msg(
            "HdPrman_TerminalSceneIndexObserver::Update -- Initialization..\n");

        // TODO
        // * Move riley scene options initialization here.
        // * Walk the scene and process populated prims.
        //   Start with the active (or available) render settings prim and
        //   connected prims (cameras, render terminals).
        //

        _initialized = true;
    } else {
        // TODO
        // * Process change notices received since the last Update call.
    }
}

// ------------------------------------------------------------------------
// HdSceneIndexObserver virtual API
// ------------------------------------------------------------------------

void
HdPrman_TerminalSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    
}

void
HdPrman_TerminalSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{

}

void
HdPrman_TerminalSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{

}

void
HdPrman_TerminalSceneIndexObserver::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{

}

PXR_NAMESPACE_CLOSE_SCOPE