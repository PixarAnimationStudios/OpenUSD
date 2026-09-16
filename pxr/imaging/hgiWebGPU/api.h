//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIWEBGPU_API_H
#define PXR_IMAGING_HGIWEBGPU_API_H

#include "pxr/base/arch/export.h"
#include <webgpu/webgpu_cpp.h>

#if defined(PXR_STATIC)
#   define HGIWEBGPU_API
#   define HGIWEBGPU_API_TEMPLATE_CLASS(...)
#   define HGIWEBGPU_API_TEMPLATE_STRUCT(...)
#   define HGIWEBGPU_LOCAL
#else
#   if defined(HGIWEBGPU_EXPORTS)
#       define HGIWEBGPU_API ARCH_EXPORT
#       define HGIWEBGPU_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIWEBGPU_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HGIWEBGPU_API ARCH_IMPORT
#       define HGIWEBGPU_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIWEBGPU_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HGIWEBGPU_LOCAL ARCH_HIDDEN
#endif

#endif
