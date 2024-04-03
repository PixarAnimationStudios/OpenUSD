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

#ifndef PXR_IMAGING_HD_CONTAINER_SCHEMA_H
#define PXR_IMAGING_HD_CONTAINER_SCHEMA_H

#include "pxr/imaging/hd/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Base class for a schema backed by a container whose children have
/// arbitrary names.
///
class HdContainerSchema : public HdSchema
{
public:

    HdContainerSchema(HdContainerDataSourceHandle container)
    : HdSchema(container)
    {}

    HD_API
    TfTokenVector GetNames() const;

    HD_API
    static HdContainerDataSourceHandle
    BuildRetained(
        size_t count,
        const TfToken *names,
        const HdDataSourceBaseHandle *values);
};

/// Template class for a schema backed by a container whose children have
/// arbitrary names but an expected data source type.
///
template<typename T>
class HdTypedContainerSchema : public HdContainerSchema
{
public:

    HdTypedContainerSchema(HdContainerDataSourceHandle container)
    : HdContainerSchema(container)
    {}

    typename T::Handle Get(const TfToken &name) {
        return _GetTypedDataSource<T>(name);
    }
};

/// Template class for a schema backed by a container whose children have
/// arbitrary names but an expected schema type.
///
template<typename Schema>
class HdSchemaBasedContainerSchema : public HdContainerSchema
{
public:

    HdSchemaBasedContainerSchema(HdContainerDataSourceHandle container)
    : HdContainerSchema(container)
    {}

    Schema Get(const TfToken &name) {
        using DataSource = typename Schema::UnderlyingDataSource;
        return Schema(_GetTypedDataSource<DataSource>(name));
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
