//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/vectorSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/*static*/
HdVectorDataSourceHandle
HdVectorSchema::BuildRetained(
    const size_t count,
    const HdDataSourceBaseHandle *values)
{
    return HdRetainedSmallVectorDataSource::New(count, values);
}

HdVectorDataSourceHandle
HdVectorSchema::GetVector()
{
    return _vector;
}

bool
HdVectorSchema::IsDefined() const
{
    if (_vector) {
        return true;
    }
    return false;
}

size_t
HdVectorSchema::GetNumElements() const
{
    if (_vector) {
        return _vector->GetNumElements();
    }
    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
