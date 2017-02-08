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
#ifndef PLUG_INTERFACEFACTORY_H
#define PLUG_INTERFACEFACTORY_H

/// \file plug/interfaceFactory.h

#include "pxr/pxr.h"
#include "pxr/base/tf/type.h"
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_abstract.hpp>

PXR_NAMESPACE_OPEN_SCOPE

// For use by \c PLUG_REGISTER_INTERFACE_SINGLETON_TYPE.
class Plug_InterfaceFactory {
public:
    struct Base : public TfType::FactoryBase {
    public:
        virtual void* New() = 0;
    };

    template <class Interface, class Implementation>
    struct SingletonFactory : public Base {
    public:
        virtual void* New()
        {
            BOOST_STATIC_ASSERT(boost::is_abstract<Interface>::value);
            static Implementation impl;
            return static_cast<Interface*>(&impl);
        }
    };
};

/// Defines the \c Interface \c TfType with a factory to return a
/// \c Implementation singleton.  This is suitable for use with
/// \c PlugStaticInterface. \p Interface must be abstract and
/// \p Implementation a concrete subclass of \p Interface.  Note
/// that this is a factory on \c Interface \b not \c Implementation.
///
/// The result of the factory is a singleton instance of \c Implementation
/// and the client of TfType::GetFactory() must not destroy it.
///
/// Clients that want to create instances of types defined in a plugin
/// but not added to the TfType system should create a singleton with
/// factory methods to create those objects.
#define PLUG_REGISTER_INTERFACE_SINGLETON_TYPE(Interface, Implementation)   \
TF_REGISTRY_FUNCTION(TfType)                                                \
{                                                                           \
    TfType::Define<Interface>()                                             \
        .SetFactory<Plug_InterfaceFactory::SingletonFactory<                \
            Interface, Implementation> >();                                 \
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PLUG_INTERFACEFACTORY_H
