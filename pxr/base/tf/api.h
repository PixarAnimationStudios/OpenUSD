//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_API_H
#define PXR_BASE_TF_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define TF_API
#   define TF_API_TEMPLATE_CLASS(...)
#   define TF_API_TEMPLATE_STRUCT(...)
#   define TF_LOCAL
#else
#   if defined(TF_EXPORTS)
#       define TF_API ARCH_EXPORT
#       define TF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define TF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define TF_API ARCH_IMPORT
#       define TF_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define TF_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define TF_LOCAL ARCH_HIDDEN
#endif

#endif
