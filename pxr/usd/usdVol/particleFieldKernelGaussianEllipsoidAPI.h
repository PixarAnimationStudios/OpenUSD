//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDVOL_GENERATED_PARTICLEFIELDKERNELGAUSSIANELLIPSOIDAPI_H
#define USDVOL_GENERATED_PARTICLEFIELDKERNELGAUSSIANELLIPSOIDAPI_H

/// \file usdVol/particleFieldKernelGaussianEllipsoidAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdVol/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdVol/particleFieldKernelBaseAPI.h"
        

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PARTICLEFIELDKERNELGAUSSIANELLIPSOIDAPI                                    //
// -------------------------------------------------------------------------- //

/// \class UsdVolParticleFieldKernelGaussianEllipsoidAPI
///
/// Defines the gaussian ellipsoid kernel for a given ParticleField.
/// 
/// An untransformed kernel (i.e. identity position, scale, rotation, opacity)
/// will define opacity at point 'p' by g(u=0;o=1;x = p.length()).  Note that
/// since the standard deviation is 1, the 3-sigma point is 3.0 and 99.7% of
/// the splat support is within a spherical region of radius 3.
/// 
/// Per-splat opacity is multiplicative with the gaussian falloff; rotation
/// and scale will transform the gaussian sphere kernel into an ellipsoid;
/// and position moves the per-splat peak falloff from the origin. 
/// 
///
class UsdVolParticleFieldKernelGaussianEllipsoidAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdVolParticleFieldKernelGaussianEllipsoidAPI on UsdPrim \p prim .
    /// Equivalent to UsdVolParticleFieldKernelGaussianEllipsoidAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdVolParticleFieldKernelGaussianEllipsoidAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdVolParticleFieldKernelGaussianEllipsoidAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdVolParticleFieldKernelGaussianEllipsoidAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdVolParticleFieldKernelGaussianEllipsoidAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDVOL_API
    virtual ~UsdVolParticleFieldKernelGaussianEllipsoidAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDVOL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdVolParticleFieldKernelGaussianEllipsoidAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdVolParticleFieldKernelGaussianEllipsoidAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDVOL_API
    static UsdVolParticleFieldKernelGaussianEllipsoidAPI
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
    USDVOL_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ParticleFieldKernelGaussianEllipsoidAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdVolParticleFieldKernelGaussianEllipsoidAPI object is returned upon success. 
    /// An invalid (or empty) UsdVolParticleFieldKernelGaussianEllipsoidAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDVOL_API
    static UsdVolParticleFieldKernelGaussianEllipsoidAPI 
    Apply(const UsdPrim &prim);

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
    /// \name ParticleFieldKernelBaseAPI
    /// 
    /// Convenience accessors for the built-in UsdVolParticleFieldKernelBaseAPI
    /// 
    /// @{

    /// Constructs and returns a UsdVolParticleFieldKernelBaseAPI object.
    /// Use this object to access UsdVolParticleFieldKernelBaseAPI custom methods.
    USDVOL_API
    UsdVolParticleFieldKernelBaseAPI ParticleFieldKernelBaseAPI() const;

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
