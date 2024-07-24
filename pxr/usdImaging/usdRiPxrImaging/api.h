//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_RI_PXR_IMAGING_API_H
#define PXR_USD_IMAGING_USD_RI_PXR_IMAGING_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDRIPXRIMAGING_API
#   define USDRIPXRIMAGING_API_TEMPLATE_CLASS(...)
#   define USDRIPXRIMAGING_API_TEMPLATE_STRUCT(...)
#   define USDRIPXRIMAGING_LOCAL
#else
#   if defined(USDRIPXRIMAGING_EXPORTS)
#       define USDRIPXRIMAGING_API ARCH_EXPORT
#       define USDRIPXRIMAGING_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDRIPXRIMAGING_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDRIPXRIMAGING_API ARCH_IMPORT
#       define USDRIPXRIMAGING_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDRIPXRIMAGING_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDRIPXRIMAGING_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_USD_IMAGING_USD_RI_PXR_IMAGING_API_H
