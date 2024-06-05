//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPHYSICS_GENERATED_COLLISIONGROUP_H
#define USDPHYSICS_GENERATED_COLLISIONGROUP_H

/// \file usdPhysics/collisionGroup.h

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdPhysics/tokens.h"

#include "pxr/usd/usd/collectionAPI.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PHYSICSCOLLISIONGROUP                                                      //
// -------------------------------------------------------------------------- //

/// \class UsdPhysicsCollisionGroup
///
/// Defines a collision group for coarse filtering. When a collision 
/// occurs between two objects that have a PhysicsCollisionGroup assigned,
/// they will collide with each other unless this PhysicsCollisionGroup pair 
/// is filtered. See filteredGroups attribute.
/// 
/// A CollectionAPI:colliders maintains a list of PhysicsCollisionAPI rel-s that 
/// defines the members of this Collisiongroup.
/// 
///
class UsdPhysicsCollisionGroup : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdPhysicsCollisionGroup on UsdPrim \p prim .
    /// Equivalent to UsdPhysicsCollisionGroup::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdPhysicsCollisionGroup(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdPhysicsCollisionGroup on the prim held by \p schemaObj .
    /// Should be preferred over UsdPhysicsCollisionGroup(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdPhysicsCollisionGroup(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDPHYSICS_API
    virtual ~UsdPhysicsCollisionGroup();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDPHYSICS_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdPhysicsCollisionGroup holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdPhysicsCollisionGroup(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDPHYSICS_API
    static UsdPhysicsCollisionGroup
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDPHYSICS_API
    static UsdPhysicsCollisionGroup
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDPHYSICS_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDPHYSICS_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDPHYSICS_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // MERGEGROUPNAME 
    // --------------------------------------------------------------------- //
    /// If non-empty, any collision groups in a stage with a matching
    /// mergeGroup should be considered to refer to the same collection. Matching
    /// collision groups should behave as if there were a single group containing
    /// referenced colliders and filter groups from both collections.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string physics:mergeGroup` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDPHYSICS_API
    UsdAttribute GetMergeGroupNameAttr() const;

    /// See GetMergeGroupNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateMergeGroupNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INVERTFILTEREDGROUPS 
    // --------------------------------------------------------------------- //
    /// Normally, the filter will disable collisions against the selected
    /// filter groups. However, if this option is set, the filter will disable
    /// collisions against all colliders except for those in the selected filter
    /// groups.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool physics:invertFilteredGroups` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDPHYSICS_API
    UsdAttribute GetInvertFilteredGroupsAttr() const;

    /// See GetInvertFilteredGroupsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateInvertFilteredGroupsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FILTEREDGROUPS 
    // --------------------------------------------------------------------- //
    /// References a list of PhysicsCollisionGroups with which 
    /// collisions should be ignored.
    ///
    USDPHYSICS_API
    UsdRelationship GetFilteredGroupsRel() const;

    /// See GetFilteredGroupsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDPHYSICS_API
    UsdRelationship CreateFilteredGroupsRel() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// Return the UsdCollectionAPI interface used for defining
    /// what colliders belong to the CollisionGroup.
    USDPHYSICS_API
    UsdCollectionAPI GetCollidersCollectionAPI() const;    

    /// Utility structure generated by ComputeCollisionGroupTable(); contains a 
    /// table describing which pairs of collision groups have collisions 
    /// enabled/disabled by the filtering rules.
    struct CollisionGroupTable
    {
        /// Return the set of all UsdPhysicsCollisionGroup which this table 
        /// contains
        USDPHYSICS_API
        const SdfPathVector& GetCollisionGroups() const;

        /// Return true if the groups at indices \a idxA and \a idxB collide
        USDPHYSICS_API
        bool IsCollisionEnabled(const unsigned int idxA, 
                const unsigned int idxB) const;

        /// Return true if the groups \a primA and \a primB collide
        USDPHYSICS_API
        bool IsCollisionEnabled(const SdfPath& primA, 
                const SdfPath& primB) const;

    protected:
        friend class UsdPhysicsCollisionGroup;
        // < All collision groups known to this table
        SdfPathVector _groups; 
        // < 2D table, with one element per collision-group-pair. Entry is false 
        // if collision is disabled by a filtering rule.
        std::vector<bool> _enabled; 
    };

    /// Compute a table encoding all the collision groups filter rules for a 
    /// stage. This can be used as a reference to validate an implementation of 
    /// the collision groups filters. The returned table is diagonally 
    /// symmetric.
    static USDPHYSICS_API
    CollisionGroupTable ComputeCollisionGroupTable(const UsdStage& stage);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
