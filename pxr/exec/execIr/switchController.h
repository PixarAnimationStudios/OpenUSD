//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXECIR_GENERATED_SWITCHCONTROLLER_H
#define EXECIR_GENERATED_SWITCHCONTROLLER_H

/// \file execIr/switchController.h

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
// IRSWITCHCONTROLLER                                                         //
// -------------------------------------------------------------------------- //

/// \class ExecIrSwitchController
///
/// 
/// An invertible controller that can determine which of several rigs are active
/// at a given time, depending on the value of a switch attribute.
/// 
/// @warning
/// The functionality provided by this schema is very limited, subject to
/// change, and not yet ready for production use. In particular, this schema is
/// hard coded to support two rigs, named "rig1" and "rig2". In the future, a
/// general switch controller schema will likely be configurable via application
/// of multi-apply schemas. This will allow switch controllers to switch among
/// more than two rigs with arbitrary names.
/// 
/// The ExecIrSwitchController has two input attributes and one output
/// attribute, which provides the value of one of the two inputs attributes,
/// depending on the value of the 'switch' attribute. Multiple switch
/// controllers can be ganged together to switch between two different rigs.    
/// 
///
class ExecIrSwitchController : public ExecIrController
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a ExecIrSwitchController on UsdPrim \p prim .
    /// Equivalent to ExecIrSwitchController::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit ExecIrSwitchController(const UsdPrim& prim=UsdPrim())
        : ExecIrController(prim)
    {
    }

    /// Construct a ExecIrSwitchController on the prim held by \p schemaObj .
    /// Should be preferred over ExecIrSwitchController(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit ExecIrSwitchController(const UsdSchemaBase& schemaObj)
        : ExecIrController(schemaObj)
    {
    }

    /// Destructor.
    EXECIR_API
    virtual ~ExecIrSwitchController();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    EXECIR_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a ExecIrSwitchController holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// ExecIrSwitchController(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    EXECIR_API
    static ExecIrSwitchController
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
    static ExecIrSwitchController
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
    // RIG1SPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// Takes as input the value of rig1's 'out:posed' space.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `matrix4d rig1:space = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )` |
    /// | C++ Type | GfMatrix4d |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Matrix4d |
    EXECIR_API
    UsdAttribute GetRig1SpaceAttr() const;

    /// See GetRig1SpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateRig1SpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // RIG2SPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// Takes as input the value of rig2's 'out:posed' space.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `matrix4d rig2:space = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )` |
    /// | C++ Type | GfMatrix4d |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Matrix4d |
    EXECIR_API
    UsdAttribute GetRig2SpaceAttr() const;

    /// See GetRig2SpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    EXECIR_API
    UsdAttribute CreateRig2SpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // OUTSPACE 
    // --------------------------------------------------------------------- //
    /// 
    /// Produces either the value of 'rig1:space' or 'rig2:space' depending on
    /// the value of the 'switch' attribute.
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
