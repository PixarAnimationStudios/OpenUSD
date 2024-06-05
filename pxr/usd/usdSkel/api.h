//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSKEL_API_H
#define USDSKEL_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDSKEL_API
#   define USDSKEL_API_TEMPLATE_CLASS(...)
#   define USDSKEL_API_TEMPLATE_STRUCT(...)
#   define USDSKEL_LOCAL
#else
#   if defined(USDSKEL_EXPORTS)
#       define USDSKEL_API ARCH_EXPORT
#       define USDSKEL_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSKEL_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDSKEL_API ARCH_IMPORT
#       define USDSKEL_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSKEL_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDSKEL_LOCAL ARCH_HIDDEN
#endif

#endif
