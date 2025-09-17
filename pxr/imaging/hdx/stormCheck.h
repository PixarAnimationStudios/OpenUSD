//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_STORM_CHECK_H
#define PXR_IMAGING_HDX_STORM_CHECK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderDelegate;

// This function should only be used to check for storm when creating a
// HdxTaskControllerSceneIndex. It is intended to be temporary since we don't
// want scene indices to configure themselves based on render delegate.
HDX_API
bool HdxIsStorm(const HdRenderDelegate* delegate);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #endif // PXR_IMAGING_HDX_STORM_CHECK_H

