//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDRENDER_API_H
#define USDRENDER_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDRENDER_API
#   define USDRENDER_API_TEMPLATE_CLASS(...)
#   define USDRENDER_API_TEMPLATE_STRUCT(...)
#   define USDRENDER_LOCAL
#else
#   if defined(USDRENDER_EXPORTS)
#       define USDRENDER_API ARCH_EXPORT
#       define USDRENDER_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDRENDER_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDRENDER_API ARCH_IMPORT
#       define USDRENDER_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDRENDER_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDRENDER_LOCAL ARCH_HIDDEN
#endif

#endif
