//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TYPE_REGISTRY_H
#define PXR_BASE_TS_TYPE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/ts/data.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TsTypeRegistry
/// \brief Type registry which provides a mapping from dynamically typed
/// objects to statically typed internal ones.
///
/// A new type may be registered by using the TS_REGISTER_TYPE macro. ie:
///
///    TS_REGISTER_TYPE(double);
///
/// The type will also need to have a traits class defined for it. See Types.h
/// for example traits classes.
///
class TsTypeRegistry {
    TsTypeRegistry(const TsTypeRegistry&) = delete;
    TsTypeRegistry& operator=(const TsTypeRegistry&) = delete;

public:
    /// Return the single instance of TsTypeRegistry
    TS_API
    static TsTypeRegistry& GetInstance() {
        return TfSingleton<TsTypeRegistry>::GetInstance();
    }

    /// A TypedDataFactory is a function which initializes an
    /// Ts_PolymorphicDataHolder instance for a given VtValue.
    typedef void(*TypedDataFactory)(
        Ts_PolymorphicDataHolder *holder,
        const VtValue &value);

    /// Map from TfTypes to TypedDataFactories
    typedef TfHashMap<TfType,TypedDataFactory,TfHash> DataFactoryMap;

    /// Registers a TypedDataFactory for a particular type
    template <class T>
    void RegisterTypedDataFactory(TypedDataFactory factory) {
        _dataFactoryMap[TfType::Find<T>()] = factory;
    }

    /// Initialize an Ts_PolymorphicDataHolder so that it holds an
    /// Ts_TypedData of the appropriate type with the provided values.
    TS_API
    void InitializeDataHolder(
        Ts_PolymorphicDataHolder *holder,
        const VtValue &value);

    /// Returns true if the type of \e value is a type we can make keyframes
    /// for.
    TS_API
    bool IsSupportedType(const TfType &type) const;

private:
    // Private constructor. Only TfSingleton may create one
    TsTypeRegistry();
    virtual ~TsTypeRegistry();

    friend class TfSingleton<TsTypeRegistry>;

    DataFactoryMap _dataFactoryMap;
};

#define TS_REGISTER_TYPE(TYPE)                                           \
TF_REGISTRY_FUNCTION(TsTypeRegistry) {                                   \
    TsTypeRegistry &reg = TsTypeRegistry::GetInstance();                 \
    reg.RegisterTypedDataFactory<TYPE>(                                  \
        [](Ts_PolymorphicDataHolder *holder, const VtValue &value) {     \
            holder->New(value.Get<TYPE>());                              \
        });                                                              \
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
