//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hd/lazyContainerDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdLazyContainerDataSource::HdLazyContainerDataSource(const Thunk &thunk)
 : _thunk(thunk)
{
}

HdLazyContainerDataSource::~HdLazyContainerDataSource() = default;

TfTokenVector
HdLazyContainerDataSource::GetNames()
{
    if (HdContainerDataSourceHandle src = _GetSrc()) {
        return src->GetNames();
    }
    return {};
}
        
HdDataSourceBaseHandle
HdLazyContainerDataSource::Get(const TfToken &name)
{
    if (HdContainerDataSourceHandle src = _GetSrc()) {
        return src->Get(name);
    }
    return nullptr;
}

HdContainerDataSourceHandle
HdLazyContainerDataSource::_GetSrc()
{
    if (HdContainerDataSourceHandle storedSrc =
            HdContainerDataSource::AtomicLoad(_src)) {
        return storedSrc;
    }

    HdContainerDataSourceHandle src = _thunk();
    HdContainerDataSource::AtomicStore(_src, src);

    return src;
}

PXR_NAMESPACE_CLOSE_SCOPE

