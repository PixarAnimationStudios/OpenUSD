//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_CONCATENATED_VECTOR_DATA_SOURCE_H
#define PXR_IMAGING_HD_CONCATENATED_VECTOR_DATA_SOURCE_H

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdConcatenatedVectorDataSource
///
/// Lazily composes two or more vector data source by concatenating all the
/// elements (in the order in which the vector data sources are listed).
class HdConcatenatedVectorDataSource : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdConcatenatedVectorDataSource);

    HD_DECLARE_DATASOURCE_INITIALIZER_LIST_NEW(
            HdConcatenatedVectorDataSource,
            HdVectorDataSourceHandle);

    /// Creates HdConcatenatedVectorDataSource from sources, but only
    /// if needed. If one of given handles is null, the other handle
    /// is returned instead.
    HD_API
    static
    HdVectorDataSourceHandle
    ConcatenatedVectorDataSources(
        const HdVectorDataSourceHandle &src1,
        const HdVectorDataSourceHandle &src2);

    HD_API
    size_t GetNumElements() override;

    HD_API
    HdDataSourceBaseHandle GetElement(size_t i) override;

private:
    HD_API
    HdConcatenatedVectorDataSource(
        const std::initializer_list<HdVectorDataSourceHandle> &sources);

    HD_API
    HdConcatenatedVectorDataSource(
        size_t count,
        HdVectorDataSourceHandle *vectors);

    HD_API
    HdConcatenatedVectorDataSource(
        const HdVectorDataSourceHandle &src1,
        const HdVectorDataSourceHandle &src2);

    HD_API
    HdConcatenatedVectorDataSource(
        const HdVectorDataSourceHandle &src1,
        const HdVectorDataSourceHandle &src2,
        const HdVectorDataSourceHandle &src3);

    using _Sources = TfSmallVector<HdVectorDataSourceHandle, 8>;
    _Sources _sources;
};

HD_DECLARE_DATASOURCE_HANDLES(HdConcatenatedVectorDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
