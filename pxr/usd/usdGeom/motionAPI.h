//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_MOTIONAPI_H
#define USDGEOM_GENERATED_MOTIONAPI_H

/// \file usdGeom/motionAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
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
// MOTIONAPI                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdGeomMotionAPI
///
/// UsdGeomMotionAPI encodes data that can live on any prim that
/// may affect computations involving:
/// - computed motion for motion blur
/// - sampling for motion blur
/// 
/// The \ref GetMotionBlurScaleAttr() "motion:blurScale" attribute allows
/// artists to scale the __amount__ of motion blur to be rendered for parts
/// of the scene without changing the recorded animation.  See
/// \ref UsdGeomMotionAPI_blurScale for use and implementation details.
/// 
/// 
///
class UsdGeomMotionAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdGeomMotionAPI on UsdPrim \p prim .
    /// Equivalent to UsdGeomMotionAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomMotionAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdGeomMotionAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomMotionAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomMotionAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomMotionAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomMotionAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomMotionAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomMotionAPI
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
    USDGEOM_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "MotionAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdGeomMotionAPI object is returned upon success. 
    /// An invalid (or empty) UsdGeomMotionAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDGEOM_API
    static UsdGeomMotionAPI 
    Apply(const UsdPrim &prim);

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
    // MOTIONBLURSCALE 
    // --------------------------------------------------------------------- //
    /// BlurScale is an __inherited__ float attribute that stipulates
    /// the rendered motion blur (as typically specified via UsdGeomCamera's
    /// _shutter:open_ and _shutter:close_ properties) should be scaled for
    /// __all objects__ at and beneath the prim in namespace on which the
    /// _motion:blurScale_ value is specified.
    /// 
    /// Without changing any other data in the scene, _blurScale_ allows artists to
    /// "dial in" the amount of blur on a per-object basis.  A _blurScale_
    /// value of zero removes all blur, a value of 0.5 reduces blur by half, 
    /// and a value of 2.0 doubles the blur.  The legal range for _blurScale_
    /// is [0, inf), although very high values may result in extremely expensive
    /// renders, and may exceed the capabilities of some renderers.
    /// 
    /// Although renderers are free to implement this feature however they see
    /// fit, see \ref UsdGeomMotionAPI_blurScale for our guidance on implementing
    /// the feature universally and efficiently.
    /// 
    /// \sa ComputeMotionBlurScale()
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float motion:blurScale = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetMotionBlurScaleAttr() const;

    /// See GetMotionBlurScaleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateMotionBlurScaleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VELOCITYSCALE 
    // --------------------------------------------------------------------- //
    /// \deprecated
    /// 
    /// VelocityScale is an **inherited** float attribute that
    /// velocity-based schemas (e.g. PointBased, PointInstancer) can consume
    /// to compute interpolated positions and orientations by applying
    /// velocity and angularVelocity, which is required for interpolating 
    /// between samples when topology is varying over time.  Although these 
    /// quantities are generally physically computed by a simulator, sometimes 
    /// we require more or less motion-blur to achieve the desired look.  
    /// VelocityScale allows artists to dial-in, as a post-sim correction, 
    /// a scale factor to be applied to the velocity prior to computing 
    /// interpolated positions from it.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float motion:velocityScale = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetVelocityScaleAttr() const;

    /// See GetVelocityScaleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVelocityScaleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NONLINEARSAMPLECOUNT 
    // --------------------------------------------------------------------- //
    /// Determines the number of position or transformation samples
    /// created when motion is described by attributes contributing non-linear
    /// terms.
    /// 
    /// To give an example, imagine an application (such as a
    /// renderer) consuming 'points' and the USD document also
    /// contains 'accelerations' for the same prim. Unless the
    /// application can consume these 'accelerations' itself, an
    /// intermediate layer has to compute samples within the sampling
    /// interval for the point positions based on the value of
    /// 'points', 'velocities' and 'accelerations'. The number of these
    /// samples is given by 'nonlinearSampleCount'. The samples are
    /// equally spaced within the sampling interval.
    /// 
    /// Another example involves the PointInstancer where
    /// 'nonlinearSampleCount' is relevant when 'angularVelocities'
    /// or 'accelerations' are authored.
    /// 
    /// 'nonlinearSampleCount' is an **inherited** attribute, also
    /// see ComputeNonlinearSampleCount()
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int motion:nonlinearSampleCount = 3` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDGEOM_API
    UsdAttribute GetNonlinearSampleCountAttr() const;

    /// See GetNonlinearSampleCountAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateNonlinearSampleCountAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// \deprecated
    ///
    /// Compute the inherited value of *velocityScale* at \p time, i.e. the 
    /// authored value on the prim closest to this prim in namespace, resolved
    /// upwards through its ancestors in namespace.
    ///
    /// \return the inherited value, or 1.0 if neither the prim nor any
    /// of its ancestors possesses an authored value.
    ///
    /// \note this is a reference implementation that is not particularly 
    /// efficient if evaluating over many prims, because it does not share
    /// inherited results.
    USDGEOM_API
    float ComputeVelocityScale(UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Compute the inherited value of *nonlinearSampleCount* at \p time,
    /// i.e. the authored value on the prim closest to this prim in namespace,
    /// resolved upwards through its ancestors in namespace.
    ///
    /// \return the inherited value, or 3 if neither the prim nor any
    /// of its ancestors possesses an authored value.
    ///
    /// \note this is a reference implementation that is not particularly 
    /// efficient if evaluating over many prims, because it does not share
    /// inherited results.
    USDGEOM_API
    int ComputeNonlinearSampleCount(
        UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Compute the inherited value of *motion:blurScale* at \p time, i.e. the 
    /// authored value on the prim closest to this prim in namespace, resolved
    /// upwards through its ancestors in namespace.
    ///
    /// \return the inherited value, or 1.0 if neither the prim nor any
    /// of its ancestors possesses an authored value.
    ///
    /// \note this is a reference implementation that is not particularly 
    /// efficient if evaluating over many prims, because it does not share
    /// inherited results.
    USDGEOM_API
    float ComputeMotionBlurScale(
        UsdTimeCode time = UsdTimeCode::Default()) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
