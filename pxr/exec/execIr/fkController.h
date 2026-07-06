//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXECIR_GENERATED_FKCONTROLLER_H
#define EXECIR_GENERATED_FKCONTROLLER_H

/// \file execIr/fkController.h

#include "pxr/pxr.h"
#include "pxr/exec/execIr/api.h"
#include "pxr/exec/execIr/controller.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/exec/execIr/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// IRFKCONTROLLER                                                             //
// -------------------------------------------------------------------------- //

/// \class ExecIrFkController
///
/// 
/// A simple invertible controller that computes a forward kinematic transform.
/// 
/// @warning
/// The functionality provided by this schema is very limited, subject to
/// change, and not yet ready for production use.
/// 
/// An FK controller takes as inputs scalar translate and rotate attributes and
/// parent and default spaces, and computes a posed space that can be used to
/// drive model posing. The parent space input allows one ExecIrFkController to
/// be the child of another, linking them in a hierarchy. A hierarchy of FK
/// controllers functions like a transform hierarchy, with the child inheriting
/// the transformation of the parent. The resulting network of controllers can
/// be used collectively to pose a model or some part of a model, such as an
/// arm.
/// 
/// An FK controller also defines an inverse computation that takes as inputs a
/// desired posed space and the parent and default spaces, and computes the FK
/// attribute values that produce the desired pose. This capability enables
/// authoring operations that need to compute inverses. E.g., a model can be
/// posed by multiple rigs, and inversion can be used to compute the
/// compensating input values needed to switch from one rig to another, while
/// maintaining the same pose.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref ExecIrTokens.
/// So to set an attribute to the value "rightHanded", use ExecIrTokens->rightHanded
/// as the value.
///
class ExecIrFkController : public ExecIrController
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a ExecIrFkController on UsdPrim \p prim .
    /// Equivalent to ExecIrFkController::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit ExecIrFkController(const UsdPrim& prim=UsdPrim())
        : ExecIrController(prim)
    {
    }

    /// Construct a ExecIrFkController on the prim held by \p schemaObj .
    /// Should be preferred over ExecIrFkController(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit ExecIrFkController(const UsdSchemaBase& schemaObj)
        : ExecIrController(schemaObj)
    {
    }

    /// Destructor.
    EXECIR_API
    virtual ~ExecIrFkController();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    EXECIR_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a ExecIrFkController holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// ExecIrFkController(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    EXECIR_API
    static ExecIrFkController
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
    EXECIR_API
    static ExecIrFkController
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    EXECIR_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    EXECIR_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    EXECIR_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // PARENTINSPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// If the FK controller has a parent controller, this attribute takes the
    /// parent's posed space as input.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `matrix4d parentIn:space = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )` |
    /// | C++ Type | GfMatrix4d |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Matrix4d |
    EXECIR_API
    UsdAttribute GetParentInSpaceAttr() const;

    /// See GetParentInSpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateParentInSpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PARENTINDEFAULTSPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// If the FK controller has a parent controller, this attribute takes the
    /// parent's default space as input.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `matrix4d parentIn:defaultSpace = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )` |
    /// | C++ Type | GfMatrix4d |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Matrix4d |
    EXECIR_API
    UsdAttribute GetParentInDefaultSpaceAttr() const;

    /// See GetParentInDefaultSpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateParentInDefaultSpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // OUTSPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// A 4x4 matrix that represents the computed output space produced by the
    /// controller.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `matrix4d out:space = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )` |
    /// | C++ Type | GfMatrix4d |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Matrix4d |
    EXECIR_API
    UsdAttribute GetOutSpaceAttr() const;

    /// See GetOutSpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateOutSpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // OUTDEFAULTSPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// Passes through the value of the 'in:defaultSpace' attribute.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `matrix4d out:defaultSpace = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )` |
    /// | C++ Type | GfMatrix4d |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Matrix4d |
    EXECIR_API
    UsdAttribute GetOutDefaultSpaceAttr() const;

    /// See GetOutDefaultSpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateOutDefaultSpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INDEFAULTSPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// A local-to-world space transform representing the "zero" position for
    /// posing.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `matrix4d in:defaultSpace = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )` |
    /// | C++ Type | GfMatrix4d |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Matrix4d |
    EXECIR_API
    UsdAttribute GetInDefaultSpaceAttr() const;

    /// See GetInDefaultSpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInDefaultSpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTX 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double in:tx = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    EXECIR_API
    UsdAttribute GetInTxAttr() const;

    /// See GetInTxAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInTxAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double in:ty = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    EXECIR_API
    UsdAttribute GetInTyAttr() const;

    /// See GetInTyAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInTyAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTZ 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double in:tz = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    EXECIR_API
    UsdAttribute GetInTzAttr() const;

    /// See GetInTzAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInTzAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INRX 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double in:rx = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    EXECIR_API
    UsdAttribute GetInRxAttr() const;

    /// See GetInRxAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInRxAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INRY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double in:ry = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    EXECIR_API
    UsdAttribute GetInRyAttr() const;

    /// See GetInRyAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInRyAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INRZ 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double in:rz = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    EXECIR_API
    UsdAttribute GetInRzAttr() const;

    /// See GetInRzAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInRzAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INRSPIN 
    // --------------------------------------------------------------------- //
    /// 
    /// The rotation attribute that gets zeroed out when solving for rotation
    /// values.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double in:rspin = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    EXECIR_API
    UsdAttribute GetInRspinAttr() const;

    /// See GetInRspinAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInRspinAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INROTATIONORDER 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token in:rotationOrder = "XYZ"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref ExecIrTokens "Allowed Values" | XYZ, XZY, YXZ, YZX, ZXY, ZYX |
    EXECIR_API
    UsdAttribute GetInRotationOrderAttr() const;

    /// See GetInRotationOrderAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateInRotationOrderAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
