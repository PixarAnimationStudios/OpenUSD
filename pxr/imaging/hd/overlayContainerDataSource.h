//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_OVERLAY_CONTAINER_DATA_SOURCE_H
#define PXR_IMAGING_HD_OVERLAY_CONTAINER_DATA_SOURCE_H

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdOverlayContainerDataSource
///
/// Lazily composes two or more container source hierarchies
/// Earlier entries on the containers array have stronger opinion strength
/// for overlapping child names. Overlapping children which are all containers
/// themselves are returned as another instance of this class
class HdOverlayContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdOverlayContainerDataSource);

    HD_DECLARE_DATASOURCE_INITIALIZER_LIST_NEW(
            HdOverlayContainerDataSource, 
            HdContainerDataSourceHandle);

    HD_API
    HdOverlayContainerDataSource(
        std::initializer_list<HdContainerDataSourceHandle> sources);

    HD_API
    HdOverlayContainerDataSource(
        size_t count,
        HdContainerDataSourceHandle *containers);

    HD_API
    HdOverlayContainerDataSource(
        const HdContainerDataSourceHandle &src1,
        const HdContainerDataSourceHandle &src2);

    HD_API
    HdOverlayContainerDataSource(
        const HdContainerDataSourceHandle &src1,
        const HdContainerDataSourceHandle &src2,
        const HdContainerDataSourceHandle &src3);
    
    /// Creates HdOverlayContainerDataSource from sources, but only
    /// if needed. If one of given handles is null, the other handle
    /// is returned instead.
    HD_API
    static
    HdContainerDataSourceHandle
    OverlayedContainerDataSources(
        const HdContainerDataSourceHandle &src1,
        const HdContainerDataSourceHandle &src2);

    HD_API
    TfTokenVector GetNames() override;
    HD_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    using _ContainerVector = TfSmallVector<HdContainerDataSourceHandle, 8>;
    _ContainerVector _containers;
};

HD_DECLARE_DATASOURCE_HANDLES(HdOverlayContainerDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
