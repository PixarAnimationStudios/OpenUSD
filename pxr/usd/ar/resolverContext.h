//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_RESOLVER_CONTEXT_H
#define PXR_USD_AR_RESOLVER_CONTEXT_H

/// \file ar/resolverContext.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/ar.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/safeTypeCompare.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
// XXX: This include is a hack to avoid build errors due to
// incompatible macro definitions in pyport.h on macOS.
#include <locale>
#include "pxr/base/tf/pyLock.h"
#endif

#include "pxr/base/tf/pyObjWrapper.h"

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArIsContextObject
///
/// Metafunction to determine whether the templated object type is a
/// valid context object.
///
template <class T>
struct ArIsContextObject
{
    static const bool value = false;
};

/// Default implementation for providing debug info on the contained context. 
template <class Context> 
std::string ArGetDebugString(const Context& context); 

// Metafunctions for determining if a variadic list of objects
// are valid for use with the ArResolverContext c'tor.
class ArResolverContext;

template <class ...Objects> struct Ar_AllValidForContext;

template <class Object, class ...Other>
struct Ar_AllValidForContext<Object, Other...>
{
    static const bool value = 
        (std::is_same<Object, ArResolverContext>::value ||
         ArIsContextObject<Object>::value) &&
        Ar_AllValidForContext<Other...>::value;
};

template <>
struct Ar_AllValidForContext<>
{
    static const bool value = true;
};

/// \class ArResolverContext
///
/// An asset resolver context allows clients to provide additional data
/// to the resolver for use during resolution. Clients may provide this
/// data via context objects of their own (subject to restrictions below).
/// An ArResolverContext is simply a wrapper around these objects that
/// allows it to be treated as a single type. Note that an ArResolverContext
/// may not hold multiple context objects with the same type.
///
/// A client-defined context object must provide the following:
///   - Default and copy constructors
///   - operator<
///   - operator==
///   - An overload for size_t hash_value(const T&)
///
/// Note that the user may define a free function:
///
/// std::string ArGetDebugString(const Context& ctx);
/// (Where Context is the type of the user's path resolver context.)
/// 
/// This is optional; a default generic implementation has been predefined.
/// This function should return a string representation of the context
/// to be utilized for debugging purposes(such as in TF_DEBUG statements).
///
/// The ArIsContextObject template must also be specialized for this
/// object to declare that it can be used as a context object. This is to 
/// avoid accidental use of an unexpected object as a context object.
/// The AR_DECLARE_RESOLVER_CONTEXT macro can be used to do this
/// as a convenience.
/// 
/// \sa AR_DECLARE_RESOLVER_CONTEXT
/// \sa ArResolver::BindContext
/// \sa ArResolver::UnbindContext
/// \sa ArResolverContextBinder
class ArResolverContext
{
public:
    /// Construct an empty asset resolver context.
    ArResolverContext()
    {
    }

    /// Construct a resolver context using the given objects \p objs.
    ///
    /// Each argument must either be an ArResolverContext or a registered
    /// context object. See class documentation for requirements on context
    /// objects.
    ///
    /// If an argument is a context object, it will be added to the
    /// constructed ArResolverContext. If an argument is an ArResolverContext,
    /// all of the context objects it holds will be added to the constructed
    /// ArResolverContext.
    ///
    /// Arguments are ordered from strong-to-weak. If a context object is
    /// encountered with the same type as a previously-added object, the
    /// previously-added object will remain and the other context object
    /// will be ignored.
    template <
        class ...Objects,
        typename std::enable_if<Ar_AllValidForContext<Objects...>::value>::type*
            = nullptr>
    ArResolverContext(const Objects&... objs)
    {
        _AddObjects(objs...);
    }

    /// Construct a resolver context using the ArResolverContexts in \p ctxs.
    ///
    /// All of the context objects held by each ArResolverContext in \p ctxs
    /// will be added to the constructed ArResolverContext.
    ///
    /// Arguments are ordered from strong-to-weak. If a context object is
    /// encountered with the same type as a previously-added object, the
    /// previously-added object will remain and the other context object
    /// will be ignored.
    AR_API
    explicit ArResolverContext(const std::vector<ArResolverContext>& ctxs);

    /// Returns whether this resolver context is empty.
    bool IsEmpty() const
    {
        return _contexts.empty();
    }

    /// Returns pointer to the context object of the given type
    /// held in this resolver context. Returns NULL if this resolver
    /// context is not holding an object of the requested type.
    template <class ContextObj>
    const ContextObj* Get() const
    {
        for (const auto& context : _contexts) {
            if (context->IsHolding(typeid(ContextObj))) {
                return &_GetTyped<ContextObj>(*context)._context;
            }
        }
        return nullptr;
    }

    /// Returns a debug string representing the contained context objects.
    AR_API
    std::string GetDebugString() const;

    /// \name Operators
    /// @{
    AR_API
    bool operator==(const ArResolverContext& rhs) const;

    bool operator!=(const ArResolverContext& rhs) const
    {
        return !(*this == rhs);
    }

    AR_API
    bool operator<(const ArResolverContext& rhs) const;

    /// @}

    /// Returns hash value for this asset resolver context.
    friend size_t hash_value(const ArResolverContext& context)
    {
        return TfHash()(context._contexts);
    }

private:
    // Type-erased storage for context objects.
    struct _Untyped;
    template <class Context> struct _Typed;

    void _AddObjects()
    {
        // Empty base case for unpacking parameter pack
    }

    template <class Object, class ...Other>
    void _AddObjects(const Object& obj, const Other&... other)
    {
        _Add(obj);
        _AddObjects(other...);
    }

    AR_API
    void _Add(const ArResolverContext& ctx);

    template <class Object>
    void _Add(const Object& obj)
    {
        _Add(std::shared_ptr<_Untyped>(new _Typed<Object>(obj)));
    }

    AR_API
    void _Add(std::shared_ptr<_Untyped>&& context);

    template <class Context> 
    static const _Typed<Context>& _GetTyped(const _Untyped& untyped)
    {
        return static_cast<const _Typed<Context>&>(untyped);
    }

    struct _Untyped 
    {
        AR_API
        virtual ~_Untyped();

        bool IsHolding(const std::type_info& ti) const
        {
            return TfSafeTypeCompare(ti, GetTypeid());
        }

        virtual _Untyped* Clone() const = 0;
        virtual const std::type_info& GetTypeid() const = 0;
        virtual bool LessThan(const _Untyped& rhs) const = 0;
        virtual bool Equals(const _Untyped& rhs) const = 0;
        virtual size_t Hash() const = 0;
        virtual std::string GetDebugString() const = 0;
        virtual TfPyObjWrapper GetPythonObj() const = 0;
    };

    template <class Context>
    struct _Typed : public _Untyped
    {
        virtual ~_Typed() { }

        _Typed(const Context& context) : _context(context)
        { 
        }

        virtual _Untyped* Clone() const
        {
            return new _Typed<Context>(_context);
        }

        virtual const std::type_info& GetTypeid() const
        {
            return typeid(Context);
        }

        virtual bool LessThan(const _Untyped& rhs) const
        {
            return _context < _GetTyped<Context>(rhs)._context;
        }

        virtual bool Equals(const _Untyped& rhs) const
        {
            return _context == _GetTyped<Context>(rhs)._context;
        }

        virtual size_t Hash() const
        {
            return hash_value(_context);
        }

        virtual std::string GetDebugString() const 
        {
            return ArGetDebugString(_context);            
        }

        virtual TfPyObjWrapper GetPythonObj() const
        {
 #ifdef PXR_PYTHON_SUPPORT_ENABLED
            TfPyLock lock;
            return pxr_boost::python::object(_context);
#else
            return {};
#endif
        }

        Context _context;
    };

    template <class HashState>
    friend void TfHashAppend(
        HashState& h, const std::shared_ptr<_Untyped>& context)
    {
        h.Append(context->Hash());
    }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    friend class Ar_ResolverContextPythonAccess;
#endif

    std::vector<std::shared_ptr<_Untyped>> _contexts;
};


// Default implementation for streaming out held contexts.
AR_API
std::string Ar_GetDebugString(const std::type_info&, void const*);

template <class Context>
std::string ArGetDebugString(const Context& context) 
{
    return Ar_GetDebugString(typeid(Context),
                             static_cast<void const*>(&context));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
