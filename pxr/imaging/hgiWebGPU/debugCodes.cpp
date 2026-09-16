//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiWebGPU/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


    TF_REGISTRY_FUNCTION(TfDebug)
    {
        TF_DEBUG_ENVIRONMENT_SYMBOL(HGIWEBGPU_DEBUG_GRAPHICS_PIPELINE,
                                    "HgiWebGPU report graphics pipeline attributes descriptors.");
        TF_DEBUG_ENVIRONMENT_SYMBOL(HGIWEBGPU_DEBUG_SHADER_CODE,
                                    "HgiWebGPU report graphics pipeline attributes descriptors.");
        TF_DEBUG_ENVIRONMENT_SYMBOL(HGIWEBGPU_DEBUG_TIMESTAMPS,
                                    "HgiWebGPU report commands timestamps.");
        TF_DEBUG_ENVIRONMENT_SYMBOL(HGIWEBGPU_DUMP_SHADER_SOURCEFILE,
                                    "Write out generated WGSL shader source code to files");

    }

PXR_NAMESPACE_CLOSE_SCOPE
