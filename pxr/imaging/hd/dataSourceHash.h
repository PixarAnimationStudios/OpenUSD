//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DATASOURCE_HASH_H
#define PXR_IMAGING_HD_DATASOURCE_HASH_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdDataSourceHashType = size_t;

/// Compute hash of a data source by using sampled from startTime to
/// endTime.
///
/// Warning: this hash is not strong and is bad for fingerprinting
/// where the hash for two data sources being equal has to imply that
/// the two data sources have equal data with high probability.
/// The has is only 64bit and makes various performance tradeoffs
/// such that is suitable for a hashtable but not for fingerprinting.
///
HD_API
HdDataSourceHashType
HdDataSourceHash(HdDataSourceBaseHandle const &ds,
                 const HdSampledDataSource::Time startTime,
                 const HdSampledDataSource::Time endTime);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
