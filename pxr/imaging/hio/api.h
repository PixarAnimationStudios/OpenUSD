//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_API_H
#define PXR_IMAGING_HIO_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HIO_API
#   define HIO_API_TEMPLATE_CLASS(...)
#   define HIO_API_TEMPLATE_STRUCT(...)
#   define HIO_LOCAL
#else
#   if defined(HIO_EXPORTS)
#       define HIO_API ARCH_EXPORT
#       define HIO_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HIO_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HIO_API ARCH_IMPORT
#       define HIO_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HIO_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HIO_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HIO_API_H
