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
#ifndef USD_REFERENCES_H
#define USD_REFERENCES_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/reference.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfPrimSpec);

/// \class UsdReferences
///
/// UsdReferences provides an interface to authoring and introspecting
/// references in Usd.
///
/// References are the primary operator for "encapsulated aggregation" of
/// scene description. \em aggregation means that references let us
/// build up rich scenes by composing scene description recorded in a (most
/// often) different layer.  A scene can reference the same layer many times at
/// different locations in a scene's namespace.  Referenced scene description
/// can be overridden in the referencing (or stronger) layers, allowing each
/// instance of the reference to be directly customized/overridden.
/// \em Encapsulated means that regardless of how much scene description is in
/// the referenced layer, only the scene description under and composed from
/// (via other composition arcs in the referenced layer) the targeted prim
/// will be composed into the aggregate scene.  Multiple references to the
/// same layer will result in the layer being opened and retained in memory
/// only once, although each referencing prim will compose unique
/// \ref PcpPrimIndex "prim indices" for the tree rooted at the referenced prim.
///
/// \section Usd_References Important Qualities and Effective Use of References
///
/// \li Any prim can host zero, one or multiple references 
///
/// \li References are \ref SdfListOp "list editable"; that is, they compose
/// differently than ordinary properties and metadata.  In any given
/// LayerStack, each authored reference operation at the same SdfPath
/// location in each layer (i.e. on the same prim) will compose into an
/// aggregate result by adding to, removing from, or replacing "weaker"
/// references.
///
/// \li References can target the same LayerStack in which they are authored,
/// as long as doing so does not introduce a cycle in the composition graph.
/// See \ref Usd_Internal_References
///
/// \li The \c identifier component of a reference in the provided API
/// can be a resolvable asset-path to some external layer, empty, in which case 
/// the reference targets the root layer of the LayerStack containing the 
/// referencing layer, or the identifier of an existing anonymous, in-memory-only
/// SdfLayer.  Care should be exercised in the latter case: calling Export() on 
/// an anonymous layer to serialize it to a file will not attempt to replace 
/// any references to anonymous layers with references to file-backed layers.
///
/// \li Opinions brought in by reference on an ancestor prim are weaker than
/// opinions brought in by references on a descendant prim.
///
/// \subsection Usd_DefaultPrim_References Expressing references without prim paths
///
/// References may omit the target prim path if the referenced layer has the
/// 'defaultPrim' metadata set.  In this case, the reference targets the
/// 'defaultPrim' in the referenced layer. A layer's defaultPrim can be
/// authored and accessed on a UsdStage whose root layer is the layer in
/// question: see UsdStage::GetDefaultPrim() and UsdStage::SetDefaultPrim().
/// One can also author defaultPrim directly on an SdfLayer - see
/// SdfLayer::GetDefaultPrim(), SdfLayer::SetDefaultPrim().
///
/// \subsection Usd_Internal_References Expressing "internal" references to the containing LayerStack
///
/// References may omit the identifier specifying the referenced layer.  This
/// creates an "internal" reference. During composition, the referenced layer 
/// will be resolved to the root layer of the LayerStack containing the 
/// layer where the reference was authored.  See AddInternalReference().
///
/// \subsection Usd_Subroot_References Referencing sub-root prims
///
/// References may target any prim in a layer. In the simplest and most
/// common case, a root prim in a layer will be referenced. However, 
/// referencing sub-root prims can be useful in a variety of other cases;
/// for example, a user might organize prims into a meaningful hierarchy
/// in a layer for display purposes, then use sub-root references to
/// reference a selection from that hierarchy into a scene.
///
/// Sub-root references have subtle behaviors with respect to opinions
/// and composition arcs authored on ancestors of the referenced prim.
/// Users should carefully consider this when deciding whether to use
/// sub-root references. These issues can be avoided by not authoring
/// any properties or metadata on ancestors of prims that are meant to
/// be referenced.
///
/// Consider the following example:
///
/// \code
/// * shot.usda                                 | * asset.usda
///                                             |
/// #usda 1.0                                   | #usda 1.0
///                                             |
/// over "Class"                                | class "Class"
/// {                                           | {
///     over "B"                                | }
///     {                                       |
///         over "Model"                        | def "A" (
///         {                                   |    inherits = </Class>
///             int a = 3                       | )
///         }                                   | {
///     }                                       |     token purpose = "render"
/// }                                           |
///                                             |     def "B" (
/// over "A"                                    |        variantSets = "type"
/// {                                           |        variants = {
///     over "B" (                              |             string type = "a"
///         # variant selection won't be used   |        }
///         variants = {                        |     )
///             string type = "b"               |     {
///         }                                   |         variantSet "type" = {
///     )                                       |             "a" {
///     {                                       |                 def "Model"
///     }                                       |                 {
/// }                                           |                     int a = 1
///                                             |                 }
/// def "ReferencedModel" (                     |             }
///     references = @./asset.usda@</A/B/Model> |             "b" {
/// )                                           |                 def "Model"
/// {                                           |                 {
/// }                                           |                     int a = 2
///                                             |                 }
///                                             |             }
///                                             |         }
///                                             |     }
///                                             | }
/// \endcode
///
/// * Property and metadata opinions on the ancestors of the referenced prim
///   *are not* present in the composed stage and will never contribute to any
///   computations. In this example, the opinion for the attribute /A.purpose 
///   in asset.usda will never be visible in the UsdStage for shot.usda.
///
/// * Property and metadata opinions due to ancestral composition arcs
///   *are* present in the composed stage. In this example, the attribute
///   /Class/B/Model.a in shot.usda will be present in the UsdStage for
///   shot.usda, even though the inherit arc is authored on an ancestor
///   of the referenced prim.
///
/// * A consequence of these rules is that users might not be able to 
///   override ancestral variant selections that affect the referenced prim.
///   In this example, the Model prim being referenced comes from the 
///   variant selection {type=a} on prim /A/B in asset.usda. The {type=b}
///   variant cannot be selected in shot.usda, even if prims with the
///   same hierarchy happen to exist there. There are various workarounds
///   for this; in this example, the {type=b} variant selection could be
///   authored on /Class/B/Model in shot.usda instead because of the
///   inherit arc that was established on prim /A.
///
/// \subsection Usd_Failing_References Reasons why adding a reference may fail, why adding a reference may succeed but still generate errors, and what it all means
///
/// AddReference() and SetReferences() can each fail for a number of
/// reasons.  If one of the specified prim targets for one of the references
/// is not a prim, we will generate an error, fail to author any scene
/// description, and return \c false.  If anything goes wrong in attempting
/// to write the reference, we also return false, and the reference will also
/// remain unauthored.  Otherwise, if the reference was successfully
/// authored, we will return \c true.  <b>A successful reference authoring
/// operation may still generate composition errors!</b> Just because the
/// reference you specified was syntactically correct and therefore
/// successfully authored, does not imply it was meaningful. If you wish to
/// ensure that the reference you are about to author will be meaningfully
/// consumable by your stage, you are strongly encouraged to <b>ensure it
/// will resolve to an actual file by using
/// UsdStage::ResolveIdentifierToEditTarget() before authoring the
/// reference.</b>
///
/// When adding an internal reference, the given prim path is expected to 
/// be in the namespace of the owning prim's stage. Sub-root prim paths
/// will be translated from this namespace to the namespace of the
/// current edit target, if necessary. If a path cannot be translated,
/// a coding error will be issued and no changes will be made. Non-sub-root
/// paths will not be translated.
///
/// Immediately upon successful authoring of the reference (before returning
/// from AddReference(), RemoveReference(), ClearReferences(), or
/// SetReferences()), the UsdStage on which the reference was authored will
/// recompose the subtree rooted at the prim hosting the reference.  If the
/// provided identifier does not resolve to a layer that is already opened or
/// that can be opened in the usd format, \em or if the provided primPath is
/// not an actual prim in that layer, the stage's recomposition will
/// fail, and pass on composition errors to the client.
///
class UsdReferences {
    friend class UsdPrim;

    explicit UsdReferences(const UsdPrim& prim) : _prim(prim) {}

public:
    /// Adds a reference to the reference listOp at the current EditTarget,
    /// in the position specified by \p position.
    /// \sa \ref Usd_Failing_References "Why adding references may fail" for
    /// explanation of expectations on \p ref and what return values and errors
    /// to expect, and \ref Usd_OM_ListOps for details on list editing and
    /// composition of listOps.
    USD_API
    bool AddReference(const SdfReference& ref,
                      UsdListPosition position=UsdListPositionTempDefault);

    /// \overload 
    USD_API
    bool AddReference(const std::string &identifier,
                      const SdfPath &primPath,
                      const SdfLayerOffset &layerOffset = SdfLayerOffset(),
                      UsdListPosition position=UsdListPositionTempDefault);

    /// \overload
    /// \sa \ref Usd_DefaultPrim_References "References Without Prim Paths"
    USD_API
    bool AddReference(const std::string &identifier,
                      const SdfLayerOffset &layerOffset = SdfLayerOffset(),
                      UsdListPosition position=UsdListPositionTempDefault);

    /// Add an internal reference to the specified prim.
    /// \sa \ref Usd_Internal_References "Internal References"
    USD_API
    bool AddInternalReference(const SdfPath &primPath,
                      const SdfLayerOffset &layerOffset = SdfLayerOffset(),
                      UsdListPosition position=UsdListPositionTempDefault);

    /// Removes the specified reference from the references listOp at the
    /// current EditTarget.  This does not necessarily eliminate the 
    /// reference completely, as it may be added or set in another layer in
    /// the same LayerStack as the current EditTarget.
    /// \sa \ref Usd_OM_ListOps 
    USD_API
    bool RemoveReference(const SdfReference& ref);

    /// Removes the authored reference listOp edits at the current EditTarget.
    /// The same caveats for Remove() apply to Clear().  In fact, Clear() may
    /// actually increase the number of composed references, if the listOp
    /// being cleared contained the "remove" operator.
    /// \sa \ref Usd_OM_ListOps 
    USD_API
    bool ClearReferences();

    /// Explicitly set the references, potentially blocking weaker opinions
    /// that add or remove items.
    /// \sa \ref Usd_Failing_References "Why adding references may fail" for
    /// explanation of expectations on \p ref and what return values and errors
    /// to expect, and \ref Usd_OM_ListOps for details on list editing and
    /// composition of listOps.
    USD_API
    bool SetReferences(const SdfReferenceVector& items);

    /// Return the prim this object is bound to.
    const UsdPrim &GetPrim() const { return _prim; }

    /// \overload
    UsdPrim GetPrim() { return _prim; }

    explicit operator bool() { return bool(_prim); }

private:

    SdfPrimSpecHandle _CreatePrimSpecForEditing();
    UsdPrim _prim;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_REFERENCES_H
