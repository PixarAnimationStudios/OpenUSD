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

#include "pxr/usd/usd/api.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"

// Helper class for UsdEditTarget.  Handles mapping scene paths to spec paths
// via a mapping function.
struct Usd_SpecPathMapping
{
    // Default ctor produces null mapping.
    USD_API
    Usd_SpecPathMapping();

    // Construct from PcpNodeRef.
    USD_API
    explicit Usd_SpecPathMapping(const PcpNodeRef &node);

    // Get an identity mapping.
    USD_API
    static Usd_SpecPathMapping Identity();

    // Swap this mapping with \a other.
    USD_API
    void Swap(Usd_SpecPathMapping &other);

    // Return true if this mapping is the same as \a other, false otherwise.
    USD_API
    bool operator==(const Usd_SpecPathMapping &other) const;

    // Return true if this mapping is not the same as \a other, false otherwise.
    USD_API
    bool operator!=(const Usd_SpecPathMapping &other) const;

    // Return true if this mapping is the null mapping.
    USD_API
    bool IsNull() const;

    // Return true if this mapping is the identity mapping.
    USD_API
    bool IsIdentity() const;

    // Map the scene path \a path to a spec path using this mapping.
    USD_API
    SdfPath MapRootToSpec(const SdfPath &path) const;

    // Get the map function in this mapping.
    USD_API
    const PcpMapFunction &GetMapFunction() const;

private:
    friend class UsdEditTarget;

    Usd_SpecPathMapping(const PcpMapFunction &mapFn,
                        const SdfPath &sitePath,
                        const SdfPath &strippedSitePath);

    PcpMapFunction _mapFn;
    SdfPath _sitePath;
    SdfPath _strippedSitePath;
};

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
    USD_API
    UsdEditTarget();

    /// Constructor.  Allow implicit conversion from SdfLayerHandle.
    /// EditTargets constructed in this way specify layers in the scene's local
    /// LayerStack.  This lets clients pass layers directly in this common case
    /// without explicitly having to construct a \a UsdEditTarget instance
    USD_API
    UsdEditTarget(const SdfLayerHandle &layer);

    /// Convenience implicit conversion from SdfLayerRefPtr.  See above
    /// constructor for more information.
    USD_API
    UsdEditTarget(const SdfLayerRefPtr &layer);

    /// Construct an EditTarget with \a layer and \a node.  The mapping
    /// will be used to map paths from the scene into the \a layer's namespace
    /// given the \a PcpNodeRef \a node's mapping.
    USD_API
    UsdEditTarget(const SdfLayerHandle &layer, const PcpNodeRef &node);

    /// Convenience constructor taking SdfLayerRefPtr.  See above
    /// constructor for more information.
    USD_API
    UsdEditTarget(const SdfLayerRefPtr &layer, const PcpNodeRef &node);

    /// Convenience constructor for editing a direct variant in a local
    /// LayerStack.  The \p varSelPath must be a prim variant selection path
    /// (see SdfPath::IsPrimVariantSelectionPath()).
    USD_API
    static UsdEditTarget
    ForLocalDirectVariant(const SdfLayerHandle &layer,
                          const SdfPath &varSelPath,
                          const PcpLayerStackIdentifier &lsid);

    /// Equality comparison.
    USD_API
    bool operator==(const UsdEditTarget &other) const;

    /// Inequality comparison.
    bool operator!=(const UsdEditTarget &other) const {
        return not (*this == other);
    }

    /// Return true if this EditTarget is null.  Null EditTargets map
    /// paths unchanged, and have no layer or LayerStack identifier.
    bool IsNull() const { return *this == UsdEditTarget(); }

    /// Return true if this EditTarget is valid, false otherwise.  Edit
    /// targets are considered valid when they have a layer.
    bool IsValid() const { return _layer; }

    /// Return the layer this EditTarget contains.
    const SdfLayerHandle &GetLayer() const { return _layer; }

    /// Return the LayerStack identifier this EditTarget contains.
    const PcpLayerStackIdentifier &GetLayerStackIdentifier() const {
        return _lsid;
    }

    /// Return true if this EditTarget has a non-null mapping, false
    /// otherwise.  Practically, an EditTarget has a mapping when it indicates
    /// a location that's not direct opinions in the local LayerStack.  In
    /// other words if it represents a point in composition across some arc,
    /// like a reference, inherit, or variant.
    USD_API
    bool HasMapping() const;

    /// Return true if this EditTarget represents editing direct
    /// opinions in a layer in the scene's local LayerStack.  False otherwise.
    USD_API
    bool IsLocalLayer() const;

    /// Map the provided \a scenePath into the a SdfSpec path for the
    /// EditTarget's layer, according to the EditTarget's mapping.  Null edit
    /// targets and EditTargets for which \a IsLocalLayer are true return
    /// scenePath unchanged.
    USD_API
    SdfPath MapToSpecPath(const SdfPath &scenePath) const;

    /// Convenience function for getting the PrimSpec in the edit
    /// target's layer for \a scenePath.  This is equivalent to
    /// target.GetLayer()->GetPrimAtPath(target.MapToSpecPath(scenePath)) if
    /// target has a valid layer.  If this target IsNull or there is no valid
    /// mapping from \a scenePath to a SdfPrimSpec path in the layer, return
    /// null.
    USD_API
    SdfPrimSpecHandle
    GetPrimSpecForScenePath(const SdfPath &scenePath) const;

    USD_API
    SdfPropertySpecHandle
    GetPropertySpecForScenePath(const SdfPath &scenePath) const;

    USD_API
    SdfSpecHandle
    GetSpecForScenePath(const SdfPath &scenePath) const;

    /// Return true if this EditTarget matches \p node.  That is, if the
    /// node's LayerStack and mapping matches this EditTarget's LayerStack
    /// and mapping.  Note that this does not check whether or not this edit
    /// target's layer is a member of the node's LayerStack.
    USD_API
    bool IsAtNode(const PcpNodeRef &node) const;

    /// Return a new EditTarget composed over \a weaker.  This is
    /// typically used to make an EditTarget "explicit".  For example, an edit
    /// target with a layer but with no mapping and no LayerStack identifier
    /// indicates a layer in the local LayerStack of a composed scene.
    /// However, an EditTarget with the same layer but an explicit identity
    /// mapping and the LayerStack identifier of the composed scene may be
    /// desired.  This can be obtained by composing a partial (e.g. layer only)
    /// EditTarget over an explicit EditTarget with layer, mapping and layer
    /// stack identifier.
    USD_API
    UsdEditTarget ComposeOver(const UsdEditTarget &weaker) const;

private:

    UsdEditTarget(const SdfLayerHandle &layer,
                  const Usd_SpecPathMapping &mapping,
                  const PcpLayerStackIdentifier &lsid);

    SdfLayerHandle _layer;
    Usd_SpecPathMapping _mapping;
    PcpLayerStackIdentifier _lsid;
};

#endif // USD_EDITTARGET_H
