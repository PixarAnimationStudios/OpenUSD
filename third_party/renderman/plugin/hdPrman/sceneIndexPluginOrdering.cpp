//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "hdPrman/extComputationPrimvarPruningSceneIndexPlugin.h"
#include "hdPrman/motionBlurSceneIndexPlugin.h"
#include "hdPrman/velocityMotionResolvingSceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

// Validate ordering of various hdPrman scene index plugins at compile time.
//
static_assert(
    HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin::GetInsertionPhase() <
    HdPrman_VelocityMotionResolvingSceneIndexPlugin::GetInsertionPhase(),
    "Resolve computed primvars before resolving velocity motion.");

static_assert(
    HdPrman_VelocityMotionResolvingSceneIndexPlugin::GetInsertionPhase() <
    HdPrman_MotionBlurSceneIndexPlugin::GetInsertionPhase(),
    "Resolve velocity motion before applying motion blur.");

PXR_NAMESPACE_CLOSE_SCOPE
