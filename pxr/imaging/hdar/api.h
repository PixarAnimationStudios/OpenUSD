//
// Copyright 2012 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDAR_API_H
#define PXR_IMAGING_HDAR_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDAR_API
#   define HDAR_API_TEMPLATE_CLASS(...)
#   define HDAR_API_TEMPLATE_STRUCT(...)
#   define HDAR_LOCAL
#else
#   if defined(HDAR_EXPORTS)
#       define HDAR_API ARCH_EXPORT
#       define HDAR_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDAR_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDAR_API ARCH_IMPORT
#       define HDAR_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDAR_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDAR_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HDAR_API_H
