//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLUX_GENERATED_NONBOUNDABLELIGHTBASE_H
#define USDLUX_GENERATED_NONBOUNDABLELIGHTBASE_H

/// \file usdLux/nonboundableLightBase.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usdGeom/xformable.h"
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
// NONBOUNDABLELIGHTBASE                                                      //
// -------------------------------------------------------------------------- //

/// \class UsdLuxNonboundableLightBase
///
/// Base class for intrinsic lights that are not boundable.
/// 
/// The primary purpose of this class is to provide a direct API to the 
/// functions provided by LightAPI for concrete derived light types.
/// 
///
class UsdLuxNonboundableLightBase : public UsdGeomXformable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractTyped;

    /// Construct a UsdLuxNonboundableLightBase on UsdPrim \p prim .
    /// Equivalent to UsdLuxNonboundableLightBase::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxNonboundableLightBase(const UsdPrim& prim=UsdPrim())
        : UsdGeomXformable(prim)
    {
    }

    /// Construct a UsdLuxNonboundableLightBase on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxNonboundableLightBase(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxNonboundableLightBase(const UsdSchemaBase& schemaObj)
        : UsdGeomXformable(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxNonboundableLightBase();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxNonboundableLightBase holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxNonboundableLightBase(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxNonboundableLightBase
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
    /// \name LightAPI
    /// 
    /// Convenience accessors for the built-in UsdLuxLightAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLuxLightAPI object.
    /// Use this object to access UsdLuxLightAPI custom methods.
    USDLUX_API
    UsdLuxLightAPI LightAPI() const;

    /// See UsdLuxLightAPI::GetShaderIdAttr().
    USDLUX_API
    UsdAttribute GetShaderIdAttr() const;

    /// See UsdLuxLightAPI::CreateShaderIdAttr().
    USDLUX_API
    UsdAttribute CreateShaderIdAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdLuxLightAPI::GetMaterialSyncModeAttr().
    USDLUX_API
    UsdAttribute GetMaterialSyncModeAttr() const;

    /// See UsdLuxLightAPI::CreateMaterialSyncModeAttr().
    USDLUX_API
    UsdAttribute CreateMaterialSyncModeAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

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
