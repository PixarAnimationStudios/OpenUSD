//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIOOPENVDB_API_H
#define PXR_IMAGING_HIOOPENVDB_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HIOOPENVDB_API
#   define HIOOPENVDB_API_TEMPLATE_CLASS(...)
#   define HIOOPENVDB_API_TEMPLATE_STRUCT(...)
#   define HIOOPENVDB_LOCAL
#else
#   if defined(HIOOPENVDB_EXPORTS)
#       define HIOOPENVDB_API ARCH_EXPORT
#       define HIOOPENVDB_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HIOOPENVDB_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HIOOPENVDB_API ARCH_IMPORT
#       define HIOOPENVDB_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HIOOPENVDB_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HIOOPENVDB_LOCAL ARCH_HIDDEN
#endif

#endif
