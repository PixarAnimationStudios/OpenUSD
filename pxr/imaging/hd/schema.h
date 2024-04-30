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

#ifndef PXR_IMAGING_HD_SCHEMA_H
#define PXR_IMAGING_HD_SCHEMA_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

/// Schema classes represent a structured view of the inherently unstructured
/// container data source passed into the constructor. They're used to
/// represent what fields a given object is expected to have.
/// Note that a schema can be applied to a container which doesn't contain
/// all of the named fields; in that case, some of the field accessors will
/// return null, and the caller should use default values for those fields.
class HdSchema
{
public:
    HdSchema(HdContainerDataSourceHandle container)
    : _container(container) {}


    /// Returns the container data source that this schema is interpreting.
    HD_API
    HdContainerDataSourceHandle GetContainer() const;
    HD_API
    bool IsDefined() const;

    /// Returns \c true if this schema is applied on top of a non-null
    /// container.
    explicit operator bool() const { return IsDefined(); }

    using UnderlyingDataSource = HdContainerDataSource;

protected:
    HdContainerDataSourceHandle _container;

    /// Returns a datasource of the requested type for the given name:
    /// schema implementations can use this to ask for child containers,
    /// sampled values, vectors, etc. If the key has the wrong type, this
    /// function returns null.
    template <typename T>
    typename T::Handle _GetTypedDataSource(TfToken name) const {
        return _container ? T::Cast(
            _container->Get(name)) : nullptr;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
