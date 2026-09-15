//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPMC_API_H
#define USDPMC_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDPMC_API
#   define USDPMC_API_TEMPLATE_CLASS(...)
#   define USDPMC_API_TEMPLATE_STRUCT(...)
#   define USDPMC_LOCAL
#else
#   if defined(USDPMC_EXPORTS)
#       define USDPMC_API ARCH_EXPORT
#       define USDPMC_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPMC_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDPMC_API ARCH_IMPORT
#       define USDPMC_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPMC_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDPMC_LOCAL ARCH_HIDDEN
#endif

#endif
