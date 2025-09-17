//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPHYSICS_API_H
#define USDPHYSICS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDPHYSICS_API
#   define USDPHYSICS_API_TEMPLATE_CLASS(...)
#   define USDPHYSICS_API_TEMPLATE_STRUCT(...)
#   define USDPHYSICS_LOCAL
#else
#   if defined(USDPHYSICS_EXPORTS)
#       define USDPHYSICS_API ARCH_EXPORT
#       define USDPHYSICS_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPHYSICS_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDPHYSICS_API ARCH_IMPORT
#       define USDPHYSICS_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPHYSICS_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDPHYSICS_LOCAL ARCH_HIDDEN
#endif

#endif
