//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_API_H
#define PXR_IMAGING_HGI_METAL_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HGIMETAL_API
#   define HGIMETAL_API_TEMPLATE_CLASS(...)
#   define HGIMETAL_API_TEMPLATE_STRUCT(...)
#   define HGIMETAL_LOCAL
#else
#   if defined(HGIMETAL_EXPORTS)
#       define HGIMETAL_API ARCH_EXPORT
#       define HGIMETAL_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIMETAL_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HGIMETAL_API ARCH_IMPORT
#       define HGIMETAL_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIMETAL_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HGIMETAL_LOCAL ARCH_HIDDEN
#endif

#endif

