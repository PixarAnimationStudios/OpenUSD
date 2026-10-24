//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_SAFE_PYTHON_H
#define PXR_BASE_TF_PY_SAFE_PYTHON_H

#include "pxr/base/arch/pragmas.h"

/// \file tf/pySafePython.h
/// Intended to replace a direct include of Python.h, which causes several
/// build problems with certain configurations and platforms (e.g., debug 
/// builds on Windows, Qt slots keyword, etc.)

// This include is a hack to avoid build errors due to incompatible
// macro definitions in pyport.h on MacOS for older versions of Python.
// See: https://bugs.python.org/issue10910
#include <locale>

// Python 3 has a conflict with the slots macro defined by the Qt library,
// so we're undef'ing here temporarily before including Python.
ARCH_PRAGMA_PUSH_MACRO(slots)
#undef slots

#if defined(_MSC_VER) && defined(_DEBUG) && !defined(Py_DEBUG)
#include <corecrt.h>
#undef _DEBUG
#include <Python.h>
#define _DEBUG 1
#else
#include <Python.h>
#endif

ARCH_PRAGMA_POP_MACRO(slots)

#include <nanobind/nb_defs.h>
#undef NB_NAMESPACE
#define NB_NAMESPACE nanobind
#if !defined(NB_BUILD)
#define NB_BUILD 1
#endif

#endif // PXR_BASE_TF_PY_SAFE_PYTHON_H
