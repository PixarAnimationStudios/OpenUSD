//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLUX_GENERATED_SHADOWAPI_H
#define USDLUX_GENERATED_SHADOWAPI_H

/// \file usdLux/shadowAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

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
// SHADOWAPI                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdLuxShadowAPI
///
/// Controls to refine a light's shadow behavior.  These are
/// non-physical controls that are valuable for visual lighting work.
///
class UsdLuxShadowAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLuxShadowAPI on UsdPrim \p prim .
    /// Equivalent to UsdLuxShadowAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxShadowAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLuxShadowAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxShadowAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxShadowAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxShadowAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxShadowAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxShadowAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxShadowAPI
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
    USDLUX_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ShadowAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLuxShadowAPI object is returned upon success. 
    /// An invalid (or empty) UsdLuxShadowAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLUX_API
    static UsdLuxShadowAPI 
    Apply(const UsdPrim &prim);

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
    // --------------------------------------------------------------------- //
    // SHADOW:ENABLE 
    // --------------------------------------------------------------------- //
    /// Enables shadows to be cast by this light.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool inputs:shadow:enable = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDLUX_API
    UsdAttribute GetShadowEnableAttr() const;

    /// See GetShadowEnableAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShadowEnableAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHADOW:COLOR 
    // --------------------------------------------------------------------- //
    /// The color of shadows cast by the light.  This is a
    /// non-physical control.  The default is to cast black shadows.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `color3f inputs:shadow:color = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Color3f |
    USDLUX_API
    UsdAttribute GetShadowColorAttr() const;

    /// See GetShadowColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShadowColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHADOW:DISTANCE 
    // --------------------------------------------------------------------- //
    /// The maximum distance shadows are cast. The distance is
    /// measured as the distance between the point on the surface and the 
    /// occluder.
    /// The default value (-1) indicates no limit.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:shadow:distance = -1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetShadowDistanceAttr() const;

    /// See GetShadowDistanceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShadowDistanceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHADOW:FALLOFF 
    // --------------------------------------------------------------------- //
    /// The size of the shadow falloff zone within the shadow max 
    /// distance, which can be used to hide the hard cut-off for shadows seen 
    /// stretching past the max distance. The falloff zone is the area that 
    /// fades from full shadowing at the beginning of the falloff zone to no 
    /// shadowing at the max distance from the occluder. The falloff zone 
    /// distance cannot exceed the shadow max distance. A falloff value equal 
    /// to or less than zero (with -1 as the default) indicates no falloff. 
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:shadow:falloff = -1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetShadowFalloffAttr() const;

    /// See GetShadowFalloffAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShadowFalloffAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHADOW:FALLOFFGAMMA 
    // --------------------------------------------------------------------- //
    /// A gamma (i.e., exponential) control over shadow strength
    /// with linear distance within the falloff zone. This controls the rate
    /// of the falloff.
    /// This requires the use of shadowDistance and shadowFalloff.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float inputs:shadow:falloffGamma = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLUX_API
    UsdAttribute GetShadowFalloffGammaAttr() const;

    /// See GetShadowFalloffGammaAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShadowFalloffGammaAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    /// Allow implicit conversion of UsdShadeConnectableAPI to
    /// UsdLuxShadowAPI.
    USDLUX_API
    UsdLuxShadowAPI(const UsdShadeConnectableAPI &connectable);

    /// Contructs and returns a UsdShadeConnectableAPI object with this shadow
    /// API prim. Note that a valid UsdLuxShadowAPI will only return a valid
    /// UsdShadeConnectableAPI if the its prim's Typed schema type is actually
    /// connectable.
    USDLUX_API
    UsdShadeConnectableAPI ConnectableAPI() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Outputs API
    ///
    /// Outputs represent a typed attribute on a shadow API whose value is 
    /// computed externally. 
    /// 
    /// @{

    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace. Outputs on a shadow API cannot be connected, as their 
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
    /// Shadow API parameters are encoded as inputs. 
    /// 
    /// @{

    /// Create an input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace. Inputs on shadow API are connectable.
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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
