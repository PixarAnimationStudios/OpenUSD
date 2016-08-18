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
#ifndef USDSHADE_GENERATED_LOOK_H
#define USDSHADE_GENERATED_LOOK_H

/// \file usdShade/look.h

#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usdShade/subgraph.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/faceSetAPI.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LOOK                                                                       //
// -------------------------------------------------------------------------- //

/// \class UsdShadeLook
///
/// \deprecated Deprecated in favor of Material.
/// 
/// A Look provides a container into which multiple "render targets"
/// can add data that defines a "shading look" for a renderer.  Typically
/// this consists of one or more UsdRelationship properties that target
/// other prims of type \em Shader - though a target/client is free to add
/// any data that is suitable.  We <b>strongly advise</b> that all targets
/// adopt the convention that all properties be prefixed with a namespace
/// that identifies the target, e.g. "rel ri:surface = </Shaders/mySurf>".
/// 
/// <b>Binding Looks</b>
/// 
/// In the UsdShading model, geometry expresses a binding to a single Look or
/// to a set of Looks partitioned by face-sets defined on the geometry;
/// it is legal to bind a Look at the root (or other sub-prim) of a model,
/// and then bind a different Look to individual gprims, but the meaning of
/// inheritance and "ancestral overriding" of Look bindings is left to each
/// render-target to determine.  Since UsdGeom has no concept of shading,
/// we provide the API for binding and unbinding geometry here, on UsdShadeLook.
/// Please see Bind(), Unbind(), GetBindingRel(), GetBoundLook().
/// 
/// <b>Look Variation</b>
/// 
/// The entire power of Usd variantSets and all the other composition 
/// operators can be brought to bear on encoding shading variation.  
/// UsdShadeLook provides facilities for a particular way of building
/// "Look variants" in which neither the identity of the Looks themselves
/// nor the geometry Look-bindings need to change - instead we vary the
/// targetted networks, interface values, and even parameter values within
/// a single variantSet.  
/// See \ref UsdShadeLook_Variations "Authoring Look Variations" for more.
/// 
/// <b>Authoring Looks for Referenced Re-use</b>
/// 
/// The shading networks that a Look may target can live anywhere in a layer's
/// namespace.  However, it is advantageous to place all of the shaders that 
/// "belong" to the Look under it in namespace, particularly when building
/// "shading libraries/palettes" that you intend to reference into other,
/// composite, more specialized Looks.  This is because Usd references compose
/// all descendant prims of the reference target into the referencer's namespace.
/// This means that all of the library Look's shader network will come along 
/// with the Look when the Look gets referenced as a sub-component of another
/// Look.
/// 
/// 
///
class UsdShadeLook : public UsdShadeMaterial
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a UsdShadeLook on UsdPrim \p prim .
    /// Equivalent to UsdShadeLook::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeLook(const UsdPrim& prim=UsdPrim())
        : UsdShadeMaterial(prim)
    {
    }

    /// Construct a UsdShadeLook on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeLook(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeLook(const UsdSchemaBase& schemaObj)
        : UsdShadeMaterial(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeLook();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdShadeLook holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadeLook(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeLook
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
    USDSHADE_API
    static UsdShadeLook
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSHADE_API
    virtual const TfType &_GetTfType() const;

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
    /// \name Binding Geometry Prims to Looks
    /// @{
    // --------------------------------------------------------------------- //

    /// Create a Look-binding relationship on \p prim and target it to this 
    /// Look prim
    ///
    /// Any UsdPrim can have a binding to at most a \em single UsdShadeLook .
    /// \return true on success
    USDSHADE_API
    bool Bind(UsdPrim& prim) const;

    /// Ensure that, when resolved up to and including the current UsdEditTarget
    /// in composition strength, the given prim has no binding to a UsdShadeLook
    ///
    /// Note that this constitutes an assertion that there be no binding - 
    /// it does \em not simply remove any binding at the current EditTarget
    /// such that a weaker binding will "shine through".  For that behavior,
    /// use GetBindingRel().ClearTargets()
    /// \return true on success
    USDSHADE_API
    static bool Unbind(UsdPrim& prim);

    /// Direct access to the binding relationship for \p prim, if it has
    /// already been created.
    ///
    /// This is how clients discover the Look to which a prim is bound,
    /// and also how one would add metadata or 
    /// \ref UsdObject::GetCustomData() "customData" .
    ///
    /// Care should be exercized when manipulating this relationship's
    /// targets directly, rather than via Bind() and Unbind(), since it
    /// will then be the client's responsibility to ensure that only a
    /// single Look prim is targetted.  In general, use 
    /// UsdRelationship::SetTargets() rather than UsdRelationship::AddTarget()
    USDSHADE_API
    static UsdRelationship GetBindingRel(const UsdPrim& prim);

    /// Follows the relationship returned by GetBindingRel and returns a
    /// valid UsdShadeLook if the relationship targets exactly one such prim.
    ///
    USDSHADE_API
    static UsdShadeLook GetBoundLook(const UsdPrim &prim);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor UsdShadeLook_Variations
    /// \name Authoring Look Variations
    /// Each UsdShadeLook prim can host data for any number of render targets
    /// (such as Renderman RSL, Renderman RIS, Arnold, or glslfx).  Each Look
    /// can /em also, however, encode variations of a particular look that can
    /// vary each target's set of bound shaders - or any other data authored
    /// on the Look.  For example, we might have a logo'd baseball cap that
    /// comes in denim, nylon, and corduroy variations.  We can encode the
    /// material variation - and the ability to select the variations - as
    /// a Look variant that varies the surface shader relationship in each of
    /// the render targets.
    ///
    /// We provide methods to aid in authoring such variations on individual
    /// Look prims, and also a facility for creating a "master" look variant
    /// on another prim (a model's root prim, or another common ancestor of
    /// all Look prims in a model) that will set the variants on each Look
    /// in concert, from making a single variant selection.
    ///
    /// <b>Note on variant vs "direct" opinions.</b>
    /// For any given location in a layer (i.e. a prim's spec in the layer),
    /// opinions expressed inside a variant of a variantSet will be
    /// \em weaker than any opinions expressed "directly" at the location,
    /// outside of any layer.
    ///
    /// Therefore, if you intend to vary relationship "surface" in a 
    /// lookVariant, make sure you author it \em only inside variant edit
    /// contexts (i.e. GetEditContextForVariant()).  If you author the
    /// relationship outside any of the variants, it will trump them all.
    /// @{
    // --------------------------------------------------------------------- //

    /// Helper function for configuring a UsdStage's editTarget to author
    /// Look variations. Takes care of creating the look variantSet and
    /// specified variant, if necessary.
    ///
    /// Let's assume that we are authoring looks into the Stage's current
    /// UsdEditTarget, and that we are iterating over the variations of a
    /// UsdShadeLook \em clothLook, and \em currVariant is the variant we are
    /// processing (e.g. "denim").
    ///
    /// In C++, then, we would use the following pattern:
    /// \code
    /// {
    ///     UsdEditContext ctxt(clothLook.GetEditContextForVariant(currVariant));
    ///
    ///     // All Usd mutation of the UsdStage on which clothLook sits will
    ///     // now go "inside" the currVariant of the "lookVariant" variantSet
    /// }
    /// \endcode
    ///
    /// In python, the pattern is:
    /// \code{.py}
    ///     with clothLook.GetEditContextForVariant(currVariant):
    ///         # Now sending mutations to currVariant
    /// \endcode
    ///
    /// If \p layer is specified, then we will use it, rather than the stage's
    /// current UsdEditTarget's layer as the destination layer for the 
    /// edit context we are building.  If \p layer does not actually contribute
    /// to the Look prim's definition, any editing will have no effect on this
    /// Look.
    ///
    /// <b>Note:</b> As just stated, using this method involves authoring
    /// a selection for the lookVariant in the stage's current EditTarget.
    /// When client is done authoring variations on this prim, they will likely
    /// want to either UsdVariantSet::SetVariantSelection() to the appropriate
    /// default selection, or possibly UsdVariantSet::ClearVariantSelection()
    /// on the UsdShadeLook::GetLookVariant() UsdVariantSet.
    /// \sa UsdVariantSet::GetVariantEditContext()
    USDSHADE_API
    std::pair<UsdStagePtr, UsdEditTarget>
    GetEditContextForVariant(const TfToken &lookVariantName,
                             const SdfLayerHandle &layer = SdfLayerHandle()) const;
    
    /// Return a UsdVariantSet object for interacting with the look variant
    /// variantSet
    USDSHADE_API
    UsdVariantSet GetLookVariant() const;

    /// Create a variantSet on \p masterPrim that will set the lookVariant on
    /// each of the given \em lookPrims.
    ///
    /// The variantSet, whose name can be specified with \p
    /// masterVariantSetName and defaults to the same lookVariant name
    /// created on Looks by GetEditContextForVariant(), will have the same
    /// variants as the Looks, and each Master variant will set every
    /// \p lookPrims' lookVariant selection to the same variant as the
    /// master. Thus, it allows all Looks to be switched with a single
    /// variant selection, on \p masterPrim.
    ///
    /// If \p masterPrim is an ancestor of any given member of \p lookPrims,
    /// then we will author variant selections directly on the lookPrims.
    /// However, it is often preferable to create a master lookVariant in
    /// a separately rooted tree from the lookPrims, so that it can be
    /// layered more strongly on top of the looks. Therefore, for any lookPrim
    /// in a different tree than masterPrim, we will create "overs" as children
    /// of masterPrim that recreate the path to the lookPrim, substituting
    /// masterPrim's full path for the lookPrim's root path component.
    ///
    /// Upon successful completion, the new variantSet we created on 
    /// \p masterPrim will have its variant selection authored to the 
    /// "last" variant (determined lexicographically).  It is up to the
    /// calling client to either UsdVariantSet::ClearVariantSelection()
    /// on \p masterPrim, or set the selection to the desired default setting.
    ///
    /// Return \c true on success. It is an error if any of \p looks
    /// have a different set of variants for the lookVariant than the others.
    USDSHADE_API
    static bool CreateMasterLookVariant(
        const UsdPrim &masterPrim,
        const std::vector<UsdPrim> &lookPrims,
        const TfToken &masterVariantSetName = TfToken());

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor UsdShadeLook_BaseLook
    /// \name BaseLook
    /// Relationship to describe child/parent inheritance.
    /// A look that derives from a BaseLook will curruntely only
    /// present/compose the properties unique to the derived look, and does
    /// not retain a live composition relationship to its BaseLook
    //
    /// \todo We plan to add a "derives" Usd composition arc to replace this.
    /// @{
    // --------------------------------------------------------------------- //

    /// Get the path to the base Look of this Look.
    /// If there is no base Look, an empty Look is returned
    USDSHADE_API
    UsdShadeLook GetBaseLook() const;

    /// Get the base Look of this Look.
    /// If there is no base look, an empty path is returned
    USDSHADE_API
    SdfPath GetBaseLookPath() const;

    /// Set the base Look of this Look.
    /// An empty Look is equivalent to clearing the base Look.
    USDSHADE_API
    void SetBaseLook(const UsdShadeLook& baseLook) const;

    /// Set the path to the base Look of this Look.
    /// An empty path is equivalent to clearing the base look.
    USDSHADE_API
    void SetBaseLookPath(const SdfPath& baseLookPath) const;

    /// Clear the base Look of this Look.
    USDSHADE_API
    void ClearBaseLook() const;

    // Check if this Look has a base Look
    USDSHADE_API
    bool HasBaseLook() const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor UsdShadeLook_FaceSet
    /// \name FaceSet
    /// 
    /// API to create and query the existence of a "look" face-set on a mesh 
    /// prim. 
    /// 
    /// \note Look bindings authored on a face-set are only honored by renderers 
    /// if it is the "look" face-set.
    /// 
    /// Here's some sample code that shows how to create and bind looks to a 
    /// "look" face-set.
    /// 
    /// \code 
    /// UsdPrim mesh = stage.GetPrimAtPath("/path/to/meshPrim");
    /// UsdPrim look1 = stage.GetPrimAtPath("/path/to/look1");
    /// USdPrim look2 = stage.GetPrimAtPath("/path/to/look2");
    /// UsdGeomFaceSetAPI lookFaceSet = UsdShaderLook::CreateLookFaceSet(mesh);
    /// VtIntArray faceIndices1, faceIndices2;
    /// //.. populate faceIndices here.
    /// lookFaceSet.AppendFaceGroup(faceIndices1, look1.GetPath(), 
    ///                             UsdTimeCode::Default());
    /// lookFaceSet.AppendFaceGroup(faceIndices2, look2.GetPath(), 
    ///                             UsdTimeCode::Default());
    /// \endcode
    /// @{
    // --------------------------------------------------------------------- //

    /// Creates a "look" face-set on the given prim. The look face-set is a 
    /// partition of faces, since no face can be bound to more than one look.
    /// 
    /// If a "look" face-set already exists, it is returned. If not, it
    /// creates one and returns it.
    /// 
    USDSHADE_API
    static UsdGeomFaceSetAPI CreateLookFaceSet(const UsdPrim &prim);

    /// Returns the "look" face-set if it exists on the given prim. If not, 
    /// returns an invalid UsdGeomFaceSetAPI object.
    /// 
    USDSHADE_API
    static UsdGeomFaceSetAPI GetLookFaceSet(const UsdPrim &prim);

    /// Returns true if the given prim has a "look" face-set. A "look" 
    /// face-set must be a partition for it to be considered valid.
    /// 
    USDSHADE_API
    static bool HasLookFaceSet(const UsdPrim &prim);

    /// @}


};

#endif
