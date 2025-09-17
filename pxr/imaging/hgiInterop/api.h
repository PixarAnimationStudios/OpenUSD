//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIINTEROP_API_H
#define PXR_IMAGING_HGIINTEROP_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HGIINTEROP_API
#   define HGIINTEROP_API_TEMPLATE_CLASS(...)
#   define HGIINTEROP_API_TEMPLATE_STRUCT(...)
#   define HGIINTEROP_LOCAL
#else
#   if defined(HGIINTEROP_EXPORTS)
#       define HGIINTEROP_API ARCH_EXPORT
#       define HGIINTEROP_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIINTEROP_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HGIINTEROP_API ARCH_IMPORT
#       define HGIINTEROP_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIINTEROP_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HGIINTEROP_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HGIINTEROP_API_H
