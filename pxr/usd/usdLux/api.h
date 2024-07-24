//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLUX_API_H
#define USDLUX_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDLUX_API
#   define USDLUX_API_TEMPLATE_CLASS(...)
#   define USDLUX_API_TEMPLATE_STRUCT(...)
#   define USDLUX_LOCAL
#else
#   if defined(USDLUX_EXPORTS)
#       define USDLUX_API ARCH_EXPORT
#       define USDLUX_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLUX_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDLUX_API ARCH_IMPORT
#       define USDLUX_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLUX_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDLUX_LOCAL ARCH_HIDDEN
#endif

#endif
