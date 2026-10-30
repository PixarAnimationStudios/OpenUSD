//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_PY_STATIC_TOKENS_H
#define PXR_BASE_TF_PY_STATIC_TOKENS_H

/// \file tf/pyStaticTokens.h

#include "pxr/pxr.h"

#include <locale>

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

#include "pxr/base/tf/pyNanobindBridge.h"

PXR_NAMESPACE_OPEN_SCOPE

#define _TF_PY_TOKEN_GET_ELEM(elem)                                         \
    _TF_PP_IFF(TF_PP_IS_TUPLE(elem),                                        \
        TF_PP_TUPLE_ELEM(0, elem), elem)

/// Macro to wrap static tokens defined with \c TF_DEFINE_PUBLIC_TOKENS to
/// Python using nanobind.
#define TF_NB_WRAP_PUBLIC_TOKENS(scope, name, key, seq)                     \
    nanobind::class_<_TF_TOKENS_STRUCT_NAME(key)>(scope, name)              \
        _TF_NB_TOKENS_WRAP_SEQ(key, seq)

#define _TF_NB_TOKENS_WRAP_MEMBER(r, key, name)                             \
    .def_prop_ro_static(TF_PP_STRINGIZE(name),                              \
        [](nanobind::handle) { return key->name.GetString(); })

#define _TF_NB_TOKENS_WRAP_ELEMENT(key, elem)                               \
    _TF_NB_TOKENS_WRAP_MEMBER(r, key, _TF_PY_TOKEN_GET_ELEM(elem))

#define _TF_NB_TOKENS_WRAP_SEQ(key, seq)                                    \
    TF_PP_SEQ_FOR_EACH(_TF_NB_TOKENS_WRAP_ELEMENT, key, seq)

#define TF_NB_WRAP_PUBLIC_TOKENS_IN_SCOPE(scope, key, seq)                  \
    (scope) _TF_NB_TOKENS_WRAP_SEQ(key, seq)

#define TF_PY_WRAP_PUBLIC_TOKENS(name, key, seq)                            \
    TF_NB_WRAP_PUBLIC_TOKENS(TfPyGetCurrentNanobindModule(), name, key, seq)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_STATIC_TOKENS_H
