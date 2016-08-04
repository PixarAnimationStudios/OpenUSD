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
#ifndef USDSHADE_GENERATED_PSHADER_H
#define USDSHADE_GENERATED_PSHADER_H

#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PSHADER                                                                    //
// -------------------------------------------------------------------------- //

///
class UsdShadePShader : public UsdTyped
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a UsdShadePShader on UsdPrim \p prim .
    /// Equivalent to UsdShadePShader::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadePShader(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdShadePShader on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadePShader(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadePShader(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadePShader();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// \brief Return a UsdShadePShader holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadePShader(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadePShader
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// \brief Attempt to ensure a \a UsdPrim adhering to this schema at \p path
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
    USDSHADE_API
    static UsdShadePShader
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSHADE_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // SLOPATH 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDSHADE_API
    UsdAttribute GetSloPathAttr() const;

    /// See GetSloPathAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateSloPathAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHADERPROTOCOL 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: std::string
    /// \n  Usd Type: SdfValueTypeNames->String
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSHADE_API
    UsdAttribute GetShaderProtocolAttr() const;

    /// See GetShaderProtocolAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateShaderProtocolAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHADERTYPE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: std::string
    /// \n  Usd Type: SdfValueTypeNames->String
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSHADE_API
    UsdAttribute GetShaderTypeAttr() const;

    /// See GetShaderTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateShaderTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISPLAYCOLOR 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Color3f
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback#
    USDSHADE_API
    UsdAttribute GetDisplayColorAttr() const;

    /// See GetDisplayColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateDisplayColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISPLAYOPACITY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDSHADE_API
    UsdAttribute GetDisplayOpacityAttr() const;

    /// See GetDisplayOpacityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateDisplayOpacityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

#endif
