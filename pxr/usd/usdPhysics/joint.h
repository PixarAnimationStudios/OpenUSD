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
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
