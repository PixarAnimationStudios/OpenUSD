//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXTRAS_IMAGING_EXAMPLES_HDUI_API_H
#define EXTRAS_IMAGING_EXAMPLES_HDUI_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDUI_API
#   define HDUI_API_TEMPLATE_CLASS(...)
#   define HDUI_API_TEMPLATE_STRUCT(...)
#   define HDUI_LOCAL
#else
#   if defined(HDUI_EXPORTS)
#       define HDUI_API ARCH_EXPORT
#       define HDUI_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDUI_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDUI_API ARCH_IMPORT
#       define HDUI_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDUI_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDUI_LOCAL ARCH_HIDDEN
#endif

#define HDUI_API_CLASS HDUI_API

#endif // EXTRAS_IMAGING_EXAMPLES_HDUI_API_H
