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
#ifndef USDHYDRA_GENERATED_UVTEXTURE_H
#define USDHYDRA_GENERATED_UVTEXTURE_H

#include "pxr/usd/usdHydra/texture.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdHydra/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// UVTEXTURE                                                                  //
// -------------------------------------------------------------------------- //

/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdHydraTokens.
/// So to set an attribute to the value "rightHanded", use UsdHydraTokens->rightHanded
/// as the value.
///
class UsdHydraUvTexture : public UsdHydraTexture
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdHydraUvTexture on UsdPrim \p prim .
    /// Equivalent to UsdHydraUvTexture::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdHydraUvTexture(const UsdPrim& prim=UsdPrim())
        : UsdHydraTexture(prim)
    {
    }

    /// Construct a UsdHydraUvTexture on the prim held by \p schemaObj .
    /// Should be preferred over UsdHydraUvTexture(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdHydraUvTexture(const UsdSchemaBase& schemaObj)
        : UsdHydraTexture(schemaObj)
    {
    }

    /// Destructor.
    virtual ~UsdHydraUvTexture();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// \brief Return a UsdHydraUvTexture holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdHydraUvTexture(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdHydraUvTexture
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // UV 
    // --------------------------------------------------------------------- //
    /// The uv coordinates at which to sample the texture.
    ///
    /// \n  C++ Type: GfVec2f
    /// \n  Usd Type: SdfValueTypeNames->Float2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetUvAttr() const;

    /// See GetUvAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateUvAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // WRAPS 
    // --------------------------------------------------------------------- //
    /// Specifies the wrap mode for this texture.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    /// \n  \ref UsdHydraTokens "Allowed Values": [clamp, repeat, mirror, black]
    UsdAttribute GetWrapSAttr() const;

    /// See GetWrapSAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateWrapSAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // WRAPT 
    // --------------------------------------------------------------------- //
    /// Specifies the wrap mode for this texture.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    /// \n  \ref UsdHydraTokens "Allowed Values": [clamp, repeat, mirror, black]
    UsdAttribute GetWrapTAttr() const;

    /// See GetWrapTAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateWrapTAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MINFILTER 
    // --------------------------------------------------------------------- //
    /// Specifies the minification filter mode for this texture.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    /// \n  \ref UsdHydraTokens "Allowed Values": [nearest, linear, linearMipmapLinear, linearMipmapNearest, nearestMipmapLinear, nearestMipmapNearest]
    UsdAttribute GetMinFilterAttr() const;

    /// See GetMinFilterAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateMinFilterAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MAGFILTER 
    // --------------------------------------------------------------------- //
    /// Specifies the magnification filter mode for this texture.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    /// \n  \ref UsdHydraTokens "Allowed Values": [nearest, linear]
    UsdAttribute GetMagFilterAttr() const;

    /// See GetMagFilterAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateMagFilterAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
