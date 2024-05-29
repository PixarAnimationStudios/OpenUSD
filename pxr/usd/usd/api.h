//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USD_API_H
#define USD_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USD_API
#   define USD_API_TEMPLATE_CLASS(...)
#   define USD_API_TEMPLATE_STRUCT(...)
#   define USD_LOCAL
#else
#   if defined(USD_EXPORTS)
#       define USD_API ARCH_EXPORT
#       define USD_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USD_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USD_API ARCH_IMPORT
#       define USD_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USD_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USD_LOCAL ARCH_HIDDEN
#endif

#endif
