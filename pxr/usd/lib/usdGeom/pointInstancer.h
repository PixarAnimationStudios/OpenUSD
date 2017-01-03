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
#ifndef USDGEOM_GENERATED_POINTINSTANCER_H
#define USDGEOM_GENERATED_POINTINSTANCER_H

/// \file usdGeom/pointInstancer.h

#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// POINTINSTANCER                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdGeomPointInstancer
///
/// Encodes vectorized instancing of multiple, potentially
/// animated, prototypes (object/instance masters), which can be arbitrary
/// prims/subtrees on a UsdStage.
/// 
/// PointInstancer is a "multi instancer", as it allows multiple prototypes
/// to be scattered among its "points".  We use a UsdRelationship
/// \em prototypes to identify and order all of the possible prototypes, by
/// targeting the root prim of each prototype.  The ordering imparted by
/// relationships associates a zero-based integer with each prototype, and
/// it is these integers we use to identify the prototype of each instance,
/// compactly, and allowing prototypes to be swapped out without needing to
/// reauthor all of the per-instance data.
/// 
/// The PointInstancer schema is designed to scale to billions of instances,
/// which motivates the choice to split the per-instance transformation into
/// position, (quaternion) orientation, and scales, rather than a
/// 4x4 matrix per-instance.  In addition to requiring fewer bytes even if
/// all elements are authored (32 bytes vs 64 for a single-precision 4x4
/// matrix), we can also be selective about which attributes need to animate
/// over time, for substantial data reduction in many cases.
/// 
/// Note that PointInstancer is \em not a Gprim, since it is not a graphical
/// primitive by any stretch of the imagination. It \em is, however,
/// Boundable, since we will sometimes want to treat the entire PointInstancer
/// similarly to a procedural, from the perspective of inclusion or framing.
/// 
/// \section UsdGeomPointInstancer_varyingTopo Varying Instance Identity over Time
/// 
/// PointInstancers originating from simulations often have the characteristic
/// that points/instances are "born", move around for some time period, and then
/// die (or leave the area of interest). In such cases, billions of instances
/// may be birthed over time, while at any \em specific time, only a much
/// smaller number are actually alive.  To encode this situation efficiently,
/// the simulator may re-use indices in the instance arrays, when a particle
/// dies, its index will be taken over by a new particle that may be birthed in
/// a much different location.  This presents challenges both for 
/// identity-tracking, and for motion-blur.
/// 
/// We facilitate identity tracking by providing an optional, animatable
/// \em ids attribute, that specifies the 64 bit integer ID of the particle
/// at each index, at each point in time.  If the simulator keeps monotonically
/// increasing a particle-count each time a new particle is birthed, it will
/// serve perfectly as particle \em ids.
/// 
/// We facilitate motion blur for varying-topology particle streams by
/// optionally allowing per-instance \em velocities and \em angularVelocities
/// to be authored.  If instance transforms are requested at a time between
/// samples and either of the velocity attributes is authored, then we will
/// not attempt to interpolate samples of \em positions or \em orientations.
/// If not authored, and the bracketing samples have the same length, then we
/// will interpolate.
/// 
/// \section UsdGeomPointInstancer_transform Computing an Instance Transform
/// 
/// Each instance's transformation is a combination of the SRT affine transform
/// described by its scale, orientation, and position, applied \em after
/// (i.e. less locally) than the transformation computed at the root of the
/// prototype it is instancing.  In other words, to put an instance of a 
/// PointInstancer into the space of the PointInstancer's parent prim:
/// 
/// 1. Apply (most locally) the authored transformation for 
/// <em>prototypes[protoIndices[i]]</em>
/// 2. If *scales* is authored, next apply the scaling matrix from *scales[i]*
/// 3. If *orientations* is authored: **if *angularVelocities* is authored**, 
/// first multiply *orientations[i]* by the unit quaternion derived by scaling 
/// *angularVelocities[i]* by the time differential from the left-bracketing 
/// timeSample for *orientation* to the requested evaluation time *t*, 
/// storing the result in *R*, **else** assign *R* directly from 
/// *orientations[i]*.  Apply the rotation matrix derived from *R*.
/// 4. Apply the translation derived from *positions[i]*. If *velocities* is 
/// authored, apply the translation deriving from *velocities[i]* scaled by 
/// the time differential from the left-bracketing timeSample for *positions* 
/// to the requested evaluation time *t*.
/// 5. Least locally, apply the transformation authored on the PointInstancer 
/// prim itself (or the UsdGeomImageable::ComputeLocalToWorldTransform() of the 
/// PointInstancer to put the instance directly into world space)
/// 
/// 
/// We provide both high and low-level API's for dealing with the
/// transformation as a matrix, both will compute the instance matrices using
/// multiple threads; the low-level API allows the client to cache unvarying
/// inputs so that they need not be read duplicately when computing over
/// time.
/// 
/// 
/// \section UsdGeomPointInstancer_masking Masking Instances: "Deactivating" and Invising
/// 
/// Often a PointInstancer is created "upstream" in a graphics pipeline, and
/// the needs of "downstream" clients necessitate eliminating some of the 
/// instances from further consideration.  Accomplishing this pruning by 
/// re-authoring all of the per-instance attributes is not very attractive,
/// since it may mean destructively editing a large quantity of data.  We
/// therefore provide means of "masking" instances by ID, such that the 
/// instance data is unmolested, but per-instance transform and primvar data
/// can be retrieved with the no-longer-desired instances eliminated from the
/// (smaller) arrays.  PointInstancer allows two independent means of masking
/// instances by ID, each with different features that meet the needs of
/// various clients in a pipeline.  Both pruning features' lists of ID's are
/// combined to produce the mask returned by ComputeMaskAtTime().
/// 
/// \note If a PointInstancer has no authored \em ids attribute, the masking
/// features will still be available, with the integers specifying element
/// position in the \em protoIndices array rather than ID.
/// 
/// \subsection UsdGeomPointInstancer_inactiveIds InactiveIds: List-edited, Unvarying Masking
/// 
/// The first masking feature encodes a list of IDs in a list-editable metadatum
/// called \em inactiveIds, which, although it does not have any similar 
/// impact to stage poopulation as \ref UsdPrim::SetActive() "prim activation",
/// it shares with that feature that its application is uniform over all time.
/// Because it is list-editable, we can \em sparsely add and remove instances
/// from it in many layers.
/// 
/// This sparse application pattern makes \em inactiveIds a good choice when
/// further downstream clients may need to reverse masking decisions made
/// upstream, in a manner that is robust to many kinds of future changes to
/// the upstream data.
/// 
/// See ActivateId(), ActivateIds(), DeactivateId(), DeactivateIds(), 
/// ActivateAllIds()
/// 
/// \subsection UsdGeomPointInstancer_invisibleIds invisibleIds: Animatable Masking
/// 
/// The second masking feature encodes a list of IDs in a time-varying
/// Int64Array-valued UsdAttribute called \em invisibleIds , since it shares
/// with \ref UsdGeomImageable::GetVisibilityAttr() "Imageable visibility"
/// the ability to animate object visibility.
/// 
/// Unlike \em inactiveIds, overriding a set of opinions for \em invisibleIds
/// is not at all straightforward, because one will, in general need to
/// reauthor (in the overriding layer) **all** timeSamples for the attribute
/// just to change one Id's visibility state, so it cannot be authored
/// sparsely.  But it can be a very useful tool for situations like encoding
/// pre-computed camera-frustum culling of geometry when either or both of
/// the instances or the camera is animated.
/// 
/// See VisId(), VisIds(), InvisId(), InvisIds(), VisAllIds()
/// 
/// \section UsdGeomPointInstancer_protoProcessing Processing and Not Processing Prototypes
/// 
/// Any prim in the scenegraph can be targetted as a prototype by the
/// \em prototypes relationship.  We do not, however, provide a specific
/// mechanism for identifying prototypes as geometry that should not be drawn
/// (or processed) in their own, local spaces in the scenegraph.  We
/// encourage organizing all prototypes as children of the PointInstancer
/// prim that consumes them, and pruning "raw" processing and drawing
/// traversals when they encounter a PointInstancer prim; this is what the
/// UsdGeomBBoxCache and UsdImaging engines do.
/// 
/// There \em is a pattern one can deploy for organizing the prototypes
/// such that they will automatically be skipped by basic UsdPrim::GetChildren()
/// or UsdTreeIterator traversals.  Usd prims each have a 
/// \ref Usd_PrimSpecifiers "specifier" of "def", "over", or "class".  The
/// default traversals skip over prims that are "pure overs" or classes.  So
/// to protect prototypes from all generic traversals and processing, place
/// them under a prim that is just an "over".  For example,
/// \code
/// 01 def PxPointInstancer "Crowd_Mid"
/// 02 {
/// 03     rel prototypes = [ </Crowd_Mid/Prototypes/MaleThin_Business>, </Crowd_Mid/Prototypes/MaleTine_Casual> ]
/// 04     
/// 05     over "Prototypes" 
/// 06     {
/// 07          def "MaleThin_Business" (
/// 08              references = [@MaleGroupA/usd/MaleGroupA.usd@</MaleGroupA>]
/// 09              variants = {
/// 10                  string modelingVariant = "Thin"
/// 11                  string costumeVariant = "BusinessAttire"
/// 12              }
/// 13          )
/// 14          { ... }
/// 15          
/// 16          def "MaleThin_Casual"
/// 17          ...
/// 18     }
/// 19 }
/// \endcode
/// 
/// \section UsdGeomPointInstancer_primvars Primvars on PointInstancer
/// 
/// \ref UsdGeomPrimvar "Primvars" authored on a PointInstancer prim should
/// always be applied to each instance with \em constant interpolation at
/// the root of the instance.  When you are authoring primvars on a 
/// PointInstancer, think about it as if you were authoring them on a 
/// point-cloud (e.g. a UsdGeomPoints gprim).  The same 
/// <A HREF="http://renderman.pixar.com/resources/current/rps/appnote.22.html#classSpecifiers">interpolation rules for points</A> apply here, substituting
/// "instance" for "point".
/// 
/// In other words, the (constant) value extracted for each instance
/// from the authored primvar value depends on the authored \em interpolation
/// and \em elementSize of the primvar, as follows:
/// \li <b>constant</b> or <b>uniform</b> : the entire authored value of the
/// primvar should be applied exactly to each instance.
/// \li <b>varying</b>, <b>vertex</b>, or <b>faceVarying</b>: the first
/// \em elementSize elements of the authored primvar array should be assigned to
/// instance zero, the second \em elementSize elements should be assigned to
/// instance one, and so forth.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomPointInstancer : public UsdGeomBoundable
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a UsdGeomPointInstancer on UsdPrim \p prim .
    /// Equivalent to UsdGeomPointInstancer::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomPointInstancer(const UsdPrim& prim=UsdPrim())
        : UsdGeomBoundable(prim)
    {
    }

    /// Construct a UsdGeomPointInstancer on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomPointInstancer(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomPointInstancer(const UsdSchemaBase& schemaObj)
        : UsdGeomBoundable(schemaObj)
    {
    }

    /// Destructor.
    virtual ~UsdGeomPointInstancer();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomPointInstancer holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomPointInstancer(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdGeomPointInstancer
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
    static UsdGeomPointInstancer
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // PROTOINDICES 
    // --------------------------------------------------------------------- //
    /// <b>Required property</b>. Per-instance index into 
    /// \em prototypes relationship that identifies what geometry should be 
    /// drawn for each instance.  <b>Topology attribute</b> - can be animated, 
    /// but at a potential performance impact for streaming.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetProtoIndicesAttr() const;

    /// See GetProtoIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateProtoIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // IDS 
    // --------------------------------------------------------------------- //
    /// Ids are optional; if authored, the ids array should be the same
    /// length as the \em protoIndices array, specifying (at each timeSample if
    /// instance identities are changing) the id of each instance. The
    /// type is signed intentionally, so that clients can encode some
    /// binary state on Id'd instances without adding a separate primvar.
    /// See also \ref UsdGeomPointInstancer_varyingTopo
    ///
    /// \n  C++ Type: VtArray<long>
    /// \n  Usd Type: SdfValueTypeNames->Int64Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetIdsAttr() const;

    /// See GetIdsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateIdsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POSITIONS 
    // --------------------------------------------------------------------- //
    /// <b>Required property</b>. Per-instance position.  See also 
    /// \ref UsdGeomPointInstancer_transform .
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Point3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetPositionsAttr() const;

    /// See GetPositionsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreatePositionsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ORIENTATIONS 
    // --------------------------------------------------------------------- //
    /// If authored, per-instance orientation of each instance about its 
    /// prototype's origin, represented as a unit length quaternion, which
    /// allows us to encode it with sufficient precision in a compact GfQuath.
    /// 
    /// It is client's responsibility to ensure that authored quaternions are
    /// unit length; the convenience API below for authoring orientations from
    /// rotation matrices will ensure that quaternions are unit length, though
    /// it will not make any attempt to select the "better (for interpolation
    /// with respect to neighboring samples)" of the two possible quaternions
    /// that encode the rotation. 
    /// 
    /// See also \ref UsdGeomPointInstancer_transform .
    ///
    /// \n  C++ Type: VtArray<GfQuath>
    /// \n  Usd Type: SdfValueTypeNames->QuathArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetOrientationsAttr() const;

    /// See GetOrientationsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateOrientationsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALES 
    // --------------------------------------------------------------------- //
    /// If authored, per-instance scale to be applied to 
    /// each instance, before any rotation is applied.
    /// 
    /// See also \ref UsdGeomPointInstancer_transform .
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Float3Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetScalesAttr() const;

    /// See GetScalesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateScalesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VELOCITIES 
    // --------------------------------------------------------------------- //
    /// If authored, per-instance velocity vector to be used for
    /// interpolating position.  Velocities should be considered mandatory if
    /// both \em protoIndices and \em positions are animated.  Magnitude of
    /// the velocity is measured in position units per UsdTimeCode . To convert to
    /// position units per second, multiply by
    /// UsdStage::GetTimeCodesPerSecond().
    /// 
    /// See also \ref UsdGeomPointInstancer_transform .
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Vector3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetVelocitiesAttr() const;

    /// See GetVelocitiesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateVelocitiesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ANGULARVELOCITIES 
    // --------------------------------------------------------------------- //
    /// If authored, per-instance angular velocity vector to be used for
    /// interoplating orientations.  Angular velocities should be considered
    /// mandatory if both \em protoIndices and \em orientations are animated.
    /// Magnitude of the angular velocity is measured in <b>degrees</b> per
    /// UsdTimeCode . To convert to degrees per second, multiply by
    /// UsdStage::GetTimeCodesPerSecond().
    /// 
    /// See also \ref UsdGeomPointInstancer_transform .
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Vector3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetAngularVelocitiesAttr() const;

    /// See GetAngularVelocitiesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateAngularVelocitiesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INVISIBLEIDS 
    // --------------------------------------------------------------------- //
    /// A list of id's to make invisible at the evaluation time.
    /// See \ref UsdGeomPointInstancer_invisibleIds .
    ///
    /// \n  C++ Type: VtArray<long>
    /// \n  Usd Type: SdfValueTypeNames->Int64Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetInvisibleIdsAttr() const;

    /// See GetInvisibleIdsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateInvisibleIdsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROTOTYPEDRAWMODE 
    // --------------------------------------------------------------------- //
    /// Draw modes that host applications can use as a hint for how each
    /// instance of each prototype should be represented.  This hint should only
    /// affect interactive/preview representations and is optional.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: fullGeom
    /// \n  \ref UsdGeomTokens "Allowed Values": [point, card, fullGeom]
    UsdAttribute GetPrototypeDrawModeAttr() const;

    /// See GetPrototypeDrawModeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreatePrototypeDrawModeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROTOTYPES 
    // --------------------------------------------------------------------- //
    /// <b>Required property</b>. Orders and targets the prototype root 
    /// prims, which can be located anywhere in the scenegraph that is convenient,
    /// although we promote organizing prototypes as children of the 
    /// PointInstancer.  The position of a prototype in this relationship defines
    /// the value an instance would specify in the \em protoIndices attribute to 
    /// instance that prototype. Since relationships are uniform, this property
    /// cannot be animated.
    ///
    UsdRelationship GetPrototypesRel() const;

    /// See GetPrototypesRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    UsdRelationship CreatePrototypesRel() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
    
    // --------------------------------------------------------------------- //
    /// \name Id-based Instance Masking/Pruning
    /// See \ref UsdGeomPointInstancer_masking
    /// @{
    // --------------------------------------------------------------------- //
 
    /// Ensure that the instance identified by \p id is active over all time.
    /// This activation is encoded sparsely, affecting no other instances.
    ///
    /// This does not guarantee that the instance will be rendered, because
    /// it may still be "invisible" due to \p id being present in the 
    /// \em invisibleIds attribute (see VisId(), InvisId())
    bool ActivateId(int64_t id) const;

    /// Ensure that the instances identified by \p ids are active over all time.
    /// This activation is encoded sparsely, affecting no other instances.
    ///
    /// This does not guarantee that the instances will be rendered, because
    /// each may still be "invisible" due to its presence in the 
    /// \em invisibleIds attribute (see VisId(), InvisId())
    bool ActivateIds(VtInt64Array const &ids) const;

    /// Ensure that all instances are active over all time.
    ///
    /// This does not guarantee that the instances will be rendered, because
    /// each may still be "invisible" due to its presence in the 
    /// \em invisibleIds attribute (see VisId(), InvisId())
    bool ActivateAllIds() const;

    /// Ensure that the instance identified by \p id is inactive over all time.
    /// This deactivation is encoded sparsely, affecting no other instances.
    ///
    /// A deactivated instance is guaranteed not to render if the renderer
    /// honors masking.
    bool DeactivateId(int64_t id) const;

    /// Ensure that the instances identified by \p ids are inactive over all time.
    /// This deactivation is encoded sparsely, affecting no other instances.
    ///
    /// A deactivated instance is guaranteed not to render if the renderer
    /// honors masking.
    bool DeactivateIds(VtInt64Array const &ids) const;


    /// Ensure that the instance identified by \p id is visible at \p time.
    /// This will cause \em invisibleIds to first be broken down (keyed)
    /// at \p time, causing all animation in weaker layers that the current
    /// UsdEditTarget to be overridden.  Has no effect on any timeSamples other
    /// than the one at \p time.  If the \em invisibleIds attribute is not 
    /// authored or is blocked, this operation is a no-op.
    ///
    /// This does not guarantee that the instance will be rendered, because
    /// it may still be "inactive" due to \p id being present in the 
    /// \em inactivevIds metadata (see ActivateId(), DeactivateId())
    bool VisId(int64_t id, UsdTimeCode const &time) const;

    /// Ensure that the instances identified by \p ids are visible at \p time.
    /// This will cause \em invisibleIds to first be broken down (keyed)
    /// at \p time, causing all animation in weaker layers that the current
    /// UsdEditTarget to be overridden.  Has no effect on any timeSamples other
    /// than the one at \p time.  If the \em invisibleIds attribute is not 
    /// authored or is blocked, this operation is a no-op.
    ///
    /// This does not guarantee that the instances will be rendered, because
    /// each may still be "inactive" due to \p id being present in the 
    /// \em inactivevIds metadata (see ActivateId(), DeactivateId())
    bool VisIds(VtInt64Array const &ids, UsdTimeCode const &time) const;

    /// Ensure that all instances are visible at \p time.
    /// Operates by authoring an empty array at \p time.
    ///
    /// This does not guarantee that the instances will be rendered, because
    /// each may still be "inactive" due to its id being present in the 
    /// \em inactivevIds metadata (see ActivateId(), DeactivateId())
    bool VisAllIds(UsdTimeCode const &time) const;

    /// Ensure that the instance identified by \p id is invisible at \p time.
    /// This will cause \em invisibleIds to first be broken down (keyed)
    /// at \p time, causing all animation in weaker layers that the current
    /// UsdEditTarget to be overridden.  Has no effect on any timeSamples other
    /// than the one at \p time.
    ///
    /// An invised instance is guaranteed not to render if the renderer
    /// honors masking.
    bool InvisId(int64_t id, UsdTimeCode const &time) const;

    /// Ensure that the instances identified by \p ids are invisible at \p time.
    /// This will cause \em invisibleIds to first be broken down (keyed)
    /// at \p time, causing all animation in weaker layers that the current
    /// UsdEditTarget to be overridden.  Has no effect on any timeSamples other
    /// than the one at \p time.
    ///
    /// An invised instance is guaranteed not to render if the renderer
    /// honors masking.
    bool InvisIds(VtInt64Array const &ids, UsdTimeCode const &time) const;

    /// Computes a presence mask to be applied to per-instance data arrays
    /// based on authored \em inactiveIds, \em invisibleIds, and \em ids .
    ///
    /// If no \em ids attribute has been authored, then the values in
    /// \em inactiveIds and \em invisibleIds will be interpreted directly
    /// as indices of \em protoIndices .
    ///
    /// If \p ids is non-NULL, it is assumed to be the id-mapping to apply,
    /// and must match the length of \em protoIndices at \p time . 
    /// If NULL, we will call GetIdsAttr().Get(time)
    ///
    /// \note If all "live" isntances at timeCode \p time pass the mask,
    /// we will return an <b>empty</b> mask so that clients can trivially
    /// recognize the common "no masking" case.
    ///
    /// The returned mask can be used with ApplyMaskToArray(), and will contain
    /// a \c true value for every element that should survive.
    std::vector<bool> ComputeMaskAtTime(UsdTimeCode time, 
                                        VtInt64Array const *ids = nullptr) const;

    /// Contract \p dataArray in-place to contain only the elements whose
    /// index in \p mask is \c true.
    ///
    /// \note an empty \p mask specifies "all pass", in which case \p dataArray
    /// is trivially unmodified
    ///
    ///  - It is an error for \p dataArray to be NULL . 
    ///  - If \em elementSize times \em mask.size() does not equal 
    ///  \em dataArray->size(), warn and fail.
    ///
    /// \return true on success, false on failure.
    /// \sa ComputeMaskAtTime()
    template <class T>
    static bool ApplyMaskToArray(std::vector<bool> const &mask,
                          VtArray<T> *dataArray,
                          const int elementSize = 1);
    
    // --------------------------------------------------------------------- //
    /// @}
    // --------------------------------------------------------------------- //
 

};

template <class T>
bool 
UsdGeomPointInstancer::ApplyMaskToArray(std::vector<bool> const &mask,
                                        VtArray<T> *dataArray,
                                        const int elementSize)
{
    if (!dataArray){
        TF_CODING_ERROR("NULL dataArray.");
        return false;
    }
    size_t size = mask.size();
    if (size == 0 || dataArray->size() == elementSize){
        return true;
    }
    else if ((size * elementSize) != dataArray->size()){
        TF_WARNING("Input mask's size (%d) is not compatible with the "
                   "input dataArray (%d) and elementSize (%d).",
                   size, dataArray->size(), elementSize);
        return false;
    }
    
    T *beginData = dataArray->GetData();
    T *currData = beginData;
    size_t numPreserved = 0;
    for (size_t i=0; i<size; ++i){
        // XXX Could add a fast-path for elementSize == 1 ?
        if (mask[i]){
            for (size_t j=0; j<elementSize; ++j){
                *currData = beginData[i] + j;
                ++currData;
            }
            numPreserved += elementSize;
        }
    }
    if (numPreserved < dataArray->size()){
        dataArray->resize(numPreserved);
    }
    return true;
}

#endif
