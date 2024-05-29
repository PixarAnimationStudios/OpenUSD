//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_PROC_IMAGING_API_H
#define PXR_USD_IMAGING_USD_PROC_IMAGING_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDPROCIMAGING_API
#   define USDPROCIMAGING_API_TEMPLATE_CLASS(...)
#   define USDPROCIMAGING_API_TEMPLATE_STRUCT(...)
#   define USDPROCIMAGING_LOCAL
#else
#   if defined(USDPROCIMAGING_EXPORTS)
#       define USDPROCIMAGING_API ARCH_EXPORT
#       define USDPROCIMAGING_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPROCIMAGING_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDPROCIMAGING_API ARCH_IMPORT
#       define USDPROCIMAGING_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPROCIMAGING_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDPROCIMAGING_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_USD_IMAGING_USD_PROC_IMAGING_API_H
