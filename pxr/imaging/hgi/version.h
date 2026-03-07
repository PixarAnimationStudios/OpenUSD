//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_VERSION_H
#define PXR_IMAGING_HGI_VERSION_H

//  1 ->  2: HgiVulkan: Removed API related to use of SPIRV-Reflect
//  2 ->  3: Moved some checks in HgiGL/Metal/Vulkan to Hgi to unify error
//           checking.

#define HGI_API_VERSION 3

#endif // PXR_IMAGING_HGI_VERSION_H
