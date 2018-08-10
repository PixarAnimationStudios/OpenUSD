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
#ifndef PLUG_STATICINTERFACE_H
#define PLUG_STATICINTERFACE_H

/// \file plug/staticInterface.h

#include "pxr/pxr.h"
#include "pxr/base/plug/api.h"

#include <atomic>
#include <type_traits>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

// Base class for common stuff.
class Plug_StaticInterfaceBase {
public:
    /// Returns \c true if we've tried to initialize the interface pointer,
    /// even if we failed.  This will not attempt to load the plugin or
    /// initialize the interface pointer.
    bool IsInitialized() const
    {
        return _initialized;
    }

#if !defined(doxygen)
    typedef void* Plug_StaticInterfaceBase::*UnspecifiedBoolType;
#endif

protected:
    PLUG_API
    void _LoadAndInstantiate(const std::type_info& type) const;

protected:
    // POD types only!
    mutable std::atomic<bool> _initialized;
    mutable void* _ptr;
};

/// \class PlugStaticInterface
///
/// Provides access to an interface into a plugin.
///
/// A plugin can provide one or more interface types through which clients
/// can access the plugin's full functionality without needing to link
/// against the plugin (if you had to link against it, it wouldn't be a
/// plugin).  This is a convenience;  you can achieve the same effect
/// with TfType::GetFactory().
///
/// Typical usage is:
/// \code
/// #include "ExtShipped/Core/SomePlugin.h"
/// #include "pxr/base/plug/staticInterface.h"
///
/// static PlugStaticInterface<SomePluginInterface> ptr;
///
/// void MyFunction() {
///     if (ptr) {
///         // Plugin is available.
///         ptr->MakePluginDoSomething();
///     }
///     else {
///         // Plugin is not available.  (An error will have been reported
///         // the first time through.)
///     }
/// }
/// \endcode
///
/// The interface must be defined correctly.  In particular, it must have
///
///   \li no data members (static or otherwise),
///   \li no static member functions,
///   \li no non-virtual member functions,
///   \li only pure virtual member functions,
///   \li no constructors except an inline protected default that does nothing
///       (or no constructors at all,)
///   \li a virtual destructor as the first member that \e must be defined
///       \e inline with an empty body even though inline virtual destructors
///       are contrary to conventional practice.
///
/// The last requirement causes the compiler to emit a copy of the interface
/// typeinfo into clients of the interface.  This typeinfo is required by
/// PlugStaticInterface internals to perform the appropriate plugin metadata
/// search for the interface type.  Note that due to limitations in the GCC
/// C++ ABI an inline virtual destructor may prevent dynamic_cast<> and typeid()
/// from working correctly; do not use those on the interface type.
///
/// For example:
/// 
/// \code
/// class SomePluginInterface {
/// public:
///     virtual ~SomePluginInterface() { }
///
///     virtual bool MakePluginDoSomething() const = 0;
///
/// protected:
///     SomePluginInterface() { }
/// };
/// \endcode
/// Note that interface types do not share a common base class.
///
/// For the plugin to work, there must be a concrete implementation of the
/// interface type, the interface type must be in plugInfo file, and the
/// interface type must be registered with TfType using
/// \c PLUG_REGISTER_INTERFACE_SINGLETON_TYPE:
/// \code
/// #include "ExtShipped/Core/SomePlugin.h"
/// #include "pxr/base/plug/interfaceFactory.h"
/// #include <iostream>
/// class SomePluginImplementation : public SomePluginInterface {
/// public:
///     virtual bool MakePluginDoSomething() const {
///         return std::cout << "Plugin did something" << std::endl;
///     }
/// };
/// PLUG_REGISTER_INTERFACE_SINGLETON_TYPE(SomePluginInterface,
///                                        SomePluginImplementation)
/// \endcode
/// This causes TfType::Find<SomePluginInterface>::Manufacture() to return
/// a pointer to a singleton instance of SomePluginImplementation.
///
/// Note that only SomePluginInterface needs to be registered in the plugInfo
/// file and with TfType; other types provided by the plugin need only be
/// defined in SomePlugin.h.  In addition, SomePluginInterface can provide
/// access to free functions in SomePlugin; clients would otherwise have to use
/// \c dlsym() to access free functions in the plugin.
///
/// Warning: the \c PlugStaticInterface construct relies upon zero-initialization
/// of global data: therefore, you can only use this structure for static data
/// member of classes, variables declared at file-scope, or variables declared
/// static within a function.  Do \e not declare a \c PlugStaticInterface object
/// as a local variable, as a member of a class or structure, or as a function
/// parameter.
///
template <class Interface>
class PlugStaticInterface : private Plug_StaticInterfaceBase {
public:
    static_assert(std::is_abstract<Interface>::value,
                  "Interface type must be abstract.");

    typedef PlugStaticInterface<Interface> This;

    using Plug_StaticInterfaceBase::IsInitialized;

    /// Load and instantiate then return \c true if the interface is valid,
    /// \c false otherwise.
    operator UnspecifiedBoolType() const
    {
        return _GetPtr() ? &This::_ptr : nullptr;
    }

    /// Load and instantiate then return \c false if the interface is valid,
    /// \c true otherwise.
    bool operator!() const
    {
        return !*this;
    }

    /// Returns the interface pointer, loading the plugin if necessary.
    /// Returns \c nullptr if the interface could not be initialized.
    Interface* Get() const
    {
        return _GetPtr();
    }

    /// Returns the interface pointer, loading the plugin if necessary.
    /// Returns \c nullptr if the interface could not be initialized.
    Interface* operator->() const
    {
        return _GetPtr();
    }

    /// Returns the interface pointer as a reference, loading the plugin
    /// if necessary.  Returns \c nullptr if the interface could not be
    /// initialized.
    Interface& operator*() const
    {
        return *_GetPtr();
    }

private:
    Interface* _GetPtr() const
    {
        if (!_initialized) {
            _LoadAndInstantiate(typeid(Interface));
        }

        // XXX: We must assume _ptr has the right type since we have
        //      no common base class to dynamic_cast<> from.
        return static_cast<Interface*>(_ptr);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PLUG_STATICINTERFACE_H
