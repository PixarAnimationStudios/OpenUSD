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
#ifndef USD_RELATIONSHIPS_H
#define USD_RELATIONSHIPS_H

#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/property.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include <string>
#include <vector>

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
/// \section usd_relationship_instancing Relationships and Instancing
///
/// Relationships may have authored targets that point to objects beneath
/// an instance prim.  Recall that these objects are exposed in the scenegraph
/// under the instance's corresponding master prim and \em not under the 
/// instance prim itself.  So, in these cases there will not be an object
/// at the authored path.  Instead, the path to the corresponding object
/// in the master prim must be used.
///
/// By default, GetTargets() and GetForwardedTargets() will automatically
/// forward authored target paths to the corresponding path in the associated
/// master prim.  This ensures that consumers will always receive the
/// appropriate target path regardless of whether the target has been made
/// instanceable.
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
/// Authoring targets to objects within masters is not allowed, since master
/// prims do not have a stable identity across runs.  Consumers must author
/// targets to the object within an instance instead.
///
/// Relationships authored in a descendent prim of a referenced prim may not
/// target the referenced prim itself or any of its immediate child properties
/// if the referencing prim is instanceable.  Allowing this would break the
/// ability for this relationship to be instanced and shared by multiple
/// instances -- it would force consumers of relationships within masters
/// to resolve targets in the context of each of that master's instances.
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
    UsdRelationship()
        : UsdProperty(UsdTypeRelationship, Usd_PrimDataHandle(), TfToken())
    {
    }

    /// \name Editing Relationships at Current EditTarget
    /// @{

    // XXX Should the mutation API be changed to take UsdObject
    // pointers so that we can enforce (as does Mf) that you can only
    // target extant scenegraph objects? (Note the API would still need to
    // validate those objects since it is easy to create a UsdAttribute
    // or UsdRelationship object not backed by scene description).

    /// Adds \p target to the list of targets.
    ///
    /// Passing paths to master prims or any other objects in masters will 
    /// cause an error to be issued. It is not valid to author targets to
    /// these objects.
    ///
    /// What data this actually authors depends on what data is currently
    /// authored in the authoring layer, with respect to list-editing
    /// semantics, which we will document soon 
    bool AddTarget(const SdfPath& target) const;

    /// Removes \p target from the list of targets.
    ///
    /// Passing paths to master prims or any other objects in masters will 
    /// cause an error to be issued. It is not valid to author targets to
    /// these objects.
    bool RemoveTarget(const SdfPath& target) const;

    /// Clears all target edits from the current EditTarget, and makes
    /// the opinion explicit, which means we are effectively resetting the
    /// composed value of the targets list to empty.
    bool BlockTargets() const;

    /// Make the authoring layer's opinion of the targets list explicit,
    /// and set exactly to \p targets.
    ///
    /// Passing paths to master prims or any other objects in masters will 
    /// cause an error to be issued. It is not valid to author targets to
    /// these objects.
    ///
    /// If any target in \p targets is invalid, no targets will be authored
    /// and this function will return false.
    bool SetTargets(const SdfPathVector& targets) const;

    /// Remove all opinions about the target list from the current edit
    /// target.
    ///
    /// Only remove the spec if \p removeSpec is true (leave the spec to
    /// preserve meta-data we may have intentionally authored on the
    /// relationship)
    bool ClearTargets(bool removeSpec) const;

    /// Compose this relationship's targets and return the result as a
    /// vector of SdfPath.
    ///
    /// By default, any relationship targets that point to a child prim or
    /// a property of a child prim beneath an instanceable prim will be
    /// forwarded to the corresponding object in the instance's master.
    /// This can be disabled by setting \p forwardToObjectsInMasters
    /// to false. This is useful when copying targets from one relationship
    /// to another, since the relationship editing API does not accept
    /// paths to prims in masters. However, note that doing this on a
    /// relationship within a master may yield different results across
    /// runs due to the selection of different instances to serve as the
    /// master's source instance.
    ///
    /// This function may return duplicate target paths if the authored
    /// targets point to objects in different instanceable prims that
    /// ultimately correspond to the same master.
    ///
    /// The result is not cached, so will be recomputed on every query.
    bool GetTargets(SdfPathVector* targets,
                    bool forwardToObjectsInMasters = true) const;

    /// Compose this relationship's \em ultimate targets, taking into
    /// account "relationship forwarding", and return the result as a vector
    /// of SdfPath. This method never returns relationship paths in the targets
    /// vector.
    ///
    /// See documentation on GetTargets for information on the 
    /// \p forwardToObjectsInMasters parameter. Note that setting this
    /// parameter to false will stop the relationship forwarding process
    /// at any targets that correspond to an object in a master. In this
    /// case, this method will never return paths to an object in a master.
    ///
    /// When composition errors occur, continue to collect successfully
    /// composed targets, but return false to indicate to the caller that
    /// errors occurred.
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
    /// See description of \ref usd_relationship_forwarding "Relationship
    /// Forwarding" for details on the semantics.
    ///
    /// The result is not cached, so will be recomputed on every query.
    bool GetForwardedTargets(SdfPathVector* targets,
                             bool forwardToObjectsInMasters = true) const;

    /// Returns true if any target path opinions have been authored. 
    /// Note that this may include opinions that clear targets and may not 
    /// indicate that target paths will exist for this relationship.
    bool HasAuthoredTargets() const;

    /// @}

private:
    friend class UsdObject;
    friend class UsdPrim;
    friend class Usd_PrimData;
    friend struct UsdPrim_TargetFinder;

    UsdRelationship(const Usd_PrimDataHandle &prim,
                    const TfToken& relName)
        : UsdProperty(UsdTypeRelationship, prim, relName) {}

    UsdRelationship(UsdObjType objType,
                    const Usd_PrimDataHandle &prim,
                    const TfToken &propName)
        : UsdProperty(objType, prim, propName) {}

    SdfRelationshipSpecHandle _CreateSpec(bool fallbackCustom=true) const;
    bool _Create(bool fallbackCustom) const;

    bool _GetForwardedTargets(SdfPathVector* targets,
                              bool includeForwardingRels,
                              bool forwardToObjectsInMasters) const;

    bool _GetForwardedTargets(SdfPathSet* visited, 
                              SdfPathSet* uniqueTargets,
                              SdfPathVector* targets,
                              bool includeForwardingRels,
                              bool forwardToObjectsInMasters) const;

    SdfPath _GetTargetForAuthoring(const SdfPath &targetPath,
                                   std::string* whyNot = 0) const;
};

#endif //USD_RELATIONSHIPS_H
