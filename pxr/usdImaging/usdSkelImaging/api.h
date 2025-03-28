//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_API_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDSKELIMAGING_API
#   define USDSKELIMAGING_API_TEMPLATE_CLASS(...)
#   define USDSKELIMAGING_API_TEMPLATE_STRUCT(...)
#   define USDSKELIMAGING_LOCAL
#else
#   if defined(USDSKELIMAGING_EXPORTS)
#       define USDSKELIMAGING_API ARCH_EXPORT
#       define USDSKELIMAGING_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSKELIMAGING_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDSKELIMAGING_API ARCH_IMPORT
#       define USDSKELIMAGING_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSKELIMAGING_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDSKELIMAGING_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_USD_IMAGING_USD_SKEL_IMAGING_API_H
