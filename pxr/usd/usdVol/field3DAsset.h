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
#ifndef USDVOL_GENERATED_FIELD3DASSET_H
#define USDVOL_GENERATED_FIELD3DASSET_H

/// \file usdVol/field3DAsset.h

#include "pxr/pxr.h"
#include "pxr/usd/usdVol/api.h"
#include "pxr/usd/usdVol/fieldAsset.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdVol/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// FIELD3DASSET                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdVolField3DAsset
///
/// Field3D field primitive. The FieldAsset filePath attribute must
/// specify a file in the Field3D format on disk.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdVolTokens.
/// So to set an attribute to the value "rightHanded", use UsdVolTokens->rightHanded
/// as the value.
///
class UsdVolField3DAsset : public UsdVolFieldAsset
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::ConcreteTyped;

    /// Construct a UsdVolField3DAsset on UsdPrim \p prim .
    /// Equivalent to UsdVolField3DAsset::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdVolField3DAsset(const UsdPrim& prim=UsdPrim())
        : UsdVolFieldAsset(prim)
    {
    }

    /// Construct a UsdVolField3DAsset on the prim held by \p schemaObj .
    /// Should be preferred over UsdVolField3DAsset(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdVolField3DAsset(const UsdSchemaBase& schemaObj)
        : UsdVolFieldAsset(schemaObj)
    {
    }

    /// Destructor.
    USDVOL_API
    virtual ~UsdVolField3DAsset();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDVOL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdVolField3DAsset holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdVolField3DAsset(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDVOL_API
    static UsdVolField3DAsset
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
    USDVOL_API
    static UsdVolField3DAsset
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDVOL_API
    UsdSchemaType _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDVOL_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDVOL_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // FIELDNAME 
    // --------------------------------------------------------------------- //
    /// Name of an individual field within the file specified by
    /// the filePath attribute. Clients which consume Field3D files
    /// should treat this as the Field3D field \p attribute.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token fieldName` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDVOL_API
    UsdAttribute GetFieldNameAttr() const;

    /// See GetFieldNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDVOL_API
    UsdAttribute CreateFieldNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FIELDPURPOSE 
    // --------------------------------------------------------------------- //
    /// Optional token which can be used to indicate the purpose or grouping
    /// of an individual field. Clients which consume Field3D files
    /// should treat this as the Field3D field \p name.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token fieldPurpose` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDVOL_API
    UsdAttribute GetFieldPurposeAttr() const;

    /// See GetFieldPurposeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDVOL_API
    UsdAttribute CreateFieldPurposeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FIELDINDEX 
    // --------------------------------------------------------------------- //
    /// A Field3D file can contain multiple fields with the same
    /// name. This attribute is an index used to disambiguate
    /// between these multiple fields with the same name.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int fieldIndex` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDVOL_API
    UsdAttribute GetFieldIndexAttr() const;

    /// See GetFieldIndexAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDVOL_API
    UsdAttribute CreateFieldIndexAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
