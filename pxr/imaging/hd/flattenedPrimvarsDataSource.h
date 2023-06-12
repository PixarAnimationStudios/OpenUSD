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
#ifndef PXR_IMAGING_HD_FLATTENED_PRIMVARS_DATA_SOURCE_H
#define PXR_IMAGING_HD_FLATTENED_PRIMVARS_DATA_SOURCE_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/dataSource.h"

#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdFlattenedPrimvarsDataSource
///
/// A container data source that inherits constant primvars
/// from a parent data source.
///
/// It is instantiated from a data source containing the
/// primvars of the prim in question (conforming to
/// HdPrimvarsSchema) and a flattened primvars data source
/// for the parent prim.
///
/// If we query a primvar and the prim does not have the prim var,
/// the flattened primvars data source for the parent prim is
/// querried for the primvar and it is used when it is constant.
///
class HdFlattenedPrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdFlattenedPrimvarsDataSource);

    // Adds names of constant primvars from parent flattened
    // primvars data source to this prim's primvars.
    HD_API
    TfTokenVector GetNames() override;

    // Queries prim's primvar source for primvar. If not found,
    // asks parent's flattened primvars data source and uses
    // it if it has constant interpolation.
    HD_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// Computes the locators that need to be dirtied for this
    /// prim and its name space descendants.
    ///
    /// Note that if the interpolation of a primvar changes,
    /// it affects the inheritance and the set of primvars
    /// of the namespace descendants changes. This means,
    /// we need to emit the "primvars" data source locator
    /// to dirty all primvars.
    ///
    /// If this method emits the "primvars" data source locator,
    /// the entire flattened primvars data source has to be
    /// dropped.
    HD_API
    static HdDataSourceLocatorSet ComputeDirtyPrimvarsLocators(
        const HdDataSourceLocatorSet &locators);

    /// Invalidate specific cached primvars.
    HD_API
    bool Invalidate(const HdDataSourceLocatorSet &locators);

private:
    HD_API
    HdFlattenedPrimvarsDataSource(HdContainerDataSourceHandle const &primvarsDataSource,
                                  Handle const &parentDataSource);

    // Get the names of the constant primvars (including inherited
    // ones)
    std::shared_ptr<std::set<TfToken>> _GetConstantPrimvarNames();
    std::set<TfToken> _GetConstantPrimvarNamesUncached();

    // Uncached version of _Get implementing the logic
    // to check the parent data source for the primvar being
    // constant.
    HdDataSourceBaseHandle _Get(const TfToken &name);

    HdContainerDataSourceHandle const _primvarsDataSource;
    Handle const _parentDataSource;

    struct _TokenHashCompare {
        static bool equal(const TfToken &a,
                          const TfToken &b) {
            return a == b;
        }
        static size_t hash(const TfToken &a) {
            return hash_value(a);
        }
    };

    using _NameToPrimvarDataSource = 
        tbb::concurrent_hash_map<
            TfToken,
            HdDataSourceBaseAtomicHandle,
            _TokenHashCompare>;
    // Cached data sources.
    //
    // We store a base rather than a container so we can
    // distinguish between the absence of a cached value
    // (nullptr) and a cached value that might be indicating
    // that the primvar might exist (can cast to
    // HdContainerDataSource) not exist (stored as bool
    // data source).
    _NameToPrimvarDataSource _nameToPrimvarDataSource;

    // Cached constant primvar names
    std::shared_ptr<std::set<TfToken>> _constantPrimvarNames;
};

HD_DECLARE_DATASOURCE_HANDLES(HdFlattenedPrimvarsDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_FLATTENED_PRIMVARS_DATA_SOURCE_H
