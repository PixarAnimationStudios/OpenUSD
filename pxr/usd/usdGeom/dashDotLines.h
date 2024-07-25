//
// Copyright 2024 Pixar
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
#ifndef USDGEOM_GENERATED_DASHDOTLINES_H
#define USDGEOM_GENERATED_DASHDOTLINES_H

/// \file usdGeom/dashDotLines.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// DASHDOTLINES                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdGeomDashDotLines
///
/// This schema is for a line primitive whose width in screen space will not change. And the primitive
/// can have dash-dot patterns. This type of curve is usually used in a sketch file, or nondiegetic visualizations.
/// 
/// The basic shape for the primitive is a set of lines or polylines. A general type curve is not supported in this
/// schema.
/// 
/// If the lines have dash-dot patterns, it must inherit from a "pattern" who applies with DashDotPatternAPI. The
/// length of the pattern can be in screen space or world space.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomDashDotLines : public UsdGeomCurves
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdGeomDashDotLines on UsdPrim \p prim .
    /// Equivalent to UsdGeomDashDotLines::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomDashDotLines(const UsdPrim& prim=UsdPrim())
        : UsdGeomCurves(prim)
    {
    }

    /// Construct a UsdGeomDashDotLines on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomDashDotLines(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomDashDotLines(const UsdSchemaBase& schemaObj)
        : UsdGeomCurves(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomDashDotLines();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomDashDotLines holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomDashDotLines(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomDashDotLines
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
    USDGEOM_API
    static UsdGeomDashDotLines
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDGEOM_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // SCREENSPACEPATTERN 
    // --------------------------------------------------------------------- //
    /// Whether the dash-dot pattern length can be varied. It is only valid when the DashDotLines primitive
    /// inherits from a "pattern" who applies with DashDotPatternAPI. If it is true, the length of the pattern is in
    /// screen space, and it will not change. If you zoom in and the line is longer on the screen, you will see the
    /// patterns will move on the line, and there will be more patterns on the line. If it is false, the length of
    /// the pattern is in world space. If you zoom in, you will see the pattern will be larger, and it will not move
    /// on the line.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool screenSpacePattern = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDGEOM_API
    UsdAttribute GetScreenSpacePatternAttr() const;

    /// See GetScreenSpacePatternAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateScreenSpacePatternAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PATTERNSCALE 
    // --------------------------------------------------------------------- //
    /// This property is a scale value to lengthen or shorten a dash-dot pattern. It is only valid when the
    /// DashDotLines primitive inherits from a "pattern" who applies with DashDotPatternAPI.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float patternScale = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDGEOM_API
    UsdAttribute GetPatternScaleAttr() const;

    /// See GetPatternScaleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreatePatternScaleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STARTCAPTYPE 
    // --------------------------------------------------------------------- //
    /// The shape of the line cap at the start of the line. It is also applied to the start cap of each dash
    /// when the line has pattern.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token startCapType = "round"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | round, square, triangle |
    USDGEOM_API
    UsdAttribute GetStartCapTypeAttr() const;

    /// See GetStartCapTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateStartCapTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ENDCAPTYPE 
    // --------------------------------------------------------------------- //
    /// The shape of the line cap at the end of the line. It is also applied to the end cap of each dash
    /// when the line has pattern.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token endCapType = "round"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | round, square, triangle |
    USDGEOM_API
    UsdAttribute GetEndCapTypeAttr() const;

    /// See GetEndCapTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateEndCapTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Convert the attribute to token.
    USDGEOM_API
    TfToken GetTokenAttr(UsdAttribute attr, UsdTimeCode timeCode) const;

    /// Convert the attribute to int.
    USDGEOM_API
    int GetIntAttr(UsdAttribute attr, UsdTimeCode timeCode) const;
    
    /// Convert the attribute to float.
    USDGEOM_API
    float GetFloatAttr(UsdAttribute attr, UsdTimeCode timeCode) const;

    /// Convert the attribute to bool.
    USDGEOM_API
    bool GetBoolAttr(UsdAttribute attr, UsdTimeCode timeCode) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
