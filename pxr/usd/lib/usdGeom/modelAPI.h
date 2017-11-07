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
#ifndef USDGEOM_GENERATED_MODELAPI_H
#define USDGEOM_GENERATED_MODELAPI_H

/// \file usdGeom/modelAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/constraintTarget.h"
#include "pxr/usd/usdGeom/imageable.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// GEOMMODELAPI                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdGeomModelAPI
///
/// UsdGeomModelAPI extends the generic UsdModelAPI schema with
/// geometry specific concepts such as cached extents for the entire model,
/// constraint targets, and geometry-inspired extensions to the payload
/// lofting process.
/// 
/// As described in GetExtentsHint() below, it is useful to cache extents
/// at the model level.  UsdGeomModelAPI provides schema for computing and
/// storing these cached extents, which can be consumed by UsdGeomBBoxCache to
/// provide fast access to precomputed extents that will be used as the model's
/// bounds ( see UsdGeomBBoxCache::UsdGeomBBoxCache() ).
/// 
/// \section UsdGeomModelAPI_drawMode Draw Modes
/// 
/// Draw modes provide optional alternate imaging behavior for USD subtrees with
/// kind model. \em model:drawMode (which is inheritable) and
/// \em model:applyDrawMode (which is not) are resolved into a decision to stop
/// traversing the scene graph at a certain point, and replace a USD subtree
/// with proxy geometry.
/// 
/// The value of \em model:drawMode determines the type of proxy geometry:
/// - \em origin - Draw the model-space basis vectors of the replaced prim.
/// - \em bounds - Draw the model-space bounding box of the replaced prim.
/// - \em cards - Draw textured quads as a placeholder for the replaced prim.
/// - \em default - An explicit opinion to draw the USD subtree as normal.
/// 
/// \em model:drawMode is inheritable so that a whole scene, a large group, or
/// all prototypes of a model hierarchy PointInstancer can be assigned a draw
/// mode with a single attribute edit.  \em model:applyDrawMode is meant to be
/// authored earlier, and provides flexibility for different asset types. For
/// example, a character assembly (composed of character, clothes, etc) might
/// have \em model:applyDrawMode set at the top of the subtree so the whole
/// group can be drawn as a single card object. An effects subtree might have
/// \em model:applyDrawMode set at the component level so each particle group
/// draws individually in bounding box mode.
/// 
/// A value for \em model:drawMode on a child prim takes precedence over a
/// value on a parent prim.
/// 
/// \section UsdGeomModelAPI_cardGeometry Cards Geometry
/// 
/// The specific geometry used in cards mode is controlled by the
/// \em model:cardGeometry attribute:
/// - \em cross - Generate a quad normal to each basis direction and negative.
/// Locate each quad so that it bisects the model extents.
/// - \em box   - Generate a quad normal to each basis direction and negative.
/// Locate each quad on a face of the model extents, facing out.
/// - \em fromTexture - Generate a quad for each supplied texture by
/// transforming the range from (-1,-1) to (1,1) by the
/// inverse of the \em worldToScreenPos transform, pulled
/// from texture metadata. The resulting positions are
/// interpreted as being in model space.
/// 
/// For \em cross and \em box mode, the extents are calculated for purposes
/// \em default, \em proxy, and \em render, at their earliest authored time.
/// If the model has no textures, all six card faces are rendered using
/// \em model:drawModeColor. If one or more textures are present, only axes
/// with one or more textures assigned are drawn.  For each axis, if both
/// textures (positive and negative) are specified, they'll be used on the
/// corresponding card faces; if only one texture is specified, it will be
/// mapped to the opposite card face after being flipped on the texture's
/// s-axis. Any card faces with invalid asset paths will be drawn with
/// \em model:drawModeColor.
/// 
/// For \em fromTexture mode, only card faces with valid textures assigned
/// are drawn.
/// 
/// All card faces are drawn and textured as double-sided.
/// 
/// \todo CreatePayload() 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomModelAPI : public UsdModelAPI
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdGeomModelAPI on UsdPrim \p prim .
    /// Equivalent to UsdGeomModelAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomModelAPI(const UsdPrim& prim=UsdPrim())
        : UsdModelAPI(prim)
    {
    }

    /// Construct a UsdGeomModelAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomModelAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomModelAPI(const UsdSchemaBase& schemaObj)
        : UsdModelAPI(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomModelAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomModelAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomModelAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomModelAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


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
    // MODELDRAWMODE 
    // --------------------------------------------------------------------- //
    /// Alternate imaging mode; applied to this prim or child prims
    /// where \em model:applyDrawMode is true, or where the prim
    /// has kind \em component. See \ref UsdGeomModelAPI_drawMode
    /// for mode descriptions.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    /// \n  \ref UsdGeomTokens "Allowed Values": [origin, bounds, cards, default]
    USDGEOM_API
    UsdAttribute GetModelDrawModeAttr() const;

    /// See GetModelDrawModeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelDrawModeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELAPPLYDRAWMODE 
    // --------------------------------------------------------------------- //
    /// If true, and this prim or parent prims have \em model:drawMode
    /// set, apply an alternate imaging mode to this prim. See
    /// \ref UsdGeomModelAPI_drawMode.
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelApplyDrawModeAttr() const;

    /// See GetModelApplyDrawModeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelApplyDrawModeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELDRAWMODECOLOR 
    // --------------------------------------------------------------------- //
    /// The base color of imaging prims inserted for alternate
    /// imaging modes. For \em origin and \em bounds modes, this
    /// controls line color; for \em cards mode, this controls the
    /// fallback quad color. If unspecified, it should be interpreted
    /// as (0.18, 0.18, 0.18).
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Float3
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelDrawModeColorAttr() const;

    /// See GetModelDrawModeColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelDrawModeColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELCARDGEOMETRY 
    // --------------------------------------------------------------------- //
    /// The geometry to generate for imaging prims inserted for \em
    /// cards imaging mode. See \ref UsdGeomModelAPI_cardGeometry for
    /// geometry descriptions. If unspecified, it should be interpreted
    /// as \em cross.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    /// \n  \ref UsdGeomTokens "Allowed Values": [cross, box, fromTexture]
    USDGEOM_API
    UsdAttribute GetModelCardGeometryAttr() const;

    /// See GetModelCardGeometryAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelCardGeometryAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELCARDTEXTUREXPOS 
    // --------------------------------------------------------------------- //
    /// In \em cards imaging mode, the texture applied to the X+ quad.
    /// The texture axes (s,t) are mapped to model-space axes (-y, -z).
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelCardTextureXPosAttr() const;

    /// See GetModelCardTextureXPosAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelCardTextureXPosAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELCARDTEXTUREYPOS 
    // --------------------------------------------------------------------- //
    /// In \em cards imaging mode, the texture applied to the Y+ quad.
    /// The texture axes (s,t) are mapped to model-space axes (x, -z).
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelCardTextureYPosAttr() const;

    /// See GetModelCardTextureYPosAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelCardTextureYPosAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELCARDTEXTUREZPOS 
    // --------------------------------------------------------------------- //
    /// In \em cards imaging mode, the texture applied to the Z+ quad.
    /// The texture axes (s,t) are mapped to model-space axes (x, -y).
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelCardTextureZPosAttr() const;

    /// See GetModelCardTextureZPosAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelCardTextureZPosAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELCARDTEXTUREXNEG 
    // --------------------------------------------------------------------- //
    /// In \em cards imaging mode, the texture applied to the X- quad.
    /// The texture axes (s,t) are mapped to model-space axes (y, -z).
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelCardTextureXNegAttr() const;

    /// See GetModelCardTextureXNegAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelCardTextureXNegAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELCARDTEXTUREYNEG 
    // --------------------------------------------------------------------- //
    /// In \em cards imaging mode, the texture applied to the Y- quad.
    /// The texture axes (s,t) are mapped to model-space axes (-x, -z).
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelCardTextureYNegAttr() const;

    /// See GetModelCardTextureYNegAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelCardTextureYNegAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MODELCARDTEXTUREZNEG 
    // --------------------------------------------------------------------- //
    /// In \em cards imaging mode, the texture applied to the Z- quad.
    /// The texture axes (s,t) are mapped to model-space axes (-x, -y).
    ///
    /// \n  C++ Type: SdfAssetPath
    /// \n  Usd Type: SdfValueTypeNames->Asset
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetModelCardTextureZNegAttr() const;

    /// See GetModelCardTextureZNegAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateModelCardTextureZNegAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// \anchor  UsdGeomModelAPIExtentsHint
    /// \name Model Extents Hint API
    /// 
    /// Methods for querying, authoring and computing the model's 
    /// "extentsHint".
    /// @{

    /// Retrieve the authored value (if any) of this model's "extentsHint"
    ///
    /// Persistent caching of bounds in USD is a potentially perilous endeavor,
    /// given that:
    /// \li It is very easy to add overrides in new super-layers that 
    /// invalidate the cached bounds, and no practical way to automatically
    /// detect when this happens
    /// \li It is possible for references to be allowed to "float", so that
    /// asset updates can flow directly into cached scenes.  Such changes in
    /// referenced scene description can also invalidate cached bounds in
    /// referencing layers.
    ///
    /// For these reasons, as a general rule, we only persistently cache
    /// leaf gprim extents in object space.  However, even with cached gprim
    /// extents, computing bounds can be expensive.  Since model-level bounds
    /// are so useful to many graphics applications, we make an exception,
    /// with some caveats. The "extentsHint" should be considered entirely
    /// optional (whereas gprim extent is not); if authored, it should 
    /// contains the extents for various values of gprim purposes.
    /// The extents for different values of purpose are stored in a linear Vec3f 
    /// array as pairs of GfVec3f values in the order specified by 
    /// UsdGeomImageable::GetOrderedPurposeTokens(). This list is trimmed to
    /// only include non-empty extents. i.e., if a model has only default and 
    /// render geoms, then it will only have 4 GfVec3f values in its 
    /// extentsHint array.  We do not skip over zero extents, so if a model
    /// has only default and proxy geom, we will author six GfVec3f's, the 
    /// middle two representing an zero extent for render geometry.
    ///
    /// A UsdGeomBBoxCache can be configured to first consult the cached
    /// extents when evaluating model roots, rather than descending into the
    /// models for the full computation.  This is not the default behavior,
    /// and gives us a convenient way to validate that the cached 
    /// extentsHint is still valid.
    ///
    /// \return \c true if a value was fetched; \c false if no value was
    /// authored, or on error.  It is an error to make this query of a prim
    /// that is not a model root.
    /// 
    /// \sa UsdGeomImageable::GetPurposeAttr(), 
    ///     UsdGeomImageable::GetOrderedPurposeTokens()
    ///
    USDGEOM_API
    bool GetExtentsHint(VtVec3fArray *extents, 
                        const UsdTimeCode &time = UsdTimeCode::Default()) const;

    /// Authors the extentsHint array for this model at the given time.
    /// 
    /// \sa GetExtentsHint()
    ///
    USDGEOM_API
    bool SetExtentsHint(VtVec3fArray const &extents, 
                        const UsdTimeCode &time = UsdTimeCode::Default());

    /// Returns the custom 'extentsHint' attribute if it exits.
    USDGEOM_API
    UsdAttribute GetExtentsHintAttr();

    /// For the given model, compute the value for the extents hint with the
    /// given \p bboxCache.  \p bboxCache should be setup with the
    /// appropriate time.  After calling this function, the \p bboxCache may
    /// have it's included purposes changed.
    ///
    /// \note \p bboxCache should not be in use by any other thread while
    /// this method is using it in a thread.
    USDGEOM_API
    VtVec3fArray ComputeExtentsHint(UsdGeomBBoxCache& bboxCache) const;

    /// @}

    /// \anchor  UsdGeomModelAPIConstraintTargets
    /// \name Model Constraint Targets API
    /// 
    /// Methods for adding and listing constraint targets.
    /// 
    /// @{
        
    /// Get the constraint target with the given name, \p constraintName.
    /// 
    /// If the requested constraint target does not exist, then an invalid 
    /// UsdConstraintTarget object is returned.
    /// 
    USDGEOM_API
    UsdGeomConstraintTarget GetConstraintTarget(
        const std::string &constraintName) const;

    /// Creates a new constraint target with the given name, \p constraintName.
    /// 
    /// If the constraint target already exists, then the existing target is 
    /// returned. If it does not exist, a new one is created and returned.
    /// 
    USDGEOM_API
    UsdGeomConstraintTarget CreateConstraintTarget(
        const std::string &constraintName) const;

    /// Returns all the constraint targets belonging to the model.
    /// 
    /// Only valid constraint targets in the "constraintTargets" namespace 
    /// are returned by this method.
    /// 
    USDGEOM_API
    std::vector<UsdGeomConstraintTarget> GetConstraintTargets() const;

    /// @}

    /// Calculate the effective model:drawMode of this prim, as defined by
    /// its closest ancestral authored opinion, if any.
    ///
    /// If no opinion for model:drawMode is authored on this prim or any of its
    /// ancestors, its computed model:drawMode is UsdGeomTokens->default_ .
    /// Otherwise, its computed model:drawMode is that of its closest ancestor
    /// with an authored model:drawMode.
    ///
    /// This function should be considered a reference implementation for
    /// correctness. <b>If called on each prim in the context of a traversal
    /// we will perform massive overcomputation, because sibling prims share
    /// sub-problems in the query that can be efficiently cached, but are not
    /// (cannot be) by this simple implementation.</b> If you have control of
    /// your traversal, it will be far more efficient to manage model:drawMode
    /// on a stack as you traverse.
    ///
    /// \sa GetModelDrawModeAttr()
    USDGEOM_API
    TfToken ComputeModelDrawMode() const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
