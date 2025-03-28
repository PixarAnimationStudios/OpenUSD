//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_API_H
#define PXR_IMAGING_HGI_GL_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HGIGL_API
#   define HGIGL_API_TEMPLATE_CLASS(...)
#   define HGIGL_API_TEMPLATE_STRUCT(...)
#   define HGIGL_LOCAL
#else
#   if defined(HGIGL_EXPORTS)
#       define HGIGL_API ARCH_EXPORT
#       define HGIGL_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIGL_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HGIGL_API ARCH_IMPORT
#       define HGIGL_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIGL_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HGIGL_LOCAL ARCH_HIDDEN
#endif

#endif

