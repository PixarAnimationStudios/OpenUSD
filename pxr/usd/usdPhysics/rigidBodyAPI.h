//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPHYSICS_GENERATED_RIGIDBODYAPI_H
#define USDPHYSICS_GENERATED_RIGIDBODYAPI_H

/// \file usdPhysics/rigidBodyAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdPhysics/tokens.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/quatf.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PHYSICSRIGIDBODYAPI                                                        //
// -------------------------------------------------------------------------- //

/// \class UsdPhysicsRigidBodyAPI
///
/// Applies physics body attributes to any UsdGeomXformable prim and
/// marks that prim to be driven by a simulation. If a simulation is running
/// it will update this prim's pose. All prims in the hierarchy below this 
/// prim should move accordingly.
///
class UsdPhysicsRigidBodyAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdPhysicsRigidBodyAPI on UsdPrim \p prim .
    /// Equivalent to UsdPhysicsRigidBodyAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdPhysicsRigidBodyAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdPhysicsRigidBodyAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdPhysicsRigidBodyAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdPhysicsRigidBodyAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDPHYSICS_API
    virtual ~UsdPhysicsRigidBodyAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDPHYSICS_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdPhysicsRigidBodyAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdPhysicsRigidBodyAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDPHYSICS_API
    static UsdPhysicsRigidBodyAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDPHYSICS_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "PhysicsRigidBodyAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdPhysicsRigidBodyAPI object is returned upon success. 
    /// An invalid (or empty) UsdPhysicsRigidBodyAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDPHYSICS_API
    static UsdPhysicsRigidBodyAPI 
    Apply(const UsdPrim &prim);

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
    // RIGIDBODYENABLED 
    // --------------------------------------------------------------------- //
    /// Determines if this PhysicsRigidBodyAPI is enabled.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool physics:rigidBodyEnabled = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDPHYSICS_API
    UsdAttribute GetRigidBodyEnabledAttr() const;

    /// See GetRigidBodyEnabledAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateRigidBodyEnabledAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // KINEMATICENABLED 
    // --------------------------------------------------------------------- //
    /// Determines whether the body is kinematic or not. A kinematic 
    /// body is a body that is moved through animated poses or through 
    /// user defined poses. The simulation derives velocities for the
    /// kinematic body based on the external motion. When a continuous motion
    /// is not desired, this kinematic flag should be set to false.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool physics:kinematicEnabled = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDPHYSICS_API
    UsdAttribute GetKinematicEnabledAttr() const;

    /// See GetKinematicEnabledAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateKinematicEnabledAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STARTSASLEEP 
    // --------------------------------------------------------------------- //
    /// Determines if the body is asleep when the simulation starts.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool physics:startsAsleep = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDPHYSICS_API
    UsdAttribute GetStartsAsleepAttr() const;

    /// See GetStartsAsleepAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateStartsAsleepAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VELOCITY 
    // --------------------------------------------------------------------- //
    /// Linear velocity in the same space as the node's xform. 
    /// Units: distance/second.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `vector3f physics:velocity = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Vector3f |
    USDPHYSICS_API
    UsdAttribute GetVelocityAttr() const;

    /// See GetVelocityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateVelocityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ANGULARVELOCITY 
    // --------------------------------------------------------------------- //
    /// Angular velocity in the same space as the node's xform. 
    /// Units: degrees/second.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `vector3f physics:angularVelocity = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Vector3f |
    USDPHYSICS_API
    UsdAttribute GetAngularVelocityAttr() const;

    /// See GetAngularVelocityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateAngularVelocityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SIMULATIONOWNER 
    // --------------------------------------------------------------------- //
    /// Single PhysicsScene that will simulate this body. By 
    /// default this is the first PhysicsScene found in the stage using 
    /// UsdStage::Traverse().
    ///
    USDPHYSICS_API
    UsdRelationship GetSimulationOwnerRel() const;

    /// See GetSimulationOwnerRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDPHYSICS_API
    UsdRelationship CreateSimulationOwnerRel() const;

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

    /// Mass information for a collision, used in ComputeMassProperties MassInformationFn callback
    struct MassInformation
    {
        float volume;           //< Collision volume
        GfMatrix3f inertia;     //< Collision inertia
        GfVec3f centerOfMass;   //< Collision center of mass
        GfVec3f localPos;       //< Collision local position with respect to the rigid body
        GfQuatf localRot;       //< Collision local rotation with respect to the rigid body
    };

    /// Mass information function signature, for given UsdPrim gather MassInformation
    typedef MassInformation MassInformationFnSig(const UsdPrim&);    
    typedef std::function<MassInformationFnSig> MassInformationFn;

    /// Compute mass properties of the rigid body
    /// \p diagonalInertia Computed diagonal of the inertial tensor for the rigid body.
    /// \p com Computed center of mass for the rigid body.
    /// \p principalAxes Inertia tensor's principal axes orienttion for the rigid body.
    /// \p massInfoFn Callback function to get collision mass information.
    /// \return Computed mass of the rigid body
    USDPHYSICS_API
    float ComputeMassProperties(GfVec3f* diagonalInertia, GfVec3f* com, GfQuatf* principalAxes, 
        const MassInformationFn& massInfoFn) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
