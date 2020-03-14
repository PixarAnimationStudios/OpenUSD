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
#ifndef USDLUX_GENERATED_SHAPINGAPI_H
#define USDLUX_GENERATED_SHAPINGAPI_H

/// \file usdLux/shapingAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

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
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::SingleApplyAPI;

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


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ShapingAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLuxShapingAPI object is returned upon success. 
    /// An invalid (or empty) UsdLuxShapingAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDLUX_API
    static UsdLuxShapingAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDLUX_API
    UsdSchemaType _GetSchemaType() const override;

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
    // SHAPINGFOCUS 
    // --------------------------------------------------------------------- //
    /// A control to shape the spread of light.  Higher focus
    /// values pull light towards the center and narrow the spread.
    /// Implemented as an off-axis cosine power exponent.
    /// TODO: clarify semantics
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float shaping:focus = 0` |
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
    // SHAPINGFOCUSTINT 
    // --------------------------------------------------------------------- //
    /// Off-axis color tint.  This tints the emission in the
    /// falloff region.  The default tint is black.
    /// TODO: clarify semantics
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `color3f shaping:focusTint = (0, 0, 0)` |
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
    // SHAPINGCONEANGLE 
    // --------------------------------------------------------------------- //
    /// Angular limit off the primary axis to restrict the
    /// light spread.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float shaping:cone:angle = 90` |
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
    // SHAPINGCONESOFTNESS 
    // --------------------------------------------------------------------- //
    /// Controls the cutoff softness for cone angle.
    /// TODO: clarify semantics
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float shaping:cone:softness = 0` |
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
    // SHAPINGIESFILE 
    // --------------------------------------------------------------------- //
    /// An IES (Illumination Engineering Society) light
    /// profile describing the angular distribution of light.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `asset shaping:ies:file` |
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
    // SHAPINGIESANGLESCALE 
    // --------------------------------------------------------------------- //
    /// Rescales the angular distribution of the IES profile.
    /// TODO: clarify semantics
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float shaping:ies:angleScale = 0` |
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
    // SHAPINGIESNORMALIZE 
    // --------------------------------------------------------------------- //
    /// Normalizes the IES profile so that it affects the shaping
    /// of the light while preserving the overall energy output.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool shaping:ies:normalize = 0` |
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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
