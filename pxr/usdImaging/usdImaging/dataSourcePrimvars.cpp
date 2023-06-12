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

#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"
#include "pxr/usdImaging/usdImaging/dataSourceRelationship.h"

#include "pxr/usdImaging/usdImaging/primvarUtils.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"

#include "pxr/base/tf/denseHashMap.h"

PXR_NAMESPACE_OPEN_SCOPE

static inline bool
_IsIndexed(const UsdAttributeQuery& indicesQuery)
{
    return indicesQuery.IsValid() && indicesQuery.HasValue();
}

static
TfToken
_GetInterpolation(const UsdAttribute &attr)
{
    // A reimplementation of UsdGeomPrimvar::GetInterpolation(),
    // but with "vertex" as the default instead of "constant"...
    TfToken interpolation;
    if (attr.GetMetadata(UsdGeomTokens->interpolation, &interpolation)) {
        return UsdImagingUsdToHdInterpolationToken(interpolation);
    }

    return HdPrimvarSchemaTokens->vertex;
}

// Reject primvars:points since we always want to get the value from
// the points attribute.
// Similar for velocities and accelerations.
static
bool
_RejectPrimvar(const TfToken &name)
{
    if (name == UsdGeomTokens->points) {
        return true;
    }
    if (name == UsdGeomTokens->velocities) {
        return true;
    }
    if (name == UsdGeomTokens->accelerations) {
        return true;
    }

    return false;
}

UsdImagingDataSourcePrimvars::UsdImagingDataSourcePrimvars(
    const SdfPath &sceneIndexPath,
    UsdPrim const &usdPrim,
    UsdGeomPrimvarsAPI usdPrimvars,
    const UsdImagingDataSourceStageGlobals & stageGlobals)
: _sceneIndexPath(sceneIndexPath)
, _usdPrim(usdPrim)
, _stageGlobals(stageGlobals)
{
    const std::vector<UsdGeomPrimvar> primvars = usdPrimvars.GetAuthoredPrimvars();
    for (const UsdGeomPrimvar & p : primvars) {
        if (!_RejectPrimvar(p.GetPrimvarName())) {
            _namespacedPrimvars[p.GetPrimvarName()] = p;
        }
    }
}


/*static*/
TfToken
UsdImagingDataSourcePrimvars::_GetPrefixedName(const TfToken &name)
{
    return TfToken(("primvars:" + name.GetString()).c_str());
}

TfTokenVector 
UsdImagingDataSourcePrimvars::GetNames()
{
    TRACE_FUNCTION();

    TfTokenVector result;
    result.reserve(_namespacedPrimvars.size());

    for (const auto & entry : _namespacedPrimvars) {
        result.push_back(entry.first);
    }

    for (UsdProperty prop :
            _usdPrim.GetAuthoredPropertiesInNamespace("primvars:")) {
        if (UsdRelationship rel = prop.As<UsdRelationship>()) {
            // strip only the "primvars:" namespace
            static const size_t prefixLength = 9;
            result.push_back(TfToken(rel.GetName().data() + prefixLength));
        }
    }

    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourcePrimvars::Get(const TfToken & name)
{
    TRACE_FUNCTION();

    const auto nsIt = _namespacedPrimvars.find(name);
    if (nsIt != _namespacedPrimvars.end()) {
        const UsdGeomPrimvar &usdPrimvar = nsIt->second;
        const UsdAttribute &attr = usdPrimvar.GetAttr();
        return UsdImagingDataSourcePrimvar::New(
                _sceneIndexPath, name, _stageGlobals,
                /* value = */ UsdAttributeQuery(attr),
                /* indices = */ UsdAttributeQuery(usdPrimvar.GetIndicesAttr()),
                HdPrimvarSchema::BuildInterpolationDataSource(
                    UsdImagingUsdToHdInterpolationToken(
                        usdPrimvar.GetInterpolation())),
                HdPrimvarSchema::BuildRoleDataSource(
                    UsdImagingUsdToHdRole(attr.GetRoleName())));
    }

    if (UsdRelationship rel =
            _usdPrim.GetRelationship(_GetPrefixedName(name))) {

        return HdPrimvarSchema::Builder()
            .SetPrimvarValue(UsdImagingDataSourceRelationship::New(
                rel, _stageGlobals))
            .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->constant))
            .Build();
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceCustomPrimvars::UsdImagingDataSourceCustomPrimvars(
        const SdfPath &sceneIndexPath,
        UsdPrim const &usdPrim,
        const Mappings &mappings,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
: _sceneIndexPath(sceneIndexPath)
, _usdPrim(usdPrim)
, _stageGlobals(stageGlobals)
{
    for (const Mapping& cp : mappings) {
        UsdAttributeQuery attrQ(
            _usdPrim.GetPrim().GetAttribute(cp.usdAttrName));
        if (attrQ.HasAuthoredValue()) {
            _customPrimvars[cp.primvarName] = { attrQ, cp.interpolation };
        }
    }
}

TfTokenVector
UsdImagingDataSourceCustomPrimvars::GetNames()
{
    TfTokenVector result;
    result.reserve(_customPrimvars.size());

    for (const auto &entry : _customPrimvars) {
        result.push_back(entry.first);
    }

    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourceCustomPrimvars::Get(const TfToken &name)
{
    const auto cIt = _customPrimvars.find(name);
    if (cIt != _customPrimvars.end()) {
        const UsdAttributeQuery &attrQ = cIt->second.first;
        const UsdAttribute &attr = attrQ.GetAttribute();
        const TfToken &interpolation = cIt->second.second;

        return UsdImagingDataSourcePrimvar::New(
            _sceneIndexPath, name, _stageGlobals,
            /* value = */ attrQ,
            /* indices = */ UsdAttributeQuery(),
            HdPrimvarSchema::BuildInterpolationDataSource(
                interpolation.IsEmpty()
                ? _GetInterpolation(attr)
                : interpolation),
            HdPrimvarSchema::BuildRoleDataSource(
                UsdImagingUsdToHdRole(attr.GetRoleName())));
    }

    return nullptr;
}



/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceCustomPrimvars::Invalidate(
        const TfTokenVector &properties,
        const Mappings &mappings)
{
    HdDataSourceLocatorSet result;

    // TODO, decide how to handle this based on the size?
    TfDenseHashMap<TfToken, TfToken, TfHash> nameMappings;
    for (const UsdImagingDataSourceCustomPrimvars::Mapping &m : mappings) {
        nameMappings[m.usdAttrName] = m.primvarName;
    }

    for (const TfToken &propertyName : properties) {
        const auto it = nameMappings.find(propertyName);
        if (it != nameMappings.end()) {
            result.insert(HdPrimvarsSchema::GetDefaultLocator().Append(
                it->second));
        }
    }

    return result;
}

// ----------------------------------------------------------------------------


UsdImagingDataSourcePrimvar::UsdImagingDataSourcePrimvar(
        const SdfPath &sceneIndexPath,
        const TfToken &name,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        UsdAttributeQuery valueQuery,
        UsdAttributeQuery indicesQuery,
        HdTokenDataSourceHandle interpolation,
        HdTokenDataSourceHandle role)
: _stageGlobals(stageGlobals)
, _valueQuery(valueQuery)
, _indicesQuery(indicesQuery)
, _interpolation(interpolation)
, _role(role)
{
    const bool indexed = _IsIndexed(_indicesQuery);
    if (indexed) {
        if (_valueQuery.ValueMightBeTimeVarying()) {
            _stageGlobals.FlagAsTimeVarying(sceneIndexPath,
                    HdDataSourceLocator(
                        HdPrimvarsSchemaTokens->primvars,
                        name,
                        HdPrimvarSchemaTokens->indexedPrimvarValue));
        }
        if (_indicesQuery.ValueMightBeTimeVarying()) {
            _stageGlobals.FlagAsTimeVarying(sceneIndexPath,
                    HdDataSourceLocator(
                        HdPrimvarsSchemaTokens->primvars,
                        name,
                        HdPrimvarSchemaTokens->indices));
        }
    } else {
        if (_valueQuery.ValueMightBeTimeVarying()) {
            _stageGlobals.FlagAsTimeVarying(sceneIndexPath,
                    HdDataSourceLocator(
                        HdPrimvarsSchemaTokens->primvars,
                        name,
                        HdPrimvarSchemaTokens->primvarValue));
        }
    }
}

TfTokenVector
UsdImagingDataSourcePrimvar::GetNames()
{
    const bool indexed = _IsIndexed(_indicesQuery);

    TfTokenVector result = {
        HdPrimvarSchemaTokens->interpolation,
        HdPrimvarSchemaTokens->role,
    };
    
    if (indexed) {
        result.push_back(HdPrimvarSchemaTokens->indexedPrimvarValue);
        result.push_back(HdPrimvarSchemaTokens->indices);
    } else {
        result.push_back(HdPrimvarSchemaTokens->primvarValue);
    }

    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourcePrimvar::Get(const TfToken & name)
{
    TRACE_FUNCTION();

    const bool indexed = _IsIndexed(_indicesQuery);

    if (indexed) {
        if (name == HdPrimvarSchemaTokens->indexedPrimvarValue) {
            return UsdImagingDataSourceAttributeNew(
                    _valueQuery, _stageGlobals);
        } else if (name == HdPrimvarSchemaTokens->indices) {
            return UsdImagingDataSourceAttributeNew(
                    _indicesQuery, _stageGlobals);
        }
    } else {
        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return UsdImagingDataSourceAttributeNew(
                    _valueQuery, _stageGlobals);
        }
    }

    if (name == HdPrimvarSchemaTokens->interpolation) {
        return _interpolation;
    } else if (name == HdPrimvarSchemaTokens->role) {
        return _role;
    }
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
