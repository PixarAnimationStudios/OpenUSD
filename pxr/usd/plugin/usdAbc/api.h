//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PLUGIN_USD_ABC_API_H
#define PXR_USD_PLUGIN_USD_ABC_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDABC_API
#   define USDABC_API_TEMPLATE_CLASS(...)
#   define USDABC_API_TEMPLATE_STRUCT(...)
#   define USDABC_LOCAL
#else
#   if defined(USDABC_EXPORTS)
#       define USDABC_API ARCH_EXPORT
#       define USDABC_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDABC_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDABC_API ARCH_IMPORT
#       define USDABC_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDABC_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDABC_LOCAL ARCH_HIDDEN
#endif

#endif
