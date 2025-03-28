//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
HdSchema::GetContainer() const
{
    return _container;
}

bool
HdSchema::IsDefined() const
{
    if (_container) {
        return true;
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
