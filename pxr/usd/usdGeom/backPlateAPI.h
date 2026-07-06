//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_BACKPLATEAPI_H
#define USDGEOM_GENERATED_BACKPLATEAPI_H

/// \file usdGeom/backPlateAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/usdGeom/camera.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// BACKPLATEAPI                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdGeomBackPlateAPI
///
/// This schema specifies a back plate i.e. a camera-constrained but 
/// not always camera-facing texture card, that exists in scene space. Multiple
/// back plates can be applied to a single camera and each plate can 
/// independently specify its own overall visibility, as well as its own 
/// opacity and depth varying over the plate.
/// 
/// UsdGeomBackplateAPI is designed to work with UsdGeomCamera, and thus 
/// specifically builds off of the camera model that the Camera schema defines,
/// adding the ability to incorporate separately imaged or captured images
/// into a synthetic 3D scene where the UsdGeomCamera can correspond to the
/// camera from which the images were taken.
/// 
/// \section UsdGeomBackPlatesAPI_AutoFocus Auto-Focus
/// By default, back plates are placed at the \ref UsdGeomCamera::GetFocusDistanceAttr "focus distance"
/// and scaled to fit the camera frustum such that the plate is always in
/// focus. If the focus distance changes, then the back plate will also shift 
/// along the optical axis to remain at focus distance and scale accordingly. 
/// This auto-focus setup is tailored for match moving and assumes that 
/// the captured photos are intended to be shown as is without any additional 
/// camera-based-depth blurring.
/// 
/// This schema provides additional rotation, translation, and scale controls,
/// suffixed with 'tweak', that are intended for local adjustments to the 
/// plate. These are applied to the center of the plate after it has been 
/// autofocused i.e. moved by the focus distance and scaled accordingly. It 
/// also has a subset of color grading controls for modifying gain, gamma, and 
/// lift values, as well as offset and normalization factors for modifying 
/// texture-refered depth information.
/// 
/// \section UsdGeomBackPlatesAPI_CameraModel Cameral Model
/// Back plates use a Gaussian thin lens model, whose origin is defined at the 
/// point where the optical axis intersects the focal plane. Note that the focus
/// distance used for back plate computations is assumed to be 
/// \ref UsdGeomCamera::GetFocusDistanceAttr "UsdGeomCamera::focusDistance" + 
/// \ref UsdGeomCamera::GetFocalLengthAttr "UsdGeomCamera::focalLength" 
/// such that our model aligns with film-production on-set practices. 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomBackPlateAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdGeomBackPlateAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdGeomBackPlateAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "backPlate:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomBackPlateAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdGeomBackPlateAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdGeomBackPlateAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdGeomBackPlateAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomBackPlateAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDGEOM_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdGeomBackPlateAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.backPlate:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdGeomBackPlateAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomBackPlateAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdGeomBackPlateAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdGeomBackPlateAPI(prim, name);
    USDGEOM_API
    static UsdGeomBackPlateAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdGeomBackPlateAPI on the 
    /// given \p prim.
    USDGEOM_API
    static std::vector<UsdGeomBackPlateAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of BackPlateAPI.
    USDGEOM_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// BackPlateAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDGEOM_API
    static bool
    IsBackPlateAPIPath(const SdfPath &path, TfToken *name);

    /// Returns true if this <b>multiple-apply</b> API schema can be applied,
    /// with the given instance name, \p name, to the given \p prim. If this 
    /// schema can not be a applied the prim, this returns false and, if 
    /// provided, populates \p whyNot with the reason it can not be applied.
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
    USDGEOM_API
    static bool 
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "BackPlateAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'BackPlateAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdGeomBackPlateAPI object is returned upon success. 
    /// An invalid (or empty) UsdGeomBackPlateAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDGEOM_API
    static UsdGeomBackPlateAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

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
    // SCALETWEAK 
    // --------------------------------------------------------------------- //
    /// The (X,Y) scale of the plate about its center. The back plate's 
    /// default dimensions will be determined by its focus distance and the size
    /// of the camera frustum.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float2 scale:tweak = (1, 1)` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    USDGEOM_API
    UsdAttribute GetScaleTweakAttr() const;

    /// See GetScaleTweakAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateScaleTweakAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ROTATEXYZTWEAK 
    // --------------------------------------------------------------------- //
    /// The (X,Y,Z) rotation of the plate about its center in 
    /// degrees. The rotation order is XYZ.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3 rotateXYZ:tweak = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3 |
    USDGEOM_API
    UsdAttribute GetRotateXYZTweakAttr() const;

    /// See GetRotateXYZTweakAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateRotateXYZTweakAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRANSLATETWEAK 
    // --------------------------------------------------------------------- //
    /// The (X,Y,Z) translation of the plate about its center after it 
    /// has been translated by the focus distance and scaled accordingly.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3 translate:tweak = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3 |
    USDGEOM_API
    UsdAttribute GetTranslateTweakAttr() const;

    /// See GetTranslateTweakAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTranslateTweakAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // IMAGE 
    // --------------------------------------------------------------------- //
    /// Asset path to the file containing a texture or sequence of 
    /// textures.  Currently, each asset must be a simple texture, not more 
    /// general media such as movie files. The images by default will be 
    /// centered on the back plate. If an invalid image is provided, then the 
    /// plate will default to black.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `asset image = @@` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    USDGEOM_API
    UsdAttribute GetImageAttr() const;

    /// See GetImageAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateImageAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ALPHAIMAGE 
    // --------------------------------------------------------------------- //
    /// Asset path to channel representing the opacity of the back 
    /// plate. If a single-channel texture is fed into this property, the alpha 
    /// values will be set to that channel. If a two-channel texture is fed 
    /// into this property, the alpha values will be set to the second channel. 
    /// If a four-channel texture is fed into this property, then the fourth 
    /// channel will be considered the alpha channel. If any other n-channel
    /// texture is fed into image then it will be ignored and the alpha will 
    /// default to 1.0.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `asset alpha:image = @@` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    USDGEOM_API
    UsdAttribute GetAlphaImageAttr() const;

    /// See GetAlphaImageAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateAlphaImageAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DEPTHIMAGE 
    // --------------------------------------------------------------------- //
    /// File path to a single-channel texture that describes the depth 
    /// associated with the image if one exists. If not then the back plate 
    /// will have uniform depth. The depth channel should be linear, and the 
    /// final depth value is a float computed as `computedDepth = (textureValue 
    /// - minOffset) * normalizingFactor + cameraSpaceOffset`, where the 
    /// offsets can be set by the user. The depth channel is not a hardware 
    /// generated z buffer as these have unknown format and are not generally 
    /// readable outside of the GPU.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `asset depth:image = @@` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    USDGEOM_API
    UsdAttribute GetDepthImageAttr() const;

    /// See GetDepthImageAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateDepthImageAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DEPTHMINOFFSET 
    // --------------------------------------------------------------------- //
    /// Offset to shift the depth values if minimum value of current 
    /// range is not set at 0 if needed.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float depth:minOffset = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetDepthMinOffsetAttr() const;

    /// See GetDepthMinOffsetAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateDepthMinOffsetAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DEPTHNORMALIZINGFACTOR 
    // --------------------------------------------------------------------- //
    /// Value to scale the texture depth value if needed.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float depth:normalizingFactor = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetDepthNormalizingFactorAttr() const;

    /// See GetDepthNormalizingFactorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateDepthNormalizingFactorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DEPTHCAMERASPACEOFFSET 
    // --------------------------------------------------------------------- //
    /// Offset to shift the depth into camera space if needed.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float depth:cameraSpaceOffset = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetDepthCameraSpaceOffsetAttr() const;

    /// See GetDepthCameraSpaceOffsetAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateDepthCameraSpaceOffsetAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LUMAGAIN 
    // --------------------------------------------------------------------- //
    /// Scales the per-channel upper bound of the normalized signal 
    /// before gamma, analogous to per-channel exposure or slope. It is applied 
    /// in the order of `colorValue = (value*(gain-lift)+lift)^(1/gamma)`.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3 luma:gain = (1, 1, 1)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3 |
    USDGEOM_API
    UsdAttribute GetLumaGainAttr() const;

    /// See GetLumaGainAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateLumaGainAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LUMALIFT 
    // --------------------------------------------------------------------- //
    /// Raises or lowers the signal floor (black level) of the 
    /// normalized value prior to the gain and gamma stages. It is applied
    /// in the order of `colorValue = (value*(gain-lift)+lift)^(1/gamma)`.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3 luma:lift = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3 |
    USDGEOM_API
    UsdAttribute GetLumaLiftAttr() const;

    /// See GetLumaLiftAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateLumaLiftAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LUMAGAMMA 
    // --------------------------------------------------------------------- //
    /// Per-channel power applied to the normalized RGB signal after 
    /// lift and gain. Must be > 0. It is applied
    /// in the order of `colorValue = (value*(gain-lift)+lift)^(1/gamma)`.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3 luma:gamma = (1, 1, 1)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3 |
    USDGEOM_API
    UsdAttribute GetLumaGammaAttr() const;

    /// See GetLumaGammaAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateLumaGammaAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PLATEVISIBILITY 
    // --------------------------------------------------------------------- //
    /// Toggles the visibility of the back plate to all cameras, only 
    /// the owning camera, or no cameras.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token plateVisibility = "solo"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdGeomTokens "Allowed Values" | all, solo, mute |
    USDGEOM_API
    UsdAttribute GetPlateVisibilityAttr() const;

    /// See GetPlateVisibilityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreatePlateVisibilityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Returns the height of the plate. Calculated by the scale and the size 
    /// of the camera frustum at a focus distance away from the intrinsic 
    /// camera origin. Note if the focus distance is less than or equal to the 
    /// focal length then the dimensions of the plate will be 0.
    ///
    USDGEOM_API
    float ComputeEffectiveDimension(bool computeWidth, UsdTimeCode time) const;

    /// Computes and sets backplate tweak::scale such that the back plate
    /// will have the desired \p width and \p height.
    ///
    USDGEOM_API
    void SetAspectRatio(float width, float height, UsdTimeCode time) const;

    /// Computes and sets backplate tweak::translate such that the back plate
    /// will be positioned at the desired coordinates in camera space.
    ///
    USDGEOM_API
    void SetCameraSpacePosition(GfVec3f pos, UsdTimeCode time) const;

    /// Computes and returns the position of the backplate in camera space.
    ///
    USDGEOM_API
    GfVec3f GetCameraSpacePosition(UsdTimeCode time) const;

    /// Computes and sets backplate tweak::translate such that the back plate
    /// will be positioned at the desired coordinates in world space.
    ///
    USDGEOM_API
    void SetWorldSpacePosition(GfVec3f pos, UsdTimeCode time) const;

    /// Computes and returns the position of the backplate in world space.
    ///
    USDGEOM_API
    GfVec3f GetWorldSpacePosition(UsdTimeCode time) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
