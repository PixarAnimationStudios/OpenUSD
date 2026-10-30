//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_MAKE_PY_CONSTRUCTOR_H
#define PXR_BASE_TF_MAKE_PY_CONSTRUCTOR_H

/// \file tf/makePyConstructor.h
/// An injected constructor mechanism that works with polymorphic wrapped
/// classes.

#ifndef TF_MAX_ARITY
#  define TF_MAX_ARITY 7
#endif // TF_MAX_ARITY


#include "pxr/pxr.h"
#include "pxr/base/tf/pyPtrHelpers.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename Func>
auto TfMakePyConstructor(Func &&func) {
    return TfNbMakePyConstructor(std::forward<Func>(func));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_MAKE_PY_CONSTRUCTOR_H
