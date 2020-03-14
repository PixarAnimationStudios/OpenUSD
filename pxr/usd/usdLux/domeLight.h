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
#ifndef USDLUX_GENERATED_DOMELIGHT_H
#define USDLUX_GENERATED_DOMELIGHT_H

/// \file usdLux/domeLight.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usdLux/light.h"
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
// DOMELIGHT                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdLuxDomeLight
///
/// Light emitted inward from a distant external environment,
/// such as a sky or IBL light probe.  The orientation of a dome light with a
/// latlong texture is expected to match the OpenEXR specification for latlong 
/// environment maps.  From the OpenEXR documentation:
/// 
/// -------------------------------------------------------------------------
/// Latitude-Longitude Map:
/// 
/// The environment is projected onto the image using polar coordinates
/// (latitude and longitude).  A pixel's x coordinate corresponds to
/// its longitude, and the y coordinate corresponds to its latitude.
/// Pixel (dataWindow.min.x, dataWindow.min.y) has latitude +pi/2 and
/// longitude +pi; pixel (dataWindow.max.x, dataWindow.max.y) has
/// latitude -pi/2 and longitude -pi.
/// 
/// In 3D space, latitudes -pi/2 and +pi/2 correspond to the negative and
/// positive y direction.  Latitude 0, longitude 0 points into positive
/// z direction; and latitude 0, longitude pi/2 points into positive x
/// direction.
/// 
/// The size of the data window should be 2*N by N pixels (width by height),
/// where N can be any integer greater than 0.
/// -------------------------------------------------------------------------
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLuxTokens.
/// So to set an attribute to the value "rightHanded", use UsdLuxTokens->rightHanded
/// as the value.
///
class UsdLuxDomeLight : public UsdLuxLight
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::ConcreteTyped;

    /// Construct a UsdLuxDomeLight on UsdPrim \p prim .
    /// Equivalent to UsdLuxDomeLight::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxDomeLight(const UsdPrim& prim=UsdPrim())
        : UsdLuxLight(prim)
    {
    }

    /// Construct a UsdLuxDomeLight on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxDomeLight(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxDomeLight(const UsdSchemaBase& schemaObj)
        : UsdLuxLight(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxDomeLight();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxDomeLight holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxDomeLight(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxDomeLight
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDLUX_API
    static UsdLuxDomeLight
    Define(const UsdStagePtr &stage, const SdfPath &path);

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
    // TEXTUREFILE 
    // --------------------------------------------------------------------- //
    /// A color texture to use on the dome, such as an HDR (high
    /// dynamic range) texture intended for IBL (image based lighting).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `asset texture:file` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    USDLUX_API
    UsdAttribute GetTextureFileAttr() const;

    /// See GetTextureFileAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateTextureFileAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TEXTUREFORMAT 
    // --------------------------------------------------------------------- //
    /// Specifies the parameterization of the color map file.
    /// Valid values are:
    /// - automatic: Tries to determine the layout from the file itself.
    /// For example, Renderman texture files embed an explicit
    /// parameterization.
    /// - latlong: Latitude as X, longitude as Y.
    /// - mirroredBall: An image of the environment reflected in a
    /// sphere, using an implicitly orthogonal projection.
    /// - angular: Similar to mirroredBall but the radial dimension
    /// is mapped linearly to the angle, providing better sampling
    /// at the edges.
    /// - cubeMapVerticalCross: A cube map with faces laid out as a
    /// vertical cross.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token texture:format = "automatic"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdLuxTokens "Allowed Values" | automatic, latlong, mirroredBall, angular, cubeMapVerticalCross |
    USDLUX_API
    UsdAttribute GetTextureFormatAttr() const;

    /// See GetTextureFormatAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateTextureFormatAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PORTALS 
    // --------------------------------------------------------------------- //
    /// Optional portals to guide light sampling.
    ///
    USDLUX_API
    UsdRelationship GetPortalsRel() const;

    /// See GetPortalsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLUX_API
    UsdRelationship CreatePortalsRel() const;

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

    /// Adds a transformation op, if neeeded, to orient the dome to align with
    /// the stage's up axis.  Uses UsdLuxTokens->orientToStageUpAxis as the op
    /// suffix.  If an op with this suffix already exists, this method assumes
    /// it is already applying the proper correction and does nothing further.
    /// If no op is required to match the stage's up axis, no op will be
    /// created.
    ///
    /// \see UsdGeomXformOp
    /// \see UsdGeomGetStageUpAxis
    USDLUX_API
    void OrientToStageUpAxis() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
