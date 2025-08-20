//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/typeRegistry.h"

#include "pxr/exec/exec/registrationBarrier.h"
#include "pxr/exec/exec/valueExtractor.h"

#include "pxr/exec/ef/time.h"
#include "pxr/exec/vdf/mask.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/visitValue.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(ExecTypeRegistry);

const ExecTypeRegistry&
ExecTypeRegistry::GetInstance()
{
    ExecTypeRegistry& instance = TfSingleton<ExecTypeRegistry>::GetInstance();
    instance._registrationBarrier->WaitUntilFullyConstructed();
    return instance;
}

ExecTypeRegistry&
ExecTypeRegistry::_GetInstanceForRegistration()
{
    return TfSingleton<ExecTypeRegistry>::GetInstance();
}

inline
ExecTypeRegistry::ExecTypeRegistry()
    : _registrationBarrier(std::make_unique<Exec_RegistrationBarrier>())
{
    TRACE_FUNCTION();

    const SdfSchema& schema = SdfSchema::GetInstance();

    // Ensure that USD value types are registered before subscribing to our
    // registry function so that plugin type registration cannot override the
    // schema fallback value.

#define _EXEC_REGISTER_VALUE_TYPE(unused, elem)                         \
    {                                                                   \
        using ValueType = SDF_VALUE_CPP_TYPE(elem);                     \
        const TfType type = TfType::Find<ValueType>();                  \
        const SdfValueTypeName name = schema.FindType(type);            \
        const VtValue &def = name.GetDefaultValue();                    \
        if (TF_VERIFY(def.IsHolding<ValueType>())) {                    \
            const ValueType &value = def.UncheckedGet<ValueType>();     \
            _RegisterType(value);                                       \
            _RegisterType(SDF_VALUE_CPP_ARRAY_TYPE(elem)());            \
        }                                                               \
    }

    TF_PP_SEQ_FOR_EACH(_EXEC_REGISTER_VALUE_TYPE, ~, SDF_VALUE_TYPES);
#undef _EXEC_REGISTER_VALUE_TYPE

    _RegisterType(EfTime());
    _RegisterType(SdfPath());
    _RegisterType(VtArray<SdfPath>());

    TfSingleton<ExecTypeRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<ExecTypeRegistry>();
    _registrationBarrier->SetFullyConstructed();
}

ExecTypeRegistry::~ExecTypeRegistry() = default;

VdfVector
ExecTypeRegistry::CreateVector(const VtValue &value) const
{
    return VtVisitValue(value, [this](const auto &value) {
        using T = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;
        // Visitors must accept a VtValue argument to handle types that aren't
        // known to VtValue.  This is exactly the purpose of the type dispatch
        // table.
        if constexpr (std::is_same_v<VtValue, T>) {
            return _createVector.Call<VdfVector>(value.GetType(), value);
        }
        // Handle Vt's known value types.  We don't need to explicitly
        // enumerate them here as VtVisitValue will do so.
        else {
            return _CreateVector<T>::Create(value);
        }
    });
}

Exec_ValueExtractor
ExecTypeRegistry::GetExtractor(const TfType type) const
{
    if (const auto it = _extractors.find(type); it != _extractors.end()) {
        return it->second;
    }

    // We could check for an unknown type before looking up in extractors but
    // finding a known type is the expected case so we always do that first.
    if (type) {
        TF_CODING_ERROR("No extractor found for type '%s'",
                        type.GetTypeName().c_str());
    }
    else {
        TF_CODING_ERROR("No extractor found for unknown type");
    }
    return Exec_ValueExtractor();
}

void
ExecTypeRegistry::_RegisterExtractor(
    const TfType type, const Exec_ValueExtractorFunction &extractor)
{
    _extractors.emplace(type, extractor);
}

PXR_NAMESPACE_CLOSE_SCOPE
