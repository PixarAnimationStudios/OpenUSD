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
#include "pxr/imaging/hd/flattenedPrimvarsDataSource.h"

#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

bool _IsConstantPrimvar(HdDataSourceBaseHandle const &primvarDataSource)
{
    HdContainerDataSourceHandle const ds =
        HdContainerDataSource::Cast(primvarDataSource);
    if (!ds) {
        return false;
    }

    HdPrimvarSchema primvarSchema(ds);
    HdTokenDataSourceHandle const interpolationSource =
        primvarSchema.GetInterpolation();
    if (!interpolationSource) {
        return false;
    }

    const TfToken interpolation = interpolationSource->GetTypedValue(0.0f);
    
    return interpolation == HdPrimvarSchemaTokens->constant;
}

}

HdFlattenedPrimvarsDataSource::HdFlattenedPrimvarsDataSource(
    HdContainerDataSourceHandle const &primvarsDataSource,
    HdFlattenedPrimvarsDataSourceHandle const &parentDataSource)
  : _primvarsDataSource(primvarsDataSource)
  , _parentDataSource(parentDataSource)
{
}

std::shared_ptr<std::set<TfToken>>
HdFlattenedPrimvarsDataSource::_GetConstantPrimvarNames()
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
HdFlattenedPrimvarsDataSource::_GetConstantPrimvarNamesUncached()
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
            if (_IsConstantPrimvar(_primvarsDataSource->Get(name))) {
                result.insert(name);
            }
        }
    }

    return result;
}


TfTokenVector
HdFlattenedPrimvarsDataSource::GetNames()
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

HdDataSourceBaseHandle
HdFlattenedPrimvarsDataSource::_Get(const TfToken &name)
{
    // Check whether this prim has this primvar.
    if (_primvarsDataSource) {
        if (HdDataSourceBaseHandle const result =
                _primvarsDataSource->Get(name)) {
            return result;
        }
    }

    // Otherwise, check the flattened data source of
    // the parent prim for the primvar and make sure it is
    // constant.
    if (_parentDataSource) {
        HdDataSourceBaseHandle const result =
            _parentDataSource->Get(name);
        if (_IsConstantPrimvar(result)) {
            return result;
        }
    }

    return HdRetainedTypedSampledDataSource<bool>::New(false);
}

HdDataSourceBaseHandle
HdFlattenedPrimvarsDataSource::Get(const TfToken &name)
{
    _NameToPrimvarDataSource::accessor a;
    _nameToPrimvarDataSource.insert(a, name);

    if (HdDataSourceBaseHandle const ds =
            HdDataSourceBase::AtomicLoad(a->second)) {
        // Cache hit.
        return HdContainerDataSource::Cast(ds);
    }

    // Cache miss.
    HdDataSourceBaseHandle const result = _Get(name);
    HdDataSourceBase::AtomicStore(a->second, result);

    return HdContainerDataSource::Cast(result);
}

static
bool
_DoesNotIntersectInterpolation(const HdDataSourceLocator &locator)
{
    return
        locator.GetElementCount() >= 3 &&
        locator.GetElement(2) != HdPrimvarSchemaTokens->interpolation;
}

HdDataSourceLocatorSet
HdFlattenedPrimvarsDataSource::ComputeDirtyPrimvarsLocators(
    const HdDataSourceLocatorSet &locators)
{
    HdDataSourceLocatorSet result;

    for (const HdDataSourceLocator &locator :
             locators.Intersection(HdPrimvarsSchema::GetDefaultLocator())) {
        if (_DoesNotIntersectInterpolation(locator)) {
            result.insert(locator);
        } else {
            // Since interpolation could have changed, it is also changing
            // whether this primvar is inherited.
            // Thus, the set of primvars is changing. We need to blow all
            // primvars.
            return { HdPrimvarsSchema::GetDefaultLocator() };
        }
    }

    return result;
}

bool
HdFlattenedPrimvarsDataSource::Invalidate(
    const HdDataSourceLocatorSet &locators)
{
    bool anyDirtied = false;

    // Iterate through all locators starting with "primvars".
    for (const HdDataSourceLocator &locator :
             locators.Intersection(HdPrimvarsSchema::GetDefaultLocator())) {
        if (_DoesNotIntersectInterpolation(locator)) {
            const TfToken primvarName = locator.GetElement(1);
            if (_nameToPrimvarDataSource.erase(primvarName)) {
                anyDirtied = true;
            }
        } else {
            // Note that this path should not be hit because clients
            // of HdFlattenedPrimvarsDataSource are supposed to
            // drop the data source when ComputeDirtyPrimvarsLocators
            // returns { HdPrimvarsSchema::GetDefaultLocator() }.
            _nameToPrimvarDataSource.clear();
            _constantPrimvarNames.reset();
            anyDirtied = true;
            break;
        }
    }

    return anyDirtied;
}

PXR_NAMESPACE_CLOSE_SCOPE
