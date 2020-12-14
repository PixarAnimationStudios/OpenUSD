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
#ifndef PXR_USD_USD_RELATIONSHIP_H
#define PXR_USD_USD_RELATIONSHIP_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/property.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class UsdRelationship;

/// A std::vector of UsdRelationships.
typedef std::vector<UsdRelationship> UsdRelationshipVector;

/// \class UsdRelationship
///
/// A UsdRelationship creates dependencies between scenegraph objects by 
/// allowing a prim to \em target other prims, attributes, or relationships.
///
/// \section usd_relationship_chars Relationship Characteristics
///
/// A UsdRelationship is a pointer to other objects, which are named by their
/// scenegraph paths.  When authoring relationships, the \em target parameters
/// should be scenegraph paths in the composed namespace of the UsdStage into
/// which you are authoring.  If your edits are targetted to a different
/// layer, across various composition arcs (because you specified a non-default
/// \ref UsdEditTarget), the target's path will be automatically translated
/// into the proper namespace.
///
/// A single UsdRelationship can target multiple other objects, which can be 
/// of UsdPrim, UsdAttribute, or UsdRelationship type.  UsdRelationship
/// participates in "list editing", which means that stronger layers in a
/// composed scene can add, remove, or reorder targets authored on the
/// relationship in weaker layers \em without stomping the weaker opinions,
/// although stomping behavior is still possible, via SetTargets().
///
/// An authored relationship creates a dependency of the targetting prim on
/// the targetted prim(s).  We consider these dependencies to be "load
/// dependencies", which means that when we load the targeting prim's
/// "load group", we will also load the targetted prims' load groups, to ensure
/// that all the data required to render the model containing the targeting
/// prim is composed and available.
///
/// Like UsdAttribute, UsdRelationship objects are meant to be ephemeral,
/// live on the stack, and be cheap to refetch from their owning UsdPrim.
///
/// Unlike UsdAttribute s, which can either be uniform over all time
/// or vary in value over time, UsdRelationship is <b>always uniform</b>.
/// 
/// \section usd_relationship_restrictions Relationship Restrictions
///
/// When authoring relationship targets in a stage's local LayerStack,
/// all target paths are legal (Note we may restrict this prior to launch
/// to only allowing targeting of already-extant scenegraph objects).  However,
/// a relationship target that is legal in a local LayerStack may become
/// unreachable when the stage's root layer is \em referenced into an 
/// aggregate, and will cause an error when attempting to load/compose
/// the aggregate.
///
/// This can happen because references encapsulate just the tree whose root
/// is targetted in the reference - no other scene description in the
/// referenced layer will be composed into the aggregate.  So if some
/// descendant prim of the referenced root targets a relationship to another
/// tree in the same layer, that relationship would dangle, and the client
/// will error in GetTargets() or GetForwardedTargets().
///
/// Authoring targets to objects within prototypes is not allowed, since
/// prototype prims do not have a stable identity across runs.  Consumers must 
/// author targets to the object within an instance instead.
///
/// Relationships authored in a descendent prim of a referenced prim may not
/// target the referenced prim itself or any of its immediate child properties
/// if the referencing prim is instanceable.  Allowing this would break the
/// ability for this relationship to be instanced and shared by multiple
/// instances -- it would force consumers of relationships within prototypes
/// to resolve targets in the context of each of that prototype's instances.
///
/// \section usd_relationship_forwarding Relationship Forwarding
///
/// Because a relationship can target another relationship, we can and do
/// provide the ability to resolve chained or \em forwarded relationships.
/// This can be useful in several situations, including:
///
/// \li Combining relationships with VariantSets to create demultiplexers.
///     A prim can host a relationship that serves as a "binding post" for
///     other prims to target.  The prim also hosts a "bindingVariant"
///     UsdVariantSet whose variants each modulate the target of the
///     binding-post relationship.  We can now change the \em forwarded target 
///     of all prims targetting the binding-post by simply switching the
///     bindingVariant VariantSet.  We will work through this example in
///     the USD reference manual.
/// \li Defining a relationship as part of a model's interface (so that it can
///     be targetted in model hierarchy with no models loaded), which, inside
///     the model's payload, forwards to prims useful to a client, the set of
///     which may vary depending on the model's configured VariantSets.
///
class UsdRelationship : public UsdProperty {
public:
    /// Construct an invalid relationship.
    UsdRelationship() : UsdProperty(_Null<UsdRelationship>()) {}

    /// \name Editing Relationships at Current EditTarget
    /// @{

    // XXX Should the mutation API be changed to take UsdObject
    // pointers so that we can enforce (as does Mf) that you can only
    // target extant scenegraph objects? (Note the API would still need to
    // validate those objects since it is easy to create a UsdAttribute
    // or UsdRelationship object not backed by scene description).

    /// Adds \p target to the list of targets, in the position specified
    /// by \p position.
    ///
    /// Passing paths to prototype prims or any other objects in prototypes
    /// will cause an error to be issued. It is not valid to author targets to
    /// these objects.
    ///
    /// What data this actually authors depends on what data is currently
    /// authored in the authoring layer, with respect to list-editing
    /// semantics, which we will document soon 
    USD_API
    bool AddTarget(const SdfPath& target,
           UsdListPosition position=UsdListPositionBackOfPrependList) const;

    /// Removes \p target from the list of targets.
    ///
    /// Passing paths to prototype prims or any other objects in prototypes
    /// will cause an error to be issued. It is not valid to author targets to
    /// these objects.
    USD_API
    bool RemoveTarget(const SdfPath& target) const;

    /// Make the authoring layer's opinion of the targets list explicit,
    /// and set exactly to \p targets.
    ///
    /// Passing paths to prototype prims or any other objects in prototypes
    /// will cause an error to be issued. It is not valid to author targets to
    /// these objects.
    ///
    /// If any target in \p targets is invalid, no targets will be authored
    /// and this function will return false.
    USD_API
    bool SetTargets(const SdfPathVector& targets) const;

    /// Remove all opinions about the target list from the current edit
    /// target.
    ///
    /// Only remove the spec if \p removeSpec is true (leave the spec to
    /// preserve meta-data we may have intentionally authored on the
    /// relationship)
    USD_API
    bool ClearTargets(bool removeSpec) const;

    /// Compose this relationship's targets and fill \p targets with the result.
    /// All preexisting elements in \p targets are lost.
    /// 
    /// Returns true if any target path opinions have been authored and no
    /// composition errors were encountered, returns false otherwise. 
    /// Note that authored opinions may include opinions that clear the targets 
    /// and a return value of true does not necessarily indicate that \p targets 
    /// will contain any target paths.
    ///
    /// See \ref Usd_ScenegraphInstancing_TargetsAndConnections for details on 
    /// behavior when targets point to objects beneath instance prims.
    ///
    /// The result is not cached, so will be recomputed on every query.
    USD_API
    bool GetTargets(SdfPathVector* targets) const;

    /// Compose this relationship's \em ultimate targets, taking into account
    /// "relationship forwarding", and fill \p targets with the result.  All
    /// preexisting elements in \p targets are lost. This method never inserts
    /// relationship paths in \p targets.
    ///
    /// Returns true if any of the visited relationships that are not 
    /// "purely forwarding" has an authored opinion for its target paths and
    /// no composition errors were encountered while computing any targets. 
    /// Purely forwarding, in this context, means the relationship has at least 
    /// one target but all of its targets are paths to other relationships.
    /// Note that authored opinions may include opinions that clear the targets 
    /// and a return value of true does not necessarily indicate that \p targets 
    /// will not be empty.
    /// 
    /// Returns false otherwise. When composition errors occur, this function 
    /// continues to collect successfully composed targets, but returns false 
    /// to indicate to the caller that errors occurred.
    /// 
    /// When a forwarded target cannot be determined, e.g. due to a composition
    /// error, no value is returned for that target; the alternative would be to
    /// return the relationship path at which the forwarded targets could not be
    /// composed, however this would require all callers of 
    /// GetForwardedTargets() to account for unexpected relationship paths
    /// being returned with the expected target results. For example, a
    /// particular caller may expect only prim paths in the target vector, but 
    /// when composition errors occur, relationships would be included,
    /// potentially triggering additional down stream errors.
    ///
    /// See \ref usd_relationship_forwarding for details on the semantics.
    ///
    /// The result is not cached, so will be recomputed on every query.
    USD_API
    bool GetForwardedTargets(SdfPathVector* targets) const;

    /// Returns true if any target path opinions have been authored. 
    /// Note that this may include opinions that clear targets and may not 
    /// indicate that target paths will exist for this relationship.
    USD_API
    bool HasAuthoredTargets() const;

    /// @}

private:
    friend class UsdObject;
    friend class UsdPrim;
    friend class Usd_PrimData;
    template <class A0, class A1>
    friend struct UsdPrim_TargetFinder;

    UsdRelationship(const Usd_PrimDataHandle &prim,
                    const SdfPath &proxyPrimPath,
                    const TfToken& relName)
        : UsdProperty(UsdTypeRelationship, prim, proxyPrimPath, relName) {}

    UsdRelationship(UsdObjType objType,
                    const Usd_PrimDataHandle &prim,
                    const SdfPath &proxyPrimPath,
                    const TfToken &propName)
        : UsdProperty(objType, prim, proxyPrimPath, propName) {}

    SdfRelationshipSpecHandle _CreateSpec(bool fallbackCustom=true) const;
    bool _Create(bool fallbackCustom) const;

    bool _GetForwardedTargets(SdfPathVector* targets,
                              bool includeForwardingRels) const;

    bool _GetForwardedTargetsImpl(SdfPathSet* visited, 
                                  SdfPathSet* uniqueTargets,
                                  SdfPathVector* targets,
                                  bool *foundAnyErrors,
                                  bool includeForwardingRels) const;

    SdfPath _GetTargetForAuthoring(const SdfPath &targetPath,
                                   std::string* whyNot = 0) const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_RELATIONSHIP_H
