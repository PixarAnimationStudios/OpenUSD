//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
