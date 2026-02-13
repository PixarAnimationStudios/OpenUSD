//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDERER_CREATE_ARGS_H
#define PXR_IMAGING_HD_RENDERER_CREATE_ARGS_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

///
/// HdRendererCreateArgs contains members indicating the resources available 
/// when creating a renderer plugin.
///
struct HdRendererCreateArgs
{
    // Whether the GPU is available or not.
    bool gpuEnabled { true };
    // An Hgi instance to check backend support against.
    Hgi* hgi { nullptr };

    bool operator==(const HdRendererCreateArgs& other) const
    {
        return gpuEnabled == other.gpuEnabled && hgi == other.hgi;
    }

    bool operator!=(const HdRendererCreateArgs& other) const
    {
        return !(*this == other);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
