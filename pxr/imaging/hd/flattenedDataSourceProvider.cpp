//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/flattenedDataSourceProvider.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

static
HdContainerDataSourceHandle _Get(
    HdContainerDataSourceHandle const &primDataSource,
    const TfToken &name)
{
    if (!primDataSource) {
        return nullptr;
    }
    return HdContainerDataSource::Cast(primDataSource->Get(name));
}

HdContainerDataSourceHandle
HdFlattenedDataSourceProvider::Context::
GetInputDataSource() const
{
    return _Get(_inputPrimDataSource, _name);
}

HdContainerDataSourceHandle
HdFlattenedDataSourceProvider::Context::
GetFlattenedDataSourceFromParentPrim() const
{
    if (_primPath.IsAbsoluteRootPath()) {
        return nullptr;
    }
    return _Get(
        _flatteningSceneIndex.GetPrim(_primPath.GetParentPath()).dataSource,
        _name);
}

HdFlattenedDataSourceProvider::~HdFlattenedDataSourceProvider() = default;

PXR_NAMESPACE_CLOSE_SCOPE
