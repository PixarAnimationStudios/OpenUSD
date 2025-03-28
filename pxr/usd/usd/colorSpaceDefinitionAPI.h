//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USD_GENERATED_COLORSPACEDEFINITIONAPI_H
#define USD_GENERATED_COLORSPACEDEFINITIONAPI_H

/// \file usd/colorSpaceDefinitionAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/base/gf/colorSpace.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// COLORSPACEDEFINITIONAPI                                                    //
// -------------------------------------------------------------------------- //

/// \class UsdColorSpaceDefinitionAPI
///
/// UsdColorSpaceDefinitionAPI is an API schema for defining a custom
/// color space. Custom color spaces become available for use on prims or for
/// assignment to attributes via the colorSpace:name property on prims that have
/// applied `UsdColorSpaceAPI`. Since color spaces inherit hierarchically, a
/// custom color space defined on a prim will be available to all descendants of
/// that prim, unless overridden by a more local color space definition bearing
/// the same name. Locally redefining color spaces within the same layer could
/// be confusing, so that practice is discouraged.
/// 
/// The default color space values are equivalent to an identity transform, so
/// applying this schema and invoking `UsdColorSpaceAPI::ComputeColorSpace()`
/// on a prim resolving to a defaulted color definition will return a color
/// space equivalent to the identity transform.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdTokens.
/// So to set an attribute to the value "rightHanded", use UsdTokens->rightHanded
/// as the value.
///
class UsdColorSpaceDefinitionAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdColorSpaceDefinitionAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdColorSpaceDefinitionAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "colorSpaceDefinition:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdColorSpaceDefinitionAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdColorSpaceDefinitionAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdColorSpaceDefinitionAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdColorSpaceDefinitionAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USD_API
    virtual ~UsdColorSpaceDefinitionAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USD_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdColorSpaceDefinitionAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.colorSpaceDefinition:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdColorSpaceDefinitionAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USD_API
    static UsdColorSpaceDefinitionAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdColorSpaceDefinitionAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdColorSpaceDefinitionAPI(prim, name);
    USD_API
    static UsdColorSpaceDefinitionAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdColorSpaceDefinitionAPI on the 
    /// given \p prim.
    USD_API
    static std::vector<UsdColorSpaceDefinitionAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of ColorSpaceDefinitionAPI.
    USD_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// ColorSpaceDefinitionAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USD_API
    static bool
    IsColorSpaceDefinitionAPIPath(const SdfPath &path, TfToken *name);

    /// Returns true if this <b>multiple-apply</b> API schema can be applied,
    /// with the given instance name, \p name, to the given \p prim. If this 
    /// schema can not be a applied the prim, this returns false and, if 
    /// provided, populates \p whyNot with the reason it can not be applied.
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
    USD_API
    static bool 
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "ColorSpaceDefinitionAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'ColorSpaceDefinitionAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdColorSpaceDefinitionAPI object is returned upon success. 
    /// An invalid (or empty) UsdColorSpaceDefinitionAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USD_API
    static UsdColorSpaceDefinitionAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USD_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // NAME 
    // --------------------------------------------------------------------- //
    /// The name of the color space defined on this prim.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token name = "custom"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USD_API
    UsdAttribute GetNameAttr() const;

    /// See GetNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // REDCHROMA 
    // --------------------------------------------------------------------- //
    /// Red chromaticity coordinates
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float2 redChroma = (1, 0)` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    USD_API
    UsdAttribute GetRedChromaAttr() const;

    /// See GetRedChromaAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateRedChromaAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // GREENCHROMA 
    // --------------------------------------------------------------------- //
    /// Green chromaticity coordinates
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float2 greenChroma = (0, 1)` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    USD_API
    UsdAttribute GetGreenChromaAttr() const;

    /// See GetGreenChromaAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateGreenChromaAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BLUECHROMA 
    // --------------------------------------------------------------------- //
    /// Blue chromaticity coordinates
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float2 blueChroma = (0, 0)` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    USD_API
    UsdAttribute GetBlueChromaAttr() const;

    /// See GetBlueChromaAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateBlueChromaAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // WHITEPOINT 
    // --------------------------------------------------------------------- //
    /// Whitepoint chromaticity coordinates
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float2 whitePoint = (0.33333334, 0.33333334)` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    USD_API
    UsdAttribute GetWhitePointAttr() const;

    /// See GetWhitePointAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateWhitePointAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // GAMMA 
    // --------------------------------------------------------------------- //
    /// Gamma value of the log section
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float gamma = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USD_API
    UsdAttribute GetGammaAttr() const;

    /// See GetGammaAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateGammaAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LINEARBIAS 
    // --------------------------------------------------------------------- //
    /// Linear bias of the log section
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float linearBias = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USD_API
    UsdAttribute GetLinearBiasAttr() const;

    /// See GetLinearBiasAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateLinearBiasAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    
    /// Creates the color space attributes with the given values.
    /// @param redChroma The red chromaticity coordinates.
    /// @param greenChroma The green chromaticity coordinates.
    /// @param blueChroma The blue chromaticity coordinates.
    /// @param whitePoint The whitepoint chromaticity coordinates.
    /// @param gamma The gamma value of the log section.
    /// @param linearBias The linear bias of the log section.
    USD_API
    void CreateColorSpaceAttrsWithChroma(const GfVec2f& redChroma,
                                         const GfVec2f& greenChroma,
                                         const GfVec2f& blueChroma,
                                         const GfVec2f& whitePoint,
                                         float gamma, float linearBias);

    /// Create the color space attributes by deriving the color space from the
    /// given RGB to XYZ matrix and linearization parameters.
    /// @param rgbToXYZ The RGB to XYZ matrix.
    /// @param gamma The gamma value of the log section.
    /// @param linearBias The linear bias of the log section.
    USD_API
    void CreateColorSpaceAttrsWithMatrix(const GfMatrix3f& rgbToXYZ,
                                         float gamma, float linearBias);

    /// Create a `GfColorSpace` object from the color space definition attributes.
    USD_API
    GfColorSpace ComputeColorSpaceFromDefinitionAttributes() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
