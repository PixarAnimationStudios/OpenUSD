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
#ifndef USDSKEL_GENERATED_BINDINGAPI_H
#define USDSKEL_GENERATED_BINDINGAPI_H

/// \file usdSkel/bindingAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdSkel/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// BINDINGAPI                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdSkelBindingAPI
///
/// Provides API for authoring and extracting all the skinning-related
/// data that lives in the "geometry hierarchy" of prims and models that want
/// to be skeletally deformed.
/// 
/// This includes binding to both skeletons and animations that drive the 
/// skeleton's joints, as well as describing the mapping and weighting of
/// joints to gprims and trees of geometry, and of gprims to the primary
/// bound Skeleton.
///
class UsdSkelBindingAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = false;

    /// Construct a UsdSkelBindingAPI on UsdPrim \p prim .
    /// Equivalent to UsdSkelBindingAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdSkelBindingAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdSkelBindingAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdSkelBindingAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdSkelBindingAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDSKEL_API
    virtual ~UsdSkelBindingAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSKEL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdSkelBindingAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdSkelBindingAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSKEL_API
    static UsdSkelBindingAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Mark this schema class as applied to the prim at \p path in the 
    /// current EditTarget. This information is stored in the apiSchemas
    /// metadata on prims.  
    ///
    /// \sa UsdPrim::GetAppliedSchemas()
    ///
    USDSKEL_API
    static UsdSkelBindingAPI 
    Apply(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSKEL_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSKEL_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // GEOMBINDTRANSFORM 
    // --------------------------------------------------------------------- //
    /// Encodes the transform that positions gprims in the space in
    /// which it is bound to a Skeleton.  If the transform is identical for a
    /// group of gprims that share a common ancestor, the transform may be
    /// authored on the ancestor, to "inherit" down to all the leaf gprims.
    /// The *geomBindTransform* is defined as moving a gprim from its own
    /// object space (untransformed by the gprim's own transform) out into
    /// Skeleton space.
    ///
    /// \n  C++ Type: GfMatrix4d
    /// \n  Usd Type: SdfValueTypeNames->Matrix4d
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetGeomBindTransformAttr() const;

    /// See GetGeomBindTransformAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateGeomBindTransformAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // JOINTINDICES 
    // --------------------------------------------------------------------- //
    /// Indices into the *joints* relationship of the closest
    /// (in namespace) bound Skeleton that affect each point of a PointBased
    /// gprim.  The primvar can be either *constant* or *vertex* interpolation.
    /// In either case, this primvar's *elementSize* will determine how many
    /// joints apply to each vertex.  See UsdGeomPrimvar for more information
    /// on interpolation and elementSize.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetJointIndicesAttr() const;

    /// See GetJointIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateJointIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // JOINTWEIGHTS 
    // --------------------------------------------------------------------- //
    /// Weights for the  joints that affect each point of a PointBased
    /// gprim.  The primvar can be either *constant* or *vertex* interpolation.
    /// In either case, this primvar's *elementSize* will determine how many
    /// joints apply to each vertex.  The length, interpolation, and 
    /// elementSize of *jointWeights* must match that of *jointIndices*.
    /// See UsdGeomPrimvar for more information on interpolation and 
    /// elementSize.
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetJointWeightsAttr() const;

    /// See GetJointWeightsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateJointWeightsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ANIMATIONSOURCE 
    // --------------------------------------------------------------------- //
    /// Animation source to be bound to this prim and its 
    /// descendants.
    ///
    USDSKEL_API
    UsdRelationship GetAnimationSourceRel() const;

    /// See GetAnimationSourceRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSKEL_API
    UsdRelationship CreateAnimationSourceRel() const;

public:
    // --------------------------------------------------------------------- //
    // SKELETON 
    // --------------------------------------------------------------------- //
    /// Skeleton to be bound to this prim and its descendents that
    /// possess a mapping and weighting to the joints of the identified
    /// Skeleton.
    ///
    USDSKEL_API
    UsdRelationship GetSkeletonRel() const;

    /// See GetSkeletonRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSKEL_API
    UsdRelationship CreateSkeletonRel() const;

public:
    // --------------------------------------------------------------------- //
    // JOINTS 
    // --------------------------------------------------------------------- //
    /// An (optional) relationship whose targets define the list of
    /// joints to which jointIndices apply, relative to the gprim itself, so
    /// that it is self-contained. If not defined, jointIndices applies to
    /// the ordered list of joints defined in the bound Skeleton's *joints*
    /// relationship.
    ///
    USDSKEL_API
    UsdRelationship GetJointsRel() const;

    /// See GetJointsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSKEL_API
    UsdRelationship CreateJointsRel() const;

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
