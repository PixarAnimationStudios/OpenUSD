//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_MAP_CONTAINER_DATA_SOURCE_H
#define PXR_IMAGING_HD_MAP_CONTAINER_DATA_SOURCE_H

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdMapContainerDataSource
///
/// Applies function to all data sources in a container data source
/// (non-recursively).
class HdMapContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdMapContainerDataSource);

    using ValueFunction =
        std::function<HdDataSourceBaseHandle(const HdDataSourceBaseHandle &)>;

    HD_API
    ~HdMapContainerDataSource() override;

    HD_API
    TfTokenVector GetNames() override;
    HD_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;
private:
    /// (Lazily) Create new container data source by applying given
    /// function to all data sources.
    HD_API
    HdMapContainerDataSource(
        const ValueFunction &f,
        const HdContainerDataSourceHandle &src);

    ValueFunction _f;
    
    HdContainerDataSourceHandle _src;
};

HD_DECLARE_DATASOURCE_HANDLES(HdMapContainerDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
