//
// Copyright 2016 Pixar
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
#include "pxr/usd/usd/editTarget.h"

#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/spec.h"

#include "pxr/base/tf/registryManager.h"

////////////////////////////////////////////////////////////////////////
// Usd_SpecPathMapping

Usd_SpecPathMapping::Usd_SpecPathMapping()
{
}

Usd_SpecPathMapping::Usd_SpecPathMapping(const PcpNodeRef &node)
    : _mapFn(node.GetMapToRoot().Evaluate())
    , _sitePath(node.GetPath())
    , _strippedSitePath(_sitePath.StripAllVariantSelections())
{
    if (not _sitePath.ContainsPrimVariantSelection()) {
        _sitePath = _strippedSitePath = SdfPath::AbsoluteRootPath();
    }
}

Usd_SpecPathMapping::Usd_SpecPathMapping(const PcpMapFunction &mapFn,
                                         const SdfPath &sitePath,
                                         const SdfPath &strippedSitePath)
    : _mapFn(mapFn)
    , _sitePath(sitePath)
    , _strippedSitePath(strippedSitePath)
{
}

/* static */
Usd_SpecPathMapping
Usd_SpecPathMapping::Identity()
{
    return Usd_SpecPathMapping(PcpMapFunction::Identity(),
                               SdfPath::AbsoluteRootPath(),
                               SdfPath::AbsoluteRootPath());
}

void
Usd_SpecPathMapping::Swap(Usd_SpecPathMapping &other)
{
    _mapFn.Swap(other._mapFn);
    swap(_sitePath, other._sitePath);
    swap(_strippedSitePath, other._strippedSitePath);
}

bool
Usd_SpecPathMapping::operator==(const Usd_SpecPathMapping &other) const
{
    return _mapFn == other._mapFn and
        _sitePath == other._sitePath and
        _strippedSitePath == other._strippedSitePath;
}

bool
Usd_SpecPathMapping::operator!=(const Usd_SpecPathMapping &other) const
{
    return not (*this == other);
}

bool
Usd_SpecPathMapping::IsNull() const
{
    return *this == Usd_SpecPathMapping();
}

bool
Usd_SpecPathMapping::IsIdentity() const {
    return *this == Identity();
}

SdfPath
Usd_SpecPathMapping::MapRootToSpec(const SdfPath &path) const
{
    // Null functions produce no result.
    if (IsNull())
        return SdfPath();

    // Run it through the map function.
    SdfPath result =
        PcpTranslatePathFromRootToNodeUsingFunction(_mapFn, path);

    // Replace the stripped prefix with the real site prefix to add back any
    // variant selections that may be present.  Don't fix target paths -- they
    // should never contain variant selections.
    return result.ReplacePrefix(
        _strippedSitePath, _sitePath, /* fixTargetPaths = */ false);
}

const PcpMapFunction &
Usd_SpecPathMapping::GetMapFunction() const {
    return _mapFn;
}



////////////////////////////////////////////////////////////////////////
// UsdEditTarget

UsdEditTarget::UsdEditTarget()
{
}

UsdEditTarget::UsdEditTarget(const SdfLayerHandle &layer)
    : _layer(layer)
{
}

UsdEditTarget::UsdEditTarget(const SdfLayerHandle &layer,
                             const PcpNodeRef &node)
    : _layer(layer)
    , _mapping(node)
    , _lsid(node.GetLayerStack()->GetIdentifier())
{
}

UsdEditTarget::UsdEditTarget(const SdfLayerRefPtr &layer)
    : _layer(layer)
{
}

UsdEditTarget::UsdEditTarget(const SdfLayerRefPtr &layer,
                             const PcpNodeRef &node)
    : _layer(layer)
    , _mapping(node)
    , _lsid(node.GetLayerStack()->GetIdentifier())
{
}

/* private */
UsdEditTarget::UsdEditTarget(const SdfLayerHandle &layer,
                             const Usd_SpecPathMapping &mapping,
                             const PcpLayerStackIdentifier &lsid)
    : _layer(layer)
    , _mapping(mapping)
    , _lsid(lsid)
{
}

UsdEditTarget
UsdEditTarget::ForLocalDirectVariant(const SdfLayerHandle &layer,
                                     const SdfPath &varSelPath,
                                     const PcpLayerStackIdentifier &lsid)
{
    if (not varSelPath.IsPrimVariantSelectionPath()) {
        TF_CODING_ERROR("Provided varSelPath <%s> must be a prim variant "
                        "selection path.", varSelPath.GetText());
        return UsdEditTarget();
    }
    return UsdEditTarget(
        layer,
        Usd_SpecPathMapping(
            PcpMapFunction::Identity(),
            varSelPath, varSelPath.StripAllVariantSelections()),
        lsid);
}

bool
UsdEditTarget::operator==(const UsdEditTarget &o) const
{
    return _layer == o._layer and _mapping == o._mapping and _lsid == o._lsid;
}

bool
UsdEditTarget::HasMapping() const
{
    return not _mapping.IsNull();
}

bool
UsdEditTarget::IsLocalLayer() const
{
    return not HasMapping() and not GetLayerStackIdentifier();
}

SdfPath
UsdEditTarget::MapToSpecPath(const SdfPath &scenePath) const
{
    return _mapping.IsNull() ? scenePath : _mapping.MapRootToSpec(scenePath);
}

SdfPrimSpecHandle
UsdEditTarget::GetPrimSpecForScenePath(const SdfPath &scenePath) const
{
    if (const SdfLayerHandle &layer = GetLayer())
        return layer->GetPrimAtPath(MapToSpecPath(scenePath));
    return TfNullPtr;
}

SdfPropertySpecHandle
UsdEditTarget::GetPropertySpecForScenePath(const SdfPath &scenePath) const
{
    if (const SdfLayerHandle &layer = GetLayer())
        return layer->GetPropertyAtPath(MapToSpecPath(scenePath));
    return TfNullPtr;
}

SdfSpecHandle
UsdEditTarget::GetSpecForScenePath(const SdfPath &scenePath) const
{
    if (const SdfLayerHandle &layer = GetLayer())
        return layer->GetObjectAtPath(MapToSpecPath(scenePath));
    return TfNullPtr;
}

bool
UsdEditTarget::IsAtNode(const PcpNodeRef &node) const
{
    return _lsid == node.GetLayerStack()->GetIdentifier() and
        _mapping == Usd_SpecPathMapping(node);
}

UsdEditTarget
UsdEditTarget::ComposeOver(const UsdEditTarget &weaker) const
{
    return UsdEditTarget(
        not _layer ? weaker._layer : _layer,
        _mapping.IsNull() ? weaker._mapping : _mapping,
        not _lsid ? weaker._lsid : _lsid);
}
