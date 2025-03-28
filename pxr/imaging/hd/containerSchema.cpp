//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/containerSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TfTokenVector
HdContainerSchema::GetNames() const
{
    if (_container) {
        return _container->GetNames();
    }
    return {};
}

HdContainerDataSourceHandle
HdContainerSchema::BuildRetained(
    const size_t count,
    const TfToken * const names,
    const HdDataSourceBaseHandle * const values)
{
    return
        HdRetainedContainerDataSource::New(count, names, values);
}

PXR_NAMESPACE_CLOSE_SCOPE
