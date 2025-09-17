//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/dataSourceLegacyTaskPrim.h"

#include "pxr/imaging/hd/legacyTaskSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class _LegacyTaskSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LegacyTaskSchemaDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdLegacyTaskSchemaTokens->factory,
            HdLegacyTaskSchemaTokens->parameters,
            HdLegacyTaskSchemaTokens->collection,
            HdLegacyTaskSchemaTokens->renderTags
        };
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdLegacyTaskSchemaTokens->factory) {
            return _ToTypedDataSource(_factory);
        }

        if (name == HdLegacyTaskSchemaTokens->parameters) {
            return HdRetainedSampledDataSource::New(
                _sceneDelegate->Get(_id, HdTokens->params));
        }

        if (name == HdLegacyTaskSchemaTokens->collection) {
            return _Get<HdRprimCollection>(HdTokens->collection);
        }

        if (name == HdLegacyTaskSchemaTokens->renderTags) {
            return _Get<TfTokenVector>(HdTokens->renderTags);
        }

        return nullptr;
    }

private:
    _LegacyTaskSchemaDataSource(
        const SdfPath& id,
        HdSceneDelegate * const sceneDelegate,
        HdLegacyTaskFactorySharedPtr factory)
      : _id(id)
      , _sceneDelegate(sceneDelegate)
      , _factory(std::move(factory))
    {
    }

    template<typename T>
    static
    HdDataSourceBaseHandle
    _ToTypedDataSource(const T &v)
    {
        return HdRetainedTypedSampledDataSource<T>::New(v);
    }
    
    template<typename T>
    HdDataSourceBaseHandle
    _Get(const TfToken &key) const {
        const VtValue value = _sceneDelegate->Get(_id, key);
        if (!value.IsHolding<T>()) {
            return nullptr;
        }
        return _ToTypedDataSource(value.UncheckedGet<T>());
    }

    const SdfPath _id;
    HdSceneDelegate * const _sceneDelegate;
    HdLegacyTaskFactorySharedPtr const _factory;
};

}

HdDataSourceLegacyTaskPrim::HdDataSourceLegacyTaskPrim(
    const SdfPath& id,
    HdSceneDelegate * const sceneDelegate,
    HdLegacyTaskFactorySharedPtr factory)
  : _id(id)
  , _sceneDelegate(sceneDelegate)
  , _factory(std::move(factory))
{
}

HdDataSourceLegacyTaskPrim::~HdDataSourceLegacyTaskPrim() = default;

TfTokenVector
HdDataSourceLegacyTaskPrim::GetNames()
{
    static const TfTokenVector result = {
        HdLegacyTaskSchemaTokens->task
    };
    return result;
}

HdDataSourceBaseHandle
HdDataSourceLegacyTaskPrim::Get(const TfToken &name)
{
    if (!TF_VERIFY(_sceneDelegate)) {
        return nullptr;
    }

    if (name == HdLegacyTaskSchemaTokens->task) {
        return _LegacyTaskSchemaDataSource::New(_id, _sceneDelegate, _factory);
    }

    return nullptr;
}



PXR_NAMESPACE_CLOSE_SCOPE
