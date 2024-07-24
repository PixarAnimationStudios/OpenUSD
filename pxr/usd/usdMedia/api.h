//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDMEDIA_API_H
#define USDMEDIA_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDMEDIA_API
#   define USDMEDIA_API_TEMPLATE_CLASS(...)
#   define USDMEDIA_API_TEMPLATE_STRUCT(...)
#   define USDMEDIA_LOCAL
#else
#   if defined(USDMEDIA_EXPORTS)
#       define USDMEDIA_API ARCH_EXPORT
#       define USDMEDIA_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDMEDIA_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDMEDIA_API ARCH_IMPORT
#       define USDMEDIA_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDMEDIA_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDMEDIA_LOCAL ARCH_HIDDEN
#endif

#endif
