//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_TYPE_DISPATCH_TABLE_H
#define PXR_EXEC_VDF_TYPE_DISPATCH_TABLE_H

/// \file

/// This file defines VdfTypeDispatchTable, a template that can be used to
/// perform runtime type dispatch.

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/tf/type.h"

#include <tbb/spin_rw_mutex.h>

#include <typeinfo>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Non-template-dependent part of dispatch table implementation.
class Vdf_TypeDispatchTableBase
{
public:
    /// Returns \c true if a function has been registered for type \p t.
    VDF_API
    bool IsTypeRegistered(TfType t) const;

protected:
    /// Register function pointer \p f as the implementation to dispatch to
    /// for type \p ti.
    VDF_API
    bool _RegisterType(const std::type_info &ti, void *f);

    /// Find a registered function pointer for type \p t.  Issues a fatal
    /// error if no function has been registered for type \p t.
    VDF_API
    void *_FindOrFatalError(TfType t) const;

    VDF_API
    Vdf_TypeDispatchTableBase();

    VDF_API
    ~Vdf_TypeDispatchTableBase();

private:
    // Type for the dispatch map, a map from keys to functions.
    using _MapType = pxr_tsl::robin_map<TfType, void *, TfHash>;

    // The type dispatch map.
    _MapType _map;

    // Guards the hash table buckets.
    mutable tbb::spin_rw_mutex _mutex;
};


/// Dispatches calls to template instantiations based on a TfType that is
/// determined at runtime.
///
/// Parameters:
///
/// \p Fn is a class template where \c Fn<T>::Call(...) is a static function to
///    be dispatched on type \c T.   Note that the function signature cannot
///    depend on the template argument and \c Fn<int>::Call(...) must be a
///    valid instantiation even if \c int is never registered as a type.
///
///
/// The given function is instantiated once for each of the types registered
/// using \c RegisterType.  The resulting function pointers are called using
/// the \c Call() member function.
///
///
/// Example:
///
/// Given this class template to be instantiated for each attribute type that
/// may be computed:
/// \code
///     template <typename T>
///     struct _ExtractExecValue {
///         static VtValue Call(const VdfVector &v, int offset)
///         {
///             const auto accessor = v.GetReadAccessor<T>();
///             return VtValue(accessor[offset]);
///         }
///     };
/// \endcode
///
/// This call defines a statically initialized type dispatch table that
/// dispatches calls to \c _ExtractExecValue, keyed off attribute types:
/// \code
///     TF_MAKE_STATIC_DATA(VdfTypeDispatchTable<_ExtractExecValue>,
///                         _extractTable)
///     {
///         _extractTable->RegisterType<AttributeType0>();
///         _extractTable->RegisterType<AttributeType1>();
///         // ...
///         _extractTable->RegisterType<AttributeTypeN>();
///     }
/// \endcode
///
/// This code calls an instance of \c _ExtractExecValue, keyed off the
/// type of \c attribute:
/// \code
///     VtValue value = _extractTable->Call<VtValue>()(
///         attribute.GetTypeName().GetType(), v, offset);
/// \endcode
///
template <template <typename> class Fn>
class VdfTypeDispatchTable
    : public Vdf_TypeDispatchTableBase
{
public:
    /// The signature of the functions dispatched by the table.
    ///
    /// \c Fn<int> is special only because it was the default instantiation
    /// used by the previous VdfTypeDispatchTable implementation to determine
    /// the function type for all \c Fn<T>.
    ///
    using FnSignature = decltype(Fn<int>::Call);

    /// Register an additional type with the type dispatch table.  This will
    /// return \c true, if \p Type has been successfully added to the dispatch 
    /// table and \c false if it was registered already.
    ///
    template <typename Type>
    bool RegisterType()
    {
        // Instantiate Fn for the given type, and put a pointer to it in
        // the type dispatch map.
        const std::type_info &ti = typeid(Type);
        FnSignature *f = &Fn<Type>::Call;
        return _RegisterType(ti, reinterpret_cast<void *>(f));
    }

    /// Invoke the function registered for \p key type.
    ///
    /// Calling this with an unregistered type is a fatal error.
    ///
    template <typename RetType, typename... Args>
    RetType Call(TfType key, Args&&... args) const
    {
        return reinterpret_cast<FnSignature *>(this->_FindOrFatalError(key))
            (std::forward<Args>(args)...);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
