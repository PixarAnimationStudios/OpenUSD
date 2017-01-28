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
#ifndef USD_EDITTARGET_H
#define USD_EDITTARGET_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_PTRS(UsdStage);

/// \class UsdEditTarget
///
/// Defines a mapping from scene graph paths to Sdf spec paths in a
/// SdfLayer where edits should be directed, or up to where to perform partial
/// composition.
///
/// A UsdEditTarget can represent an arbitrary point in a composition graph for
/// the purposes of placing edits and resolving values.  This enables editing
/// and resolving across references, classes, variants, and payloads.
///
/// In the simplest case, an EditTarget represents a single layer in a stage's
/// local LayerStack.  In this case, the mapping that transforms scene graph
/// paths to spec paths in the layer is the identity function.  That is, the
/// UsdAttribute path '/World/Foo.avar' would map to the SdfPropertySpec path
/// '/World/Foo.avar'.
///
/// For a more complex example, suppose '/World/Foo' in 'Shot.usda' is a
/// reference to '/Model' in 'Model.usda'.  One can construct a UsdEditTarget
/// that maps scene graph paths from the 'Shot.usda' stage across the reference
/// to the appropriate paths in the 'Model.usda' layer.  For example, the
/// UsdAttribute '/World/Foo.avar' would map to the SdfPropertySpec
/// '/Model.avar'.  Paths in the stage composed at 'Shot.usda' that weren't
/// prefixed by '/World/Foo' would not have a valid mapping to 'Model.usda'.
///
/// EditTargets may also work for any other kind of arc or series of arcs.
/// This allows for editing across variants, classes, and payloads, or in a
/// variant on the far side of a reference, for example.
///
/// In addition to mapping scene paths to spec paths for editing, EditTargets
/// may also be used to identify points in the composition graph for partial
/// composition.  Though it doesn't currently exist, a UsdCompose API that takes
/// UsdEditTarget arguments may someday be provided.
///
/// For convenience and deployment ease, SdfLayerHandles will implicitly convert
/// to UsdEditTargets.  A UsdEditTarget constructed in this way means direct
/// opinions in a layer in a stage's local LayerStack.
///
class UsdEditTarget
{
public:

    /// Construct a null EditTarget.  A null EditTarget will return paths
    /// unchanged when asked to map paths.
    UsdEditTarget();

    /// Constructor.  Allow implicit conversion from SdfLayerHandle.
    /// EditTargets constructed in this way specify layers in the scene's local
    /// LayerStack.  This lets clients pass layers directly in this common case
    /// without explicitly having to construct a \a UsdEditTarget instance.
    /// To automatically supply the appropriate layer offset for the given
    /// layer, see UsdStage::GetEditTargetForLayer().
    UsdEditTarget(const SdfLayerHandle &layer,
                  SdfLayerOffset offset = SdfLayerOffset());

    /// Convenience implicit conversion from SdfLayerRefPtr.  See above
    /// constructor for more information.
    UsdEditTarget(const SdfLayerRefPtr &layer,
                  SdfLayerOffset offset = SdfLayerOffset());

    /// Construct an EditTarget with \a layer and \a node.  The mapping
    /// will be used to map paths from the scene into the \a layer's namespace
    /// given the \a PcpNodeRef \a node's mapping.
    UsdEditTarget(const SdfLayerHandle &layer, const PcpNodeRef &node);

    /// Convenience constructor taking SdfLayerRefPtr.  See above
    /// constructor for more information.
    UsdEditTarget(const SdfLayerRefPtr &layer, const PcpNodeRef &node);

    /// Convenience constructor for editing a direct variant in a local
    /// LayerStack.  The \p varSelPath must be a prim variant selection path
    /// (see SdfPath::IsPrimVariantSelectionPath()).
    static UsdEditTarget
    ForLocalDirectVariant(const SdfLayerHandle &layer,
                          const SdfPath &varSelPath);

    /// Equality comparison.
    bool operator==(const UsdEditTarget &other) const;

    /// Inequality comparison.
    bool operator!=(const UsdEditTarget &other) const {
        return !(*this == other);
    }

    /// Return true if this EditTarget is null.  Null EditTargets map
    /// paths unchanged, and have no layer or LayerStack identifier.
    bool IsNull() const { return *this == UsdEditTarget(); }

    /// Return true if this EditTarget is valid, false otherwise.  Edit
    /// targets are considered valid when they have a layer.
    bool IsValid() const { return _layer; }

    /// Return the layer this EditTarget contains.
    const SdfLayerHandle &GetLayer() const { return _layer; }

    /// Map the provided \a scenePath into the a SdfSpec path for the
    /// EditTarget's layer, according to the EditTarget's mapping.  Null edit
    /// targets and EditTargets for which \a IsLocalLayer are true return
    /// scenePath unchanged.
    SdfPath MapToSpecPath(const SdfPath &scenePath) const;

    /// Convenience function for getting the PrimSpec in the edit
    /// target's layer for \a scenePath.  This is equivalent to
    /// target.GetLayer()->GetPrimAtPath(target.MapToSpecPath(scenePath)) if
    /// target has a valid layer.  If this target IsNull or there is no valid
    /// mapping from \a scenePath to a SdfPrimSpec path in the layer, return
    /// null.
    SdfPrimSpecHandle
    GetPrimSpecForScenePath(const SdfPath &scenePath) const;

    SdfPropertySpecHandle
    GetPropertySpecForScenePath(const SdfPath &scenePath) const;

    SdfSpecHandle
    GetSpecForScenePath(const SdfPath &scenePath) const;

    /// Returns the PcpMapFunction representing the map from source
    /// specs (including any variant selections) to the stage.
    const PcpMapFunction &
    GetMapFunction() const { return _mapping; }

    /// Return a new EditTarget composed over \a weaker.  This is
    /// typically used to make an EditTarget "explicit".  For example, an edit
    /// target with a layer but with no mapping and no LayerStack identifier
    /// indicates a layer in the local LayerStack of a composed scene.
    /// However, an EditTarget with the same layer but an explicit identity
    /// mapping and the LayerStack identifier of the composed scene may be
    /// desired.  This can be obtained by composing a partial (e.g. layer only)
    /// EditTarget over an explicit EditTarget with layer, mapping and layer
    /// stack identifier.
    UsdEditTarget ComposeOver(const UsdEditTarget &weaker) const;

private:

    UsdEditTarget(const SdfLayerHandle &layer,
                  const PcpMapFunction &mapping);

    SdfLayerHandle _layer;
    PcpMapFunction _mapping;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_EDITTARGET_H
