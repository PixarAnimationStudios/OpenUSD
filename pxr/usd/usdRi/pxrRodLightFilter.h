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
#ifndef USDRI_GENERATED_PXRRODLIGHTFILTER_H
#define USDRI_GENERATED_PXRRODLIGHTFILTER_H

/// \file usdRi/pxrRodLightFilter.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/usd/usdLux/lightFilter.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRi/tokens.h"

#include "pxr/usd/usdRi/splineAPI.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PXRRODLIGHTFILTER                                                          //
// -------------------------------------------------------------------------- //

/// \class UsdRiPxrRodLightFilter
///
/// Simulates a rod or capsule-shaped region to modulate light.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdRiTokens.
/// So to set an attribute to the value "rightHanded", use UsdRiTokens->rightHanded
/// as the value.
///
class UsdRiPxrRodLightFilter : public UsdLuxLightFilter
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::ConcreteTyped;

    /// Construct a UsdRiPxrRodLightFilter on UsdPrim \p prim .
    /// Equivalent to UsdRiPxrRodLightFilter::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRiPxrRodLightFilter(const UsdPrim& prim=UsdPrim())
        : UsdLuxLightFilter(prim)
    {
    }

    /// Construct a UsdRiPxrRodLightFilter on the prim held by \p schemaObj .
    /// Should be preferred over UsdRiPxrRodLightFilter(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRiPxrRodLightFilter(const UsdSchemaBase& schemaObj)
        : UsdLuxLightFilter(schemaObj)
    {
    }

    /// Destructor.
    USDRI_API
    virtual ~UsdRiPxrRodLightFilter();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRiPxrRodLightFilter holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRiPxrRodLightFilter(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRI_API
    static UsdRiPxrRodLightFilter
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
    USDRI_API
    static UsdRiPxrRodLightFilter
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDRI_API
    UsdSchemaType _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDRI_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDRI_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // WIDTH 
    // --------------------------------------------------------------------- //
    /// Width of the inner region of the rod (X axis).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float width = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetWidthAttr() const;

    /// See GetWidthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateWidthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HEIGHT 
    // --------------------------------------------------------------------- //
    /// Height of the inner region of the rod (Y axis).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float height = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetHeightAttr() const;

    /// See GetHeightAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateHeightAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DEPTH 
    // --------------------------------------------------------------------- //
    /// Depth of the inner region of the rod (Z axis).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float depth = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetDepthAttr() const;

    /// See GetDepthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateDepthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // RADIUS 
    // --------------------------------------------------------------------- //
    /// Radius of the corners of the inner rod box.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float radius = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetRadiusAttr() const;

    /// See GetRadiusAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateRadiusAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EDGETHICKNESS 
    // --------------------------------------------------------------------- //
    /// Thickness of the edge region.  Larger values will
    /// soften the edge shape.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float edgeThickness = 0.25` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetEdgeThicknessAttr() const;

    /// See GetEdgeThicknessAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateEdgeThicknessAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALEWIDTH 
    // --------------------------------------------------------------------- //
    /// Scale the width of the inner rod shape.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float scale:width = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetScaleWidthAttr() const;

    /// See GetScaleWidthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateScaleWidthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALEHEIGHT 
    // --------------------------------------------------------------------- //
    /// Scale the height of the inner rod shape.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float scale:height = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetScaleHeightAttr() const;

    /// See GetScaleHeightAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateScaleHeightAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALEDEPTH 
    // --------------------------------------------------------------------- //
    /// Scale the depth of the inner rod shape.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float scale:depth = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetScaleDepthAttr() const;

    /// See GetScaleDepthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateScaleDepthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // REFINETOP 
    // --------------------------------------------------------------------- //
    /// Additional offset adjustment to the top region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float refine:top = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetRefineTopAttr() const;

    /// See GetRefineTopAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateRefineTopAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // REFINEBOTTOM 
    // --------------------------------------------------------------------- //
    /// Additional offset adjustment to the top region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float refine:bottom = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetRefineBottomAttr() const;

    /// See GetRefineBottomAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateRefineBottomAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // REFINELEFT 
    // --------------------------------------------------------------------- //
    /// Additional offset adjustment to the left region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float refine:left = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetRefineLeftAttr() const;

    /// See GetRefineLeftAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateRefineLeftAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // REFINERIGHT 
    // --------------------------------------------------------------------- //
    /// Additional offset adjustment to the left region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float refine:right = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetRefineRightAttr() const;

    /// See GetRefineRightAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateRefineRightAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // REFINEFRONT 
    // --------------------------------------------------------------------- //
    /// Additional offset adjustment to the front region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float refine:front = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetRefineFrontAttr() const;

    /// See GetRefineFrontAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateRefineFrontAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // REFINEBACK 
    // --------------------------------------------------------------------- //
    /// Additional offset adjustment to the back region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float refine:back = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetRefineBackAttr() const;

    /// See GetRefineBackAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateRefineBackAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EDGESCALETOP 
    // --------------------------------------------------------------------- //
    /// Additional edge scale adjustment to the top region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float edgeScale:top = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetEdgeScaleTopAttr() const;

    /// See GetEdgeScaleTopAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateEdgeScaleTopAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EDGESCALEBOTTOM 
    // --------------------------------------------------------------------- //
    /// Additional edge scale adjustment to the top region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float edgeScale:bottom = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetEdgeScaleBottomAttr() const;

    /// See GetEdgeScaleBottomAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateEdgeScaleBottomAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EDGESCALELEFT 
    // --------------------------------------------------------------------- //
    /// Additional edge scale adjustment to the left region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float edgeScale:left = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetEdgeScaleLeftAttr() const;

    /// See GetEdgeScaleLeftAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateEdgeScaleLeftAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EDGESCALERIGHT 
    // --------------------------------------------------------------------- //
    /// Additional edge scale adjustment to the left region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float edgeScale:right = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetEdgeScaleRightAttr() const;

    /// See GetEdgeScaleRightAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateEdgeScaleRightAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EDGESCALEFRONT 
    // --------------------------------------------------------------------- //
    /// Additional edge scale adjustment to the front region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float edgeScale:front = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetEdgeScaleFrontAttr() const;

    /// See GetEdgeScaleFrontAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateEdgeScaleFrontAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EDGESCALEBACK 
    // --------------------------------------------------------------------- //
    /// Additional edge scale adjustment to the back region.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float edgeScale:back = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetEdgeScaleBackAttr() const;

    /// See GetEdgeScaleBackAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateEdgeScaleBackAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLORSATURATION 
    // --------------------------------------------------------------------- //
    /// Saturation of the result (0=greyscale, 1=normal,
    /// >1=boosted colors).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float color:saturation = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDRI_API
    UsdAttribute GetColorSaturationAttr() const;

    /// See GetColorSaturationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateColorSaturationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FALLOFF 
    // --------------------------------------------------------------------- //
    /// Controls the transition from the core to the edge.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int falloff = 6` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDRI_API
    UsdAttribute GetFalloffAttr() const;

    /// See GetFalloffAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateFalloffAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FALLOFFKNOTS 
    // --------------------------------------------------------------------- //
    /// Knots of the falloff spline.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float[] falloff:knots = [0, 0, 0.3, 0.7, 1, 1]` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    USDRI_API
    UsdAttribute GetFalloffKnotsAttr() const;

    /// See GetFalloffKnotsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateFalloffKnotsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FALLOFFFLOATS 
    // --------------------------------------------------------------------- //
    /// Float values of the falloff spline.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float[] falloff:floats = [0, 0, 0.2, 0.8, 1, 1]` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    USDRI_API
    UsdAttribute GetFalloffFloatsAttr() const;

    /// See GetFalloffFloatsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateFalloffFloatsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FALLOFFINTERPOLATION 
    // --------------------------------------------------------------------- //
    /// Falloff spline type. 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token falloff:interpolation = "bspline"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdRiTokens "Allowed Values" | linear, catmull-rom, bspline, constant |
    USDRI_API
    UsdAttribute GetFalloffInterpolationAttr() const;

    /// See GetFalloffInterpolationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateFalloffInterpolationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLORRAMP 
    // --------------------------------------------------------------------- //
    /// Controls the color gradient for the transition.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int colorRamp = 4` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDRI_API
    UsdAttribute GetColorRampAttr() const;

    /// See GetColorRampAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateColorRampAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLORRAMPKNOTS 
    // --------------------------------------------------------------------- //
    /// Knots of the colorRamp spline.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float[] colorRamp:knots = [0, 0, 1, 1]` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    USDRI_API
    UsdAttribute GetColorRampKnotsAttr() const;

    /// See GetColorRampKnotsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateColorRampKnotsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLORRAMPCOLORS 
    // --------------------------------------------------------------------- //
    /// Color values of the colorRamp spline.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `color3f[] colorRamp:colors = [(1, 1, 1), (1, 1, 1), (1, 1, 1), (1, 1, 1)]` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Color3fArray |
    USDRI_API
    UsdAttribute GetColorRampColorsAttr() const;

    /// See GetColorRampColorsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateColorRampColorsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLORRAMPINTERPOLATION 
    // --------------------------------------------------------------------- //
    /// ColorRamp spline type. 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token colorRamp:interpolation = "linear"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdRiTokens "Allowed Values" | linear, catmull-rom, bspline, constant |
    USDRI_API
    UsdAttribute GetColorRampInterpolationAttr() const;

    /// See GetColorRampInterpolationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateColorRampInterpolationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Return the UsdRiSplineAPI interface used for examining and modifying
    /// the falloff ramp.  The values of the spline knots are of type float.
    USDRI_API
    UsdRiSplineAPI GetFalloffRampAPI() const;

    /// Return the UsdRiSplineAPI interface used for examining and modifying
    /// the color ramp.  The values of the spline knots are of type GfVec3f,
    /// representing RGB colors.
    USDRI_API
    UsdRiSplineAPI GetColorRampAPI() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
