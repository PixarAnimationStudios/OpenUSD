//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDTEXT_API_H
#define USDTEXT_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDTEXT_API
#   define USDTEXT_API_TEMPLATE_CLASS(...)
#   define USDTEXT_API_TEMPLATE_STRUCT(...)
#   define USDTEXT_LOCAL
#else
#   if defined(USDTEXT_EXPORTS)
#       define USDTEXT_API ARCH_EXPORT
#       define USDTEXT_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDTEXT_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDTEXT_API ARCH_IMPORT
#       define USDTEXT_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDTEXT_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDTEXT_LOCAL ARCH_HIDDEN
#endif

#endif
