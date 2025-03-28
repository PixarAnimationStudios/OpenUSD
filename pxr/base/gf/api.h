//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_API_H
#define PXR_BASE_GF_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define GF_API
#   define GF_API_TEMPLATE_CLASS(...)
#   define GF_API_TEMPLATE_STRUCT(...)
#   define GF_LOCAL
#else
#   if defined(GF_EXPORTS)
#       define GF_API ARCH_EXPORT
#       define GF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define GF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define GF_API ARCH_IMPORT
#       define GF_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define GF_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define GF_LOCAL ARCH_HIDDEN
#endif

#endif
