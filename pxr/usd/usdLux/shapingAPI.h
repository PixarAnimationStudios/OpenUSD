//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLUX_GENERATED_SHAPINGAPI_H
#define USDLUX_GENERATED_SHAPINGAPI_H

/// \file usdLux/shapingAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// SHAPINGAPI                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdLuxShapingAPI
///
/// Controls for shaping a light's emission.
///
class UsdLuxShapingAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLuxShapingAPI on UsdPrim \p prim .
    /// Equivalent to UsdLuxShapingAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxShapingAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLuxShapingAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxShapingAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxShapingAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxShapingAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxShapingAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxShapingAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxShapingAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLUX_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ShapingAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLuxShapingAPI object is returned upon success. 
    /// An invalid (or empty) UsdLuxShapingAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLUX_API
    static UsdLuxShapingAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLUX_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLUX_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLUX_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // SHAPING:FOCUS 
    // --------------------------------------------------------------------- //
    /// A control to shape the spread of light.  Higher focus
    /// values pull light towards the center and narrow the spread.
    /// 
    /// This is implemented as a multiplication with the absolute value of the
    /// dot product between the light's surface normal and the emission
    /// direction, raised to the power `focus`.  See `inputs:shaping:focusTint`
    /// for the complete formula, but if we assume a default `focusTint` of
    /// pure black, then that formula simplifies to:
    /// 
    /// <center><b>
    /// focusFactor = ｜emissionDirection • lightNormal｜<sup>focus</sup>
    /// 
    /// L<sub>Color</sub> = focusFactor ⋅ L<sub>Color</sub>
    /// </b></center>
    /// 
    /// Values < 0 are ignored.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:shaping:focus = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetShapingFocusAttr() const;

    /// See GetShapingFocusAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShapingFocusAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHAPING:FOCUSTINT 
    // --------------------------------------------------------------------- //
    /// Off-axis color tint.  This tints the emission in the
    /// falloff region.  The default tint is black.
    /// 
    /// This is implemented as a linear interpolation between `focusTint` and
    /// white, by the factor computed from the focus attribute, in other words:
    /// 
    /// <center><b>
    /// focusFactor = ｜emissionDirection • lightNormal｜<sup>focus</sup>
    /// 
    /// focusColor = lerp(focusFactor, focusTint, [1, 1, 1])
    /// 
    /// L<sub>Color</sub> =
    /// componentwiseMultiply(focusColor, L<sub>Color</sub>)
    /// </b></center>
    /// 
    /// Note that this implies that a focusTint of pure white will disable
    /// focus.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `color3f inputs:shaping:focusTint = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Color3f |
    USDLUX_API
    UsdAttribute GetShapingFocusTintAttr() const;

    /// See GetShapingFocusTintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShapingFocusTintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHAPING:CONE:ANGLE 
    // --------------------------------------------------------------------- //
    /// Angular limit off the primary axis to restrict the light
    /// spread, in degrees.
    /// 
    /// Light emissions at angles off the primary axis greater than this are
    /// guaranteed to be zero, i.e.:
    /// 
    /// 
    /// <center><b>
    /// 𝛳<sub>offAxis</sub> = acos(lightAxis • emissionDir)
    /// 
    /// 𝛳<sub>cutoff</sub> = toRadians(coneAngle)
    /// 
    /// 
    /// 𝛳<sub>offAxis</sub> > 𝛳<sub>cutoff</sub>
    /// ⟹ L<sub>Scalar</sub> = 0
    /// 
    /// </b></center>
    /// 
    /// For angles < coneAngle, see the documentation for `shaping:cone:softness`.
    /// However, at the default of coneSoftness = 0, the luminance is
    /// unaltered if emissionOffAxisAngle <= coneAngle, so the coneAngle
    /// functions as a hard binary "off" toggle for all angles > coneAngle.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:shaping:cone:angle = 90` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetShapingConeAngleAttr() const;

    /// See GetShapingConeAngleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShapingConeAngleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHAPING:CONE:SOFTNESS 
    // --------------------------------------------------------------------- //
    /// Controls the cutoff softness for cone angle.
    /// 
    /// At the default of coneSoftness = 0, the luminance is unaltered if 
    /// emissionOffAxisAngle <= coneAngle, and 0 if
    /// emissionOffAxisAngle > coneAngle, so in this situation the coneAngle
    /// functions as a hard binary "off" toggle for all angles > coneAngle.
    /// 
    /// For coneSoftness in the range (0, 1], it defines the proportion of the
    /// non-cutoff angles over which the luminance is smoothly interpolated from
    /// 0 to 1. Mathematically:
    /// 
    /// <center><b>
    /// 𝛳<sub>offAxis</sub> = acos(lightAxis • emissionDir)
    /// 
    /// 𝛳<sub>cutoff</sub> = toRadians(coneAngle)
    /// 
    /// 𝛳<sub>smoothStart</sub> = lerp(coneSoftness, 𝛳<sub>cutoff</sub>, 0)
    /// 
    /// L<sub>Scalar</sub> = L<sub>Scalar</sub> ⋅
    /// (1 - smoothStep(𝛳<sub>offAxis</sub>,
    /// 𝛳<sub>smoothStart</sub>,
    /// 𝛳<sub>cutoff</sub>)
    /// </b></center>
    /// 
    /// Values outside of the [0, 1] range are clamped to the range.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:shaping:cone:softness = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetShapingConeSoftnessAttr() const;

    /// See GetShapingConeSoftnessAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShapingConeSoftnessAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHAPING:IES:FILE 
    // --------------------------------------------------------------------- //
    /// An IES (Illumination Engineering Society) light
    /// profile describing the angular distribution of light.
    /// 
    /// For full details on the .ies file format, see the full specification,
    /// ANSI/IES LM-63-19:
    /// 
    /// https://store.ies.org/product/lm-63-19-approved-method-ies-standard-file-format-for-the-electronic-transfer-of-photometric-data-and-related-information/
    /// 
    /// The luminous intensity values in the IES profile are sampled using
    /// the emission direction in the light's local space (after a possible
    /// transformation by a non-zero shaping:ies:angleScale, see below). The
    /// sampled value is then potentially normalized by the overall power of the
    /// profile if shaping:ies:normalize is enabled, and then used as a scaling
    /// factor on the returned luminance:
    /// 
    /// 
    /// <center><b>
    /// 𝛳<sub>light</sub>, 𝜙 =
    /// toPolarCoordinates(emissionDirectionInLightSpace)
    /// 
    /// 𝛳<sub>ies</sub> = applyAngleScale(𝛳<sub>light</sub>, angleScale)
    /// 
    /// iesSample = sampleIES(iesFile, 𝛳<sub>ies</sub>, 𝜙)
    /// 
    /// iesNormalize ⟹ iesSample = iesSample ⋅ iesProfilePower(iesFile)
    /// 
    /// L<sub>Color</sub> = iesSample ⋅ L<sub>Color</sub>
    /// </b></center>
    /// 
    /// See `inputs:shaping:ies:angleScale` for a description of
    /// `applyAngleScale`, and `inputs:shaping:ies:normalize` for how
    /// `iesProfilePower` is calculated.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `asset inputs:shaping:ies:file` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    USDLUX_API
    UsdAttribute GetShapingIesFileAttr() const;

    /// See GetShapingIesFileAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShapingIesFileAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHAPING:IES:ANGLESCALE 
    // --------------------------------------------------------------------- //
    /// Rescales the angular distribution of the IES profile.
    /// 
    /// Applies a scaling factor to the latitudinal theta/vertical polar
    /// coordinate before sampling the IES profile, to shift the samples more
    /// toward the "top" or "bottom" of the profile. The scaling origin varies
    /// depending on whether `angleScale` is positive or negative. If it is
    /// positive, the scaling origin is theta = 0. If it is negative, the
    /// scaling origin is theta = pi (180 degrees).  Values where
    /// |angleScale| < 1 will "shrink" the angular range in which the
    /// iesProfile is applied, while values where |angleScale| > 1 will
    /// "grow" the angular range to which the iesProfile is mapped.
    /// 
    /// If <i>𝛳<sub>light</sub></i> is the latitudinal theta polar
    /// coordinate of the emission direction in the light's local space, and
    /// <em>𝛳<sub>ies</sub></em> is the value that will be used when
    /// actually sampling the profile, then the exact formula is:
    /// 
    /// * <i>if angleScale > 0:</i>
    /// <center><b>
    /// 𝛳<sub>ies</sub> = 𝛳<sub>light</sub> / angleScale
    /// </b></center>
    /// 
    /// * <i>if angleScale = 0:</i>
    /// <center><b>
    /// 𝛳<sub>ies</sub> = 𝛳<sub>light</sub>
    /// </b></center>
    /// 
    /// * <i>if angleScale < 0:</i>
    /// <center><b>
    /// 𝛳<sub>ies</sub> = (𝛳<sub>light</sub> - π) / -angleScale
    /// </b></center>
    /// 
    /// Usage guidelines for artists / lighting TDs:
    /// 
    /// **If you have an IES profile for a spotlight aimed "down":**
    /// 
    /// - You should use a positive angleScale (> 0).
    /// - Values where 0 < angleScale < 1 will narrow the spotlight beam.
    /// - Values where angleScale > 1 will broaden the spotlight beam.
    /// 
    /// For example, if the original IES profile is a downward spotlight with
    /// a total cone angle of 60°, then angleScale = .5 will narrow it to
    /// have a cone angle of 30°, and an angleScale of 1.5 will broaden it
    /// to have a cone angle of 90°.
    /// 
    /// **If you have an IES profile for a spotlight aimed "up":**
    /// 
    /// - You should use a negative angleScale (< 0).
    /// - Values where -1 < angleScale < 0 will narrow the spotlight beam.
    /// - Values where angleScale < -1 will broaden the spotlight beam.
    /// 
    /// For example, if the original IES profile is an upward spotlight with
    /// a total cone angle of 60°, then angleScale = -.5 will narrow it to
    /// have a cone angle of 30°, and an angleScale of -1.5 will broaden
    /// it to have a cone angle of 90°.
    /// 
    /// **If you have an IES profile that's isn't clearly "aimed" in a single
    /// direction, OR it's aimed in a direction other than straight up or
    /// down:**
    /// 
    /// - Applying angleScale will alter the vertical angle mapping for your
    /// IES light, but it may be difficult to have a clear intuitive sense
    /// of how varying the angleScale will affect the shape of your light
    /// 
    /// If you violate the above rules (i.e., use a negative angleScale for a
    /// spotlight aimed down), then angleScale will still alter the vertical-
    /// angle mapping, but in more non-intuitive ways (i.e., broadening /
    /// narrowing may seem inverted, and the IES profile may seem to "translate"
    /// through the vertical angles, rather than uniformly scale).
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:shaping:ies:angleScale = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetShapingIesAngleScaleAttr() const;

    /// See GetShapingIesAngleScaleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShapingIesAngleScaleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHAPING:IES:NORMALIZE 
    // --------------------------------------------------------------------- //
    /// Normalizes the IES profile so that it affects the shaping
    /// of the light while preserving the overall energy output.
    /// 
    /// The sampled luminous intensity is scaled by the overall power of the
    /// IES profile if this is on, where the total power is calculated by
    /// integrating the luminous intensity over all solid angle patches
    /// defined in the profile.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool inputs:shaping:ies:normalize = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDLUX_API
    UsdAttribute GetShapingIesNormalizeAttr() const;

    /// See GetShapingIesNormalizeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShapingIesNormalizeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    // -------------------------------------------------------------------------
    /// \name Conversion to and from UsdShadeConnectableAPI
    /// 
    /// @{

    /// Constructor that takes a ConnectableAPI object.
    /// Allow implicit conversion of UsdShadeConnectableAPI to
    /// UsdLuxShapingAPI.
    USDLUX_API
    UsdLuxShapingAPI(const UsdShadeConnectableAPI &connectable);

    /// Contructs and returns a UsdShadeConnectableAPI object with this shaping
    /// API prim. Note that a valid UsdLuxShapingAPI will only return a valid
    /// UsdShadeConnectableAPI if the its prim's Typed schema type is actually
    /// connectable.
    USDLUX_API
    UsdShadeConnectableAPI ConnectableAPI() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Outputs API
    ///
    /// Outputs represent a typed attribute on a shaping API whose value is 
    /// computed externally. 
    /// 
    /// @{

    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace. Outputs on a shaping API cannot be connected, as their 
    /// value is assumed to be computed externally.
    /// 
    USDLUX_API
    UsdShadeOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName);

    /// Return the requested output if it exists.
    /// 
    USDLUX_API
    UsdShadeOutput GetOutput(const TfToken &name) const;

    /// Outputs are represented by attributes in the "outputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDLUX_API
    std::vector<UsdShadeOutput> GetOutputs(bool onlyAuthored=true) const;

    /// @}

    // ------------------------------------------------------------------------- 

    /// \name Inputs API
    ///
    /// Inputs are connectable attribute with a typed value. 
    /// 
    /// Shaping API parameters are encoded as inputs. 
    /// 
    /// @{

    /// Create an input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace. Inputs on shaping API are connectable.
    /// 
    USDLUX_API
    UsdShadeInput CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName);

    /// Return the requested input if it exists.
    /// 
    USDLUX_API
    UsdShadeInput GetInput(const TfToken &name) const;

    /// Inputs are represented by attributes in the "inputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDLUX_API
    std::vector<UsdShadeInput> GetInputs(bool onlyAuthored=true) const;

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
