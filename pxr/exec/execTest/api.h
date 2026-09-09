//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_TEST_API_H
#define PXR_EXEC_EXEC_TEST_API_H

/// \file

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

#include "pxr/base/arch/export.h"

#if defined(EXECTEST_EXPORTS)
#    define EXECTEST_API ARCH_EXPORT
#    define EXECTEST_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#    define EXECTEST_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#else
#    define EXECTEST_API ARCH_IMPORT
#    define EXECTEST_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#    define EXECTEST_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#endif
#define EXECTEST_LOCAL ARCH_HIDDEN

PXR_NAMESPACE_CLOSE_SCOPE

#endif