//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLIGHTFIELD_GENERATED_PARTICLEFIELD_3DGAUSSIANSPLAT_H
#define USDLIGHTFIELD_GENERATED_PARTICLEFIELD_3DGAUSSIANSPLAT_H

/// \file usdLightField/particleField_3DGaussianSplat.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLightField/api.h"
#include "pxr/usd/usdLightField/particleField.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLightField/tokens.h"

            #include "pxr/usd/usdLightField/positionAttributeAPI.h"
            #include "pxr/usd/usdLightField/orientationAttributeAPI.h"
            #include "pxr/usd/usdLightField/scaleAttributeAPI.h"
            #include "pxr/usd/usdLightField/opacityAttributeAPI.h"
            #include "pxr/usd/usdLightField/kernelGaussianEllipsoidAPI.h"
            #include "pxr/usd/usdLightField/sphericalHarmonicsAttributeAPI.h"
        

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PARTICLEFIELD_3DGAUSSIANSPLAT                                              //
// -------------------------------------------------------------------------- //

/// \class UsdLightFieldParticleField_3DGaussianSplat
///
/// This is a concrete ParticleField representing the original 3D
/// Gaussian Splats technique (https://arxiv.org/abs/2308.04079).
/// 
/// It inherits from the ParticleField base prim, and has a set of
/// applied schema automatically applied to provide the required
/// attributes for original 3DGS.
/// 
/// Also contains some rendering hints that can optionally inform
/// how the splats should be rendered.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLightFieldTokens.
/// So to set an attribute to the value "rightHanded", use UsdLightFieldTokens->rightHanded
/// as the value.
///
class UsdLightFieldParticleField_3DGaussianSplat : public UsdLightFieldParticleField
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdLightFieldParticleField_3DGaussianSplat on UsdPrim \p prim .
    /// Equivalent to UsdLightFieldParticleField_3DGaussianSplat::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLightFieldParticleField_3DGaussianSplat(const UsdPrim& prim=UsdPrim())
        : UsdLightFieldParticleField(prim)
    {
    }

    /// Construct a UsdLightFieldParticleField_3DGaussianSplat on the prim held by \p schemaObj .
    /// Should be preferred over UsdLightFieldParticleField_3DGaussianSplat(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLightFieldParticleField_3DGaussianSplat(const UsdSchemaBase& schemaObj)
        : UsdLightFieldParticleField(schemaObj)
    {
    }

    /// Destructor.
    USDLIGHTFIELD_API
    virtual ~UsdLightFieldParticleField_3DGaussianSplat();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLIGHTFIELD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLightFieldParticleField_3DGaussianSplat holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLightFieldParticleField_3DGaussianSplat(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLIGHTFIELD_API
    static UsdLightFieldParticleField_3DGaussianSplat
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
    USDLIGHTFIELD_API
    static UsdLightFieldParticleField_3DGaussianSplat
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLIGHTFIELD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLIGHTFIELD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLIGHTFIELD_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // PROJECTIONMODEHINT 
    // --------------------------------------------------------------------- //
    /// A hint for the renderer on how to project the gaussian to
    /// achieve a perspective correct view. Renderers are free to
    /// ignore this, but the hint is often valuable to tune the
    /// rendering of the scene.
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
    /// | \ref UsdLightFieldTokens "Allowed Values" | perspective, tangential |
    USDLIGHTFIELD_API
    UsdAttribute GetProjectionModeHintAttr() const;

    /// See GetProjectionModeHintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateProjectionModeHintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SORTINGMODEHINT 
    // --------------------------------------------------------------------- //
    /// A hint for the renderer on how to sort the gaussians while
    /// drawing. Renderers are free to ignore this, but the hint is often
    /// valuable to tune the rendering of the scene.
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
    /// | \ref UsdLightFieldTokens "Allowed Values" | zDepth, cameraDistance |
    USDLIGHTFIELD_API
    UsdAttribute GetSortingModeHintAttr() const;

    /// See GetSortingModeHintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateSortingModeHintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    /// \name LightFieldPositionAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdLightFieldPositionAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLightFieldPositionAttributeAPI object.
    /// Use this object to access UsdLightFieldPositionAttributeAPI custom methods.
    USDLIGHTFIELD_API
    UsdLightFieldPositionAttributeAPI LightFieldPositionAttributeAPI() const;

    /// See UsdLightFieldPositionAttributeAPI::GetPositionsAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetPositionsAttr() const;

    /// See UsdLightFieldPositionAttributeAPI::CreatePositionsAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreatePositionsAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLightFieldPositionAttributeAPI::GetPositionshAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetPositionshAttr() const;

    /// See UsdLightFieldPositionAttributeAPI::CreatePositionshAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreatePositionshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name LightFieldOrientationAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdLightFieldOrientationAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLightFieldOrientationAttributeAPI object.
    /// Use this object to access UsdLightFieldOrientationAttributeAPI custom methods.
    USDLIGHTFIELD_API
    UsdLightFieldOrientationAttributeAPI LightFieldOrientationAttributeAPI() const;

    /// See UsdLightFieldOrientationAttributeAPI::GetOrientationsAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetOrientationsAttr() const;

    /// See UsdLightFieldOrientationAttributeAPI::CreateOrientationsAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateOrientationsAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLightFieldOrientationAttributeAPI::GetOrientationshAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetOrientationshAttr() const;

    /// See UsdLightFieldOrientationAttributeAPI::CreateOrientationshAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateOrientationshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name LightFieldScaleAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdLightFieldScaleAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLightFieldScaleAttributeAPI object.
    /// Use this object to access UsdLightFieldScaleAttributeAPI custom methods.
    USDLIGHTFIELD_API
    UsdLightFieldScaleAttributeAPI LightFieldScaleAttributeAPI() const;

    /// See UsdLightFieldScaleAttributeAPI::GetScalesAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetScalesAttr() const;

    /// See UsdLightFieldScaleAttributeAPI::CreateScalesAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateScalesAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLightFieldScaleAttributeAPI::GetScaleshAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetScaleshAttr() const;

    /// See UsdLightFieldScaleAttributeAPI::CreateScaleshAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateScaleshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name LightFieldOpacityAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdLightFieldOpacityAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLightFieldOpacityAttributeAPI object.
    /// Use this object to access UsdLightFieldOpacityAttributeAPI custom methods.
    USDLIGHTFIELD_API
    UsdLightFieldOpacityAttributeAPI LightFieldOpacityAttributeAPI() const;

    /// See UsdLightFieldOpacityAttributeAPI::GetOpacitiesAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetOpacitiesAttr() const;

    /// See UsdLightFieldOpacityAttributeAPI::CreateOpacitiesAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateOpacitiesAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLightFieldOpacityAttributeAPI::GetOpacitieshAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetOpacitieshAttr() const;

    /// See UsdLightFieldOpacityAttributeAPI::CreateOpacitieshAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateOpacitieshAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// @}
public:
    /// \name LightFieldKernelGaussianEllipsoidAPI
    /// 
    /// Convenience accessors for the built-in UsdLightFieldKernelGaussianEllipsoidAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLightFieldKernelGaussianEllipsoidAPI object.
    /// Use this object to access UsdLightFieldKernelGaussianEllipsoidAPI custom methods.
    USDLIGHTFIELD_API
    UsdLightFieldKernelGaussianEllipsoidAPI LightFieldKernelGaussianEllipsoidAPI() const;

    /// @}
public:
    /// \name LightFieldSphericalHarmonicsAttributeAPI
    /// 
    /// Convenience accessors for the built-in UsdLightFieldSphericalHarmonicsAttributeAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLightFieldSphericalHarmonicsAttributeAPI object.
    /// Use this object to access UsdLightFieldSphericalHarmonicsAttributeAPI custom methods.
    USDLIGHTFIELD_API
    UsdLightFieldSphericalHarmonicsAttributeAPI LightFieldSphericalHarmonicsAttributeAPI() const;

    /// See UsdLightFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsDegreeAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetRadianceSphericalHarmonicsDegreeAttr() const;

    /// See UsdLightFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsDegreeAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateRadianceSphericalHarmonicsDegreeAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLightFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientsAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetRadianceSphericalHarmonicsCoefficientsAttr() const;

    /// See UsdLightFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientsAttr().
    USDLIGHTFIELD_API
    UsdAttribute CreateRadianceSphericalHarmonicsCoefficientsAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLightFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientshAttr().
    USDLIGHTFIELD_API
    UsdAttribute GetRadianceSphericalHarmonicsCoefficientshAttr() const;

    /// See UsdLightFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientshAttr().
    USDLIGHTFIELD_API
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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
