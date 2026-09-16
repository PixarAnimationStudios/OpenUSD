//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPHYSICS_GENERATED_JOINT_H
#define USDPHYSICS_GENERATED_JOINT_H

/// \file usdPhysics/joint.h

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/api.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdPhysics/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;
class UsdGeomXformCache;

// -------------------------------------------------------------------------- //
// PHYSICSJOINT                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdPhysicsJoint
///
/// A joint constrains the movement of rigid bodies. Joint can be 
/// created between two rigid bodies or between one rigid body and world.
/// By default joint primitive defines a D6 joint where all degrees of 
/// freedom are free. Three linear and three angular degrees of freedom.
/// Note that default behavior is to disable collision between jointed bodies.
/// 
///
class UsdPhysicsJoint : public UsdGeomImageable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdPhysicsJoint on UsdPrim \p prim .
    /// Equivalent to UsdPhysicsJoint::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdPhysicsJoint(const UsdPrim& prim=UsdPrim())
        : UsdGeomImageable(prim)
    {
    }

    /// Construct a UsdPhysicsJoint on the prim held by \p schemaObj .
    /// Should be preferred over UsdPhysicsJoint(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdPhysicsJoint(const UsdSchemaBase& schemaObj)
        : UsdGeomImageable(schemaObj)
    {
    }

    /// Destructor.
    USDPHYSICS_API
    virtual ~UsdPhysicsJoint();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDPHYSICS_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdPhysicsJoint holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdPhysicsJoint(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDPHYSICS_API
    static UsdPhysicsJoint
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
    static UsdPhysicsJoint
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
    // LOCALPOS0 
    // --------------------------------------------------------------------- //
    /// Relative position of the joint frame to body0's frame.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `point3f physics:localPos0 = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Point3f |
    USDPHYSICS_API
    UsdAttribute GetLocalPos0Attr() const;

    /// See GetLocalPos0Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateLocalPos0Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LOCALROT0 
    // --------------------------------------------------------------------- //
    /// Relative orientation of the joint frame to body0's frame.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `quatf physics:localRot0 = (1, 0, 0, 0)` |
    /// | C++ Type | GfQuatf |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Quatf |
    USDPHYSICS_API
    UsdAttribute GetLocalRot0Attr() const;

    /// See GetLocalRot0Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateLocalRot0Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LOCALPOS1 
    // --------------------------------------------------------------------- //
    /// Relative position of the joint frame to body1's frame.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `point3f physics:localPos1 = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Point3f |
    USDPHYSICS_API
    UsdAttribute GetLocalPos1Attr() const;

    /// See GetLocalPos1Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateLocalPos1Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LOCALROT1 
    // --------------------------------------------------------------------- //
    /// Relative orientation of the joint frame to body1's frame.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `quatf physics:localRot1 = (1, 0, 0, 0)` |
    /// | C++ Type | GfQuatf |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Quatf |
    USDPHYSICS_API
    UsdAttribute GetLocalRot1Attr() const;

    /// See GetLocalRot1Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateLocalRot1Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // JOINTENABLED 
    // --------------------------------------------------------------------- //
    /// Determines if the joint is enabled.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool physics:jointEnabled = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDPHYSICS_API
    UsdAttribute GetJointEnabledAttr() const;

    /// See GetJointEnabledAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateJointEnabledAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLLISIONENABLED 
    // --------------------------------------------------------------------- //
    /// Determines if the jointed subtrees should collide or not.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool physics:collisionEnabled = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDPHYSICS_API
    UsdAttribute GetCollisionEnabledAttr() const;

    /// See GetCollisionEnabledAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateCollisionEnabledAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXCLUDEFROMARTICULATION 
    // --------------------------------------------------------------------- //
    /// Determines if the joint can be included in an Articulation.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool physics:excludeFromArticulation = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDPHYSICS_API
    UsdAttribute GetExcludeFromArticulationAttr() const;

    /// See GetExcludeFromArticulationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateExcludeFromArticulationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BREAKFORCE 
    // --------------------------------------------------------------------- //
    /// Joint break force. If set, joint is to break when this force
    /// limit is reached. (Used for linear DOFs.) 
    /// Units: mass * distance / second / second
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float physics:breakForce = inf` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDPHYSICS_API
    UsdAttribute GetBreakForceAttr() const;

    /// See GetBreakForceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateBreakForceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BREAKTORQUE 
    // --------------------------------------------------------------------- //
    /// Joint break torque. If set, joint is to break when this torque
    /// limit is reached. (Used for angular DOFs.) 
    /// Units: mass * distance * distance / second / second
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float physics:breakTorque = inf` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDPHYSICS_API
    UsdAttribute GetBreakTorqueAttr() const;

    /// See GetBreakTorqueAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateBreakTorqueAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BODY0 
    // --------------------------------------------------------------------- //
    /// Relationship to any UsdGeomXformable.
    ///
    USDPHYSICS_API
    UsdRelationship GetBody0Rel() const;

    /// See GetBody0Rel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDPHYSICS_API
    UsdRelationship CreateBody0Rel() const;

public:
    // --------------------------------------------------------------------- //
    // BODY1 
    // --------------------------------------------------------------------- //
    /// Relationship to any UsdGeomXformable.
    ///
    USDPHYSICS_API
    UsdRelationship GetBody1Rel() const;

    /// See GetBody1Rel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDPHYSICS_API
    UsdRelationship CreateBody1Rel() const;

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

    /// Get the rigid body driven by this joint's body0 relationship.
    ///
    /// The relationship target need not itself be a rigid body: it may point at
    /// a collider or other prim, in which case the owning body is the nearest
    /// ancestor (or the target itself) with an enabled UsdPhysicsRigidBodyAPI.
    /// A body whose physics:rigidBodyEnabled resolves to false takes no part in
    /// simulation and is skipped, so the search continues to the nearest
    /// enabled body above it. When body0 has no target, an invalid prim is
    /// returned (a joint with an unset body attaches to the simulation world).
    ///
    /// \return The enabled rigid body prim resolved from body0, or an invalid
    /// prim when body0 is unset or no enabled body exists in its ancestry.
    USDPHYSICS_API
    UsdPrim GetBody0() const;

    /// Get the rigid body driven by this joint's body1 relationship.
    ///
    /// See GetBody0() for how the relationship target is resolved to an enabled
    /// rigid body.
    ///
    /// \return The enabled rigid body prim resolved from body1, or an invalid
    /// prim when body1 is unset or no enabled body exists in its ancestry.
    USDPHYSICS_API
    UsdPrim GetBody1() const;

    /// Compute this joint's local pose for body0, expressed in the frame of the
    /// prim the joint attaches to on that side.
    ///
    /// This is the physically resolved pose, not simply the authored
    /// physics:localPos0 / physics:localRot0. It starts from those authored
    /// values and, when body0's relationship target is not itself the prim the
    /// joint attaches to (for example the target is a collider under the body,
    /// or an arbitrary non-body anchor prim used to place the mechanism in the
    /// world), rebases the pose into that attachment prim's frame.
    ///
    /// \note The attachment prim resolved here is more permissive than
    /// GetBody0(): even when there is no enabled rigid body in the target's
    /// ancestry, the authored pose is mapped through the target prim's world
    /// transform (this is how a mechanism is placed via a non-body anchor
    /// root). So a pose is still produced in cases where GetBody0() returns an
    /// invalid prim.
    ///
    /// \note The resolved frame's scale is baked into the returned position.
    /// Physics simulation has no notion of scale, so any scale on the body (or
    /// anchor) is folded into the local translation here. This is deliberate
    /// and means the returned position will not match the authored
    /// physics:localPos0 attribute whenever a scale is present in that frame.
    /// The orientation is normalized.
    ///
    /// \param position Set to the (scale-baked) local position.
    /// \param orientation Set to the normalized local orientation.
    /// \param xformCache Optional cache reused for the local-to-world
    /// computations. Pass one when resolving many joints in a nested mechanism
    /// so overlapping ancestor transforms are computed once rather than per
    /// call.
    /// \return Always fills the outputs. Returns true when a pose is well
    /// defined, including when body0 has no relationship target: the joint is
    /// then anchored to the simulation world and the outputs are the authored
    /// physics:localPos0 / physics:localRot0 (identity when unauthored).
    /// Returns false only when body0's relationship target is a path that does
    /// not resolve to a prim on the stage (a dangling relationship / malformed
    /// authoring); the outputs are still the authored physics:localPos0 /
    /// physics:localRot0, passed through unchanged since no frame resolved.
    USDPHYSICS_API
    bool GetLocalPose0(GfVec3f* position, GfQuatf* orientation,
                       UsdGeomXformCache* xformCache = nullptr) const;

    /// Compute this joint's local pose for body1, expressed in the frame of the
    /// prim the joint attaches to on that side.
    ///
    /// See GetLocalPose0() for how the pose is resolved and, in particular, for
    /// the notes that the attachment resolution is more permissive than
    /// GetBody1(), that the resolved frame's scale is baked into the returned
    /// position, and that an unset body1 yields the authored world-anchor pose
    /// (returning true) while a dangling target passes the authored pose
    /// through and returns false.
    ///
    /// \param position Set to the (scale-baked) local position.
    /// \param orientation Set to the normalized local orientation.
    /// \param xformCache Optional cache reused for the local-to-world
    /// computations; see GetLocalPose0().
    /// \return true if a pose is well defined (including the unset world-anchor
    /// case); false only when body1's relationship target does not resolve to a
    /// prim on the stage, in which case the authored pose is passed through.
    USDPHYSICS_API
    bool GetLocalPose1(GfVec3f* position, GfQuatf* orientation,
                       UsdGeomXformCache* xformCache = nullptr) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
