//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executionTypeRegistry.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(VdfExecutionTypeRegistry);

VdfExecutionTypeRegistry &
VdfExecutionTypeRegistry::GetInstance()
{
    return TfSingleton<VdfExecutionTypeRegistry>::GetInstance();
}

inline
VdfExecutionTypeRegistry::VdfExecutionTypeRegistry()
{
    TfSingleton<VdfExecutionTypeRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<VdfExecutionTypeRegistry>();
}

VdfExecutionTypeRegistry::~VdfExecutionTypeRegistry() = default;

TfType
VdfExecutionTypeRegistry::CheckForRegistration(const std::type_info &typeInfo)
{
    // Because VdfExecutionTypeRegistry::Define may also define types with
    // TfType, ensure that registration function subscription happens before
    // the lookup into TfType below.
    const VdfExecutionTypeRegistry &self = GetInstance();

    const TfType type = TfType::Find(typeInfo);
    if (ARCH_UNLIKELY(type.IsUnknown())) {
        TF_FATAL_ERROR("Type '%s' not registered with TfType",
                       ArchGetDemangled(typeInfo).c_str());
    }

    tbb::spin_rw_mutex::scoped_lock lock(
        self._fallbackMapMutex, /* write = */ false);
    if (self._fallbackMap.find(type) == self._fallbackMap.end()) {
        TF_FATAL_ERROR("No fallback value registered for \"%s\"",
            type.GetTypeName().c_str());
    }

    return type;
}

VdfVector
VdfExecutionTypeRegistry::CreateEmptyVector(TfType type)
{
    return GetInstance()._createEmptyVectorTable.Call<VdfVector>(type);
}

void
VdfExecutionTypeRegistry::FillVector(
    TfType type, size_t numElements, VdfVector *vector)
{
    const VdfExecutionTypeRegistry &self = GetInstance();
    const _Value &fallback = self._GetFallback(type);
    self._fillVectorDispatchTable.Call<bool>(
        type, fallback, numElements, vector);
}

std::pair<const VdfExecutionTypeRegistry::_Value &, bool>
VdfExecutionTypeRegistry::_InsertRegistration(TfType type, _Value &&fallback)
{
    if (ARCH_UNLIKELY(type.IsUnknown())) {
        TF_FATAL_ERROR("Attempted to register fallback value with "
                       "unknown type");
    }

    tbb::spin_rw_mutex::scoped_lock lock(
        _fallbackMapMutex, /* write = */ true);
    const auto [it, emplaced] = _fallbackMap.emplace(type, std::move(fallback));
    return {it->second, emplaced};
}

const VdfExecutionTypeRegistry::_Value &
VdfExecutionTypeRegistry::_GetFallback(TfType type) const
{
    tbb::spin_rw_mutex::scoped_lock lock(
        _fallbackMapMutex, /* write = */ false);
    const std::map<TfType, _Value>::const_iterator iter =
        _fallbackMap.find(type);

    if (iter == _fallbackMap.end()) {
        TF_FATAL_ERROR("No fallback value registered for \"%s\"",
            type.GetTypeName().c_str());
    }
    return iter->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
