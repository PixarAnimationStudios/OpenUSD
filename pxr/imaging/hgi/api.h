//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_API_H
#define PXR_IMAGING_HGI_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HGI_API
#   define HGI_API_TEMPLATE_CLASS(...)
#   define HGI_API_TEMPLATE_STRUCT(...)
#   define HGI_LOCAL
#else
#   if defined(HGI_EXPORTS)
#       define HGI_API ARCH_EXPORT
#       define HGI_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGI_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HGI_API ARCH_IMPORT
#       define HGI_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGI_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HGI_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HGI_API_H
