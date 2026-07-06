//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_LOD_VALIDATORS_API_H
#define PXR_USD_VALIDATION_USD_LOD_VALIDATORS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDLODVALIDATORS_API
#   define USDLODVALIDATORS_API_TEMPLATE_CLASS(...)
#   define USDLODVALIDATORS_API_TEMPLATE_STRUCT(...)
#   define USDLODVALIDATORS_API_LOCAL
#else
#   if defined(USDLODVALIDATORS_EXPORTS)
#       define USDLODVALIDATORS_API ARCH_EXPORT
#       define USDLODVALIDATORS_API_TEMPLATE_CLASS(...)                     \
           ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLODVALIDATORS_API_TEMPLATE_STRUCT(...)                    \
           ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDLODVALIDATORS_API ARCH_IMPORT
#       define USDLODVALIDATORS_API_TEMPLATE_CLASS(...)                     \
           ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLODVALIDATORS_API_TEMPLATE_STRUCT(...)                    \
           ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#       define USDLODVALIDATORS_API_LOCAL ARCH_HIDDEN
#endif

#endif
