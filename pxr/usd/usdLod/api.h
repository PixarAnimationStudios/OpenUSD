//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_API_H
#define USDLOD_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDLOD_API
#   define USDLOD_API_TEMPLATE_CLASS(...)
#   define USDLOD_API_TEMPLATE_STRUCT(...)
#   define USDLOD_LOCAL
#else
#   if defined(USDLOD_EXPORTS)
#       define USDLOD_API ARCH_EXPORT
#       define USDLOD_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLOD_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDLOD_API ARCH_IMPORT
#       define USDLOD_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLOD_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDLOD_LOCAL ARCH_HIDDEN
#endif

#endif
