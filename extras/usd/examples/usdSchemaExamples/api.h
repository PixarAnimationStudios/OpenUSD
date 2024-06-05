//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSCHEMAEXAMPLES_API_H
#define USDSCHEMAEXAMPLES_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDSCHEMAEXAMPLES_API
#   define USDSCHEMAEXAMPLES_API_TEMPLATE_CLASS(...)
#   define USDSCHEMAEXAMPLES_API_TEMPLATE_STRUCT(...)
#   define USDSCHEMAEXAMPLES_LOCAL
#else
#   if defined(USDSCHEMAEXAMPLES_EXPORTS)
#       define USDSCHEMAEXAMPLES_API ARCH_EXPORT
#       define USDSCHEMAEXAMPLES_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSCHEMAEXAMPLES_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDSCHEMAEXAMPLES_API ARCH_IMPORT
#       define USDSCHEMAEXAMPLES_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDSCHEMAEXAMPLES_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDSCHEMAEXAMPLES_LOCAL ARCH_HIDDEN
#endif

#endif
