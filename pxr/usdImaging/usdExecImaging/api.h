//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_API_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDEXECIMAGING_API
#   define USDEXECIMAGING_API_TYPE
#   define USDEXECIMAGING_API_TEMPLATE_CLASS(...)
#   define USDEXECIMAGING_API_TEMPLATE_STRUCT(...)
#   define USDEXECIMAGING_LOCAL
#else
#   if defined(USDEXECIMAGING_EXPORTS)
#       define USDEXECIMAGING_API ARCH_EXPORT
#       define USDEXECIMAGING_API_TYPE ARCH_EXPORT_TYPE
#       define USDEXECIMAGING_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDEXECIMAGING_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDEXECIMAGING_API ARCH_IMPORT
#       define USDEXECIMAGING_API_TYPE
#       define USDEXECIMAGING_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDEXECIMAGING_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDEXECIMAGING_LOCAL ARCH_HIDDEN
#endif

#endif
