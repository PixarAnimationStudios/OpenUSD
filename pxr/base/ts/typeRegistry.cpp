//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/typeRegistry.h"
#include "pxr/base/ts/data.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(TsTypeRegistry);

TsTypeRegistry::TsTypeRegistry() {
    // We have to mark this instance as constructed before calling
    // SubcribeTo<TsTypeRegistry>() below.
    TfSingleton<TsTypeRegistry>::SetInstanceConstructed(*this);    

    // Cause the registry to initialize
    TfRegistryManager::GetInstance().SubscribeTo<TsTypeRegistry>();
}

TsTypeRegistry::~TsTypeRegistry() {
    TfRegistryManager::GetInstance().UnsubscribeFrom<TsTypeRegistry>();
}

void
TsTypeRegistry::InitializeDataHolder(
        Ts_PolymorphicDataHolder *holder,
        const VtValue &value)
{
    static TypedDataFactory const &doubleDataFactory =
        _dataFactoryMap.find(TfType::Find<double>())->second;

    // Double-valued keyframes are super common, so special-case them here.
    if (ARCH_LIKELY(value.IsHolding<double>())) {
        doubleDataFactory(holder, value);
        return;
    }

    // Find a data factory for the type held by the VtValue
    // If it can't be found, see if we haven't yet loaded its plugin.
    DataFactoryMap::const_iterator i = _dataFactoryMap.find(value.GetType());
    if (ARCH_UNLIKELY(i == _dataFactoryMap.end())) {
        PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(value.GetType());
        if (plugin) {
            plugin->Load();
            // Try again to see if loading the plugin provided a factory.
            // Failing that, issue an error.
            i = _dataFactoryMap.find(value.GetType());
        }
        if (i == _dataFactoryMap.end()) {
            TF_CODING_ERROR("cannot create keyframes of type %s",
                            value.GetTypeName().c_str());
            holder->New(TsTraits<double>::zero);
            return;
        }
    }

    // Execute the data factory
    i->second(holder,value);
}

bool
TsTypeRegistry::IsSupportedType(const TfType &type) const
{
    return (_dataFactoryMap.find(type) != _dataFactoryMap.end());
}

// Will eventually be handled by TsSpline
TS_REGISTER_TYPE(double);
TS_REGISTER_TYPE(float);

// Will eventually be handled by TsLerpSeries
TS_REGISTER_TYPE(VtArray<double>);
TS_REGISTER_TYPE(VtArray<float>);
TS_REGISTER_TYPE(GfVec2d);
TS_REGISTER_TYPE(GfVec2f);
TS_REGISTER_TYPE(GfVec3d);
TS_REGISTER_TYPE(GfVec3f);
TS_REGISTER_TYPE(GfVec4d);
TS_REGISTER_TYPE(GfVec4f);
TS_REGISTER_TYPE(GfMatrix2d);
TS_REGISTER_TYPE(GfMatrix3d);
TS_REGISTER_TYPE(GfMatrix4d);

// Will eventually be handled by TsQuatSeries
TS_REGISTER_TYPE(GfQuatd);
TS_REGISTER_TYPE(GfQuatf);

// Will eventually be handled by TsHeldSeries
TS_REGISTER_TYPE(bool);
TS_REGISTER_TYPE(int);
TS_REGISTER_TYPE(std::string);
TS_REGISTER_TYPE(TfToken);


PXR_NAMESPACE_CLOSE_SCOPE
