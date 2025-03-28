//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_PLUG_INTERFACE_FACTORY_H
#define PXR_BASE_PLUG_INTERFACE_FACTORY_H

/// \file plug/interfaceFactory.h

#include "pxr/pxr.h"
#include "pxr/base/tf/type.h"

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
            static_assert(std::is_abstract<Interface>::value,
                          "Interface type must be abstract.");
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

#endif // PXR_BASE_PLUG_INTERFACE_FACTORY_H
