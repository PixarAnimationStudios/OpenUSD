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
#ifndef USDLUX_GENERATED_LIGHT_H
#define USDLUX_GENERATED_LIGHT_H

/// \file usdLux/light.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LIGHT                                                                      //
// -------------------------------------------------------------------------- //

/// \class UsdLuxLight
///
/// Base class for all lights.
/// 
/// <b>Linking</b>
/// 
/// Lights can be linked to geometry.  Linking controls which geometry
/// a light illuminates, and which geometry casts shadows from the light.
/// 
/// Linking is specified as collections (UsdCollectionAPI) which can
/// be accessed via GetLightLinkCollection() and GetShadowLinkCollection().
/// Note that these collections have their includeRoot set to true,
/// so that lights will illuminate and cast shadows from all objects
/// by default.  To illuminate only a specific set of objects, there
/// are two options.  One option is to modify the collection paths
/// to explicitly exclude everything else, assuming it is known;
/// the other option is to set includeRoot to false and explicitly
/// include the desired objects.  These are complementary approaches
/// that may each be preferable depending on the scenario and how
/// to best express the intent of the light setup.
/// 
///
class UsdLuxLight : public UsdGeomXformable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractTyped;

    /// \deprecated
    /// Same as schemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    static const UsdSchemaKind schemaType = UsdSchemaKind::AbstractTyped;

    /// Construct a UsdLuxLight on UsdPrim \p prim .
    /// Equivalent to UsdLuxLight::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxLight(const UsdPrim& prim=UsdPrim())
        : UsdGeomXformable(prim)
    {
    }

    /// Construct a UsdLuxLight on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxLight(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxLight(const UsdSchemaBase& schemaObj)
        : UsdGeomXformable(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxLight();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxLight holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxLight(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxLight
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLUX_API
    UsdSchemaKind _GetSchemaKind() const override;

    /// \deprecated
    /// Same as _GetSchemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    USDLUX_API
    UsdSchemaKind _GetSchemaType() const override;

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
    // INTENSITY 
    // --------------------------------------------------------------------- //
    /// Scales the power of the light linearly.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:intensity = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetIntensityAttr() const;

    /// See GetIntensityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateIntensityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPOSURE 
    // --------------------------------------------------------------------- //
    /// Scales the power of the light exponentially as a power
    /// of 2 (similar to an F-stop control over exposure).  The result
    /// is multiplied against the intensity.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:exposure = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetExposureAttr() const;

    /// See GetExposureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateExposureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DIFFUSE 
    // --------------------------------------------------------------------- //
    /// A multiplier for the effect of this light on the diffuse
    /// response of materials.  This is a non-physical control.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:diffuse = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetDiffuseAttr() const;

    /// See GetDiffuseAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateDiffuseAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SPECULAR 
    // --------------------------------------------------------------------- //
    /// A multiplier for the effect of this light on the specular
    /// response of materials.  This is a non-physical control.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:specular = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetSpecularAttr() const;

    /// See GetSpecularAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateSpecularAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMALIZE 
    // --------------------------------------------------------------------- //
    /// Normalizes power by the surface area of the light.
    /// This makes it easier to independently adjust the power and shape
    /// of the light, by causing the power to not vary with the area or
    /// angular size of the light.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool inputs:normalize = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDLUX_API
    UsdAttribute GetNormalizeAttr() const;

    /// See GetNormalizeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateNormalizeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLOR 
    // --------------------------------------------------------------------- //
    /// The color of emitted light, in energy-linear terms.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `color3f inputs:color = (1, 1, 1)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Color3f |
    USDLUX_API
    UsdAttribute GetColorAttr() const;

    /// See GetColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ENABLECOLORTEMPERATURE 
    // --------------------------------------------------------------------- //
    /// Enables using colorTemperature.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool inputs:enableColorTemperature = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDLUX_API
    UsdAttribute GetEnableColorTemperatureAttr() const;

    /// See GetEnableColorTemperatureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateEnableColorTemperatureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLORTEMPERATURE 
    // --------------------------------------------------------------------- //
    /// Color temperature, in degrees Kelvin, representing the
    /// white point.  The default is a common white point, D65.  Lower
    /// values are warmer and higher values are cooler.  The valid range
    /// is from 1000 to 10000. Only takes effect when
    /// enableColorTemperature is set to true.  When active, the
    /// computed result multiplies against the color attribute.
    /// See UsdLuxBlackbodyTemperatureAsRgb().
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:colorTemperature = 6500` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetColorTemperatureAttr() const;

    /// See GetColorTemperatureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateColorTemperatureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FILTERS 
    // --------------------------------------------------------------------- //
    /// Relationship to the light filters that apply to this light.
    ///
    USDLUX_API
    UsdRelationship GetFiltersRel() const;

    /// See GetFiltersRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLUX_API
    UsdRelationship CreateFiltersRel() const;

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

    // -------------------------------------------------------------------------
    /// \name Conversion to and from UsdShadeConnectableAPI
    /// 
    /// @{

    /// Constructor that takes a ConnectableAPI object.
    /// Allow implicit conversion of a UsdShadeConnectableAPI to UsdLuxLight
    USDLUX_API
    UsdLuxLight(const UsdShadeConnectableAPI &connectable);

    /// Contructs and returns a UsdShadeConnectableAPI object with this light.
    ///
    /// Note that most tasks can be accomplished without explicitly constructing 
    /// a UsdShadeConnectable API, since connection-related API such as
    /// UsdShadeConnectableAPI::ConnectToSource() are static methods, and 
    /// UsdLuxLight will auto-convert to a UsdShadeConnectableAPI when 
    /// passed to functions that want to act generically on a connectable
    /// UsdShadeConnectableAPI object.
    USDLUX_API
    UsdShadeConnectableAPI ConnectableAPI() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Outputs API
    ///
    /// Outputs represent a typed attribute on a light whose value is computed 
    /// externally. 
    /// 
    /// @{
        
    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace. Outputs on a light cannot be connected, as their 
    /// value is assumed to be computed externally.
    /// 
    USDLUX_API
    UsdShadeOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName);

    /// Return the requested output if it exists.
    /// 
    USDLUX_API
    UsdShadeOutput GetOutput(const TfToken &name) const;

    /// Outputs are represented by attributes in the "outputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDLUX_API
    std::vector<UsdShadeOutput> GetOutputs(bool onlyAuthored=true) const;

    /// @}

    // ------------------------------------------------------------------------- 

    /// \name Inputs API
    ///
    /// Inputs are connectable attribute with a typed value. 
    /// 
    /// Light parameters are encoded as inputs. 
    /// 
    /// @{
        
    /// Create an input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace. Inputs on lights are connectable.
    /// 
    USDLUX_API
    UsdShadeInput CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName);

    /// Return the requested input if it exists.
    /// 
    USDLUX_API
    UsdShadeInput GetInput(const TfToken &name) const;

    /// Inputs are represented by attributes in the "inputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDLUX_API
    std::vector<UsdShadeInput> GetInputs(bool onlyAuthored=true) const;

    /// @}

    /// Computes the base emission (aka radiant flux density, aka energy
    /// per unit area), incorporating the parameters for intensity,
    /// exposure, color, and colorTemperature attributes.
    ///
    /// This "base" emission method exists solely as a reference example
    /// implementation of how to interpret these parameters.  It is
    /// expected that most rendering backends will consume the parameter
    /// values directly rather than call this method.
    ///
    /// The base emission is only one step in the process of sampling
    /// light radiance. It does not incorporate effects from:
    ///
    /// - textural/procedural modifications
    /// - normalization by area
    /// - specular/diffuse multipliers
    ///
    USDLUX_API
    GfVec3f ComputeBaseEmission() const;

    /// Return the UsdCollectionAPI interface used for examining and
    /// modifying the light-linking of this light.  Light-linking
    /// controls which geometry this light illuminates.
    USDLUX_API
    UsdCollectionAPI GetLightLinkCollectionAPI() const;

    /// Return the UsdCollectionAPI interface used for examining and
    /// modifying the shadow-linking of this light.  Shadow-linking
    /// controls which geometry casts shadows from this light.
    USDLUX_API
    UsdCollectionAPI GetShadowLinkCollectionAPI() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
