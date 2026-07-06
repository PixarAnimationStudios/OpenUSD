//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPROFILES_API_H
#define USDPROFILES_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDPROFILES_API
#   define USDPROFILES_API_TEMPLATE_CLASS(...)
#   define USDPROFILES_API_TEMPLATE_STRUCT(...)
#   define USDPROFILES_LOCAL
#else
#   if defined(USDPROFILES_EXPORTS)
#       define USDPROFILES_API ARCH_EXPORT
#       define USDPROFILES_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPROFILES_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDPROFILES_API ARCH_IMPORT
#       define USDPROFILES_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPROFILES_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDPROFILES_LOCAL ARCH_HIDDEN
#endif

#endif
