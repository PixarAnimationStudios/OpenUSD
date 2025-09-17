//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_PHYSICS_VALIDATORS_API_H
#define PXR_USD_VALIDATION_USD_PHYSICS_VALIDATORS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDPHYSICSVALIDATORS_API
#   define USDPHYSICSVALIDATORS_API_TEMPLATE_CLASS(...)
#   define USDPHYSICSVALIDATORS_API_TEMPLATE_STRUCT(...)
#   define USDPHYSICSVALIDATORS_API_LOCAL
#else
#   if defined(USDPHYSICSVALIDATORS_EXPORTS)
#       define USDPHYSICSVALIDATORS_API ARCH_EXPORT
#       define USDPHYSICSVALIDATORS_API_TEMPLATE_CLASS(...)                      \
           ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPHYSICSVALIDATORS_API_TEMPLATE_STRUCT(...)                     \
           ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDPHYSICSVALIDATORS_API ARCH_IMPORT
#       define USDPHYSICSVALIDATORS_API_TEMPLATE_CLASS(...)                      \
           ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPHYSICSVALIDATORS_API_TEMPLATE_STRUCT(...)                     \
           ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#       define USDPHYSICSVALIDATORS_API_LOCAL ARCH_HIDDEN
#endif

#endif
