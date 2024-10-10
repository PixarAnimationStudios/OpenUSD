//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

    typename T::Handle Get(const TfToken &name) const {
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

    Schema Get(const TfToken &name) const {
        using DataSource = typename Schema::UnderlyingDataSource;
        return Schema(_GetTypedDataSource<DataSource>(name));
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
