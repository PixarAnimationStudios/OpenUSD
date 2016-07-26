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
#ifndef AR_DEFINE_RESOLVER_CONTEXT_H
#define AR_DEFINE_RESOLVER_CONTEXT_H

#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/pyResolverContext.h"

/// \def AR_DECLARE_RESOLVER_CONTEXT(ContextObject)
/// Declare that the specified ContextObject type may be used as an asset
/// resolver context object for ArResolverContext. This typically
/// would be invoked in the header where the ContextObject is
/// declared.
#define AR_DECLARE_RESOLVER_CONTEXT(context)           \
template <>                                            \
struct ArIsContextObject<context>                      \
{                                                      \
    static const bool value = true;                    \
}

/// Register the specified type as a context object that may be
/// converted from a Python into a ArResolverContext object
/// in C++ and vice versa. This typically would be called in the
/// source file where the Python wrapping for the context object
/// is defined.
template <class Context>
void 
ArWrapResolverContextForPython();

#endif // AR_DEFINE_RESOLVER_CONTEXT_H
