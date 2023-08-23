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