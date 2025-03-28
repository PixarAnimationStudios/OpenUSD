//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSEMANTICS_API_H
#define USDSEMANTICS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDSEMANTICS_API
#   define USDSEMANTICS_API_TEMPLATE_CLASS(...)
#   define USDSEMANTICS_API_TEMPLATE_STRUCT(...)
#   define USDSEMANTICS_LOCAL
#else
#   if defined(USDSEMANTICS_EXPORTS)
#       define USDSEMANTICS_API ARCH_EXPORT
#       define USDSEMANTICS_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSEMANTICS_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDSEMANTICS_API ARCH_IMPORT
#       define USDSEMANTICS_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSEMANTICS_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDSEMANTICS_LOCAL ARCH_HIDDEN
#endif

#endif
