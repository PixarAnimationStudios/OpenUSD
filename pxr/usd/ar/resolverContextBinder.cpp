//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

ArResolverContextBinder::ArResolverContextBinder(
    ArResolver* resolver,
    const ArResolverContext& context)
    : _resolver(resolver)
    , _context(context)
{
    if (_resolver) {
        _resolver->BindContext(_context, &_bindingData);
    }
}

ArResolverContextBinder::ArResolverContextBinder(
    const ArResolverContext& context)
    : _resolver(&ArGetResolver())
    , _context(context)
{
    if (_resolver) {
        _resolver->BindContext(_context, &_bindingData);
    }
}

ArResolverContextBinder::~ArResolverContextBinder()
{
    if (_resolver) {
        _resolver->UnbindContext(_context, &_bindingData);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
