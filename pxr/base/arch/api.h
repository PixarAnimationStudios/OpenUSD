//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_API_H
#define PXR_BASE_ARCH_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define ARCH_API
#   define ARCH_API_TEMPLATE_CLASS(...)
#   define ARCH_API_TEMPLATE_STRUCT(...)
#   define ARCH_LOCAL
#else
#   if defined(ARCH_EXPORTS)
#       define ARCH_API ARCH_EXPORT
#       define ARCH_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define ARCH_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define ARCH_API ARCH_IMPORT
#       define ARCH_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define ARCH_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define ARCH_LOCAL ARCH_HIDDEN
#endif

#endif
