//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_PY_BUFFER_UTILS_H
#define PXR_BASE_GF_PY_BUFFER_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// Format strings matching Python buffer proto / struct module scheme.

// This function template is explicitly instantiated for T =
//    bool, [unsigned] (char, short, int, long), half, float, and double.
template <class T>
char *Gf_GetPyBufferFmtFor();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_PY_BUFFER_UTILS_H
