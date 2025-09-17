//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_SKEL_VALIDATORS_API_H
#define PXR_USD_VALIDATION_USD_SKEL_VALIDATORS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDSKELVALIDATORS_API
#   define USDSKELVALIDATORS_API_TEMPLATE_CLASS(...)
#   define USDSKELVALIDATORS_API_TEMPLATE_STRUCT(...)
#   define USDSKELVALIDATORS_API_LOCAL
#else
#   if defined(USDSKELVALIDATORS_EXPORTS)
#       define USDSKELVALIDATORS_API ARCH_EXPORT
#       define USDSKELVALIDATORS_API_TEMPLATE_CLASS(...)                      \
           ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSKELVALIDATORS_API_TEMPLATE_STRUCT(...)                     \
           ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDSKELVALIDATORS_API ARCH_IMPORT
#       define USDSKELVALIDATORS_API_TEMPLATE_CLASS(...)                      \
           ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSKELVALIDATORS_API_TEMPLATE_STRUCT(...)                     \
           ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#       define USDSKELVALIDATORS_API_LOCAL ARCH_HIDDEN
#endif

#endif
