//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef TF_PYSTATICTOKENS_H
#define TF_PYSTATICTOKENS_H

/// \file tf/pyStaticTokens.h

#include "pxr/pxr.h"

#include <locale>

#include "pxr/base/tf/staticTokens.h"

#include <boost/bind.hpp>
#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>

PXR_NAMESPACE_OPEN_SCOPE

// TODO: Should wrap token arrays to Python.

/// Macro to wrap static tokens defined with \c TF_DEFINE_PUBLIC_TOKENS to
/// Python.  It creates a class of name \p name in the current scope
/// containing just the tokens in \p seq in the static tokens named by \p key.
/// Arrays are not wrapped but their components are.
///
/// \hideinitializer
#define TF_PY_WRAP_PUBLIC_TOKENS(name, key, seq)                            \
    boost::python::class_<_TF_TOKENS_STRUCT_NAME(key), boost::noncopyable>( \
            name, boost::python::no_init)                                   \
        _TF_PY_TOKENS_WRAP_SEQ(key, _TF_PY_TOKENS_EXPAND(seq))

/// Macro to wrap static tokens defined with \c TF_DEFINE_PUBLIC_TOKENS to
/// Python. This wraps tokens in \p seq in the static tokens named by \p key
/// as attributes on the current boost python scope. Arrays are not wrapped
/// but their components are.
///
/// \hideinitializer
#define TF_PY_WRAP_PUBLIC_TOKENS_IN_CURRENT_SCOPE(key, seq)                 \
    _TF_PY_TOKENS_WRAP_ATTR_SEQ(key, _TF_PY_TOKENS_EXPAND(seq))

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _TfPyWrapStaticToken {
public:
    _TfPyWrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

// Private macros to add a single data member.
#define _TF_PY_TOKENS_WRAP_ATTR_MEMBER(r, key, name)                        \
    boost::python::scope().attr(                                            \
        BOOST_PP_STRINGIZE(name)) = key->name.GetString();

#define _TF_PY_TOKENS_WRAP_MEMBER(r, key, name)                             \
    .add_static_property(BOOST_PP_STRINGIZE(name),                          \
        boost::python::make_function(_TfPyWrapStaticToken((&key->name)),    \
            boost::python::return_value_policy<                             \
                boost::python::return_by_value>(),                          \
            boost::mpl::vector1<std::string>()))

#define _TF_PY_TOKENS_EXPAND(seq)                                           \
    BOOST_PP_SEQ_FILTER(_TF_TOKENS_IS_NOT_ARRAY, ~, seq)                    \
    _TF_TOKENS_EXPAND_ARRAY_ELEMENTS(seq)

// Private macros to wrap a single element in a sequence.
#define _TF_PY_TOKENS_WRAP_ELEMENT(r, key, elem)                            \
    _TF_PY_TOKENS_WRAP_MEMBER(r, key, _TF_PY_TOKEN_GET_ELEM(elem))

#define _TF_PY_TOKENS_WRAP_ATTR_ELEMENT(r, key, elem)                       \
    _TF_PY_TOKENS_WRAP_ATTR_MEMBER(r, key, _TF_PY_TOKEN_GET_ELEM(elem))

#define _TF_PY_TOKEN_GET_ELEM(elem)                                         \
    BOOST_PP_IIF(TF_PP_IS_TUPLE(elem),                                      \
        BOOST_PP_TUPLE_ELEM(2, 0, elem), elem)

// Private macros to wrap a sequence.
#define _TF_PY_TOKENS_WRAP_SEQ(key, seq)                                    \
    BOOST_PP_SEQ_FOR_EACH(_TF_PY_TOKENS_WRAP_ELEMENT, key, seq)

#define _TF_PY_TOKENS_WRAP_ATTR_SEQ(key, seq)                               \
    BOOST_PP_SEQ_FOR_EACH(_TF_PY_TOKENS_WRAP_ATTR_ELEMENT, key, seq)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_PYSTATICTOKENS_H
