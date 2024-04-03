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

#ifndef PXR_IMAGING_HD_VECTOR_SCHEMA_H
#define PXR_IMAGING_HD_VECTOR_SCHEMA_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

/// Base class wrapping a vector data source.
///
class HdVectorSchema
{
public:
    HdVectorSchema(HdVectorDataSourceHandle const &vector)
      : _vector(vector) {}

    HD_API
    static HdVectorDataSourceHandle
    BuildRetained(
        size_t count,
        const HdDataSourceBaseHandle *values);

    /// Returns the vector data source that this schema is interpreting.
    HD_API
    HdVectorDataSourceHandle GetVector();
    HD_API
    bool IsDefined() const;

    /// Returns \c true if this schema is applied on top of a non-null
    /// vector.
    explicit operator bool() const { return IsDefined(); }

    /// Number of elements in the vector.
    HD_API
    size_t GetNumElements() const;

    using UnderlyingDataSource = HdVectorDataSource;

protected:
    template<typename T>
    typename T::Handle _GetTyped(const size_t element) const {
        return
            _vector
            ? T::Cast(_vector->GetElement(element))
            : nullptr;
    }

    HdVectorDataSourceHandle _vector;
};

/// Template class wrapping a vector data source whose children are
/// data source of an expected type.
///
template<typename T>
class HdTypedVectorSchema : public HdVectorSchema
{
public:
    HdTypedVectorSchema(HdVectorDataSourceHandle const &vector)
      : HdVectorSchema(vector) {}

    typename T::Handle GetElement(const size_t element) const {
        return _GetTyped<T>(element);
    }
};

/// Template class wrapping a vector data source whose children are
/// container data source conforming to an expected schema.
///
template<typename Schema>
class HdSchemaBasedVectorSchema : public HdVectorSchema
{
public:
    HdSchemaBasedVectorSchema(HdVectorDataSourceHandle const &vector)
      : HdVectorSchema(vector) {}
    
    Schema GetElement(const size_t element) const {
        using DataSource = typename Schema::UnderlyingDataSource;
        return Schema(_GetTyped<DataSource>(element));
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
