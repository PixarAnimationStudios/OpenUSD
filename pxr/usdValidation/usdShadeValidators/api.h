//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDAION_USD_SHADE_VALIDATORS_API_H
#define PXR_USD_VALIDAION_USD_SHADE_VALIDATORS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDSHADEVALIDATORS_API
#   define USDSHADEVALIDATORS_API_TEMPLATE_CLASS(...)
#   define USDSHADEVALIDATORS_API_TEMPLATE_STRUCT(...)
#   define USDSHADEVALIDATORS_API_LOCAL
#else
#   if defined(USDSHADEVALIDATORS_API_EXPORTS)
#       define USDSHADEVALIDATORS_API ARCH_EXPORT
#       define USDSHADEVALIDATORS_API_TEMPLATE_CLASS(...)                     \
           ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSHADEVALIDATORS_API_TEMPLATE_STRUCT(...)                    \
           ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDSHADEVALIDATORS_API ARCH_IMPORT
#       define USDSHADEVALIDATORS_API_TEMPLATE_CLASS(...)                     \
           ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSHADEVALIDATORS_API_TEMPLATE_STRUCT(...)                    \
           ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#       define USDSHADEVALIDATORS_API_LOCAL ARCH_HIDDEN
#endif

#endif
