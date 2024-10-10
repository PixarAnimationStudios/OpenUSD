//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDRENDER_GENERATED_SETTINGSBASE_H
#define USDRENDER_GENERATED_SETTINGSBASE_H

/// \file usdRender/settingsBase.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRender/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRender/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// RENDERSETTINGSBASE                                                         //
// -------------------------------------------------------------------------- //

/// \class UsdRenderSettingsBase
///
/// Abstract base class that defines render settings that
/// can be specified on either a RenderSettings prim or a RenderProduct 
/// prim.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdRenderTokens.
/// So to set an attribute to the value "rightHanded", use UsdRenderTokens->rightHanded
/// as the value.
///
class UsdRenderSettingsBase : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractTyped;

    /// Construct a UsdRenderSettingsBase on UsdPrim \p prim .
    /// Equivalent to UsdRenderSettingsBase::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRenderSettingsBase(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdRenderSettingsBase on the prim held by \p schemaObj .
    /// Should be preferred over UsdRenderSettingsBase(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRenderSettingsBase(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDRENDER_API
    virtual ~UsdRenderSettingsBase();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRENDER_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRenderSettingsBase holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRenderSettingsBase(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRENDER_API
    static UsdRenderSettingsBase
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDRENDER_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDRENDER_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDRENDER_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // RESOLUTION 
    // --------------------------------------------------------------------- //
    /// The image pixel resolution, corresponding to the
    /// camera's screen window.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform int2 resolution = (2048, 1080)` |
    /// | C++ Type | GfVec2i |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int2 |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetResolutionAttr() const;

    /// See GetResolutionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateResolutionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PIXELASPECTRATIO 
    // --------------------------------------------------------------------- //
    /// The aspect ratio (width/height) of image pixels..
    /// The default ratio 1.0 indicates square pixels.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float pixelAspectRatio = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetPixelAspectRatioAttr() const;

    /// See GetPixelAspectRatioAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreatePixelAspectRatioAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ASPECTRATIOCONFORMPOLICY 
    // --------------------------------------------------------------------- //
    /// 
    /// Indicates the policy to use to resolve an aspect
    /// ratio mismatch between the camera aperture and image settings.
    /// 
    /// This policy allows a standard render setting to do something
    /// reasonable given varying camera inputs.
    /// 
    /// The camera aperture aspect ratio is determined by the
    /// aperture atributes on the UsdGeomCamera.
    /// 
    /// The image aspect ratio is determined by the resolution and
    /// pixelAspectRatio attributes in the render settings.
    /// 
    /// - "expandAperture": if necessary, expand the aperture to
    /// fit the image, exposing additional scene content
    /// - "cropAperture": if necessary, crop the aperture to fit
    /// the image, cropping scene content
    /// - "adjustApertureWidth": if necessary, adjust aperture width
    /// to make its aspect ratio match the image
    /// - "adjustApertureHeight": if necessary, adjust aperture height
    /// to make its aspect ratio match the image
    /// - "adjustPixelAspectRatio": compute pixelAspectRatio to
    /// make the image exactly cover the aperture; disregards
    /// existing attribute value of pixelAspectRatio
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token aspectRatioConformPolicy = "expandAperture"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdRenderTokens "Allowed Values" | expandAperture, cropAperture, adjustApertureWidth, adjustApertureHeight, adjustPixelAspectRatio |
    USDRENDER_API
    UsdAttribute GetAspectRatioConformPolicyAttr() const;

    /// See GetAspectRatioConformPolicyAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateAspectRatioConformPolicyAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DATAWINDOWNDC 
    // --------------------------------------------------------------------- //
    /// dataWindowNDC specifies the axis-aligned rectangular
    /// region in the adjusted aperture window within which the renderer
    /// should produce data.
    /// 
    /// It is specified as (xmin, ymin, xmax, ymax) in normalized
    /// device coordinates, where the range 0 to 1 corresponds to the
    /// aperture.  (0,0) corresponds to the bottom-left
    /// corner and (1,1) corresponds to the upper-right corner.
    /// 
    /// Specifying a window outside the unit square will produce
    /// overscan data. Specifying a window that does not cover the unit
    /// square will produce a cropped render.
    /// 
    /// A pixel is included in the rendered result if the pixel
    /// center is contained by the data window.  This is consistent
    /// with standard rules used by polygon rasterization engines.
    /// \ref UsdRenderRasterization
    /// 
    /// The data window is expressed in NDC so that cropping and
    /// overscan may be resolution independent.  In interactive
    /// workflows, incremental cropping and resolution adjustment
    /// may be intermixed to isolate and examine parts of the scene.
    /// In compositing workflows, overscan may be used to support
    /// image post-processing kernels, and reduced-resolution proxy
    /// renders may be used for faster iteration.
    /// 
    /// The dataWindow:ndc coordinate system references the
    /// aperture after any adjustments required by
    /// aspectRatioConformPolicy.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float4 dataWindowNDC = (0, 0, 1, 1)` |
    /// | C++ Type | GfVec4f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float4 |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetDataWindowNDCAttr() const;

    /// See GetDataWindowNDCAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateDataWindowNDCAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INSTANTANEOUSSHUTTER 
    // --------------------------------------------------------------------- //
    /// Deprecated - use disableMotionBlur instead. Override
    /// the targeted _camera_'s _shutterClose_ to be equal to the
    /// value of its _shutterOpen_, to produce a zero-width shutter
    /// interval.  This gives us a convenient way to disable motion
    /// blur.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool instantaneousShutter = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetInstantaneousShutterAttr() const;

    /// See GetInstantaneousShutterAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateInstantaneousShutterAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISABLEMOTIONBLUR 
    // --------------------------------------------------------------------- //
    /// Disable all motion blur by setting the shutter interval
    /// of the targeted camera to [0,0] - that is, take only one sample,
    /// namely at the current time code.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool disableMotionBlur = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetDisableMotionBlurAttr() const;

    /// See GetDisableMotionBlurAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateDisableMotionBlurAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISABLEDEPTHOFFIELD 
    // --------------------------------------------------------------------- //
    /// Disable all depth of field by setting F-stop of the targeted
    /// camera to infinity.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool disableDepthOfField = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetDisableDepthOfFieldAttr() const;

    /// See GetDisableDepthOfFieldAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateDisableDepthOfFieldAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CAMERA 
    // --------------------------------------------------------------------- //
    /// The _camera_ relationship specifies the primary
    /// camera to use in a render.  It must target a UsdGeomCamera.
    ///
    USDRENDER_API
    UsdRelationship GetCameraRel() const;

    /// See GetCameraRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRENDER_API
    UsdRelationship CreateCameraRel() const;

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
