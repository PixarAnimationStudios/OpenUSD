//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/mapContainerDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMapContainerDataSource::HdMapContainerDataSource(
    const ValueFunction &f,
    const HdContainerDataSourceHandle &src)
  : _f(f)
  , _src(src)
{
}

HdMapContainerDataSource::~HdMapContainerDataSource() = default;

TfTokenVector
HdMapContainerDataSource::GetNames()
{
    if (!_src) {
        return {};
    }
    return _src->GetNames();
}

HdDataSourceBaseHandle
HdMapContainerDataSource::Get(const TfToken &name)
{
    if (!_src) {
        return nullptr;
    }

    if (!_f) {
        return _src->Get(name);
    }
    return _f(_src->Get(name));
}

PXR_NAMESPACE_CLOSE_SCOPE
