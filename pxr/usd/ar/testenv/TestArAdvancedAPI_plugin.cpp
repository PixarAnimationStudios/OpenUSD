//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_USING_DIRECTIVE

class _TestResolver1;
class _TestResolver2;

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
        const std::vector<TfType> expected = {
            TfType::Find<ArDefaultResolver>()
        };

        TF_AXIOM(resolvers == expected);
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
        const std::vector<TfType> expected = {
            TfType::Find<_TestResolver1>(), 
            TfType::Find<ArDefaultResolver>()
        };

        TF_AXIOM(resolvers == expected);

        std::unique_ptr<ArResolver> subresolver = 
            ArCreateResolver(resolvers.front());
        TF_AXIOM(dynamic_cast<_TestResolver1*>(subresolver.get()));
    }
};

AR_DEFINE_RESOLVER(_TestResolver1, ArDefaultResolver);
AR_DEFINE_RESOLVER(_TestResolver2, ArDefaultResolver);

