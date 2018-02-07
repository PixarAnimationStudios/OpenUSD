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
#ifndef USDSHADE_GENERATED_MATERIAL_H
#define USDSHADE_GENERATED_MATERIAL_H

/// \file usdShade/material.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usdShade/nodeGraph.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/faceSetAPI.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// MATERIAL                                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdShadeMaterial
///
/// A Material provides a container into which multiple "render targets"
/// can add data that defines a "shading material" for a renderer.  Typically
/// this consists of one or more UsdRelationship properties that target
/// other prims of type \em Shader - though a target/client is free to add
/// any data that is suitable.  We <b>strongly advise</b> that all targets
/// adopt the convention that all properties be prefixed with a namespace
/// that identifies the target, e.g. "rel ri:surface = </Shaders/mySurf>".
/// 
/// ## Binding Materials
/// 
/// In the UsdShading model, geometry expresses a binding to a single Material or
/// to a set of Materials partitioned by UsdGeomSubsets defined beneath the
/// geometry; it is legal to bind a Material at the root (or other sub-prim) of 
/// a model, and then bind a different Material to individual gprims, but the
/// meaning of inheritance and "ancestral overriding" of Material bindings is 
/// left to each render-target to determine.  Since UsdGeom has no concept of 
/// shading, we provide the API for binding and unbinding geometry on the API 
/// schema UsdShadeMaterialBindingAPI.
/// 
/// ## Material Variation
/// 
/// The entire power of USD VariantSets and all the other composition 
/// operators can leveraged when encoding shading variation.  
/// UsdShadeMaterial provides facilities for a particular way of building
/// "Material variants" in which neither the identity of the Materials themselves
/// nor the geometry Material-bindings need to change - instead we vary the
/// targeted networks, interface values, and even parameter values within
/// a single variantSet.  
/// See \ref UsdShadeMaterial_Variations "Authoring Material Variations" 
/// for more details.
/// 
/// ## Materials Encapsulate their Networks in Namespace
/// 
/// UsdShade requires that all of the shaders that "belong" to the Material 
/// live under the Material in namespace. This supports powerful, easy reuse
/// of Materials, because it allows us to *reference* a Material from one
/// asset (the asset might be a library of Materials) into another asset: USD 
/// references compose all descendant prims of the reference target into the 
/// referencer's namespace, which means that all of the referenced Material's 
/// shader networks will come along with the Material. When referenced in this
/// way, Materials can also be [instanced](http://openusd.org/docs/USD-Glossary.html#USDGlossary-Instancing), for ease of deduplication and compactness.
/// Finally, Material encapsulation also allows us to 
/// \ref UsdShadeMaterial_BaseMaterial "specialize" child materials from 
/// parent materials.
/// 
/// 
///
class UsdShadeMaterial : public UsdShadeNodeGraph
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = true;

    /// Construct a UsdShadeMaterial on UsdPrim \p prim .
    /// Equivalent to UsdShadeMaterial::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeMaterial(const UsdPrim& prim=UsdPrim())
        : UsdShadeNodeGraph(prim)
    {
    }

    /// Construct a UsdShadeMaterial on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeMaterial(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeMaterial(const UsdSchemaBase& schemaObj)
        : UsdShadeNodeGraph(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeMaterial();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdShadeMaterial holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadeMaterial(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeMaterial
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
    static UsdShadeMaterial
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSHADE_API
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
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    // --------------------------------------------------------------------- //
    /// \name Helpful Types
    /// @{
    // --------------------------------------------------------------------- //

    /// A function type that takes a path and returns a bool.
    typedef std::function<bool (const SdfPath &)> PathPredicate;

    /// @}

    // --------------------------------------------------------------------- //
    /// \name Binding Geometry Prims to Materials
    /// \deprecated
    /// This API is now deprecated. Please use UsdShadeMaterialBindingAPI
    /// instead.
    /// @{
    // --------------------------------------------------------------------- //

    /// \deprecated
    /// Create a Material-binding relationship on \p prim and target it to this 
    /// Material prim
    ///
    /// Any UsdPrim can have a binding to at most a \em single UsdShadeMaterial .
    /// \return true on success
    USDSHADE_API
    //[[deprecated("Please use UsdShadeMaterialBindingAPI instead.")]]
    bool Bind(const UsdPrim& prim) const;

    /// \deprecated
    /// Ensure that, when resolved up to and including the current UsdEditTarget
    /// in composition strength, the given prim has no binding to a UsdShadeMaterial
    ///
    /// Note that this constitutes an assertion that there be no binding - 
    /// it does \em not simply remove any binding at the current EditTarget
    /// such that a weaker binding will "shine through".  For that behavior,
    /// use GetBindingRel().ClearTargets()
    /// \return true on success
    USDSHADE_API
    //[[deprecated("Please use UsdShadeMaterialBindingAPI instead.")]]
    static bool Unbind(const UsdPrim& prim);

    /// \deprecated
    /// Direct access to the binding relationship for \p prim, if it has
    /// already been created.
    ///
    /// This is how clients discover the Material to which a prim is bound,
    /// and also how one would add metadata or 
    /// \ref UsdObject::GetCustomData() "customData" .
    ///
    /// Care should be exercized when manipulating this relationship's
    /// targets directly, rather than via Bind() and Unbind(), since it
    /// will then be the client's responsibility to ensure that only a
    /// single Material prim is targetted.  In general, use 
    /// UsdRelationship::SetTargets() rather than UsdRelationship::AddTarget()
    //[[deprecated("Please use UsdShadeMaterialBindingAPI instead.")]]
    USDSHADE_API
    static UsdRelationship GetBindingRel(const UsdPrim& prim);

    /// \deprecated
    /// Follows the relationship returned by GetBindingRel and returns a
    /// valid UsdShadeMaterial if the relationship targets exactly one such prim.
    ///
    //[[deprecated("Please use UsdShadeMaterialBindingAPI instead.")]]
    USDSHADE_API
    static UsdShadeMaterial GetBoundMaterial(const UsdPrim &prim);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor UsdShadeMaterial_Variations
    /// \name Authoring Material Variations
    /// Each UsdShadeMaterial prim can host data for any number of render targets
    /// (such as Renderman RIS, Arnold, or glslfx).
    ///
    /// A single UsdShadeMaterial group can, however, encode variations on
    /// appearance, varying any data authored on the material and its contents.
    /// For example, we might have a logo'd baseball cap that
    /// comes in denim, nylon, and corduroy variations.
    ///
    /// We provide methods to aid in authoring such variations on individual
    /// Material prims, and also a facility for creating a "master" look
    /// variant on another prim (e.g.  a model's root prim, or another common 
    /// ancestor of all Material prims in a model) that will be able to modify
    /// Materials, bindings, connections and values at once.
    ///
    /// <b>Note on variant vs "direct" opinions.</b>
    /// For any given prim's spec in a layer, opinions expressed inside a 
    /// variant of a variantSet will be /weaker/ than any opinions expressed 
    /// "directly" at the location, outside of any layer.
    ///
    /// Therefore, if you intend to author a default variant that is weaker than
    /// more explicit variants, you will need to have those opinions be weaker 
    /// by setting them across a reference arc such as the following:
    ///
    /// \code
    /// def "MyMaterial" (
    ///     add references = </MyMaterial_defaultShadingVariant>
    ///     variants = {
    ///         string materialVariant = "SomeVariant"
    ///     }
    ///     add variantSets = "materialVariant"
    /// )
    /// {
    ///     float strongerThanVariantOpinion
    /// 
    ///     variantSet "materialVariant" = {
    ///         "SomeVariant" {
    ///             float variantOpinion
    ///         }
    ///     }
    /// }
    ///
    /// over "MyMaterial_defaultShadingVariant"
    /// {
    ///     float weakerThanVariantOpinion
    /// }
    /// \endcode
    /// 
    /// @{
    ///
    // --------------------------------------------------------------------- //
    /// Helper function for configuring a UsdStage's UsdEditTarget to author
    /// Material variations. Takes care of creating the Material variantSet and
    /// specified variant, if necessary.
    ///
    /// Let's assume that we are authoring Materials into the Stage's current
    /// UsdEditTarget, and that we are iterating over the variations of a
    /// UsdShadeMaterial \em clothMaterial, and \em currVariant is the variant we are
    /// processing (e.g. "denim").
    ///
    /// In C++, then, we would use the following pattern:
    /// \code
    /// {
    ///     UsdEditContext ctxt(clothMaterial.GetEditContextForVariant(currVariant));
    ///
    ///     // All USD mutation of the UsdStage on which clothMaterial sits will
    ///     // now go "inside" the currVariant of the "MaterialVariant" variantSet
    /// }
    /// \endcode
    ///
    /// In python, the pattern is:
    /// \code{.py}
    ///     with clothMaterial.GetEditContextForVariant(currVariant):
    ///         # Now sending mutations to currVariant
    /// \endcode
    ///
    /// If \p layer is specified, then we will use it, rather than the stage's
    /// current UsdEditTarget's layer as the destination layer for the 
    /// edit context we are building.  If \p layer does not actually contribute
    /// to the Material prim's definition, any editing will have no effect on this
    /// Material.
    ///
    /// <b>Note:</b> As just stated, using this method involves authoring
    /// a selection for the MaterialVariant in the stage's current EditTarget.
    /// When client is done authoring variations on this prim, they will likely
    /// want to either UsdVariantSet::SetVariantSelection() to the appropriate
    /// default selection, or possibly UsdVariantSet::ClearVariantSelection()
    /// on the UsdShadeMaterial::GetMaterialVariant() UsdVariantSet.
    /// \sa UsdVariantSet::GetVariantEditContext()
    USDSHADE_API
    std::pair<UsdStagePtr, UsdEditTarget>
    GetEditContextForVariant(const TfToken &MaterialVariantName,
                             const SdfLayerHandle &layer = SdfLayerHandle()) const;
    
    /// Return a UsdVariantSet object for interacting with the Material variant
    /// variantSet
    USDSHADE_API
    UsdVariantSet GetMaterialVariant() const;

    /// Create a variantSet on \p masterPrim that will set the MaterialVariant on
    /// each of the given \em MaterialPrims.
    ///
    /// The variantSet, whose name can be specified with \p
    /// masterVariantSetName and defaults to the same MaterialVariant name
    /// created on Materials by GetEditContextForVariant(), will have the same
    /// variants as the Materials, and each Master variant will set every
    /// \p MaterialPrims' MaterialVariant selection to the same variant as the
    /// master. Thus, it allows all Materials to be switched with a single
    /// variant selection, on \p masterPrim.
    ///
    /// If \p masterPrim is an ancestor of any given member of \p MaterialPrims,
    /// then we will author variant selections directly on the MaterialPrims.
    /// However, it is often preferable to create a master MaterialVariant in
    /// a separately rooted tree from the MaterialPrims, so that it can be
    /// layered more strongly on top of the Materials. Therefore, for any MaterialPrim
    /// in a different tree than masterPrim, we will create "overs" as children
    /// of masterPrim that recreate the path to the MaterialPrim, substituting
    /// masterPrim's full path for the MaterialPrim's root path component.
    ///
    /// Upon successful completion, the new variantSet we created on 
    /// \p masterPrim will have its variant selection authored to the 
    /// "last" variant (determined lexicographically).  It is up to the
    /// calling client to either UsdVariantSet::ClearVariantSelection()
    /// on \p masterPrim, or set the selection to the desired default setting.
    ///
    /// Return \c true on success. It is an error if any of \p Materials
    /// have a different set of variants for the MaterialVariant than the others.
    USDSHADE_API
    static bool CreateMasterMaterialVariant(
        const UsdPrim &masterPrim,
        const std::vector<UsdPrim> &MaterialPrims,
        const TfToken &masterVariantSetName = TfToken());

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor UsdShadeMaterial_BaseMaterial
    /// \name BaseMaterial
    /// A specialize arc describes child/parent inheritance.
    /// A Material that derives from a BaseMaterial will retain a live 
    /// composition relationship to its BaseMaterial
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Get the path to the base Material of this Material.
    /// If there is no base Material, an empty Material is returned
    USDSHADE_API
    UsdShadeMaterial GetBaseMaterial() const;

    /// Get the base Material of this Material.
    /// If there is no base Material, an empty path is returned
    USDSHADE_API
    SdfPath GetBaseMaterialPath() const;

    /// Given a PcpPrimIndex, searches it for an arc to a parent material.
    ///
    /// This is a public static function to support applications that use
    /// Pcp but not Usd. Most clients should call \ref GetBaseMaterialPath,
    /// which uses this function when appropriate.
    USDSHADE_API
    static SdfPath FindBaseMaterialPathInPrimIndex(
        const PcpPrimIndex & primIndex,
        const PathPredicate & pathIsMaterialPredicate);

    /// Set the base Material of this Material.
    /// An empty Material is equivalent to clearing the base Material.
    USDSHADE_API
    void SetBaseMaterial(const UsdShadeMaterial& baseMaterial) const;

    /// Set the path to the base Material of this Material.
    /// An empty path is equivalent to clearing the base Material.
    USDSHADE_API
    void SetBaseMaterialPath(const SdfPath& baseMaterialPath) const;

    /// Clear the base Material of this Material.
    USDSHADE_API
    void ClearBaseMaterial() const;

    // Check if this Material has a base Material
    USDSHADE_API
    bool HasBaseMaterial() const;

    /// @}


    // --------------------------------------------------------------------- //
    /// \anchor UsdShadeMaterial_Subsets
    /// \name Binding materials to subsets
    /// \deprecated This API is now deprecated. Please use the equivalent API 
    ///             available on UsdShadeMaterialBindingAPI.
    /// 
    /// API to create, access and query the presence of GeomSubsets below an 
    /// imageable prim, that are created for the purpose of binding materials.
    /// 
    /// \note Material bindings authored on GeomSubsets are honored by renderers
    /// only if their familyName is <b>UsdShadeTokens->materialBind</b>.
    /// 
    /// Here's some sample code that shows how to create "face" subsets and 
    /// and bind materials to them.
    /// \code
    /// UsdGeomImageable mesh = UsdGeomImageable::Get(stage,
    ///         SdfPath("/path/to/meshPrim");
    /// UsdShadeMaterial plastic = UsdShadeMaterial::Get(stage, 
    ///         SdfPath("/path/to/PlasticMaterial");
    /// UsdShadeMaterial metal = UsdShadeMaterial::Get(stage, 
    ///         SdfPath("/path/to/MetalMaterial");    
    ///
    /// VtIntArray plasticFaces, metalFaces;
    /// //.. populate faceIndices here.
    /// //.. 
    /// 
    /// UsdGeomSubset plasticSubset = 
    ///         UsdShaderMaterial::CreateMaterialBindSubset(mesh, 
    ///                 "plasticSubset", plasticFaces);
    /// UsdGeomSubset metalSubset = 
    ///         UsdShaderMaterial::CreateMaterialBindSubset(mesh, 
    ///                 "metalSubset", metalFaces);
    /// plastic.Bind(plasticSubset.GetPrim())
    /// metal.Bind(metalSubset.GetPrim())
    /// 
    /// \endcode
    /// @{

    /// \deprecated 
    /// Creates a GeomSubset named \p subsetName with element type, 
    /// \p elementType and familyName <b>materialBind<b> below the given
    /// imageable prim, \p geom. 
    /// 
    /// If a GeomSubset named \p subsetName already exists, then its 
    /// "familyName" is updated to be UsdShadeTokens->materialBind and its 
    /// indices (at <i>default</i> timeCode) are updated with the provided 
    /// \p indices value before returning. 
    /// 
    /// This method forces the familyType of the "materialBind" family of 
    /// subsets to UsdGeomTokens->nonOverlapping if it's unset or explicitly set
    /// to UsdGeomTokens->unrestricted.
    /// 
    /// The default value \p elementType is UsdGeomTokens->face, as we expect 
    /// materials to be bound most often to subsets of faces on meshes.
    USDSHADE_API
    static UsdGeomSubset CreateMaterialBindSubset(
        const UsdGeomImageable &geom,
        const TfToken &subsetName,
        const VtIntArray &indices,
        const TfToken &elementType=UsdGeomTokens->face);

    /// \deprecated
    /// Returns all the existing GeomSubsets with 
    /// familyName=UsdShadeTokens->materialBind below the given imageable prim, 
    /// \p geom.
    USDSHADE_API
    static std::vector<UsdGeomSubset> GetMaterialBindSubsets(
        const UsdGeomImageable &geom);
    
    /// \deprecated
    /// Encodes whether the family of "materialBind" subsets form a valid 
    /// partition of the set of all faces on the imageable prim, \p geom.
    USDSHADE_API
    static bool SetMaterialBindSubsetsFamilyType(
        const UsdGeomImageable &geom,
        const TfToken &familyType);

    /// \deprecated
    /// Returns the familyType of the family of "materialBind" subsets under
    /// \p geom. 
    /// 
    /// By default materialBind subsets have familyType="nonOverlapping", but
    /// their can also be tagged as a "partition", using 
    /// SetMaterialBindFaceSubsetsFamilyType(). 
    /// 
    /// \sa UsdGeomSubset::SetFamilyType
    /// \sa UsdGeomSubset::GetFamilyNameAttr
    /// 
    USDSHADE_API
    static TfToken GetMaterialBindSubsetsFamilyType(
        const UsdGeomImageable &geom);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor UsdShadeMaterial_FaceSet
    /// \name FaceSet
    /// 
    /// \deprecated 
    /// \note This API is now deprecated as the has-A schema UsdGeomFaceSetAPI 
    /// has been deprecated in favor of the new concrete (typed) 
    /// <b>UsdGeomSubset</b> schema.
    /// 
    /// API to create and query the existence of a "Material" face-set on a mesh 
    /// prim. 
    /// 
    /// \note Material bindings authored on a face-set are only honored by renderers 
    /// if it is the "Material" face-set.
    /// 
    /// Here's some sample code that shows how to create and bind Materials to a 
    /// "Material" face-set.
    /// 
    /// \code 
    /// UsdPrim mesh = stage.GetPrimAtPath("/path/to/meshPrim");
    /// UsdPrim Material1 = stage.GetPrimAtPath("/path/to/Material1");
    /// USdPrim Material2 = stage.GetPrimAtPath("/path/to/Material2");
    /// UsdGeomFaceSetAPI MaterialFaceSet = UsdShaderMaterial::CreateMaterialFaceSet(mesh);
    /// VtIntArray faceIndices1, faceIndices2;
    /// //.. populate faceIndices here.
    /// MaterialFaceSet.AppendFaceGroup(faceIndices1, Material1.GetPath(), 
    ///                             UsdTimeCode::Default());
    /// MaterialFaceSet.AppendFaceGroup(faceIndices2, Material2.GetPath(), 
    ///                             UsdTimeCode::Default());
    /// \endcode
    /// @{
    // --------------------------------------------------------------------- //

    /// \deprecated 
    /// Creates a "Material" face-set on the given prim. The Material face-set is a 
    /// partition of faces, since no face can be bound to more than one Material.
    /// 
    /// If a "Material" face-set already exists, it is returned. If not, it
    /// creates one and returns it.
    /// 
    USDSHADE_API
    static UsdGeomFaceSetAPI CreateMaterialFaceSet(const UsdPrim &prim);

    /// \deprecated 
    /// Returns the "Material" face-set if it exists on the given prim. If not, 
    /// returns an invalid UsdGeomFaceSetAPI object.
    /// 
    USDSHADE_API
    static UsdGeomFaceSetAPI GetMaterialFaceSet(const UsdPrim &prim);

    /// \deprecated 
    /// Returns true if the given prim has a "Material" face-set. A "Material" 
    /// face-set must be a partition for it to be considered valid.
    /// 
    USDSHADE_API
    static bool HasMaterialFaceSet(const UsdPrim &prim);

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
