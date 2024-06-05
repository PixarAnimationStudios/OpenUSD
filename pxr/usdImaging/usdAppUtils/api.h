//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_APP_UTILS_API_H
#define PXR_USD_IMAGING_USD_APP_UTILS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDAPPUTILS_API
#   define USDAPPUTILS_API_TEMPLATE_CLASS(...)
#   define USDAPPUTILS_API_TEMPLATE_STRUCT(...)
#   define USDAPPUTILS_LOCAL
#else
#   if defined(USDAPPUTILS_EXPORTS)
#       define USDAPPUTILS_API ARCH_EXPORT
#       define USDAPPUTILS_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDAPPUTILS_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDAPPUTILS_API ARCH_IMPORT
#       define USDAPPUTILS_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDAPPUTILS_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDAPPUTILS_LOCAL ARCH_HIDDEN
#endif

#endif
