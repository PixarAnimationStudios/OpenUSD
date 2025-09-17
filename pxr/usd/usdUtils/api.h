//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_API_H
#define PXR_USD_USD_UTILS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDUTILS_API
#   define USDUTILS_API_TEMPLATE_CLASS(...)
#   define USDUTILS_API_TEMPLATE_STRUCT(...)
#   define USDUTILS_LOCAL
#else
#   if defined(USDUTILS_EXPORTS)
#       define USDUTILS_API ARCH_EXPORT
#       define USDUTILS_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDUTILS_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDUTILS_API ARCH_IMPORT
#       define USDUTILS_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDUTILS_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDUTILS_LOCAL ARCH_HIDDEN
#endif

#endif
