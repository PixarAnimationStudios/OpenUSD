//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_DEBUG_CODES_H
#define PXR_IMAGING_HGI_WEBGPU_DEBUG_CODES_H

/// \file hgiWebGPU/debugCodes.h

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE

    TF_DEBUG_CODES(

            HGIWEBGPU_DEBUG_GRAPHICS_PIPELINE,
            HGIWEBGPU_DEBUG_SHADER_CODE,
            HGIWEBGPU_DEBUG_TIMESTAMPS,
            HGIWEBGPU_DUMP_SHADER_SOURCEFILE

    );

PXR_NAMESPACE_CLOSE_SCOPE

#endif
