//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDAION_USD_LUX_VALIDATORS_API_H
#define PXR_USD_VALIDAION_USD_LUX_VALIDATORS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDLUXVALIDATORS_API
#   define USDLUXVALIDATORS_API_TEMPLATE_CLASS(...)
#   define USDLUXVALIDATORS_API_TEMPLATE_STRUCT(...)
#   define USDLUXVALIDATORS_API_LOCAL
#else
#   if defined(USDLUXVALIDATORS_EXPORTS)
#       define USDLUXVALIDATORS_API ARCH_EXPORT
#       define USDLUXVALIDATORS_API_TEMPLATE_CLASS(...)                     \
           ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLUXVALIDATORS_API_TEMPLATE_STRUCT(...)                    \
           ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDLUXVALIDATORS_API ARCH_IMPORT
#       define USDLUXVALIDATORS_API_TEMPLATE_CLASS(...)                     \
           ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLUXVALIDATORS_API_TEMPLATE_STRUCT(...)                    \
           ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#       define USDLUXVALIDATORS_API_LOCAL ARCH_HIDDEN
#endif

#endif
