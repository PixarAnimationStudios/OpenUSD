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
#ifndef AR_DEFINE_RESOLVER_H
#define AR_DEFINE_RESOLVER_H

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/base/tf/preprocessorUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/if.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \def AR_DEFINE_RESOLVER(ResolverClass, BaseClass1, BaseClass2, ...)
///
/// Performs registrations required for the specified resolver class
/// to be discovered by Ar's plugin mechanism. This typically would be
/// invoked in the source file defining the resolver class.
///
#define AR_DEFINE_RESOLVER(c, ...)                                  \
TF_REGISTRY_FUNCTION(TfType) {                                      \
    TfType t = TfType::Define<                                      \
        c BOOST_PP_COMMA_IF(TF_NUM_ARGS(__VA_ARGS__))               \
        BOOST_PP_IF(TF_NUM_ARGS(__VA_ARGS__),                       \
            TfType::Bases<__VA_ARGS__>, BOOST_PP_EMPTY) >();        \
    t.SetFactory<ArResolverFactory<c> >();                          \
}

class AR_API ArResolverFactoryBase : public TfType::FactoryBase {
public:
    virtual ArResolver* New() const = 0;
};

template <class T>
class ArResolverFactory : public ArResolverFactoryBase {
public:
    virtual ArResolver* New() const
    {
        return new T;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AR_DEFINE_RESOLVER_H
