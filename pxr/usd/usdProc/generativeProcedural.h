//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPROC_GENERATED_GENERATIVEPROCEDURAL_H
#define USDPROC_GENERATED_GENERATIVEPROCEDURAL_H

/// \file usdProc/generativeProcedural.h

#include "pxr/pxr.h"
#include "pxr/usd/usdProc/api.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdProc/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// GENERATIVEPROCEDURAL                                                       //
// -------------------------------------------------------------------------- //

/// \class UsdProcGenerativeProcedural
///
/// 
/// Represents an abstract generative procedural prim which delivers its input
/// parameters via properties (including relationships) within the "primvars:"
/// namespace.
/// 
/// It does not itself have any awareness or participation in the execution of
/// the procedural but rather serves as a means of delivering a procedural's
/// definition and input parameters.
/// 
/// The value of its "proceduralSystem" property (either authored or provided
/// by API schema fallback) indicates to which system the procedural definition
/// is meaningful.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdProcTokens.
/// So to set an attribute to the value "rightHanded", use UsdProcTokens->rightHanded
/// as the value.
///
class UsdProcGenerativeProcedural : public UsdGeomBoundable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdProcGenerativeProcedural on UsdPrim \p prim .
    /// Equivalent to UsdProcGenerativeProcedural::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdProcGenerativeProcedural(const UsdPrim& prim=UsdPrim())
        : UsdGeomBoundable(prim)
    {
    }

    /// Construct a UsdProcGenerativeProcedural on the prim held by \p schemaObj .
    /// Should be preferred over UsdProcGenerativeProcedural(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdProcGenerativeProcedural(const UsdSchemaBase& schemaObj)
        : UsdGeomBoundable(schemaObj)
    {
    }

    /// Destructor.
    USDPROC_API
    virtual ~UsdProcGenerativeProcedural();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDPROC_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdProcGenerativeProcedural holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdProcGenerativeProcedural(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDPROC_API
    static UsdProcGenerativeProcedural
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
    USDPROC_API
    static UsdProcGenerativeProcedural
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDPROC_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDPROC_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDPROC_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // PROCEDURALSYSTEM 
    // --------------------------------------------------------------------- //
    /// The name or convention of the system responsible for evaluating
    /// the procedural.
    /// NOTE: A fallback value for this is typically set via an API schema.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token proceduralSystem` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDPROC_API
    UsdAttribute GetProceduralSystemAttr() const;

    /// See GetProceduralSystemAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDPROC_API
    UsdAttribute CreateProceduralSystemAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
