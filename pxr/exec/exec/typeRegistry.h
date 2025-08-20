//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_TYPE_REGISTRY_H
#define PXR_EXEC_EXEC_TYPE_REGISTRY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/valueExtractorFunction.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/typeDispatchTable.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

#include <tbb/concurrent_unordered_map.h>

#include <algorithm>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_RegistrationBarrier;
class Exec_ValueExtractor;
class VdfMask;

/// Singleton used to register and access value types used by exec computations.
///
/// Value types that are used for exec computation input and output values must
/// be registered with this registry.
///
/// The registry is initialized with all value types that Sdf suports for
/// attribute and metadata values.
///
class ExecTypeRegistry
{
public:
    ExecTypeRegistry(ExecTypeRegistry const&) = delete;
    ExecTypeRegistry& operator=(ExecTypeRegistry const&) = delete;

    ~ExecTypeRegistry();

    /// Provides access to the singleton instance, first ensuring it is
    /// constructed.
    ///
    EXEC_API
    static const ExecTypeRegistry &GetInstance();

    /// Registers \p ValueType as a value type that exec computations can use
    /// for input and output values, with the fallback value \p fallback.
    ///
    /// In any circumstance that requires a fallback value, i.e., when an
    /// arbitrary value of type \p ValueType must be produced, \p fallback will
    /// be used.
    ///
    /// \warning
    /// If a given \p ValueType is registered more than once, all calls must
    /// specify the same \p fallback; otherwise, which fallback value wins is
    /// indeterminate. If an equality operator is defined for \p ValueType, that
    /// operator will be used to verify that all fallback values have the same
    /// value. Otherwise, multiple registrations are allowed, with no
    /// verification that the fallback values match.
    ///
    template <typename ValueType>
    static void RegisterType(const ValueType &fallback) {
        static_assert(!VtIsArray<ValueType>::value,
                      "VtArray is not a supported execution value type");
        _GetInstanceForRegistration()._RegisterType(fallback);
    }

    /// Confirms that \p ValueType has been registered.
    ///
    /// If \p ValueType has been registered with the ExecTypeRegistry, the
    /// corresponding TfType is returned.
    ///
    /// \warning
    /// If \p ValueType has not been registerd, a fatal error is emitted.
    ///
    template <typename ValueType>
    TfType CheckForRegistration() const {
        return VdfExecutionTypeRegistry::CheckForRegistration<ValueType>();
    }

    /// Construct a VdfVector whose value is copied from \p value.
    EXEC_API
    VdfVector CreateVector(const VtValue &value) const;

    /// Returns an extractor that produces a VtValue from values held in
    /// execution.
    /// 
    /// Note that \p type is the type that should be held in the VtValue
    /// extraction result.  This is distinct from the execution data-flow
    /// type.
    ///
    EXEC_API
    Exec_ValueExtractor GetExtractor(TfType type) const;

private:
    // Only TfSingleton can create instances.
    friend class TfSingleton<ExecTypeRegistry>;

    // Provides access for registraion of types only.
    EXEC_API
    static ExecTypeRegistry& _GetInstanceForRegistration();

    ExecTypeRegistry();

    template <typename ValueType>
    void _RegisterType(ValueType const &fallback);

    template <typename T>
    struct _CreateVector {
        // Interface for VdfTypeDispatchTable.
        static VdfVector Call(const VtValue &value) {
            return Create(value.UncheckedGet<T>());
        }
        // Typed implementation of CreateVector.
        //
        // This is separate from Call so that it can be shared with the
        // Vt known type optimization in CreateVector.
        static VdfVector Create(const T &value);
    };

    // Returns the appropriate value extractor for T.
    //
    // When T is a VtArray type, the returned extractor expects a VdfVector
    // holding T::value_type elements as its input.
    //
    template <typename T>
    static auto _MakeExtractorFunction();

    // Specify that values of \p type should be extracted using \p function.
    EXEC_API
    void _RegisterExtractor(
        TfType type,
        const Exec_ValueExtractorFunction &extractor);

private:
    std::unique_ptr<Exec_RegistrationBarrier> _registrationBarrier;

    VdfTypeDispatchTable<_CreateVector> _createVector;

    // Type-erased conversions from VdfVector to VtValue.
    //
    // Inside of execution, there is no distinction between a scalar value and
    // an array value of length 1.  However, systems that interact with
    // execution may desire single values be returned directly in VtValue or
    // as a VtValue holding a VtArray depending on the context.  The type key
    // specifies the type held in the resulting VtValue.  There are separate
    // extractors for T and VtArray<T> but they both accepts VdfVectors
    // holding T.
    //
    // Note that this must support the possibility that one thread is querying
    // extractors at the same time that another thread is registering
    // additional types.
    //
    tbb::concurrent_unordered_map<TfType, Exec_ValueExtractor, TfHash>
        _extractors;
};

template <typename ValueType>
void
ExecTypeRegistry::_RegisterType(ValueType const &fallback)
{
    const TfType type = VdfExecutionTypeRegistry::Define(fallback);

    // CreateVector has internal handling for value types known to Vt so we do
    // not need to register them here.
    if constexpr (!VtIsKnownValueType<ValueType>()) {
        _createVector.RegisterType<ValueType>();
    }

    _RegisterExtractor(type, *+_MakeExtractorFunction<ValueType>());
}

template <typename T>
VdfVector
ExecTypeRegistry::_CreateVector<T>::Create(const T &value)
{
    if constexpr (!VtIsArray<T>::value) {
        VdfVector v = VdfTypedVector<T>();
        v.Set(value);
        return v;
    }
    else {
        using ElementType = typename T::value_type;

        const size_t size = value.size();

        Vdf_BoxedContainer<ElementType> execValue(size);
        std::copy_n(value.cdata(), size, execValue.data());

        VdfVector v = VdfTypedVector<ElementType>();
        v.Set(std::move(execValue));
        return v;
    }
}

template <typename T>
auto
ExecTypeRegistry::_MakeExtractorFunction()
{
    if constexpr (!VtIsArray<T>::value) {
        return [](const VdfVector &v, const VdfMask::Bits &mask) {
            const VdfVector::ReadAccessor access =
                v.GetReadAccessor<T>();

            if (access.IsEmpty()) {
                TF_VERIFY(mask.GetNumSet() == 0);
                return VtValue();
            }

            if (!TF_VERIFY(mask.GetNumSet() == 1)) {
                return VtValue();
            }

            const int offset = mask.GetFirstSet();
            return VtValue(access[offset]);
        };
    }
    else {
        return [](const VdfVector &v, const VdfMask::Bits &mask) {
            using ElementType = typename T::value_type;

            if (!TF_VERIFY(mask.AreContiguouslySet())) {
                return VtValue();
            }

            const VdfVector::ReadAccessor access =
                v.GetReadAccessor<ElementType>();

            const int offset = mask.GetFirstSet();
            const size_t numValues = access.IsBoxed()
                ? access.GetNumValues()
                : mask.GetNumSet();
            return VtValue(v.ExtractAsVtArray<ElementType>(numValues, offset));
        };
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
