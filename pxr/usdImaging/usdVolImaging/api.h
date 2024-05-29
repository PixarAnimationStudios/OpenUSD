//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_VOL_IMAGING_API_H
#define PXR_USD_IMAGING_USD_VOL_IMAGING_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDVOLIMAGING_API
#   define USDVOLIMAGING_API_TEMPLATE_CLASS(...)
#   define USDVOLIMAGING_API_TEMPLATE_STRUCT(...)
#   define USDVOLIMAGING_LOCAL
#else
#   if defined(USDVOLIMAGING_EXPORTS)
#       define USDVOLIMAGING_API ARCH_EXPORT
#       define USDVOLIMAGING_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDVOLIMAGING_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDVOLIMAGING_API ARCH_IMPORT
#       define USDVOLIMAGING_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDVOLIMAGING_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDVOLIMAGING_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_USD_IMAGING_USD_VOL_IMAGING_API_H
