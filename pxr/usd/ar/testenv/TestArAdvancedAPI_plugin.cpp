//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/tf/diagnosticLite.h"

PXR_NAMESPACE_USING_DIRECTIVE

class _TestResolver1;
class _TestResolver2;

static bool
_HasType(const TfType& type, const std::vector<TfType>& types)
{
    return std::find(types.begin(), types.end(), type) != types.end();
}

class _TestResolver1 : public ArDefaultResolver
{
public:
    _TestResolver1()
    {
        printf("Constructing _TestResolver1\n");

        // _TestResolver1 is only ever constructed via _TestResolver2's
        // constructor. When it is being constructed, neither it nor
        // _TestResolver1 should appear in the result of 
        // ArGetAvailableResolvers().
        const std::vector<TfType> resolvers = ArGetAvailableResolvers();
        TF_AXIOM(!_HasType(TfType::Find<_TestResolver1>(), resolvers));
        TF_AXIOM(!_HasType(TfType::Find<_TestResolver2>(), resolvers));

        // ArDefaultResolver should always be the last element
        // in the available resolvers list.
        TF_AXIOM(resolvers.back() == TfType::Find<ArDefaultResolver>());
    }
};

class _TestResolver2 : public ArDefaultResolver
{
public:
    _TestResolver2()
    {
        printf("Constructing _TestResolver2\n");

        // When _TestResolver1 is being constructed, it should not 
        // appear in the result of ArGetAvailableResolvers().
        const std::vector<TfType> resolvers = ArGetAvailableResolvers();
        TF_AXIOM(_HasType(TfType::Find<_TestResolver1>(), resolvers));
        TF_AXIOM(!_HasType(TfType::Find<_TestResolver2>(), resolvers));

        // ArDefaultResolver should always be the last element
        // in the available resolvers list.
        TF_AXIOM(resolvers.back() == TfType::Find<ArDefaultResolver>());

        std::unique_ptr<ArResolver> subresolver = 
            ArCreateResolver(TfType::Find<_TestResolver1>());
        TF_AXIOM(dynamic_cast<_TestResolver1*>(subresolver.get()));
    }
};

AR_DEFINE_RESOLVER(_TestResolver1, ArDefaultResolver);
AR_DEFINE_RESOLVER(_TestResolver2, ArDefaultResolver);

