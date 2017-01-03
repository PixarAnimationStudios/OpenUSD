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
#ifndef USDGEOM_GENERATED_MODEL_H
#define USDGEOM_GENERATED_MODEL_H

#include "pxr/usd/usdGeom/bboxCache.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3f.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;
class UsdGeomConstraintTarget;

#include <vector>
#include <string>

/// \class UsdGeomModelAPI
///
/// UsdGeomModelAPI extends the generic UsdModelAPI schema with geometry
/// specific concepts such as cached extents for the entire model,
/// constraint targets, and geometry-inspired extensions to the payload
/// lofting process.
///
/// As described in GetExtentsHint() below, it is useful to cache extents
/// at the model level.  UsdGeomModelAPI provides schema for computing and storing
/// these cached extents, which can be consumed by UsdGeomBBoxCache to provide
/// fast access to precomputed extents that will be used as the model's bounds
/// (see UsdGeomBBoxCache::UsdGeomBBoxCache() ).
///
/// \todo CreatePayload()
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
    explicit UsdGeomModelAPI(const UsdPrim& prim)
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
    virtual ~UsdGeomModelAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved
    /// (such as primvars created by UsdGeomImageable).
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class declaration with }; and complete the
    // include guard with #endif
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
    bool GetExtentsHint(VtVec3fArray *extents, 
                        const UsdTimeCode &time = UsdTimeCode::Default()) const;

    /// Authors the extentsHint array for this model at the given time.
    /// 
    /// \sa GetExtentsHint()
    ///
    bool SetExtentsHint(VtVec3fArray const &extents, 
                        const UsdTimeCode &time = UsdTimeCode::Default());

    /// Returns the custom 'extentsHint' attribute if it exits.
    UsdAttribute GetExtentsHintAttr();

    /// For the given model, compute the value for the extents hint with the
    /// given \p bboxCache.  \p bboxCache should be setup with the
    /// appropriate time.  After calling this function, the \p bboxCache may
    /// have it's included purposes changed.
    ///
    /// \note \p bboxCache should not be in use by any other thread while
    /// this method is using it in a thread.
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
    UsdGeomConstraintTarget GetConstraintTarget(
        const std::string &constraintName) const;

    /// Creates a new constraint target with the given name, \p constraintName.
    /// 
    /// If the constraint target already exists, then the existing target is 
    /// returned. If it does not exist, a new one is created and returned.
    /// 
    UsdGeomConstraintTarget CreateConstraintTarget(
        const std::string &constraintName) const;

    /// Returns all the constraint targets belonging to the model.
    /// 
    /// Only valid constraint targets in the "constraintTargets" namespace 
    /// are returned by this method.
    /// 
    std::vector<UsdGeomConstraintTarget> GetConstraintTargets() const;

    /// @}
};

#endif
