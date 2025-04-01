//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_API_H
#define PXR_IMAGING_HGIPRESENT_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HGIPRESENT_API
#   define HGIPRESENT_API_TEMPLATE_CLASS(...)
#   define HGIPRESENT_API_TEMPLATE_STRUCT(...)
#   define HGIPRESENT_LOCAL
#else
#   if defined(HGIPRESENT_EXPORTS)
#       define HGIPRESENT_API ARCH_EXPORT
#       define HGIPRESENT_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIPRESENT_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HGIPRESENT_API ARCH_IMPORT
#       define HGIPRESENT_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIPRESENT_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HGIPRESENT_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HGIPRESENT_API_H
