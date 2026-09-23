//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_MEDIA_VALIDATORS_API_H
#define PXR_USD_VALIDATION_USD_MEDIA_VALIDATORS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDMEDIAVALIDATORS_API
#   define USDMEDIAVALIDATORS_API_TEMPLATE_CLASS(...)
#   define USDMEDIAVALIDATORS_API_TEMPLATE_STRUCT(...)
#   define USDMEDIAVALIDATORS_API_LOCAL
#else
#   if defined(USDMEDIAVALIDATORS_EXPORTS)
#       define USDMEDIAVALIDATORS_API ARCH_EXPORT
#       define USDMEDIAVALIDATORS_API_TEMPLATE_CLASS(...)                    \
           ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDMEDIAVALIDATORS_API_TEMPLATE_STRUCT(...)                   \
           ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDMEDIAVALIDATORS_API ARCH_IMPORT
#       define USDMEDIAVALIDATORS_API_TEMPLATE_CLASS(...)                    \
           ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDMEDIAVALIDATORS_API_TEMPLATE_STRUCT(...)                   \
           ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#       define USDMEDIAVALIDATORS_API_LOCAL ARCH_HIDDEN
#endif

#endif
