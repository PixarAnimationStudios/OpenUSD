//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDUI_API_H
#define USDUI_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDUI_API
#   define USDUI_API_TEMPLATE_CLASS(...)
#   define USDUI_API_TEMPLATE_STRUCT(...)
#   define USDUI_LOCAL
#else
#   if defined(USDUI_EXPORTS)
#       define USDUI_API ARCH_EXPORT
#       define USDUI_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDUI_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDUI_API ARCH_IMPORT
#       define USDUI_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDUI_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDUI_LOCAL ARCH_HIDDEN
#endif

#endif
