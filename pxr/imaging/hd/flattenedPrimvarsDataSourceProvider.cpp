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

    using _PrimvarDsMap =
        std::unordered_map<TfToken, HdContainerDataSourceHandle,
                           TfToken::HashFunctor>;

    // Get the the constant primvars (including inherited ones)
    std::shared_ptr<_PrimvarDsMap> _GetConstantPrimvars();

    HdContainerDataSourceHandle const _primvarsDataSource;
    Handle const _parentDataSource;

    // Cached constant primvars, including those from parent
    std::shared_ptr<_PrimvarDsMap> _constantPrimvars;
};

_PrimvarsDataSource::_PrimvarsDataSource(
    HdContainerDataSourceHandle const &primvarsDataSource,
    Handle const &parentDataSource)
  : _primvarsDataSource(primvarsDataSource)
  , _parentDataSource(parentDataSource)
{
}

std::shared_ptr<_PrimvarsDataSource::_PrimvarDsMap>
_PrimvarsDataSource::_GetConstantPrimvars()
{
    std::shared_ptr<_PrimvarDsMap> result =
        std::atomic_load(&_constantPrimvars);

    if (!result) {
        // Cache miss

        // Get constant primvars from flattened primvars data source from
        // parent prim.
        bool primvarMapIsUnique = false;
        if (_parentDataSource) {
            result = _parentDataSource->_GetConstantPrimvars();
        } else {
            result.reset(new _PrimvarDsMap);
            primvarMapIsUnique = true;
        }

        // Add constant primvars from this prim.
        if (_primvarsDataSource) {
            for (const TfToken &name : _primvarsDataSource->GetNames()) {
                HdContainerDataSourceHandle primvarDs =
                    HdContainerDataSource::Cast(_primvarsDataSource->Get(name));
                if (_IsConstantPrimvar(primvarDs)) {
                    // We can no longer share result with the parent.
                    if (!primvarMapIsUnique) {
                        result.reset(new _PrimvarDsMap(*result));
                        primvarMapIsUnique = true;
                    }
                    // Add (or replace inherited) data source for this key.
                    (*result)[name] = primvarDs;
                }
            }
        }

        // It is possible for another thread to race this one and
        // have stored a result in the meantime.  The results will
        // be equivalent.
        std::atomic_store(&_constantPrimvars, result);
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

    // Get constant primvars from parent prim's flattened
    // primvar source.
    if (_parentDataSource) {
        if (std::shared_ptr<_PrimvarDsMap> parentConstantPrimvars =
            _parentDataSource->_GetConstantPrimvars()) {
            for (const auto& entry: *parentConstantPrimvars) {
                // Add primvar name if not already in result.
                // O(N^2) but we expect small N.
                if (std::find(result.begin(), result.end(),
                              entry.first) == result.end()) {
                    result.push_back(entry.first);
                }
            }
        }
    }
    
    return result;
}

HdDataSourceBaseHandle
_PrimvarsDataSource::Get(const TfToken &name)
{
    // Check whether this prim has this primvar.
    if (_primvarsDataSource) {
        if (HdContainerDataSourceHandle const result =
                HdContainerDataSource::Cast(_primvarsDataSource->Get(name))) {
            return result;
        }
    }

    // Otherwise, check inherited constant primvars.
    if (_parentDataSource) {
        if (std::shared_ptr<_PrimvarDsMap> parentConstantPrimvars =
            _parentDataSource->_GetConstantPrimvars()) {
            const auto it = parentConstantPrimvars->find(name);
            if (it != parentConstantPrimvars->end()) {
                return it->second;
            }
        }
    }

    return nullptr;
}

bool
_PrimvarsDataSource::Invalidate(
    const HdDataSourceLocatorSet &locators)
{
    bool anyDirtied = false;

    for (const HdDataSourceLocator &locator : locators) {
        if (_DoesLocatorIntersectInterpolation(locator)) {
            // This path should not be hit because
            // ComputeDirtyLocatorsForDescendants would return
            // the UniversalSet if the locators intersect
            // with interpolation.
            //
            // The HdFlatteningSceneIndex is then supposed to
            // drop the data source rather than invalidate it.
            _constantPrimvars.reset();
            anyDirtied = true;
            break;
        }
    }

    return anyDirtied;
}

}

HdContainerDataSourceHandle
HdFlattenedPrimvarsDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    // Unlike other flattened data source providers, we cannot use
    // the parent's flattened data source in the case where this
    // prim provides no additional input data source.  The reason
    // is that only interpolation=constant primvars inherit, and
    // _PrimvarsDataSource() is what provides this filtering.
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
