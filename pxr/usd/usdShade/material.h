//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSHADE_GENERATED_MATERIAL_H
#define USDSHADE_GENERATED_MATERIAL_H

/// \file usdShade/material.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usdShade/nodeGraph.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdShade/connectableAPI.h"

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
/// A Material provides a container into which multiple "render contexts"
/// can add data that defines a "shading material" for a renderer. Typically 
/// this consists of one or more UsdShadeOutput properties connected to outputs 
/// of nested Shader prims - though a context/client is free to add
/// any data that is suitable. We <b>strongly advise</b> that all contexts
/// adopt the convention that all properties be prefixed with a namespace
/// that identifies the context e.g. "token outputs:ri:surface.connect = 
/// </MyselfMaterial/previewSurface.outputs:surface". 
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
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdShadeTokens.
/// So to set an attribute to the value "rightHanded", use UsdShadeTokens->rightHanded
/// as the value.
///
class UsdShadeMaterial : public UsdShadeNodeGraph
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

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

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDSHADE_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSHADE_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSHADE_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // SURFACE 
    // --------------------------------------------------------------------- //
    /// Represents the universal "surface" output terminal of a
    /// material.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token outputs:surface` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDSHADE_API
    UsdAttribute GetSurfaceAttr() const;

    /// See GetSurfaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateSurfaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISPLACEMENT 
    // --------------------------------------------------------------------- //
    /// Represents the universal "displacement" output terminal of a 
    /// material.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token outputs:displacement` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDSHADE_API
    UsdAttribute GetDisplacementAttr() const;

    /// See GetDisplacementAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateDisplacementAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VOLUME 
    // --------------------------------------------------------------------- //
    /// Represents the universal "volume" output terminal of a
    /// material.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token outputs:volume` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDSHADE_API
    UsdAttribute GetVolumeAttr() const;

    /// See GetVolumeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateVolumeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    /// \anchor UsdShadeMaterial_Outputs
    /// \name Standard Material Terminal Outputs
    /// A UsdShadeMaterial can have any number of "terminal" outputs. These 
    /// outputs are generally used to point to outputs of shader prims or 
    /// NodeGraphs that describe certain properties of the material that a 
    /// renderer might wish to consume. There are three standard output 
    /// terminals that are supported by the core API: <b>surface</b>, 
    /// <b>displacement</b> and <b>volume</b>. 
    /// 
    /// Each terminal output can further be qualified by a token-valued 
    /// <b>renderContext</b>. When a non-empty renderContext value is specified 
    /// to the API, the output is considered to have a specific or restricted 
    /// renderContext. If the renderContext value is empty (i.e. equal to 
    /// UsdShadeTokens->universalRenderContext), then the output is considered 
    /// to be a "universal", meaning it could apply to any render contexts. 
    /// Render context token values is typically driven by the rendering backend
    /// consuming the terminal output (eg, RI or glslfx).
    /// @{
        
    /// Creates and returns the "surface" output on this material for the 
    /// specified \p renderContext.
    /// 
    /// If the output already exists on the material, it is returned and no 
    /// authoring is performed. The returned output will always have the 
    /// requested renderContext.
    USDSHADE_API 
    UsdShadeOutput CreateSurfaceOutput(const TfToken &renderContext
            =UsdShadeTokens->universalRenderContext) const;

    /// Returns the "surface" output of this material for the specified
    /// \p renderContext. The returned output will always have the requested 
    /// renderContext. 
    /// 
    /// An invalid output is returned if an output corresponding to the 
    /// requested specific-renderContext does not exist.
    /// 
    /// \sa UsdShadeMaterial::ComputeSurfaceSource()
    USDSHADE_API
    UsdShadeOutput GetSurfaceOutput(const TfToken &renderContext
            =UsdShadeTokens->universalRenderContext) const;

    /// Returns the "surface" outputs of this material for all available
    /// renderContexts.
    ///
    /// The returned vector will include all authored "surface" outputs with
    /// the <i>universal</i> renderContext output first, if present. Outputs
    /// are returned regardless of whether they are connected to a valid
    /// source.
    USDSHADE_API
    std::vector<UsdShadeOutput> GetSurfaceOutputs() const;

    /// \deprecated Use the form that takes a TfTokenVector or renderContexts.
    USDSHADE_API
    UsdShadeShader ComputeSurfaceSource(
        const TfToken &renderContext,
        TfToken *sourceName=nullptr, 
        UsdShadeAttributeType *sourceType=nullptr) const;

    /// Computes the resolved "surface" output source for the given 
    /// \p contextVector. Using the earliest renderContext in the contextVector
    /// that produces a valid Shader object.
    /// 
    /// If a "surface" output corresponding to each of the renderContexts
    /// does not exist <b>or</b> is not connected to a valid source, then this 
    /// checks the <i>universal</i> surface output.
    /// 
    /// Returns an empty Shader object if there is no valid <i>surface</i> 
    /// output source for any of the renderContexts in the \p contextVector.
    /// The python version of this method returns a tuple containing three 
    /// elements (the source surface shader, sourceName, sourceType).
    USDSHADE_API
    UsdShadeShader ComputeSurfaceSource(
        const TfTokenVector &contextVector={UsdShadeTokens->universalRenderContext},
        TfToken *sourceName=nullptr, 
        UsdShadeAttributeType *sourceType=nullptr) const;

    /// Creates and returns the "displacement" output on this material for the 
    /// specified \p renderContext.
    /// 
    /// If the output already exists on the material, it is returned and no 
    /// authoring is performed. The returned output will always have the 
    /// requested renderContext.
    USDSHADE_API 
    UsdShadeOutput CreateDisplacementOutput(const TfToken &renderContext
            =UsdShadeTokens->universalRenderContext) const;

    /// Returns the "displacement" output of this material for the specified
    /// renderContext. The returned output will always have the requested 
    /// renderContext. 
    /// 
    /// An invalid output is returned if an output corresponding to the 
    /// requested specific-renderContext does not exist.
    /// 
    /// \sa UsdShadeMaterial::ComputeDisplacementSource()
    USDSHADE_API 
    UsdShadeOutput GetDisplacementOutput(const TfToken &renderContext
            =UsdShadeTokens->universalRenderContext) const;

    /// Returns the "displacement" outputs of this material for all available
    /// renderContexts.
    ///
    /// The returned vector will include all authored "displacement" outputs
    /// with the <i>universal</i> renderContext output first, if present.
    /// Outputs are returned regardless of whether they are connected to a
    /// valid source.
    USDSHADE_API
    std::vector<UsdShadeOutput> GetDisplacementOutputs() const;

    /// \deprecated Use the form that takes a TfTokenVector or renderContexts 
    USDSHADE_API
    UsdShadeShader ComputeDisplacementSource(
        const TfToken &renderContext,
        TfToken *sourceName=nullptr, 
        UsdShadeAttributeType *sourceType=nullptr) const;

    /// Computes the resolved "displacement" output source for the given
    /// \p contextVector. Using the earliest renderContext in the contextVector
    /// that produces a valid Shader object.
    /// 
    /// If a "displacement" output corresponding to each of the renderContexts
    /// does not exist <b>or</b> is not connected to a valid source, then this 
    /// checks the <i>universal</i> displacement output.
    /// 
    /// Returns an empty Shader object if there is no valid <i>displacement</i>
    /// output source for any of the renderContexts in the \p contextVector.
    /// The python version of this method returns a tuple containing three 
    /// elements (the source displacement shader, sourceName, sourceType).
    USDSHADE_API
    UsdShadeShader ComputeDisplacementSource(
        const TfTokenVector &contextVector={UsdShadeTokens->universalRenderContext},
        TfToken *sourceName=nullptr, 
        UsdShadeAttributeType *sourceType=nullptr) const;

    /// Creates and returns the "volume" output on this material for the 
    /// specified \p renderContext.
    /// 
    /// If the output already exists on the material, it is returned and no 
    /// authoring is performed. The returned output will always have the 
    /// requested renderContext.
    USDSHADE_API 
    UsdShadeOutput CreateVolumeOutput(const TfToken &renderContext
            =UsdShadeTokens->universalRenderContext) const;

    /// Returns the "volume" output of this material for the specified
    /// renderContext. The returned output will always have the requested 
    /// renderContext. 
    /// 
    /// An invalid output is returned if an output corresponding to the 
    /// requested specific-renderContext does not exist.
    /// 
    /// \sa UsdShadeMaterial::ComputeVolumeSource()
    USDSHADE_API 
    UsdShadeOutput GetVolumeOutput(const TfToken &renderContext
            =UsdShadeTokens->universalRenderContext) const;

    /// Returns the "volume" outputs of this material for all available
    /// renderContexts.
    ///
    /// The returned vector will include all authored "volume" outputs with the
    /// <i>universal</i> renderContext output first, if present. Outputs are
    /// returned regardless of whether they are connected to a valid source.
    USDSHADE_API
    std::vector<UsdShadeOutput> GetVolumeOutputs() const;

    /// \deprecated Use the form that takes a TfTokenVector or renderContexts 
    USDSHADE_API
    UsdShadeShader ComputeVolumeSource(
        const TfToken &renderContext,
        TfToken *sourceName=nullptr, 
        UsdShadeAttributeType *sourceType=nullptr) const;

    /// Computes the resolved "volume" output source for the given 
    /// \p contextVector. Using the earliest renderContext in the contextVector
    /// that produces a valid Shader object.
    /// 
    /// If a "volume" output corresponding to each of the renderContexts
    /// does not exist <b>or</b> is not connected to a valid source, then this 
    /// checks the <i>universal</i> volume output.
    /// 
    /// Returns an empty Shader object if there is no valid <i>volume</i> output 
    /// output source for any of the renderContexts in the \p contextVector.
    /// The python version of this method returns a tuple containing three 
    /// elements (the source volume shader, sourceName, sourceType).
    USDSHADE_API
    UsdShadeShader ComputeVolumeSource(
        const TfTokenVector &contextVector={UsdShadeTokens->universalRenderContext},
        TfToken *sourceName=nullptr, 
        UsdShadeAttributeType *sourceType=nullptr) const;

    /// @}

private:
    // Helper method to compute the sources of a given output, identified by its 
    // baseName, for the renderContexts in the specified contextVector.
    UsdShadeAttributeVector _ComputeNamedOutputSources(
        const TfToken &baseName,
        const TfTokenVector &contextVector) const;

    // Helper method to compute the source shader of a given output, identified 
    // by its baseName, for the renderContexts in the specified contextVector.
    UsdShadeShader _ComputeNamedOutputShader(
        const TfToken &baseName,
        const TfTokenVector &contextVector,
        TfToken *sourceName,
        UsdShadeAttributeType *sourceType) const;

    // Helper method to retrieve outputs in all renderContexts that match the
    // given terminalName.
    std::vector<UsdShadeOutput> _GetOutputsForTerminalName(
        const TfToken& terminalName) const;

public:
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

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
