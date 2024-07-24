//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef {{ Upper(libraryName) }}_API_H
#define {{ Upper(libraryName) }}_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define {{ Upper(libraryName) }}_API
#   define {{ Upper(libraryName) }}_API_TEMPLATE_CLASS(...)
#   define {{ Upper(libraryName) }}_API_TEMPLATE_STRUCT(...)
#   define {{ Upper(libraryName) }}_LOCAL
#else
#   if defined({{ Upper(libraryName) }}_EXPORTS)
#       define {{ Upper(libraryName) }}_API ARCH_EXPORT
#       define {{ Upper(libraryName) }}_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define {{ Upper(libraryName) }}_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define {{ Upper(libraryName) }}_API ARCH_IMPORT
#       define {{ Upper(libraryName) }}_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define {{ Upper(libraryName) }}_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define {{ Upper(libraryName) }}_LOCAL ARCH_HIDDEN
#endif

#endif
