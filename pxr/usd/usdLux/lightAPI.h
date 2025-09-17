//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLUX_GENERATED_LIGHTAPI_H
#define USDLUX_GENERATED_LIGHTAPI_H

/// \file usdLux/lightAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

#include "pxr/usd/usd/collectionAPI.h"
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
// LIGHTAPI                                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdLuxLightAPI
///
/// API schema that imparts the quality of being a light onto a prim. 
/// 
/// A light is any prim that has this schema applied to it.  This is true 
/// regardless of whether LightAPI is included as a built-in API of the prim 
/// type (e.g. RectLight or DistantLight) or is applied directly to a Gprim 
/// that should be treated as a light.
/// 
/// <b>Quantities and Units</b>
/// 
/// Most renderers consuming OpenUSD today are RGB renderers, rather than
/// spectral. Units in RGB renderers are tricky to define as each of the red,
/// green and blue channels transported by the renderer represents the
/// convolution of a spectral exposure distribution, e.g. CIE Illuminant D65,
/// with a sensor response function, e.g. CIE 1931 𝓍̅. Thus the main quantity
/// in an RGB renderer is neither radiance nor luminance, but "integrated
/// radiance" or "tristimulus weight".
/// 
/// The emission of a default light with `intensity` 1 and `color` [1, 1, 1] is
/// an Illuminant D spectral distribution with chromaticity matching the
/// rendering color space white point, normalized such that a ray normally
/// incident upon the sensor with EV0 exposure settings will generate a pixel
/// value of [1, 1, 1] in the rendering color space.
/// 
/// Given the above definition, that means that the luminance of said default
/// light will be 1 *nit (cd∕m²)* and its emission spectral radiance
/// distribution is easily computed by appropriate normalization.
/// 
/// For brevity, the term *emission* will be used in the documentation to mean
/// "emitted spectral radiance" or "emitted integrated radiance/tristimulus
/// weight", as appropriate.
/// 
/// The method of "uplifting" an RGB color to a spectral distribution is
/// unspecified other than that it should round-trip under the rendering
/// illuminant to the limits of numerical accuracy.
/// 
/// Note that some color spaces, most notably ACES, define their white points
/// by chromaticity coordinates that do not exactly line up to any value of a
/// standard illuminant. Because we do not define the method of uplift beyond
/// the round-tripping requirement, we discourage the use of such color spaces
/// as the rendering color space, and instead encourage the use of color spaces
/// whose white point has a well-defined spectral representation, such as D65.
/// 
/// <b>Linking</b>
/// 
/// Lights can be linked to geometry.  Linking controls which geometry
/// a light illuminates, and which geometry casts shadows from the light.
/// 
/// Linking is specified as collections (UsdCollectionAPI) which can
/// be accessed via GetLightLinkCollection() and GetShadowLinkCollection().
/// Note that these collections have their includeRoot set to true,
/// so that lights will illuminate and cast shadows from all objects
/// by default.  To illuminate only a specific set of objects, there
/// are two options.  One option is to modify the collection paths
/// to explicitly exclude everything else, assuming it is known;
/// the other option is to set includeRoot to false and explicitly
/// include the desired objects.  These are complementary approaches
/// that may each be preferable depending on the scenario and how
/// to best express the intent of the light setup.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLuxTokens.
/// So to set an attribute to the value "rightHanded", use UsdLuxTokens->rightHanded
/// as the value.
///
class UsdLuxLightAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLuxLightAPI on UsdPrim \p prim .
    /// Equivalent to UsdLuxLightAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxLightAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLuxLightAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxLightAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxLightAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxLightAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxLightAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxLightAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxLightAPI
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
    /// This information is stored by adding "LightAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLuxLightAPI object is returned upon success. 
    /// An invalid (or empty) UsdLuxLightAPI object is returned upon 
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
    static UsdLuxLightAPI 
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
    // SHADERID 
    // --------------------------------------------------------------------- //
    /// Default ID for the light's shader. 
    /// This defines the shader ID for this light when a render context specific
    /// shader ID is not available. 
    /// 
    /// The default shaderId for the intrinsic UsdLux lights (RectLight, 
    /// DistantLight, etc.) are set to default to the light's type name. For 
    /// each intrinsic UsdLux light, we will always register an SdrShaderNode in
    /// the SdrRegistry, with the identifier matching the type name and the 
    /// source type "USD", that corresponds to the light's inputs.
    /// \see GetShaderId
    /// \see GetShaderIdAttrForRenderContext
    /// \see SdrRegistry::GetShaderNodeByIdentifier
    /// \see SdrRegistry::GetShaderNodeByIdentifierAndType
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token light:shaderId = ""` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDLUX_API
    UsdAttribute GetShaderIdAttr() const;

    /// See GetShaderIdAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShaderIdAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MATERIALSYNCMODE 
    // --------------------------------------------------------------------- //
    /// 
    /// For a LightAPI applied to geometry that has a bound Material, 
    /// which is entirely or partly emissive, this specifies the relationship 
    /// of the Material response to the lighting response.
    /// Valid values are:
    /// - materialGlowTintsLight: All primary and secondary rays see the 
    /// emissive/glow response as dictated by the bound Material while the 
    /// base color seen by light rays (which is then modulated by all of the 
    /// other LightAPI controls) is the multiplication of the color feeding 
    /// the emission/glow input of the Material (i.e. its surface or volume 
    /// shader) with the scalar or pattern input to *inputs:color*.
    /// This allows the light's color to tint the geometry's glow color while 
    /// preserving access to intensity and other light controls as ways to 
    /// further modulate the illumination.
    /// - independent: All primary and secondary rays see the emissive/glow 
    /// response as dictated by the bound Material, while the base color seen 
    /// by light rays is determined solely by *inputs:color*. Note that for 
    /// partially emissive geometry (in which some parts are reflective 
    /// rather than emissive), a suitable pattern must be connected to the 
    /// light's color input, or else the light will radiate uniformly from 
    /// the geometry.
    /// - noMaterialResponse: The geometry behaves as if there is no Material
    /// bound at all, i.e. there is no diffuse, specular, or transmissive 
    /// response. The base color of light rays is entirely controlled by the
    /// *inputs:color*. This is the standard mode for "canonical" lights in 
    /// UsdLux and indicates to renderers that a Material will either never 
    /// be bound or can always be ignored.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token light:materialSyncMode = "noMaterialResponse"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdLuxTokens "Allowed Values" | materialGlowTintsLight, independent, noMaterialResponse |
    USDLUX_API
    UsdAttribute GetMaterialSyncModeAttr() const;

    /// See GetMaterialSyncModeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateMaterialSyncModeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTENSITY 
    // --------------------------------------------------------------------- //
    /// Scales the brightness of the light linearly.
    /// 
    /// Expresses the "base", unmultiplied luminance emitted (L) of the light,
    /// in nits (cd∕m²):
    /// 
    /// <center><b>
    /// L<sub>Scalar</sub> = intensity
    /// </b></center>
    /// 
    /// Normatively, the lights' emission is in units of spectral radiance
    /// normalized such that a directly visible light with `intensity` 1 and
    /// `exposure` 0 normally incident upon the sensor plane will generate a
    /// pixel value of [1, 1, 1] in an RGB renderer, and thus have a luminance
    /// of 1 nit. A light with `intensity` 2 and `exposure` 0 would therefore
    /// have a luminance of 2 nits.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:intensity = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetIntensityAttr() const;

    /// See GetIntensityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateIntensityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPOSURE 
    // --------------------------------------------------------------------- //
    /// Scales the brightness of the light exponentially as a power
    /// of 2 (similar to an F-stop control over exposure).  The result
    /// is multiplied against the intensity:
    /// 
    /// <center><b>
    /// L<sub>Scalar</sub> = L<sub>Scalar</sub> ⋅ 2<sup>exposure</sup>
    /// </b></center>
    /// 
    /// Normatively, the lights' emission is in units of spectral radiance
    /// normalized such that a directly visible light with `intensity` 1 and
    /// `exposure` 0 normally incident upon the sensor plane will generate a
    /// pixel value of [1, 1, 1] in an RGB renderer, and thus have a luminance
    /// of 1 nit (cd∕m²). A light with `intensity` 1 and `exposure` 2 would
    /// therefore have a luminance of 4 nits.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:exposure = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetExposureAttr() const;

    /// See GetExposureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateExposureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DIFFUSE 
    // --------------------------------------------------------------------- //
    /// A multiplier for the effect of this light on the diffuse
    /// response of materials.  This is a non-physical control.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:diffuse = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetDiffuseAttr() const;

    /// See GetDiffuseAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateDiffuseAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SPECULAR 
    // --------------------------------------------------------------------- //
    /// A multiplier for the effect of this light on the specular
    /// response of materials.  This is a non-physical control.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:specular = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetSpecularAttr() const;

    /// See GetSpecularAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateSpecularAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMALIZE 
    // --------------------------------------------------------------------- //
    /// Normalizes the emission such that the power of the light
    /// remains constant while altering the size of the light, by dividing the
    /// luminance by the world-space surface area of the light.
    /// 
    /// This makes it easier to independently adjust the brightness and size
    /// of the light, by causing the total illumination provided by a light to
    /// not vary with the area or angular size of the light.
    /// 
    /// Mathematically, this means that the luminance of the light will be
    /// divided by a factor representing the "size" of the light:
    /// 
    /// <center><b>
    /// L<sub>Scalar</sub> = L<sub>Scalar</sub> / sizeFactor
    /// </b></center>
    /// 
    /// ...where `sizeFactor` = 1 if `normalize` is off, and is calculated
    /// depending on the family of the light as described below if `normalize`
    /// is on.
    /// 
    /// ### DomeLight / PortalLight:
    /// 
    /// For a dome light (and its associated PortalLight), this attribute is
    /// ignored:
    /// 
    /// <center><b>
    /// sizeFactor<sub>dome</sub> = 1
    /// </b></center>
    /// 
    /// ### Area Lights:
    /// 
    /// For an area light, the `sizeFactor` is the surface area (in world
    /// space) of the shape of the light, including any scaling applied to the
    /// light by its transform stack. This includes the boundable light types
    /// which have a calculable surface area:
    /// 
    /// - MeshLightAPI
    /// - DiskLight
    /// - RectLight
    /// - SphereLight
    /// - CylinderLight
    /// 
    /// <center><b>
    /// sizeFactor<sub>area</sub> = worldSpaceSurfaceArea(light)
    /// </b></center>
    /// 
    /// ### DistantLight:
    /// 
    /// For distant lights, we first define 𝛳<sub>max</sub> as:
    /// 
    /// <center><b>
    /// 𝛳<sub>max</sub> = clamp(toRadians(distantLightAngle) / 2, 0, 𝜋)
    /// </b></center>
    /// 
    /// Then we use the following formula:
    /// 
    /// * <i>if 𝛳<sub>max</sub> = 0:</i>
    /// <center><b>
    /// sizeFactor<sub>distant</sub> = 1
    /// </b></center>
    /// 
    /// * <i>if 0 < 𝛳<sub>max</sub> ≤ 𝜋 / 2:</i>
    /// <center><b>
    /// sizeFactor<sub>distant</sub> = sin²𝛳<sub>max</sub> ⋅ 𝜋
    /// </b></center>
    /// 
    /// * <i>if 𝜋 / 2 < 𝛳<sub>max</sub> ≤ 𝜋:</i>
    /// <center><b>
    /// sizeFactor<sub>distant</sub> =
    /// (2 - sin²𝛳<sub>max</sub>) ⋅ 𝜋
    /// </b></center>
    /// 
    /// This formula is used because it satisfies the following two properties:
    /// 
    /// 1. When normalize is enabled, the received illuminance from this light
    /// on a surface normal to the light's primary direction is held constant
    /// when angle changes, and the "intensity" property becomes a measure of
    /// the illuminance, expressed in lux, for a light with 0 exposure.
    /// 
    /// 2. If we assume that our distant light is an approximation for a "very
    /// far" sphere light (like the sun), then (for
    /// *0 < 𝛳<sub>max</sub> ≤ 𝜋/2*) this definition agrees with the
    /// definition used for area lights - i.e., the total power of this distant
    /// sphere light is constant when the "size" (i.e., angle) changes, and our
    /// sizeFactor is proportional to the total surface area of this sphere.
    /// 
    /// ### Other Lights
    /// 
    /// The above taxonomy describes behavior for all built-in light types.
    /// (Note that the above is based on schema *family* - i.e., `DomeLight_1`
    /// follows the rules for a `DomeLight`, and ignores `normalize`).
    /// 
    /// Lights from other third-party plugins / schemas must document their
    /// expected behavior with regards to normalize.  However, some general
    /// guidelines are:
    /// 
    /// - Lights that either inherit from or are strongly associated with one of
    /// the built-in types should follow the behavior of the built-in type
    /// they inherit/resemble; i.e., a renderer-specific "MyRendererRectLight"
    /// should have its size factor be its world-space surface area.
    /// - Lights that are boundable and have a calculable surface area should
    /// follow the rules for an Area Light, and have their sizeFactor be their
    /// world-space surface area.
    /// - Lights that are non-boundable and/or have no way to concretely or even
    /// "intuitively" associate them with a "size" will ignore this attribute
    /// (and always set sizeFactor = 1).
    /// 
    /// Lights that don't clearly meet any of the above criteria may either
    /// ignore the normalize attribute or try to implement support using
    /// whatever heuristic seems to make sense. For instance,
    /// MyMandelbulbLight might use a sizeFactor equal to the world-space
    /// surface area of a sphere which "roughly" bounds it.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool inputs:normalize = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDLUX_API
    UsdAttribute GetNormalizeAttr() const;

    /// See GetNormalizeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateNormalizeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR 
    // --------------------------------------------------------------------- //
    /// The color of emitted light, in the rendering color space.
    /// 
    /// This color is just multiplied with the emission:
    /// 
    /// <center><b>
    /// L<sub>Color</sub> = L<sub>Scalar</sub> ⋅ color
    /// </b></center>
    /// 
    /// In the case of a spectral renderer, this color should be uplifted such
    /// that it round-trips to within the limit of numerical accuracy under the
    /// rendering illuminant. We recommend the use of a rendering color space
    /// well defined in terms of a Illuminant D illuminant (ideally a D
    /// illuminant whose white point has a well-defined spectral representation,
    /// such as D65), to avoid unspecified uplift. See: \ref usdLux_quantities
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `color3f inputs:color = (1, 1, 1)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Color3f |
    USDLUX_API
    UsdAttribute GetColorAttr() const;

    /// See GetColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ENABLECOLORTEMPERATURE 
    // --------------------------------------------------------------------- //
    /// Enables using colorTemperature.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool inputs:enableColorTemperature = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDLUX_API
    UsdAttribute GetEnableColorTemperatureAttr() const;

    /// See GetEnableColorTemperatureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateEnableColorTemperatureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLORTEMPERATURE 
    // --------------------------------------------------------------------- //
    /// Color temperature, in degrees Kelvin, representing the
    /// white point.  The default is a common white point, D65.  Lower
    /// values are warmer and higher values are cooler.  The valid range
    /// is from 1000 to 10000. Only takes effect when
    /// enableColorTemperature is set to true.  When active, the
    /// computed result multiplies against the color attribute.
    /// See UsdLuxBlackbodyTemperatureAsRgb().
    /// 
    /// This is always calculated as an RGB color using a D65 white point,
    /// regardless of the rendering color space, normalized such that the
    /// default value of 6500 will always result in white, and then should be
    /// transformed to the rendering color space.
    /// 
    /// Spectral renderers should do the same and then uplift the resulting
    /// color after multiplying with the `color` attribute.  We recommend the
    /// use of a rendering color space well defined in terms of a Illuminant D
    /// illuminant, to avoid unspecified uplift.  See: \ref usdLux_quantities
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:colorTemperature = 6500` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetColorTemperatureAttr() const;

    /// See GetColorTemperatureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateColorTemperatureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FILTERS 
    // --------------------------------------------------------------------- //
    /// Relationship to the light filters that apply to this light.
    ///
    USDLUX_API
    UsdRelationship GetFiltersRel() const;

    /// See GetFiltersRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLUX_API
    UsdRelationship CreateFiltersRel() const;

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
    /// Allow implicit conversion of a UsdShadeConnectableAPI to UsdLuxLightAPI
    USDLUX_API
    UsdLuxLightAPI(const UsdShadeConnectableAPI &connectable);

    /// Contructs and returns a UsdShadeConnectableAPI object with this light.
    ///
    /// Note that most tasks can be accomplished without explicitly constructing 
    /// a UsdShadeConnectable API, since connection-related API such as
    /// UsdShadeConnectableAPI::ConnectToSource() are static methods, and 
    /// UsdLuxLightAPI will auto-convert to a UsdShadeConnectableAPI when 
    /// passed to functions that want to act generically on a connectable
    /// UsdShadeConnectableAPI object.
    USDLUX_API
    UsdShadeConnectableAPI ConnectableAPI() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Outputs API
    ///
    /// Outputs represent a typed attribute on a light whose value is computed 
    /// externally. 
    /// 
    /// @{
        
    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace. Outputs on a light cannot be connected, as their 
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
    /// Light parameters are encoded as inputs. 
    /// 
    /// @{
        
    /// Create an input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace. Inputs on lights are connectable.
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

    /// Return the UsdCollectionAPI interface used for examining and
    /// modifying the light-linking of this light.  Light-linking
    /// controls which geometry this light illuminates.
    USDLUX_API
    UsdCollectionAPI GetLightLinkCollectionAPI() const;

    /// Return the UsdCollectionAPI interface used for examining and
    /// modifying the shadow-linking of this light.  Shadow-linking
    /// controls which geometry casts shadows from this light.
    USDLUX_API
    UsdCollectionAPI GetShadowLinkCollectionAPI() const;

    /// Returns the shader ID attribute for the given \p renderContext.
    ///
    /// If \p renderContext is non-empty, this will try to return an attribute
    /// named _light:shaderId_ with the namespace prefix \p renderContext. For 
    /// example, if the passed in render context is "ri" then the attribute 
    /// returned by this function would have the following signature:
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token ri:light:shaderId` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// 
    /// If the render context is empty, this will return the default shader ID 
    /// attribute as returned by GetShaderIdAttr().
    USDLUX_API
    UsdAttribute GetShaderIdAttrForRenderContext(
        const TfToken &renderContext) const;

    /// Creates the shader ID attribute for the given \p renderContext.
    ///
    /// See GetShaderIdAttrForRenderContext(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShaderIdAttrForRenderContext(
        const TfToken &renderContext,
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// Return the light's shader ID for the given list of available 
    /// \p renderContexts.
    ///
    /// The shader ID returned by this function is the identifier to use when 
    /// looking up the shader definition for this light in the 
    /// \ref SdrRegistry "shader registry".
    /// 
    /// The render contexts are expected to be listed in priority order, so
    /// for each render context provided, this will try to find the shader ID 
    /// attribute specific to that render context (see 
    /// GetShaderIdAttrForRenderContext()) and will return the 
    /// value of the first one found that has a non-empty value. If no shader ID
    /// value can be found for any of the given render contexts or 
    /// \p renderContexts is empty, then this will return the value of the 
    /// default shader ID attribute (see GetShaderIdAttr()).
    USDLUX_API
    TfToken GetShaderId(const TfTokenVector &renderContexts) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
