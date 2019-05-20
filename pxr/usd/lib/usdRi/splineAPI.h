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
#ifndef USDRI_GENERATED_SPLINEAPI_H
#define USDRI_GENERATED_SPLINEAPI_H

/// \file usdRi/splineAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// RISPLINEAPI                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdRiSplineAPI
///
/// RiSplineAPI is a general purpose API schema used to describe
/// a named spline stored as a set of attributes on a prim.
/// 
/// It is an add-on schema that can be applied many times to a prim with
/// different spline names. All the attributes authored by the schema
/// are namespaced under "$NAME:spline:", with the name of the
/// spline providing a namespace for the attributes.
/// 
/// The spline describes a 2D piecewise cubic curve with a position and
/// value for each knot. This is chosen to give straightforward artistic
/// control over the shape. The supported basis types are:
/// 
/// - linear (UsdRiTokens->linear)
/// - bspline (UsdRiTokens->bspline)
/// - Catmull-Rom (UsdRiTokens->catmullRom)
/// 
///
class UsdRiSplineAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::SingleApplyAPI;

    /// Construct a UsdRiSplineAPI on UsdPrim \p prim .
    /// Equivalent to UsdRiSplineAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRiSplineAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdRiSplineAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdRiSplineAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRiSplineAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDRI_API
    virtual ~UsdRiSplineAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRiSplineAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRiSplineAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRI_API
    static UsdRiSplineAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "RiSplineAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdRiSplineAPI object is returned upon success. 
    /// An invalid (or empty) UsdRiSplineAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDRI_API
    static UsdRiSplineAPI 
    Apply(const UsdPrim &prim);

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

    /// Construct a UsdRiSplineAPI with the given \p splineName on 
    /// the UsdPrim \p prim .
    explicit UsdRiSplineAPI(const UsdPrim& prim, const TfToken &splineName,
                            const SdfValueTypeName &valuesTypeName,
                            bool doesDuplicateBSplineEndpoints)
        : UsdAPISchemaBase(prim)
        , _splineName(splineName)
        , _valuesTypeName(valuesTypeName)
        , _duplicateBSplineEndpoints(doesDuplicateBSplineEndpoints)
    {
    }

    /// Construct a UsdRiSplineAPI with the given \p splineName on 
    /// the prim held by \p schemaObj .
    explicit UsdRiSplineAPI(const UsdSchemaBase& schemaObj, 
                            const TfToken &splineName,
                            const SdfValueTypeName &valuesTypeName,
                            bool doesDuplicateBSplineEndpoints)
        : UsdAPISchemaBase(schemaObj.GetPrim())
        , _splineName(splineName)
        , _valuesTypeName(valuesTypeName)
        , _duplicateBSplineEndpoints(doesDuplicateBSplineEndpoints)
    {
    }

    /// Returns true if this UsdRiSplineAPI is configured to ensure
    /// the endpoints are duplicated when using a bspline basis.
    ///
    /// Duplicating the endpoints ensures that the spline reaches
    /// those points at either end of the parameter range.
    USDRI_API
    bool DoesDuplicateBSplineEndpoints() const {
        return _duplicateBSplineEndpoints;
    }

    /// Returns the intended typename of the values attribute of the spline.
    USDRI_API
    SdfValueTypeName GetValuesTypeName() const {
        return _valuesTypeName;
    }

    // --------------------------------------------------------------------- //
    // INTERPOLATION 
    // --------------------------------------------------------------------- //
    /// Interpolation method for the spline.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: linear
    /// \n  \ref UsdRiTokens "Allowed Values": [linear, constant, bspline, catmullRom]
    USDRI_API
    UsdAttribute GetInterpolationAttr() const;

    /// See GetInterpolationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateInterpolationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

    // --------------------------------------------------------------------- //
    // POSITIONS 
    // --------------------------------------------------------------------- //
    /// Positions of the knots.
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDRI_API
    UsdAttribute GetPositionsAttr() const;

    /// See GetPositionsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreatePositionsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

    // --------------------------------------------------------------------- //
    // VALUES 
    // --------------------------------------------------------------------- //
    /// Values of the knots.
    ///
    /// \n  C++ Type: See GetValuesTypeName()
    /// \n  Usd Type: See GetValuesTypeName()
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDRI_API
    UsdAttribute GetValuesAttr() const;

    /// See GetValuesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateValuesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

    /// \anchor UsdRiSplineAPI_Validation
    /// \name Spline Validation API
    /// 
    /// API for validating the properties of a spline.
    /// 
    /// @{

    /// Validates the attribute values belonging to the spline. Returns true 
    /// if the spline has all valid attribute values. Returns false and 
    /// populates the \p reason output argument if the spline has invalid 
    /// attribute values.
    /// 
    /// Here's the list of validations performed by this method:
    /// \li the SplineAPI must be fully initialized
    /// \li interpolation attribute must exist and use an allowed value
    /// \li the positions array must be a float array
    /// \li the positions array must be sorted by increasing value
    /// \li the values array must use the correct value type
    /// \li the positions and values array must have the same size
    /// 
    USDRI_API
    bool Validate(std::string *reason) const;

    /// @}

private:
    /// Returns the properly-scoped form of the given property name,
    /// accounting for the spline name.
    TfToken _GetScopedPropertyName(const TfToken &baseName) const;

private:
    const TfToken _splineName;
    const SdfValueTypeName _valuesTypeName;
    bool _duplicateBSplineEndpoints;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
