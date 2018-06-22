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
#ifndef USDGEOM_GENERATED_GPRIM_H
#define USDGEOM_GENERATED_GPRIM_H

/// \file usdGeom/gprim.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/boundable.h"
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
// GPRIM                                                                      //
// -------------------------------------------------------------------------- //

/// \class UsdGeomGprim
///
/// Base class for all geometric primitives.  
/// 
/// Gprim encodes basic graphical properties such as \em doubleSided and
/// \em orientation, and provides primvars for "display color" and "display
/// opacity" that travel with geometry to be used as shader overrides.  
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomGprim : public UsdGeomBoundable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::AbstractTyped;

    /// Construct a UsdGeomGprim on UsdPrim \p prim .
    /// Equivalent to UsdGeomGprim::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomGprim(const UsdPrim& prim=UsdPrim())
        : UsdGeomBoundable(prim)
    {
    }

    /// Construct a UsdGeomGprim on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomGprim(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomGprim(const UsdSchemaBase& schemaObj)
        : UsdGeomBoundable(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomGprim();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomGprim holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomGprim(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomGprim
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDGEOM_API
    virtual UsdSchemaType _GetSchemaType() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // DISPLAYCOLOR 
    // --------------------------------------------------------------------- //
    /// It is useful to have an "official" colorSet that can be used
    /// as a display or modeling color, even in the absence of any specified
    /// shader for a gprim.  DisplayColor serves this role; because it is a
    /// UsdGeomPrimvar, it can also be used as a gprim override for any shader
    /// that consumes a \em displayColor parameter.
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Color3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetDisplayColorAttr() const;

    /// See GetDisplayColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateDisplayColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISPLAYOPACITY 
    // --------------------------------------------------------------------- //
    /// Companion to \em displayColor that specifies opacity, broken
    /// out as an independent attribute rather than an rgba color, both so that
    /// each can be independently overridden, and because shaders rarely consume
    /// rgba parameters.
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetDisplayOpacityAttr() const;

    /// See GetDisplayOpacityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateDisplayOpacityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DOUBLESIDED 
    // --------------------------------------------------------------------- //
    /// Although some renderers treat all parametric or polygonal
    /// surfaces as if they were effectively laminae with outward-facing
    /// normals on both sides, some renderers derive significant optimizations
    /// by considering these surfaces to have only a single outward side,
    /// typically determined by control-point winding order and/or 
    /// \em orientation.  By doing so they can perform "backface culling" to
    /// avoid drawing the many polygons of most closed surfaces that face away
    /// from the viewer.
    /// 
    /// However, it is often advantageous to model thin objects such as paper
    /// and cloth as single, open surfaces that must be viewable from both
    /// sides, always.  Setting a gprim's \em doubleSided attribute to 
    /// \c true instructs all renderers to disable optimizations such as
    /// backface culling for the gprim, and attempt (not all renderers are able
    /// to do so, but the USD reference GL renderer always will) to provide
    /// forward-facing normals on each side of the surface for lighting
    /// calculations.
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: False
    USDGEOM_API
    UsdAttribute GetDoubleSidedAttr() const;

    /// See GetDoubleSidedAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateDoubleSidedAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ORIENTATION 
    // --------------------------------------------------------------------- //
    /// Orientation specifies whether the gprim's surface normal 
    /// should be computed using the right hand rule, or the left hand rule.
    /// Please see \ref UsdGeom_WindingOrder for a deeper explanation and
    /// generalization of orientation to composed scenes with transformation
    /// hierarchies.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: rightHanded
    /// \n  \ref UsdGeomTokens "Allowed Values": [rightHanded, leftHanded]
    USDGEOM_API
    UsdAttribute GetOrientationAttr() const;

    /// See GetOrientationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateOrientationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Convenience function to get the displayColor Attribute as a Primvar.
    ///
    /// \sa GetDisplayColorAttr(), CreateDisplayColorPrimvar()
    USDGEOM_API
    UsdGeomPrimvar GetDisplayColorPrimvar() const;

    /// Convenience function to create the displayColor primvar, optionally
    /// specifying interpolation and elementSize
    ///
    /// \sa CreateDisplayColorAttr(), GetDisplayColorPrimvar()
    USDGEOM_API
    UsdGeomPrimvar CreateDisplayColorPrimvar(
                                const TfToken& interpolation = TfToken(),
                                int elementSize = -1) const;

    /// Convenience function to get the displayOpacity Attribute as a Primvar.
    ///
    /// \sa GetDisplayOpacityAttr(), CreateDisplayOpacityPrimvar()
    USDGEOM_API
    UsdGeomPrimvar GetDisplayOpacityPrimvar() const;

    /// Convenience function to create the displayOpacity primvar, optionally
    /// specifying interpolation and elementSize
    ///
    /// \sa CreateDisplayOpacityAttr(), GetDisplayOpacityPrimvar()
    USDGEOM_API
    UsdGeomPrimvar CreateDisplayOpacityPrimvar(
                                const TfToken& interpolation = TfToken(),
                                int elementSize = -1) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
