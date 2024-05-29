//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDHYDRA_API_H
#define USDHYDRA_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDHYDRA_API
#   define USDHYDRA_API_TEMPLATE_CLASS(...)
#   define USDHYDRA_API_TEMPLATE_STRUCT(...)
#   define USDHYDRA_LOCAL
#else
#   if defined(USDHYDRA_EXPORTS)
#       define USDHYDRA_API ARCH_EXPORT
#       define USDHYDRA_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDHYDRA_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDHYDRA_API ARCH_IMPORT
#       define USDHYDRA_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDHYDRA_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDHYDRA_LOCAL ARCH_HIDDEN
#endif

#endif
