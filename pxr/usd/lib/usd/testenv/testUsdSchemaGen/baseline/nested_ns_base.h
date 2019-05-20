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
#ifndef USDCONTRIVED_GENERATED_BASE_H
#define USDCONTRIVED_GENERATED_BASE_H

/// \file usdContrived/base.h

#include "pxr/pxr.h"
#include "pxr/usd/usdContrived/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdContrived/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

namespace foo { namespace bar { namespace baz {

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// BASE                                                                       //
// -------------------------------------------------------------------------- //

/// \class UsdContrivedBase
///
/// This doc should only exist on the "Base" class.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdContrivedTokens.
/// So to set an attribute to the value "rightHanded", use UsdContrivedTokens->rightHanded
/// as the value.
///
class UsdContrivedBase : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::AbstractTyped;

    /// Construct a UsdContrivedBase on UsdPrim \p prim .
    /// Equivalent to UsdContrivedBase::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdContrivedBase(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdContrivedBase on the prim held by \p schemaObj .
    /// Should be preferred over UsdContrivedBase(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdContrivedBase(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDCONTRIVED_API
    virtual ~UsdContrivedBase();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDCONTRIVED_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdContrivedBase holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdContrivedBase(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDCONTRIVED_API
    static UsdContrivedBase
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDCONTRIVED_API
    UsdSchemaType _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDCONTRIVED_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDCONTRIVED_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // MYVARYINGTOKEN 
    // --------------------------------------------------------------------- //
    /// VariableToken attribute docs.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: VariableTokenDefault
    /// \n  \ref UsdContrivedTokens "Allowed Values": [VariableTokenAllowed1, VariabletokenAllowed2]
    USDCONTRIVED_API
    UsdAttribute GetMyVaryingTokenAttr() const;

    /// See GetMyVaryingTokenAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMyVaryingTokenAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MYUNIFORMBOOL 
    // --------------------------------------------------------------------- //
    /// Uniform bool, default false
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: False
    USDCONTRIVED_API
    UsdAttribute GetMyUniformBoolAttr() const;

    /// See GetMyUniformBoolAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMyUniformBoolAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MYDOUBLE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetMyDoubleAttr() const;

    /// See GetMyDoubleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMyDoubleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BOOL 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: True
    USDCONTRIVED_API
    UsdAttribute GetBoolAttr() const;

    /// See GetBoolAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateBoolAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UCHAR 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: unsigned char
    /// \n  Usd Type: SdfValueTypeNames->UChar
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0
    USDCONTRIVED_API
    UsdAttribute GetUcharAttr() const;

    /// See GetUcharAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateUcharAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: int
    /// \n  Usd Type: SdfValueTypeNames->Int
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0
    USDCONTRIVED_API
    UsdAttribute GetIntAttr() const;

    /// See GetIntAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateIntAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UINT 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: unsigned int
    /// \n  Usd Type: SdfValueTypeNames->UInt
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0
    USDCONTRIVED_API
    UsdAttribute GetUintAttr() const;

    /// See GetUintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateUintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT64 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: int64_t
    /// \n  Usd Type: SdfValueTypeNames->Int64
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0
    USDCONTRIVED_API
    UsdAttribute GetInt64Attr() const;

    /// See GetInt64Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt64Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UINT64 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: uint64_t
    /// \n  Usd Type: SdfValueTypeNames->UInt64
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0
    USDCONTRIVED_API
    UsdAttribute GetUint64Attr() const;

    /// See GetUint64Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateUint64Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALF 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfHalf
    /// \n  Usd Type: SdfValueTypeNames->Half
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
    USDCONTRIVED_API
    UsdAttribute GetHalfAttr() const;

    /// See GetHalfAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalfAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOAT 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
    USDCONTRIVED_API
    UsdAttribute GetFloatAttr() const;

    /// See GetFloatAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloatAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
    USDCONTRIVED_API
    UsdAttribute GetDoubleAttr() const;

    /// See GetDoubleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDoubleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STRING 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: std::string
    /// \n  Usd Type: SdfValueTypeNames->String
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: string
    USDCONTRIVED_API
    UsdAttribute GetStringAttr() const;

    /// See GetStringAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateStringAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TOKEN 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: token
    USDCONTRIVED_API
    UsdAttribute GetTokenAttr() const;

    /// See GetTokenAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateTokenAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ASSET 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: @asset@
    USDCONTRIVED_API
    UsdAttribute GetAssetAttr() const;

    /// See GetAssetAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateAssetAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT2 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec2i
    /// \n  Usd Type: SdfValueTypeNames->Int2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0)
    USDCONTRIVED_API
    UsdAttribute GetInt2Attr() const;

    /// See GetInt2Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt2Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT3 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3i
    /// \n  Usd Type: SdfValueTypeNames->Int3
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetInt3Attr() const;

    /// See GetInt3Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt3Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT4 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec4i
    /// \n  Usd Type: SdfValueTypeNames->Int4
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetInt4Attr() const;

    /// See GetInt4Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt4Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALF2 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec2h
    /// \n  Usd Type: SdfValueTypeNames->Half2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0)
    USDCONTRIVED_API
    UsdAttribute GetHalf2Attr() const;

    /// See GetHalf2Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalf2Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALF3 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3h
    /// \n  Usd Type: SdfValueTypeNames->Half3
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetHalf3Attr() const;

    /// See GetHalf3Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalf3Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALF4 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec4h
    /// \n  Usd Type: SdfValueTypeNames->Half4
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetHalf4Attr() const;

    /// See GetHalf4Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalf4Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOAT2 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec2f
    /// \n  Usd Type: SdfValueTypeNames->Float2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0)
    USDCONTRIVED_API
    UsdAttribute GetFloat2Attr() const;

    /// See GetFloat2Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloat2Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOAT3 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Float3
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetFloat3Attr() const;

    /// See GetFloat3Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloat3Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOAT4 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec4f
    /// \n  Usd Type: SdfValueTypeNames->Float4
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetFloat4Attr() const;

    /// See GetFloat4Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloat4Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLE2 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec2d
    /// \n  Usd Type: SdfValueTypeNames->Double2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0)
    USDCONTRIVED_API
    UsdAttribute GetDouble2Attr() const;

    /// See GetDouble2Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDouble2Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLE3 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3d
    /// \n  Usd Type: SdfValueTypeNames->Double3
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetDouble3Attr() const;

    /// See GetDouble3Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDouble3Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLE4 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec4d
    /// \n  Usd Type: SdfValueTypeNames->Double4
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetDouble4Attr() const;

    /// See GetDouble4Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDouble4Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POINT3H 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3h
    /// \n  Usd Type: SdfValueTypeNames->Point3h
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetPoint3hAttr() const;

    /// See GetPoint3hAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreatePoint3hAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POINT3F 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Point3f
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetPoint3fAttr() const;

    /// See GetPoint3fAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreatePoint3fAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POINT3D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3d
    /// \n  Usd Type: SdfValueTypeNames->Point3d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetPoint3dAttr() const;

    /// See GetPoint3dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreatePoint3dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VECTOR3D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3d
    /// \n  Usd Type: SdfValueTypeNames->Vector3d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetVector3dAttr() const;

    /// See GetVector3dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateVector3dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VECTOR3F 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Vector3f
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetVector3fAttr() const;

    /// See GetVector3fAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateVector3fAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VECTOR3H 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3h
    /// \n  Usd Type: SdfValueTypeNames->Vector3h
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetVector3hAttr() const;

    /// See GetVector3hAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateVector3hAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMAL3D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3d
    /// \n  Usd Type: SdfValueTypeNames->Normal3d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetNormal3dAttr() const;

    /// See GetNormal3dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateNormal3dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMAL3F 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Normal3f
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetNormal3fAttr() const;

    /// See GetNormal3fAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateNormal3fAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMAL3H 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3h
    /// \n  Usd Type: SdfValueTypeNames->Normal3h
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetNormal3hAttr() const;

    /// See GetNormal3hAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateNormal3hAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR3D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3d
    /// \n  Usd Type: SdfValueTypeNames->Color3d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetColor3dAttr() const;

    /// See GetColor3dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor3dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR3F 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Color3f
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetColor3fAttr() const;

    /// See GetColor3fAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor3fAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR3H 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec3h
    /// \n  Usd Type: SdfValueTypeNames->Color3h
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetColor3hAttr() const;

    /// See GetColor3hAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor3hAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR4D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec4d
    /// \n  Usd Type: SdfValueTypeNames->Color4d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetColor4dAttr() const;

    /// See GetColor4dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor4dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR4F 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec4f
    /// \n  Usd Type: SdfValueTypeNames->Color4f
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetColor4fAttr() const;

    /// See GetColor4fAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor4fAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR4H 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfVec4h
    /// \n  Usd Type: SdfValueTypeNames->Color4h
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetColor4hAttr() const;

    /// See GetColor4hAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor4hAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // QUATD 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfQuatd
    /// \n  Usd Type: SdfValueTypeNames->Quatd
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (1, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetQuatdAttr() const;

    /// See GetQuatdAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateQuatdAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // QUATF 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfQuatf
    /// \n  Usd Type: SdfValueTypeNames->Quatf
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (1, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetQuatfAttr() const;

    /// See GetQuatfAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateQuatfAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // QUATH 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfQuath
    /// \n  Usd Type: SdfValueTypeNames->Quath
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (1, 0, 0, 0)
    USDCONTRIVED_API
    UsdAttribute GetQuathAttr() const;

    /// See GetQuathAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateQuathAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MATRIX2D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfMatrix2d
    /// \n  Usd Type: SdfValueTypeNames->Matrix2d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: ( (1, 0), (0, 1) )
    USDCONTRIVED_API
    UsdAttribute GetMatrix2dAttr() const;

    /// See GetMatrix2dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMatrix2dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MATRIX3D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfMatrix3d
    /// \n  Usd Type: SdfValueTypeNames->Matrix3d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: ( (1, 0, 0), (0, 1, 0), (0, 0, 1) )
    USDCONTRIVED_API
    UsdAttribute GetMatrix3dAttr() const;

    /// See GetMatrix3dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMatrix3dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MATRIX4D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfMatrix4d
    /// \n  Usd Type: SdfValueTypeNames->Matrix4d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )
    USDCONTRIVED_API
    UsdAttribute GetMatrix4dAttr() const;

    /// See GetMatrix4dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMatrix4dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FRAME4D 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: GfMatrix4d
    /// \n  Usd Type: SdfValueTypeNames->Frame4d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )
    USDCONTRIVED_API
    UsdAttribute GetFrame4dAttr() const;

    /// See GetFrame4dAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFrame4dAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BOOLARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<bool>
    /// \n  Usd Type: SdfValueTypeNames->BoolArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetBoolArrayAttr() const;

    /// See GetBoolArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateBoolArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UCHARARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<unsigned char>
    /// \n  Usd Type: SdfValueTypeNames->UCharArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetUcharArrayAttr() const;

    /// See GetUcharArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateUcharArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetIntArrayAttr() const;

    /// See GetIntArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateIntArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UINTARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<unsigned int>
    /// \n  Usd Type: SdfValueTypeNames->UIntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetUintArrayAttr() const;

    /// See GetUintArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateUintArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT64ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<int64_t>
    /// \n  Usd Type: SdfValueTypeNames->Int64Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetInt64ArrayAttr() const;

    /// See GetInt64ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt64ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UINT64ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<uint64_t>
    /// \n  Usd Type: SdfValueTypeNames->UInt64Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetUint64ArrayAttr() const;

    /// See GetUint64ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateUint64ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALFARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfHalf>
    /// \n  Usd Type: SdfValueTypeNames->HalfArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetHalfArrayAttr() const;

    /// See GetHalfArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalfArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOATARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetFloatArrayAttr() const;

    /// See GetFloatArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloatArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLEARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<double>
    /// \n  Usd Type: SdfValueTypeNames->DoubleArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetDoubleArrayAttr() const;

    /// See GetDoubleArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDoubleArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STRINGARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<std::string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetStringArrayAttr() const;

    /// See GetStringArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateStringArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TOKENARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<TfToken>
    /// \n  Usd Type: SdfValueTypeNames->TokenArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetTokenArrayAttr() const;

    /// See GetTokenArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateTokenArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ASSETARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<SdfAssetPath>
    /// \n  Usd Type: SdfValueTypeNames->AssetArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetAssetArrayAttr() const;

    /// See GetAssetArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateAssetArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT2ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec2i>
    /// \n  Usd Type: SdfValueTypeNames->Int2Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetInt2ArrayAttr() const;

    /// See GetInt2ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt2ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT3ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3i>
    /// \n  Usd Type: SdfValueTypeNames->Int3Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetInt3ArrayAttr() const;

    /// See GetInt3ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt3ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INT4ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec4i>
    /// \n  Usd Type: SdfValueTypeNames->Int4Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetInt4ArrayAttr() const;

    /// See GetInt4ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateInt4ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALF2ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec2h>
    /// \n  Usd Type: SdfValueTypeNames->Half2Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetHalf2ArrayAttr() const;

    /// See GetHalf2ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalf2ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALF3ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3h>
    /// \n  Usd Type: SdfValueTypeNames->Half3Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetHalf3ArrayAttr() const;

    /// See GetHalf3ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalf3ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HALF4ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec4h>
    /// \n  Usd Type: SdfValueTypeNames->Half4Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetHalf4ArrayAttr() const;

    /// See GetHalf4ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateHalf4ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOAT2ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec2f>
    /// \n  Usd Type: SdfValueTypeNames->Float2Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetFloat2ArrayAttr() const;

    /// See GetFloat2ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloat2ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOAT3ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Float3Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetFloat3ArrayAttr() const;

    /// See GetFloat3ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloat3ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FLOAT4ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec4f>
    /// \n  Usd Type: SdfValueTypeNames->Float4Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetFloat4ArrayAttr() const;

    /// See GetFloat4ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFloat4ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLE2ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec2d>
    /// \n  Usd Type: SdfValueTypeNames->Double2Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetDouble2ArrayAttr() const;

    /// See GetDouble2ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDouble2ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLE3ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3d>
    /// \n  Usd Type: SdfValueTypeNames->Double3Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetDouble3ArrayAttr() const;

    /// See GetDouble3ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDouble3ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLE4ARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec4d>
    /// \n  Usd Type: SdfValueTypeNames->Double4Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetDouble4ArrayAttr() const;

    /// See GetDouble4ArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateDouble4ArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POINT3HARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3h>
    /// \n  Usd Type: SdfValueTypeNames->Point3hArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetPoint3hArrayAttr() const;

    /// See GetPoint3hArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreatePoint3hArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POINT3FARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Point3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetPoint3fArrayAttr() const;

    /// See GetPoint3fArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreatePoint3fArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POINT3DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3d>
    /// \n  Usd Type: SdfValueTypeNames->Point3dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetPoint3dArrayAttr() const;

    /// See GetPoint3dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreatePoint3dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VECTOR3HARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3h>
    /// \n  Usd Type: SdfValueTypeNames->Vector3hArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetVector3hArrayAttr() const;

    /// See GetVector3hArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateVector3hArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VECTOR3FARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Vector3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetVector3fArrayAttr() const;

    /// See GetVector3fArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateVector3fArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VECTOR3DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3d>
    /// \n  Usd Type: SdfValueTypeNames->Vector3dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetVector3dArrayAttr() const;

    /// See GetVector3dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateVector3dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMAL3HARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3h>
    /// \n  Usd Type: SdfValueTypeNames->Normal3hArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetNormal3hArrayAttr() const;

    /// See GetNormal3hArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateNormal3hArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMAL3FARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Normal3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetNormal3fArrayAttr() const;

    /// See GetNormal3fArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateNormal3fArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMAL3DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3d>
    /// \n  Usd Type: SdfValueTypeNames->Normal3dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetNormal3dArrayAttr() const;

    /// See GetNormal3dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateNormal3dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR3HARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3h>
    /// \n  Usd Type: SdfValueTypeNames->Color3hArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetColor3hArrayAttr() const;

    /// See GetColor3hArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor3hArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR3FARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Color3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetColor3fArrayAttr() const;

    /// See GetColor3fArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor3fArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR3DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec3d>
    /// \n  Usd Type: SdfValueTypeNames->Color3dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetColor3dArrayAttr() const;

    /// See GetColor3dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor3dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR4HARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec4h>
    /// \n  Usd Type: SdfValueTypeNames->Color4hArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetColor4hArrayAttr() const;

    /// See GetColor4hArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor4hArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR4FARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec4f>
    /// \n  Usd Type: SdfValueTypeNames->Color4fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetColor4fArrayAttr() const;

    /// See GetColor4fArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor4fArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR4DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfVec4d>
    /// \n  Usd Type: SdfValueTypeNames->Color4dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetColor4dArrayAttr() const;

    /// See GetColor4dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateColor4dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // QUATHARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfQuath>
    /// \n  Usd Type: SdfValueTypeNames->QuathArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetQuathArrayAttr() const;

    /// See GetQuathArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateQuathArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // QUATFARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfQuatf>
    /// \n  Usd Type: SdfValueTypeNames->QuatfArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetQuatfArrayAttr() const;

    /// See GetQuatfArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateQuatfArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // QUATDARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfQuatd>
    /// \n  Usd Type: SdfValueTypeNames->QuatdArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetQuatdArrayAttr() const;

    /// See GetQuatdArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateQuatdArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MATRIX2DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfMatrix2d>
    /// \n  Usd Type: SdfValueTypeNames->Matrix2dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetMatrix2dArrayAttr() const;

    /// See GetMatrix2dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMatrix2dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MATRIX3DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfMatrix3d>
    /// \n  Usd Type: SdfValueTypeNames->Matrix3dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetMatrix3dArrayAttr() const;

    /// See GetMatrix3dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMatrix3dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MATRIX4DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfMatrix4d>
    /// \n  Usd Type: SdfValueTypeNames->Matrix4dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetMatrix4dArrayAttr() const;

    /// See GetMatrix4dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateMatrix4dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FRAME4DARRAY 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<GfMatrix4d>
    /// \n  Usd Type: SdfValueTypeNames->Frame4dArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetFrame4dArrayAttr() const;

    /// See GetFrame4dArrayAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateFrame4dArrayAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with }}}
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

}}}

#endif
