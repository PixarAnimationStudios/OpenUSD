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
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/registryManager.h"

////////////////////////////////////////////////////////////////////////
// UsdEditTarget

static PcpMapFunction
_ComposeMappingForNode(const SdfLayerHandle layer, const PcpNodeRef &node)
{
    PcpMapFunction result = node.GetMapToRoot().Evaluate();

    // Pick up any variant selections in the node site.
    // Pcp deliberately keeps variant selections out of the node's
    // map function, but we want a combined mapping.
    const SdfPath& path = node.GetPath();
    if (path.ContainsPrimVariantSelection()) {
        PcpMapFunction::PathMap pathMap = PcpMapFunction::IdentityPathMap();
        pathMap[ path ] = path.StripAllVariantSelections();
        PcpMapFunction varMap =
            PcpMapFunction::Create(pathMap, SdfLayerOffset());
        result = result.Compose(varMap);
    }

    // Pick up any layer offset to the given layer.
    if (const SdfLayerOffset *layerOffset =
        node.GetLayerStack()->GetLayerOffsetForLayer(layer)) {
        PcpMapFunction offsetMap =
            PcpMapFunction::Create( PcpMapFunction::IdentityPathMap(),
                                    *layerOffset);
        result = result.Compose(offsetMap);
    }

    return result;
}

UsdEditTarget::UsdEditTarget()
{
}

UsdEditTarget::UsdEditTarget(const SdfLayerHandle &layer,
                             SdfLayerOffset offset)
    : _layer(layer)
{
    if (offset.IsIdentity()) {
        _mapping = PcpMapFunction::Identity();
    } else {
        _mapping = PcpMapFunction::Create( PcpMapFunction::IdentityPathMap(),
                                           offset );
    }
}

UsdEditTarget::UsdEditTarget(const SdfLayerHandle &layer,
                             const PcpNodeRef &node)
    : _layer(layer)
    , _mapping(_ComposeMappingForNode(layer, node))
{
}

UsdEditTarget::UsdEditTarget(const SdfLayerRefPtr &layer,
                             SdfLayerOffset offset)
    : _layer(layer)
{
    if (offset.IsIdentity()) {
        _mapping = PcpMapFunction::Identity();
    } else {
        _mapping = PcpMapFunction::Create( PcpMapFunction::IdentityPathMap(),
                                           offset );
    }
}

UsdEditTarget::UsdEditTarget(const SdfLayerRefPtr &layer,
                             const PcpNodeRef &node)
    : _layer(layer)
    , _mapping(_ComposeMappingForNode(layer, node))
{
}

/* private */
UsdEditTarget::UsdEditTarget(const SdfLayerHandle &layer,
                             const PcpMapFunction &mapping)
    : _layer(layer)
    , _mapping(mapping)
{
}

UsdEditTarget
UsdEditTarget::ForLocalDirectVariant(const SdfLayerHandle &layer,
                                     const SdfPath &varSelPath)
{
    if (!varSelPath.IsPrimVariantSelectionPath()) {
        TF_CODING_ERROR("Provided varSelPath <%s> must be a prim variant "
                        "selection path.", varSelPath.GetText());
        return UsdEditTarget();
    }

    // Create a map function that represents the variant selections.
    PcpMapFunction::PathMap pathMap = PcpMapFunction::IdentityPathMap();
    pathMap[ varSelPath ] = varSelPath.StripAllVariantSelections();
    PcpMapFunction mapping = PcpMapFunction::Create(pathMap,
                                                    SdfLayerOffset());

    return UsdEditTarget(layer, mapping);
}

bool
UsdEditTarget::operator==(const UsdEditTarget &o) const
{
    return _layer == o._layer && _mapping == o._mapping;
}

SdfPath
UsdEditTarget::MapToSpecPath(const SdfPath &scenePath) const
{
    SdfPath result = _mapping.MapTargetToSource(scenePath);

    // Translate any target paths, stripping variant selections.
    if (result.ContainsTargetPath()) {
        SdfPathVector targetPaths;
        result.GetAllTargetPathsRecursively(&targetPaths);
        for (const SdfPath &targetPath: targetPaths) {
            const SdfPath translatedTargetPath = 
                _mapping.MapTargetToSource(targetPath)
                .StripAllVariantSelections();
            if (translatedTargetPath.IsEmpty()) {
                return SdfPath();
            }
            result = result.ReplacePrefix(targetPath, translatedTargetPath);
        }
    }

    return result;
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

UsdEditTarget
UsdEditTarget::ComposeOver(const UsdEditTarget &weaker) const
{
    return UsdEditTarget(
        !_layer ? weaker._layer : _layer,
        _mapping.Compose(weaker._mapping));
}
