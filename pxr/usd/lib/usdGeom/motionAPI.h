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
/// For example, UsdGeomMotionAPI provides *velocityScale* 
/// (GetVelocityScaleAttr()) for controlling how motion-blur samples should
/// be computed by velocity-consuming schemas.
///
class UsdGeomMotionAPI : public UsdAPISchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = false;

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


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "MotionAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdGeomMotionAPI object is returned upon success. 
    /// An invalid (or empty) UsdGeomMotionAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDGEOM_API
    static UsdGeomMotionAPI 
    Apply(const UsdPrim &prim);

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
    // VELOCITYSCALE 
    // --------------------------------------------------------------------- //
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
    /// See also ComputeVelocityScale()
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 1.0
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

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
