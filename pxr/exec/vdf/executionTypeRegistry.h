//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTION_TYPE_REGISTRY_H
#define PXR_EXEC_VDF_EXECUTION_TYPE_REGISTRY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/outputSpec.h"
#include "pxr/exec/vdf/typeDispatchTable.h"

#include "pxr/base/tf/anyUniquePtr.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/type.h"

#include <tbb/spin_rw_mutex.h>

#include <map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Manages low-level value type functionality used within execution.
///
/// All types used as values in execution must be registered with
/// VdfExecutionTypeRegistry::Define.
///
/// All API is thread safe.
///
class VdfExecutionTypeRegistry
{
public:

    // Non-copyable
    //
    VdfExecutionTypeRegistry(const VdfExecutionTypeRegistry &) = delete;
    VdfExecutionTypeRegistry& operator=(
        const VdfExecutionTypeRegistry &) = delete;

    VDF_API
    ~VdfExecutionTypeRegistry();

    /// Returns the VdfExecutionTypeRegistry singleton instance.
    ///
    VDF_API
    static VdfExecutionTypeRegistry &GetInstance();

    /// Registers `T` with execution's runtime type dispatch system.
    ///
    /// \p fallback is required because execution supports types that are not
    /// default constructible as well as types for which a default-constructed
    /// value is not the desired fallback value.
    ///
    /// This call will also define `T` with TfType if it is not registered
    /// yet.
    ///
    template <typename T>
    static TfType Define(const T &fallback);

    /// Registers `T` with execution's runtime type dispatch system as a
    /// derived type of `B`.
    ///
    template <typename T, typename B>
    static TfType Define(const T &fallback);

    /// Returns the registered fallback value for `T` from the registry.
    ///
    /// It is a fatal error to query types that are not registered. 
    ///
    template <typename T>
    inline const T &GetFallback() const;

    /// Checks if `T` is defined as an execution value type.
    ///
    /// Returns the TfType of `T`. If the check fails, a fatal error will be
    /// issued. The intent is to make sure that all required types are
    /// registered at the time this method is called.
    ///
    template <typename T>
    static TfType CheckForRegistration() {
        return CheckForRegistration(typeid(T));
    }

    /// Checks if \p ti is defined as an execution value type.
    ///
    /// This method will issue a fatal error if the type isn't registered.
    ///
    VDF_API
    static TfType CheckForRegistration(const std::type_info &typeInfo);

    /// Create an empty VdfVector holding empty data of the given TfType.
    ///
    /// Note this creates an empty vector, not a fallback-valued vector.
    /// See also VdfTypedVector for creating empty vectors by type.
    ///
    VDF_API
    static VdfVector CreateEmptyVector(TfType type);

    /// Fills \p vector with the fallback value registered for the given type.
    ///
    VDF_API
    static void FillVector(TfType type, size_t numElements, VdfVector *vector);

private:

    friend class TfSingleton<VdfExecutionTypeRegistry>;
    VdfExecutionTypeRegistry();

    // Provides functionality common to both overloads of Define().
    template <typename T>
    void _Define(const T &fallback, TfType scalarType);

    // A very simple type-erased container used to hold fallback values.
    class _Value;

    // Inserts fallback as the value for type.
    //
    // Attempting to register a fallback with unknown type is a fatal error.
    //
    VDF_API
    std::pair<const _Value &, bool> _InsertRegistration(
        TfType type, _Value &&fallback);

    // Helper method to share code for looking up fallback values that is
    // common to all instantiations of GetFallback.
    //
    VDF_API
    const _Value &_GetFallback(TfType type) const;

    // Typed implementation of CreateEmptyVector.
    template <typename T>
    struct _CreateEmptyVector {
        static VdfVector Call() {
            return VdfTypedVector<T>();
        }
    };

    // Typed implementation of FillVector.
    template <typename T>
    struct _FillVector {
        static bool Call(
            const _Value &fallback, size_t numElements, VdfVector *vector);
    };

private:

    // Map of (scalar) value types to their type-erased fallback values and
    // mutex that serializes access to it.
    //
    // Writes occur once per type definition but reads occur many times during
    // various phases of execution.
    std::map<TfType, _Value> _fallbackMap;
    mutable tbb::spin_rw_mutex _fallbackMapMutex;

    VdfTypeDispatchTable<_CreateEmptyVector> _createEmptyVectorTable;
    VdfTypeDispatchTable<_FillVector> _fillVectorDispatchTable;
};

// A very simple type-erased container.
//
// This provides only functionality that is relevant to storing execution
// fallback values.  Other, more general type-erased containers can cause
// substantial compilation time increases because we store many types and
// their unused functionality must still be emitted.
//
class VdfExecutionTypeRegistry::_Value
{
public:
    template <typename T>
    explicit _Value(const T &fallback)
        : _ptr(TfAnyUniquePtr::New(fallback))
    {}

    // Returns a reference to the held value.
    //
    // There is no checked Get.  The registry must ensure that nobody is
    // able to register values for the wrong type.
    //
    template <typename T>
    const T & UncheckedGet() const {
        return *static_cast<const T*>(_ptr.Get());
    }

    // Compares values for types that define equality comparision; returns
    // true if not equality comparable.
    //
    template <typename T>
    bool Equals(const T &rhs) const {
        if constexpr (VdfIsEqualityComparable<T>) {
            return UncheckedGet<T>() == rhs;
        }
        else {
            return true;
        }
    }

private:
    TfAnyUniquePtr _ptr;
};

template <typename T>
TfType
VdfExecutionTypeRegistry::Define(const T &fallback)
{
    TfAutoMallocTag mallocTag("Vdf", "VdfExecutionTypeRegistry::Define");
    // Define type with Tf if needed.
    TfType scalarType = TfType::Find<T>();
    if (scalarType.IsUnknown() ||
        !TF_VERIFY(scalarType.GetTypeid() != typeid(void),
            "Type '%s' was declared but not defined",
            scalarType.GetTypeName().c_str())) {

        scalarType = TfType::Define<T>();
    }

    VdfExecutionTypeRegistry::GetInstance()._Define(fallback, scalarType);
    return scalarType;
}

template <typename T, typename B>
TfType
VdfExecutionTypeRegistry::Define(const T &fallback)
{
    TfAutoMallocTag mallocTag("Vdf", "VdfExecutionTypeRegistry::Define");
    // Define type with Tf if needed.
    TfType scalarType = TfType::Find<T>();
    if (scalarType.IsUnknown() ||
        !TF_VERIFY(scalarType.GetTypeid() != typeid(void),
            "Type '%s' was declared but not defined",
            scalarType.GetTypeName().c_str())) {

        scalarType = TfType::Define<T, B>();
    }

    GetInstance()._Define(fallback, scalarType);
    return scalarType;
}

template <typename T>
void
VdfExecutionTypeRegistry::_Define(const T &fallback, TfType scalarType)
{
    // Use the presence of an entry in the fallback map to avoid redundant
    // registration with other maps in cases where multiple plugins register
    // the same type, which is allowed.
    if (const auto [registeredFallback, inserted] = _InsertRegistration(
            scalarType, _Value(fallback)); !inserted) {
        TF_VERIFY(
            registeredFallback.Equals(fallback),
            "Type %s registered more than once with different fallback "
            "values.",
            scalarType.GetTypeName().c_str());
        return;
    }

    VdfOutputSpec::_RegisterType<T>();
    _createEmptyVectorTable.RegisterType<T>();
    _fillVectorDispatchTable.RegisterType<T>();
}

template <typename T>
const T &
VdfExecutionTypeRegistry::GetFallback() const
{
    const _Value &fallback = _GetFallback(TfType::Find<T>());
    return fallback.UncheckedGet<T>();
}

template <typename T>
bool
VdfExecutionTypeRegistry::_FillVector<T>::Call(
    const _Value &fallback,
    size_t numElements,
    VdfVector *vector)
{
    const T &fallbackValue = fallback.UncheckedGet<T>();

    vector->Resize<T>(numElements);
    VdfVector::ReadWriteAccessor<T> accessor = 
        vector->GetReadWriteAccessor<T>();
    for (size_t i = 0; i < numElements; ++i) {
        accessor[i] = fallbackValue;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
