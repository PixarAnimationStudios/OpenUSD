//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDVOL_GENERATED_PARTICLEFIELD3DGAUSSIANSPLAT_H
#define USDVOL_GENERATED_PARTICLEFIELD3DGAUSSIANSPLAT_H

/// \file usdVol/particleField3DGaussianSplat.h

#include "pxr/pxr.h"
#include "pxr/usd/usdVol/api.h"
#include "pxr/usd/usdVol/particleField.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdVol/tokens.h"

#include "pxr/usd/usdVol/particleFieldPositionAttributeAPI.h"
#include "pxr/usd/usdVol/particleFieldOrientationAttributeAPI.h"
#include "pxr/usd/usdVol/particleFieldScaleAttributeAPI.h"
#include "pxr/usd/usdVol/particleFieldOpacityAttributeAPI.h"
#include "pxr/usd/usdVol/particleFieldKernelGaussianEllipsoidAPI.h"
#include "pxr/usd/usdVol/particleFieldSphericalHarmonicsAttributeAPI.h"
        

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PARTICLEFIELD3DGAUSSIANSPLAT                                               //
// -------------------------------------------------------------------------- //

/// \class UsdVolParticleField3DGaussianSplat
///
/// This is a concrete ParticleField representing the original 3D
/// Gaussian Splats technique (https://arxiv.org/abs/2308.04079).
/// 
/// It inherits from the ParticleField base prim, and has a set of
/// applied schema automatically applied to provide the required
/// attributes to define the necessary data from the original 3DGS paper.
/// 
/// It also contains some rendering hints that can optionally inform
/// how the splats should be rendered. These hints typically
/// corrolate with choices that were made when the data was trained.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdVolTokens.
/// So to set an attribute to the value "rightHanded", use UsdVolTokens->rightHanded
/// as the value.
///
class UsdVolParticleField3DGaussianSplat : public UsdVolParticleField
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdVolParticleField3DGaussianSplat on UsdPrim \p prim .
    /// Equivalent to UsdVolParticleField3DGaussianSplat::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdVolParticleField3DGaussianSplat(const UsdPrim& prim=UsdPrim())
        : UsdVolParticleField(prim)
    {
    }

    /// Construct a UsdVolParticleField3DGaussianSplat on the prim held by \p schemaObj .
    /// Should be preferred over UsdVolParticleField3DGaussianSplat(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdVolParticleField3DGaussianSplat(const UsdSchemaBase& schemaObj)
        : UsdVolParticleField(schemaObj)
    {
    }

    /// Destructor.
    USDVOL_API
    virtual ~UsdVolParticleField3DGaussianSplat();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDVOL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdVolParticleField3DGaussianSplat holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdVolParticleField3DGaussianSplat(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDVOL_API
    static UsdVolParticleField3DGaussianSplat
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
    USDVOL_API
    static UsdVolParticleField3DGaussianSplat
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDVOL_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDVOL_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDVOL_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // PROJECTIONMODEHINT 
    // --------------------------------------------------------------------- //
    /// A hint for the renderer on how to project the gaussian to
    /// achieve a perspective correct view. Renderers are free to
    /// ignore this, but the hint is often valuable to tune the
    /// rendering of the scene. It often corresponds to a choice made
    /// when training the data.
    /// 
    /// 'Perspective' projection is similar to standard object rendering
    /// from a camera view. Gaussians are projected with scaling and
    /// distortion based on depth.
    /// 
    /// 'Tangential' projection treats the image plane as a tangent to
    /// the viewing sphere. Gaussians are projected orthogonally,
    /// preserving shape and scale better, which helps reduce distortion
    /// for certain rendering applications like novel view synthesis.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token projectionModeHint = "perspective"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdVolTokens "Allowed Values" | perspective, tangential |
    USDVOL_API
    UsdAttribute GetProjectionModeHintAttr() const;

    /// See GetProjectionModeHintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDVOL_API
    UsdAttribute CreateProjectionModeHintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SORTINGMODEHINT 
    // --------------------------------------------------------------------- //
    /// A hint for the renderer on how to sort the gaussians while
    /// drawing. Renderers are free to ignore this, but the hint is often
    /// valuable to tune the rendering of the scene. It often corresponds to a
    /// choice made when training the data.
    /// 
    /// Rasterizers usually sort gaussians from back to front. This attribute
    /// is a hint for the metric used to sort the gaussians with respect to the
    /// camera.
    /// 
    /// 'zDepth': The particles are sorted based on the z component of the
    /// particle position when transformed in to the cameras local space.
    /// 
    /// 'cameraDistance': The particles are sorted based on the euclidian
    /// distance from the particle to the camera.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token sortingModeHint = "zDepth"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdVolTokens "Allowed Values" | zDepth, cameraDistance |
    USDVOL_API
    UsdAttribute GetSortingModeHintAttr() const;

    /// See GetSortingModeHintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDVOL_API
    UsdAttribute CreateSortingModeHintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    /// \name ParticleFieldPositionAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdVolParticleFieldPositionAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdVolParticleFieldPositionAttributeAPI object.
    /// Use this object to access UsdVolParticleFieldPositionAttributeAPI custom methods.
    USDVOL_API
    UsdVolParticleFieldPositionAttributeAPI ParticleFieldPositionAttributeAPI() const;

    /// See UsdVolParticleFieldPositionAttributeAPI::GetPositionsAttr().
    USDVOL_API
    UsdAttribute GetPositionsAttr() const;

    /// See UsdVolParticleFieldPositionAttributeAPI::CreatePositionsAttr().
    USDVOL_API
    UsdAttribute CreatePositionsAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdVolParticleFieldPositionAttributeAPI::GetPositionshAttr().
    USDVOL_API
    UsdAttribute GetPositionshAttr() const;

    /// See UsdVolParticleFieldPositionAttributeAPI::CreatePositionshAttr().
    USDVOL_API
    UsdAttribute CreatePositionshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name ParticleFieldOrientationAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdVolParticleFieldOrientationAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdVolParticleFieldOrientationAttributeAPI object.
    /// Use this object to access UsdVolParticleFieldOrientationAttributeAPI custom methods.
    USDVOL_API
    UsdVolParticleFieldOrientationAttributeAPI ParticleFieldOrientationAttributeAPI() const;

    /// See UsdVolParticleFieldOrientationAttributeAPI::GetOrientationsAttr().
    USDVOL_API
    UsdAttribute GetOrientationsAttr() const;

    /// See UsdVolParticleFieldOrientationAttributeAPI::CreateOrientationsAttr().
    USDVOL_API
    UsdAttribute CreateOrientationsAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdVolParticleFieldOrientationAttributeAPI::GetOrientationshAttr().
    USDVOL_API
    UsdAttribute GetOrientationshAttr() const;

    /// See UsdVolParticleFieldOrientationAttributeAPI::CreateOrientationshAttr().
    USDVOL_API
    UsdAttribute CreateOrientationshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name ParticleFieldScaleAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdVolParticleFieldScaleAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdVolParticleFieldScaleAttributeAPI object.
    /// Use this object to access UsdVolParticleFieldScaleAttributeAPI custom methods.
    USDVOL_API
    UsdVolParticleFieldScaleAttributeAPI ParticleFieldScaleAttributeAPI() const;

    /// See UsdVolParticleFieldScaleAttributeAPI::GetScalesAttr().
    USDVOL_API
    UsdAttribute GetScalesAttr() const;

    /// See UsdVolParticleFieldScaleAttributeAPI::CreateScalesAttr().
    USDVOL_API
    UsdAttribute CreateScalesAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdVolParticleFieldScaleAttributeAPI::GetScaleshAttr().
    USDVOL_API
    UsdAttribute GetScaleshAttr() const;

    /// See UsdVolParticleFieldScaleAttributeAPI::CreateScaleshAttr().
    USDVOL_API
    UsdAttribute CreateScaleshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name ParticleFieldOpacityAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdVolParticleFieldOpacityAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdVolParticleFieldOpacityAttributeAPI object.
    /// Use this object to access UsdVolParticleFieldOpacityAttributeAPI custom methods.
    USDVOL_API
    UsdVolParticleFieldOpacityAttributeAPI ParticleFieldOpacityAttributeAPI() const;

    /// See UsdVolParticleFieldOpacityAttributeAPI::GetOpacitiesAttr().
    USDVOL_API
    UsdAttribute GetOpacitiesAttr() const;

    /// See UsdVolParticleFieldOpacityAttributeAPI::CreateOpacitiesAttr().
    USDVOL_API
    UsdAttribute CreateOpacitiesAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdVolParticleFieldOpacityAttributeAPI::GetOpacitieshAttr().
    USDVOL_API
    UsdAttribute GetOpacitieshAttr() const;

    /// See UsdVolParticleFieldOpacityAttributeAPI::CreateOpacitieshAttr().
    USDVOL_API
    UsdAttribute CreateOpacitieshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name ParticleFieldKernelGaussianEllipsoidAPI
    /// 
    /// Convenience accessors for the built-in UsdVolParticleFieldKernelGaussianEllipsoidAPI
    /// 
    /// @{

    /// Constructs and returns a UsdVolParticleFieldKernelGaussianEllipsoidAPI object.
    /// Use this object to access UsdVolParticleFieldKernelGaussianEllipsoidAPI custom methods.
    USDVOL_API
    UsdVolParticleFieldKernelGaussianEllipsoidAPI ParticleFieldKernelGaussianEllipsoidAPI() const;

    /// @}
public:
    /// \name ParticleFieldSphericalHarmonicsAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdVolParticleFieldSphericalHarmonicsAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdVolParticleFieldSphericalHarmonicsAttributeAPI object.
    /// Use this object to access UsdVolParticleFieldSphericalHarmonicsAttributeAPI custom methods.
    USDVOL_API
    UsdVolParticleFieldSphericalHarmonicsAttributeAPI ParticleFieldSphericalHarmonicsAttributeAPI() const;

    /// See UsdVolParticleFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsDegreeAttr().
    USDVOL_API
    UsdAttribute GetRadianceSphericalHarmonicsDegreeAttr() const;

    /// See UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsDegreeAttr().
    USDVOL_API
    UsdAttribute CreateRadianceSphericalHarmonicsDegreeAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdVolParticleFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientsAttr().
    USDVOL_API
    UsdAttribute GetRadianceSphericalHarmonicsCoefficientsAttr() const;

    /// See UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientsAttr().
    USDVOL_API
    UsdAttribute CreateRadianceSphericalHarmonicsCoefficientsAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdVolParticleFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientshAttr().
    USDVOL_API
    UsdAttribute GetRadianceSphericalHarmonicsCoefficientshAttr() const;

    /// See UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientshAttr().
    USDVOL_API
    UsdAttribute CreateRadianceSphericalHarmonicsCoefficientshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
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

    /// Determines if we should prefer positions over positionsh based on
    /// whether positions has been authored to a non empty array. Assumes that
    /// positions is empty if the earliest time sample or default value if
    /// there are no time samples are empty.
    /// \param positionsAttr the outparam for the corresponding attribute,
    ///        either positions or positionsh.
    USDVOL_API
    bool UsesFloatPositions(UsdAttribute *positionsAttr) const;

    /// \overload same functionality as UsesFloatPositions but populates a
    /// TfToken instead.
    /// \param positionsToken (optional) the outparam for the corresponding
    ///        attribute name, either positions or positionsh.
    USDVOL_API
    bool UsesFloatPositions(TfToken *positionsToken = nullptr) const;

    /// Determines if we should prefer orientations over orientationsh based on
    /// whether orientations has been authored to a non empty array. Assumes
    /// that orientations is empty if the earliest time sample or default value
    /// if there are no time samples are empty.
    /// \param orientationsAttr the outparam for the corresponding attribute,
    ///        either orientations or orientationsh.
    USDVOL_API
    bool UsesFloatOrientations(UsdAttribute *orientationsAttr) const;

    /// \overload same functionality as UsesFloatOrientations but populates a
    /// TfToken instead.
    /// \param orientationsToken (optional) the outparam for the corresponding
    ///        attribute name, either orientations or orientationsh.
    USDVOL_API
    bool UsesFloatOrientations(TfToken *orientationsToken = nullptr) const;

    /// Determines if we should prefer scales over scalesh based on whether
    /// scales has been authored to a non empty array. Assumes that scales is
    /// empty if the earliest time sample or default value if there are no time
    /// samples are empty.
    /// \param scalesAttr the outparam for the corresponding attribute, either
    ///	       scales or scalesh.
    USDVOL_API
    bool UsesFloatScales(UsdAttribute *scalesAttr) const;

    /// \overload same functionality as UsesFloatScales but populates a TfToken
    /// instead.
    /// \param scalesToken (optional) the outparam for the corresponding
    ///        attribute name, either scales or scalesh.
    USDVOL_API
    bool UsesFloatScales(TfToken *scalesToken = nullptr) const;

    /// Determines if we should prefer opacities over opacitiesh based on
    /// whether opacities has been authored to a non empty array. Assumes that
    /// opacities is empty if the earliest time sample or default value if
    /// there are no time samples are empty.
    /// \param opacitiesAttr the outparam for the corresponding attribute,
    ///        either opacities or opacitiesh.
    USDVOL_API
    bool UsesFloatOpacities(UsdAttribute *opacitiesAttr) const;

    /// \overload same functionality as UsesFloatOpacities but populates a
    /// TfToken instead.
    /// \param opacitiesToken (optional) the outparam for the corresponding
    ///        attribute name, either opacities or opacitiesh.
    USDVOL_API
    bool UsesFloatOpacities(TfToken *opacitiesToken = nullptr) const;

    /// Determines if we should prefer radiance:sphericalHarmonicsCoefficients
    /// over radiance:sphericalHarmonicsCoefficientsh based on whether
    /// radiance:sphericalHarmonicsCoefficients has been authored to a non
    /// empty array. Assumes that radiance:sphericalHarmonicsCoefficients is
    /// empty if the earliest time sample or default value if there are no time
    /// samples are empty.
    /// \param radianceCoefficientsAttr the outparam for the corresponding
    ///        attribute, either radiance:sphericalHarmonicsCoefficients or
    ///        radiance:sphericalHarmonicsCoefficientsh.
    USDVOL_API
    bool UsesFloatRadianceCoefficients(
        UsdAttribute *radianceCoefficientsAttr) const;

    /// \overload same functionality as UsesFloatRadianceCoefficients but
    /// populates a TfToken instead.
    /// \param radianceCoefficientsToken (optional) the outparam for the
    ///        corresponding attribute name, either
    ///        radiance:sphericalHarmonicsCoefficients or
    ///        radiance:sphericalHarmonicsCoefficientsh.
    USDVOL_API
    bool UsesFloatRadianceCoefficients(
        TfToken *radianceCoefficientsToken = nullptr) const;

private:
    /// Helper function for UsesFloatFoo
    bool _UsesFloatAttr(
        TfToken const& floatName,
        TfToken const& halfName,
        UsdAttribute *outAttr) const;

    /// Helper function for UsesFloatFoo
    bool _UsesFloatAttr(
        TfToken const& floatName,
        TfToken const& halfName,
        TfToken *outToken) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
