//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_DEFINE_RESOLVER_CONTEXT_H
#define PXR_USD_AR_DEFINE_RESOLVER_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/resolverContext.h"

/// \file ar/defineResolverContext.h
/// Macros for defining an object for use with ArResolverContext

PXR_NAMESPACE_OPEN_SCOPE

/// \def AR_DECLARE_RESOLVER_CONTEXT
///
/// Declare that the specified ContextObject type may be used as an asset
/// resolver context object for ArResolverContext. This typically
/// would be invoked in the header where the ContextObject is
/// declared.
///
#ifdef doxygen
#define AR_DECLARE_RESOLVER_CONTEXT(ContextObject)
#else
#define AR_DECLARE_RESOLVER_CONTEXT(context)           \
template <>                                            \
struct ArIsContextObject<context>                      \
{                                                      \
    static const bool value = true;                    \
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_DEFINE_RESOLVER_CONTEXT_H
