//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_LAZY_CONTAINER_DATA_SOURCE_H
#define PXR_IMAGING_HD_LAZY_CONTAINER_DATA_SOURCE_H

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdLazyContainerDataSource
///
/// A container data source lazily evaluating the given thunk to
/// forward all calls to the container data source computed by the thunk.
///
class HdLazyContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdLazyContainerDataSource);

    using Thunk = std::function<HdContainerDataSourceHandle()>;

    HD_API
    TfTokenVector GetNames() override;
    HD_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    HD_API
    ~HdLazyContainerDataSource();

protected:
    HD_API
    HdLazyContainerDataSource(const Thunk &thunk);

private:
    HdContainerDataSourceHandle _GetSrc();

    Thunk _thunk;
    HdContainerDataSourceAtomicHandle _src;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
