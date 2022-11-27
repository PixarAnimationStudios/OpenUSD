//
// Copyright 2022 Pixar
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

    /// (Lazily) Create new container data source by applying given
    /// function to all data sources.
    HD_API
    HdMapContainerDataSource(
        const ValueFunction &f,
        const HdContainerDataSourceHandle &src);

    HD_API
    ~HdMapContainerDataSource() override;

    HD_API
    TfTokenVector GetNames() override;
    HD_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;
private:
    ValueFunction _f;
    
    HdContainerDataSourceHandle _src;
};

HD_DECLARE_DATASOURCE_HANDLES(HdMapContainerDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
