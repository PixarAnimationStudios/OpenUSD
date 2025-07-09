//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLIGHTFIELD_API_H
#define USDLIGHTFIELD_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDLIGHTFIELD_API
#   define USDLIGHTFIELD_API_TEMPLATE_CLASS(...)
#   define USDLIGHTFIELD_API_TEMPLATE_STRUCT(...)
#   define USDLIGHTFIELD_LOCAL
#else
#   if defined(USDLIGHTFIELD_EXPORTS)
#       define USDLIGHTFIELD_API ARCH_EXPORT
#       define USDLIGHTFIELD_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLIGHTFIELD_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDLIGHTFIELD_API ARCH_IMPORT
#       define USDLIGHTFIELD_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDLIGHTFIELD_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDLIGHTFIELD_LOCAL ARCH_HIDDEN
#endif

#endif
