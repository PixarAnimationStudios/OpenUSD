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
#ifndef USDLUX_GENERATED_BOUNDABLELIGHTBASE_H
#define USDLUX_GENERATED_BOUNDABLELIGHTBASE_H

/// \file usdLux/boundableLightBase.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/lightAPI.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// BOUNDABLELIGHTBASE                                                         //
// -------------------------------------------------------------------------- //

/// \class UsdLuxBoundableLightBase
///
/// Base class for intrinsic lights that are boundable.
/// 
/// The primary purpose of this class is to provide a direct API to the 
/// functions provided by LightAPI for concrete derived light types.
/// 
///
class UsdLuxBoundableLightBase : public UsdGeomBoundable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractTyped;

    /// Construct a UsdLuxBoundableLightBase on UsdPrim \p prim .
    /// Equivalent to UsdLuxBoundableLightBase::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxBoundableLightBase(const UsdPrim& prim=UsdPrim())
        : UsdGeomBoundable(prim)
    {
    }

    /// Construct a UsdLuxBoundableLightBase on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxBoundableLightBase(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxBoundableLightBase(const UsdSchemaBase& schemaObj)
        : UsdGeomBoundable(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxBoundableLightBase();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxBoundableLightBase holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxBoundableLightBase(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxBoundableLightBase
    Get(const UsdStagePtr &stage, const SdfPath &path);


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

    /// \name LightAPI
    /// 
    /// Convenience accessors for the light's built-in UsdLuxLightAPI
    /// 
    /// @{

    /// Contructs and returns a UsdLuxLightAPI object for this light.
    USDLUX_API
    UsdLuxLightAPI LightAPI() const;

    /// See UsdLuxLightAPI::GetIntensityAttr().
    USDLUX_API
    UsdAttribute GetIntensityAttr() const;

    /// See UsdLuxLightAPI::CreateIntensityAttr().
    USDLUX_API
    UsdAttribute CreateIntensityAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetExposureAttr().
    USDLUX_API
    UsdAttribute GetExposureAttr() const;

    /// See UsdLuxLightAPI::CreateExposureAttr().
    USDLUX_API
    UsdAttribute CreateExposureAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetDiffuseAttr().
    USDLUX_API
    UsdAttribute GetDiffuseAttr() const;

    /// See UsdLuxLightAPI::CreateDiffuseAttr().
    USDLUX_API
    UsdAttribute CreateDiffuseAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetSpecularAttr().
    USDLUX_API
    UsdAttribute GetSpecularAttr() const;

    /// See UsdLuxLightAPI::CreateSpecularAttr().
    USDLUX_API
    UsdAttribute CreateSpecularAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetNormalizeAttr().
    USDLUX_API
    UsdAttribute GetNormalizeAttr() const;

    /// See UsdLuxLightAPI::CreateNormalizeAttr().
    USDLUX_API
    UsdAttribute CreateNormalizeAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetColorAttr().
    USDLUX_API
    UsdAttribute GetColorAttr() const;

    /// See UsdLuxLightAPI::CreateColorAttr().
    USDLUX_API
    UsdAttribute CreateColorAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetEnableColorTemperatureAttr().
    USDLUX_API
    UsdAttribute GetEnableColorTemperatureAttr() const;

    /// See UsdLuxLightAPI::CreateEnableColorTemperatureAttr().
    USDLUX_API
    UsdAttribute CreateEnableColorTemperatureAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetColorTemperatureAttr().
    USDLUX_API
    UsdAttribute GetColorTemperatureAttr() const;

    /// See UsdLuxLightAPI::CreateColorTemperatureAttr().
    USDLUX_API
    UsdAttribute CreateColorTemperatureAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetFiltersRel().
    USDLUX_API
    UsdRelationship GetFiltersRel() const;

    /// See UsdLuxLightAPI::CreateFiltersRel().
    USDLUX_API
    UsdRelationship CreateFiltersRel() const;

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
