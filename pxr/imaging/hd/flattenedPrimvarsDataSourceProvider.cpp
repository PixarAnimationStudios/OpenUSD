//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/flattenedPrimvarsDataSourceProvider.h"

#include "pxr/imaging/hd/invalidatableContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"

#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

bool _IsConstantPrimvar(HdContainerDataSourceHandle const &primvarDataSource)
{
    HdPrimvarSchema primvarSchema(primvarDataSource);
    HdTokenDataSourceHandle const interpolationSource =
        primvarSchema.GetInterpolation();
    if (!interpolationSource) {
        return false;
    }

    const TfToken interpolation = interpolationSource->GetTypedValue(0.0f);
    
    return interpolation == HdPrimvarSchemaTokens->constant;
}

bool _DoesLocatorIntersectInterpolation(const HdDataSourceLocator &locator)
{
    return
        locator.GetElementCount() < 2 ||
        locator.GetElement(1) == HdPrimvarSchemaTokens->interpolation;
}

/// \class _PrimvarsDataSource
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
class _PrimvarsDataSource : public HdInvalidatableContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    // Adds names of constant primvars from parent flattened
    // primvars data source to this prim's primvars.
    TfTokenVector GetNames() override;

    // Queries prim's primvar source for primvar. If not found,
    // asks parent's flattened primvars data source and uses
    // it if it has constant interpolation.
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// Invalidate specific cached primvars.
    bool Invalidate(const HdDataSourceLocatorSet &locators) override;

private:
    _PrimvarsDataSource(HdContainerDataSourceHandle const &primvarsDataSource,
                        Handle const &parentDataSource);

    // Get the names of the constant primvars (including inherited
    // ones)
    std::shared_ptr<std::set<TfToken>> _GetConstantPrimvarNames();
    std::set<TfToken> _GetConstantPrimvarNamesUncached();

    // Uncached version of _Get implementing the logic
    // to check the parent data source for the primvar being
    // constant.
    HdContainerDataSourceHandle _GetUncached(const TfToken &name);

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

_PrimvarsDataSource::_PrimvarsDataSource(
    HdContainerDataSourceHandle const &primvarsDataSource,
    Handle const &parentDataSource)
  : _primvarsDataSource(primvarsDataSource)
  , _parentDataSource(parentDataSource)
{
}

std::shared_ptr<std::set<TfToken>>
_PrimvarsDataSource::_GetConstantPrimvarNames()
{
    std::shared_ptr<std::set<TfToken>> result =
        std::atomic_load(&_constantPrimvarNames);

    if (!result) {
        // Cache miss
        result = std::make_shared<std::set<TfToken>>(
            _GetConstantPrimvarNamesUncached());
        std::atomic_store(&_constantPrimvarNames, result);
    }

    return result;
}

std::set<TfToken>
_PrimvarsDataSource::_GetConstantPrimvarNamesUncached()
{
    std::set<TfToken> result;

    // Get constant primvars from flattened primvars data source from
    // parent prim.
    if (_parentDataSource) {
        result = *_parentDataSource->_GetConstantPrimvarNames();
    }

    // Add constant primvars from this prim.
    if (_primvarsDataSource) {
        for (const TfToken &name : _primvarsDataSource->GetNames()) {
            if (_IsConstantPrimvar(
                    HdContainerDataSource::Cast(
                        _primvarsDataSource->Get(name)))) {
                result.insert(name);
            }
        }
    }

    return result;
}


TfTokenVector
_PrimvarsDataSource::GetNames()
{
    TfTokenVector result;
    // First get primvars from this prim.
    if (_primvarsDataSource) {
        result = _primvarsDataSource->GetNames();
    }

    if (!_parentDataSource) {
        return result;
    }

    // Get constant primvars from parent prim's flattened
    // primvar source.
    std::set<TfToken> constantPrimvars =
        *(_parentDataSource->_GetConstantPrimvarNames());
    if (constantPrimvars.empty()) {
        return result;
    }

    // To avoid duplicates, erase this prim's primvars
    // from constant primvars.
    for (const TfToken &name : result) {
        constantPrimvars.erase(name);
    }

    // And add the constant primvars not already in the
    // result to the result.
    result.insert(result.end(),
                  constantPrimvars.begin(), constantPrimvars.end());
    
    return result;
}

HdContainerDataSourceHandle
_PrimvarsDataSource::_GetUncached(const TfToken &name)
{
    // Check whether this prim has this primvar.
    if (_primvarsDataSource) {
        if (HdContainerDataSourceHandle const result =
                HdContainerDataSource::Cast(_primvarsDataSource->Get(name))) {
            return result;
        }
    }

    // Otherwise, check the flattened data source of
    // the parent prim for the primvar and make sure it is
    // constant.
    if (_parentDataSource) {
        HdContainerDataSourceHandle const result =
            HdContainerDataSource::Cast(_parentDataSource->Get(name));
        if (_IsConstantPrimvar(result)) {
            return result;
        }
    }

    return nullptr;
}

HdDataSourceBaseHandle
_PrimvarsDataSource::Get(const TfToken &name)
{
    _NameToPrimvarDataSource::accessor a;
    _nameToPrimvarDataSource.insert(a, name);

    if (HdDataSourceBaseHandle const ds =
            HdDataSourceBase::AtomicLoad(a->second)) {
        // Cache hit.
        return HdContainerDataSource::Cast(ds);
    }

    // Cache miss.
    HdContainerDataSourceHandle const result = _GetUncached(name);
    if (result) {
        HdDataSourceBase::AtomicStore(
            a->second, result);
    } else {
        HdDataSourceBase::AtomicStore(
            a->second, HdRetainedTypedSampledDataSource<bool>::New(false));
    }
    return result;
}

bool
_PrimvarsDataSource::Invalidate(
    const HdDataSourceLocatorSet &locators)
{
    bool anyDirtied = false;

    // Iterate through all locators starting with "primvars".
    for (const HdDataSourceLocator &locator : locators) {
        if (_DoesLocatorIntersectInterpolation(locator)) {
            // This path should not be hit because
            // ComputeDirtyLocatorsForDescendants would return
            // the UniversalSet if the locators intersect
            // with interpolation.
            //
            // The HdFlatteningSceneIndex is then supposed to
            // drop the data source rather than invalidate it.
            _nameToPrimvarDataSource.clear();
            _constantPrimvarNames.reset();
            anyDirtied = true;
            break;
        }
        
        const TfToken &primvarName = locator.GetFirstElement();
        if (_nameToPrimvarDataSource.erase(primvarName)) {
            anyDirtied = true;
        }
    }

    return anyDirtied;
}

}

HdContainerDataSourceHandle
HdFlattenedPrimvarsDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    return 
        _PrimvarsDataSource::New(
            ctx.GetInputDataSource(),
            _PrimvarsDataSource::Cast(
                ctx.GetFlattenedDataSourceFromParentPrim()));
}

void
HdFlattenedPrimvarsDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    for (const HdDataSourceLocator &locator : *locators) {
        if (_DoesLocatorIntersectInterpolation(locator)) {
            // Since interpolation could have changed, it is also changing
            // whether this primvar is inherited.
            // Thus, the set of primvars is changing. We need to blow all
            // primvars.
            *locators = HdDataSourceLocatorSet::UniversalSet();
            break;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
