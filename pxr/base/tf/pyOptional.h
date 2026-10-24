//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_OPTIONAL_H
#define PXR_BASE_TF_PY_OPTIONAL_H

/// \file tf/pyOptional.h

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"
#include <nanobind/stl/optional.h>

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

namespace TfPyOptional {

template <typename T>
struct python_optional
{
    python_optional(const python_optional&) = delete;
    python_optional& operator=(const python_optional&) = delete;
    explicit python_optional() = default;
};

} // namespace TfPyOptional

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_OPTIONAL_H
