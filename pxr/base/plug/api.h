//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_PLUG_API_H
#define PXR_BASE_PLUG_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define PLUG_API
#   define PLUG_API_TEMPLATE_CLASS(...)
#   define PLUG_API_TEMPLATE_STRUCT(...)
#   define PLUG_LOCAL
#else
#   if defined(PLUG_EXPORTS)
#       define PLUG_API ARCH_EXPORT
#       define PLUG_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define PLUG_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define PLUG_API ARCH_IMPORT
#       define PLUG_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define PLUG_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define PLUG_LOCAL ARCH_HIDDEN
#endif

#endif
