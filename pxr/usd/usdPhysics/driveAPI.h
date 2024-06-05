//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPHYSICS_GENERATED_DRIVEAPI_H
#define USDPHYSICS_GENERATED_DRIVEAPI_H

/// \file usdPhysics/driveAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
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
// PHYSICSDRIVEAPI                                                            //
// -------------------------------------------------------------------------- //

/// \class UsdPhysicsDriveAPI
///
/// The PhysicsDriveAPI when applied to any joint primitive will drive
/// the joint towards a given target. The PhysicsDriveAPI is a multipleApply 
/// schema: drive can be set per axis "transX", "transY", "transZ", "rotX", 
/// "rotY", "rotZ" or its "linear" for prismatic joint or "angular" for revolute 
/// joints. Setting these as a multipleApply schema TfToken name will 
/// define the degree of freedom the DriveAPI is applied to. Each drive is an 
/// implicit force-limited damped spring: 
/// Force or acceleration = stiffness * (targetPosition - position) 
/// + damping * (targetVelocity - velocity)
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdPhysicsTokens.
/// So to set an attribute to the value "rightHanded", use UsdPhysicsTokens->rightHanded
/// as the value.
///
class UsdPhysicsDriveAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdPhysicsDriveAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdPhysicsDriveAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "drive:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdPhysicsDriveAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdPhysicsDriveAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdPhysicsDriveAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdPhysicsDriveAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDPHYSICS_API
    virtual ~UsdPhysicsDriveAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDPHYSICS_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDPHYSICS_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdPhysicsDriveAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.drive:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdPhysicsDriveAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDPHYSICS_API
    static UsdPhysicsDriveAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdPhysicsDriveAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdPhysicsDriveAPI(prim, name);
    USDPHYSICS_API
    static UsdPhysicsDriveAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdPhysicsDriveAPI on the 
    /// given \p prim.
    USDPHYSICS_API
    static std::vector<UsdPhysicsDriveAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of PhysicsDriveAPI.
    USDPHYSICS_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// PhysicsDriveAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDPHYSICS_API
    static bool
    IsPhysicsDriveAPIPath(const SdfPath &path, TfToken *name);

    /// Returns true if this <b>multiple-apply</b> API schema can be applied,
    /// with the given instance name, \p name, to the given \p prim. If this 
    /// schema can not be a applied the prim, this returns false and, if 
    /// provided, populates \p whyNot with the reason it can not be applied.
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
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "PhysicsDriveAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'PhysicsDriveAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdPhysicsDriveAPI object is returned upon success. 
    /// An invalid (or empty) UsdPhysicsDriveAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDPHYSICS_API
    static UsdPhysicsDriveAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

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
    // TYPE 
    // --------------------------------------------------------------------- //
    /// Drive spring is for the acceleration at the joint (rather 
    /// than the force).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token physics:type = "force"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdPhysicsTokens "Allowed Values" | force, acceleration |
    USDPHYSICS_API
    UsdAttribute GetTypeAttr() const;

    /// See GetTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MAXFORCE 
    // --------------------------------------------------------------------- //
    /// Maximum force that can be applied to drive. Units: 
    /// if linear drive: mass*DIST_UNITS/second/second
    /// if angular drive: mass*DIST_UNITS*DIST_UNITS/second/second
    /// inf means not limited. Must be non-negative.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float physics:maxForce = inf` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDPHYSICS_API
    UsdAttribute GetMaxForceAttr() const;

    /// See GetMaxForceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateMaxForceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TARGETPOSITION 
    // --------------------------------------------------------------------- //
    /// Target value for position. Units: 
    /// if linear drive: distance
    /// if angular drive: degrees.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float physics:targetPosition = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDPHYSICS_API
    UsdAttribute GetTargetPositionAttr() const;

    /// See GetTargetPositionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateTargetPositionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TARGETVELOCITY 
    // --------------------------------------------------------------------- //
    /// Target value for velocity. Units: 
    /// if linear drive: distance/second
    /// if angular drive: degrees/second.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float physics:targetVelocity = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDPHYSICS_API
    UsdAttribute GetTargetVelocityAttr() const;

    /// See GetTargetVelocityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateTargetVelocityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DAMPING 
    // --------------------------------------------------------------------- //
    /// Damping of the drive. Units: 
    /// if linear drive: mass/second
    /// If angular drive: mass*DIST_UNITS*DIST_UNITS/second/degrees.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float physics:damping = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDPHYSICS_API
    UsdAttribute GetDampingAttr() const;

    /// See GetDampingAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateDampingAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STIFFNESS 
    // --------------------------------------------------------------------- //
    /// Stiffness of the drive. Units:
    /// if linear drive: mass/second/second
    /// if angular drive: mass*DIST_UNITS*DIST_UNITS/degrees/second/second.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float physics:stiffness = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDPHYSICS_API
    UsdAttribute GetStiffnessAttr() const;

    /// See GetStiffnessAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPHYSICS_API
    UsdAttribute CreateStiffnessAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
