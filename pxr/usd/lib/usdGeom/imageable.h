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
#ifndef USDGEOM_GENERATED_IMAGEABLE_H
#define USDGEOM_GENERATED_IMAGEABLE_H

/// \file usdGeom/imageable.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/usd/usdGeom/primvar.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// IMAGEABLE                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdGeomImageable
///
/// Base class for all prims that may require rendering or 
/// visualization of some sort. The primary attributes of Imageable 
/// are \em visibility and \em purpose, which each provide instructions for
/// what geometry should be included for processing by rendering and other
/// computations.
/// 
/// \deprecated Imageable also provides API for accessing primvars, which
/// has been moved to the UsdGeomPrimvarsAPI schema, because primvars can now
/// be applied on non-Imageable prim types.  This API is planned
/// to be removed, UsdGeomPrimvarsAPI should be used directly instead.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomImageable : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::AbstractTyped;

    /// Construct a UsdGeomImageable on UsdPrim \p prim .
    /// Equivalent to UsdGeomImageable::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomImageable(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdGeomImageable on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomImageable(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomImageable(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomImageable();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomImageable holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomImageable(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomImageable
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDGEOM_API
    UsdSchemaType _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // VISIBILITY 
    // --------------------------------------------------------------------- //
    /// Visibility is meant to be the simplest form of "pruning" 
    /// visibility that is supported by most DCC apps.  Visibility is 
    /// animatable, allowing a sub-tree of geometry to be present for some 
    /// segment of a shot, and absent from others; unlike the action of 
    /// deactivating geometry prims, invisible geometry is still 
    /// available for inspection, for positioning, for defining volumes, etc.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: inherited
    /// \n  \ref UsdGeomTokens "Allowed Values": [inherited, invisible]
    USDGEOM_API
    UsdAttribute GetVisibilityAttr() const;

    /// See GetVisibilityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVisibilityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PURPOSE 
    // --------------------------------------------------------------------- //
    /// Purpose is a concept we have found useful in our pipeline for 
    /// classifying geometry into categories that can each be independently
    /// included or excluded from traversals of prims on a stage, such as
    /// rendering or bounding-box computation traversals.  The fallback
    /// purpose, \em default indicates that a prim has "no special purpose"
    /// and should generally be included in all traversals.  Subtrees rooted
    /// at a prim with purpose \em render should generally only be included
    /// when performing a "final quality" render.  Subtrees rooted at a prim
    /// with purpose \em proxy should generally only be included when 
    /// performing a lightweight proxy render (such as openGL).  Finally,
    /// subtrees rooted at a prim with purpose \em guide should generally
    /// only be included when an interactive application has been explicitly
    /// asked to "show guides". 
    /// 
    /// In the previous paragraph, when we say "subtrees rooted at a prim",
    /// we mean the most ancestral or tallest subtree that has an authored,
    /// non-default opinion.  If the purpose of </RootPrim> is set to 
    /// "render", then the effective purpose of </RootPrim/ChildPrim> will
    /// be "render" even if that prim has a different authored value for
    /// purpose.  <b>See ComputePurpose() for details of how purpose 
    /// inherits down namespace</b>.
    /// 
    /// As demonstrated in UsdGeomBBoxCache, a traverser should be ready to 
    /// accept combinations of included purposes as an input.
    /// 
    /// Purpose \em render can be useful in creating "light blocker"
    /// geometry for raytracing interior scenes.  Purposes \em render and
    /// \em proxy can be used together to partition a complicated model
    /// into a lightweight proxy representation for interactive use, and a
    /// fully realized, potentially quite heavy, representation for rendering.
    /// One can use UsdVariantSets to create proxy representations, but doing
    /// so requires that we recompose parts of the UsdStage in order to change
    /// to a different runtime level of detail, and that does not interact
    /// well with the needs of multithreaded rendering. Purpose provides us with
    /// a better tool for dynamic, interactive complexity management.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: default
    /// \n  \ref UsdGeomTokens "Allowed Values": [default, render, proxy, guide]
    USDGEOM_API
    UsdAttribute GetPurposeAttr() const;

    /// See GetPurposeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreatePurposeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROXYPRIM 
    // --------------------------------------------------------------------- //
    /// The \em proxyPrim relationship allows us to link a
    /// prim whose \em purpose is "render" to its (single target)
    /// purpose="proxy" prim.  This is entirely optional, but can be
    /// useful in several scenarios:
    /// 
    /// \li In a pipeline that does pruning (for complexity management)
    /// by deactivating prims composed from asset references, when we
    /// deactivate a purpose="render" prim, we will be able to discover
    /// and additionally deactivate its associated purpose="proxy" prim,
    /// so that preview renders reflect the pruning accurately.
    /// 
    /// \li DCC importers may be able to make more aggressive optimizations
    /// for interactive processing and display if they can discover the proxy
    /// for a given render prim.
    /// 
    /// \li With a little more work, a Hydra-based application will be able
    /// to map a picked proxy prim back to its render geometry for selection.
    /// 
    /// \note It is only valid to author the proxyPrim relationship on
    /// prims whose purpose is "render".
    ///
    USDGEOM_API
    UsdRelationship GetProxyPrimRel() const;

    /// See GetProxyPrimRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDGEOM_API
    UsdRelationship CreateProxyPrimRel() const;

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
    /// \name Primvar Creation and Introspection
    /// @{
    // --------------------------------------------------------------------- //
 
    /// \deprecated Please use UsdGeomPrimvarsAPI::CreatePrimvar() instead.
    USDGEOM_API
    UsdGeomPrimvar CreatePrimvar(const TfToken& attrName,
                                 const SdfValueTypeName &typeName,
                                 const TfToken& interpolation = TfToken(),
                                 int elementSize = -1) const;

    /// \deprecated Please use UsdGeomPrimvarsAPI::GetPrimvar() instead.
    USDGEOM_API
    UsdGeomPrimvar GetPrimvar(const TfToken &name) const;
    
    /// \deprecated Please use UsdGeomPrimvarsAPI::GetPrimvars() instead.
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetPrimvars() const;

    /// \deprecated Please use UsdGeomPrimvarsAPI::GetAuthoredPrimvars() instead.
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetAuthoredPrimvars() const;

    /// \deprecated Please use UsdGeomPrimvarsAPI::HasPrimvar() instead.
    USDGEOM_API
    bool HasPrimvar(const TfToken &name) const;


    /// Returns an ordered list of allowed values of the purpose attribute.
    /// 
    /// The ordering is important because it defines the protocol between 
    /// UsdGeomModelAPI and UsdGeomBBoxCache for caching and retrieving extents 
    /// hints by purpose. 
    ///
    /// The order is: [default, render, proxy, guide]
    /// 
    /// See \sa UsdGeomModelAPI::GetExtentsHint().
    /// 
    /// \sa GetOrderedPurposeTokens()
    USDGEOM_API
    static const TfTokenVector &GetOrderedPurposeTokens();

    // --------------------------------------------------------------------- //
    /// \name Visibility Authoring Helpers
    /// \anchor usdGeom_Visibility_Authoring_Helpers
    /// Convenience API for making an imageable visible or invisible.
    /// @{
    // --------------------------------------------------------------------- //

    /// Make the imageable visible if it is invisible at the given time.
    /// 
    /// Since visibility is pruning, this may need to override some 
    /// ancestor's visibility and all-but-one of the ancestor's children's 
    /// visibility, for all the ancestors of this prim up to the highest 
    /// ancestor that is explicitly invisible, to preserve the visibility state.
    /// 
    /// If MakeVisible() (or MakeInvisible()) is going to be applied to all 
    /// the prims on a stage, ancestors must be processed prior to descendants 
    /// to get the correct behavior.
    /// 
    /// \note When visibility is animated, this only works when it is 
    /// invoked sequentially at increasing time samples. If visibility is 
    /// already authored and animated in the scene, calling MakeVisible() at 
    /// an arbitrary (in-between) frame isn't guaranteed to work. 
    /// 
    /// \note This will only work properly if all ancestor prims of the 
    /// imageable are <b>defined</b>, as the imageable schema is only valid on 
    /// defined prims.
    /// 
    /// \note Be sure to set the edit target to the layer containing the 
    /// strongest visibility opinion or to a stronger layer.
    ///
    /// \sa MakeInvisible()
    /// \sa ComputeVisibility()
    /// 
    USDGEOM_API
    void MakeVisible(const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Makes the imageable invisible if it is visible at the given time.
    ///
    /// \note When visibility is animated, this only works when it is 
    /// invoked sequentially at increasing time samples. If visibility is 
    /// already authored and animated in the scene, calling MakeVisible() at 
    /// an arbitrary (in-between) frame isn't guaranteed to work. 
    /// 
    /// \note Be sure to set the edit target to the layer containing the 
    /// strongest visibility opinion or to a stronger layer.
    /// 
    /// \sa MakeVisible()
    /// \sa ComputeVisibility()
    ///
    USDGEOM_API
    void MakeInvisible(const UsdTimeCode &time=UsdTimeCode::Default()) const;

    ///@}

    // --------------------------------------------------------------------- //
    /// \name Computed Attribute Helpers
    /// \anchor usdGeom_Computed_Attribute_Helpers
    /// Visbility, Purpose, Bounds (World, Local, and Untransformed), and
    /// Transform (LocalToWorld and ParentToWorld) are all qualities of a
    /// prim's location in namespace that require non-local data and
    /// computation.  Computing these efficiently requires a stage-level
    /// cache, but when performance is not a concern, it is convenient to
    /// query these quantities directly on a prim, so we provide convenience
    /// API here for doing so.
    /// @{
    // --------------------------------------------------------------------- //

    /// Calculate the effective visibility of this prim, as defined by its
    /// most ancestral authored "invisible" opinion, if any.
    ///
    /// A prim is considered visible at the current \p time if none of its
    /// Imageable ancestors express an authored "invisible" opinion, which is
    /// what leads to the "simple pruning" behavior described in 
    /// GetVisibilityAttr().
    ///
    /// This function should be considered a reference implementation for
    /// correctness. <b>If called on each prim in the context of a traversal
    /// we will perform massive overcomputation, because sibling prims share
    /// sub-problems in the query that can be efficiently cached, but are not
    /// (cannot be) by this simple implementation.</b> If you have control of
    /// your traversal, it will be far more efficient to manage visibility
    /// on a stack as you traverse.
    ///
    /// \sa GetVisibilityAttr()
    USDGEOM_API
    TfToken ComputeVisibility(UsdTimeCode const &time = UsdTimeCode::Default()) const;

    /// \overload 
    /// Calculates the effective visibility of this prim, given the computed 
    /// visibility of its parent prim at the given \p time.
    /// 
    /// \sa GetVisibilityAttr()
    USDGEOM_API
    TfToken ComputeVisibility(const TfToken &parentVisibility,
                              UsdTimeCode const &time = UsdTimeCode::Default()) const;

    /// Calculate the effective purpose of this prim, as defined by its
    /// most ancestral authored non-"default" opinion, if any.
    ///
    /// If no opinion for purpose is authored on prim or any of its
    /// ancestors, its computed purpose is UsdGeomTokens->default_ .
    /// Otherwise, its computed purpose is that of its highest ancestor
    /// with an authored purpose of something other than UsdGeomTokens->default_
    ///
    /// In other words, all of a stage's root prims inherit the *purpose*
    /// UsdGeomTokens->default_ from the pseudoroot, and that value will be
    /// **inherited** by all of their descendants, until a descendant
    /// </Some/path/to/nonDefault> contains some other, authored value of
    /// *purpose* . The computed purpose of that prim **and all of its
    /// descendants** will be that prim's authored value, regardless of what
    /// *putpose* opinions its own descendant prims may express.
    ///
    /// This function should be considered a reference implementation for
    /// correctness. <b>If called on each prim in the context of a traversal
    /// we will perform massive overcomputation, because sibling prims share
    /// sub-problems in the query that can be efficiently cached, but are not
    /// (cannot be) by this simple implementation.</b> If you have control of
    /// your traversal, it will be far more efficient to manage purpose, along
    /// with visibility, on a stack as you traverse.
    ///
    /// \sa GetPurposeAttr()
    USDGEOM_API
    TfToken ComputePurpose() const;

    /// \overload 
    /// Calculates the effective purpose of this prim, given the computed 
    /// purpose of its parent prim.
    /// 
    /// \sa GetPurposeAttr()
    USDGEOM_API
    TfToken ComputePurpose(const TfToken &parentPurpose) const;

    /// Find the prim whose purpose is \em proxy that serves as the proxy
    /// for this prim, as established by the GetProxyPrimRel(), or an
    /// invalid UsdPrim if this prim has no proxy.
    ///
    /// This method will find the proxy for \em any prim whose computed
    /// purpose (see ComputePurpose()) is \em render.  If provided and a proxy 
    /// was found, we will set *renderPrim to the root of the \em render
    /// subtree upon which the renderProxy relationship was authored.
    ///
    /// If the renderProxy relationship has more than one target, we will
    /// issue a warning and return an invalid UsdPrim.  If the targeted prim
    /// does not have a resolved purpose of \em proxy, we will warn and
    /// return an invalid prim.
    ///
    /// This function should be considered a reference implementation for
    /// correctness. <b>If called on each prim in the context of a traversal
    /// we will perform massive overcomputation, because sibling prims share
    /// sub-problems in the query that can be efficiently cached, but are not
    /// (cannot be) by this simple implementation.</b> If you have control of
    /// your traversal, it will be far more efficient to compute proxy-prims
    /// on a stack as you traverse.
    ///
    /// \note Currently the returned prim will not contain any instancing
    /// context if it is inside a master - its path will be relative to the
    /// master's root.  Once UsdPrim is instancing-aware in the core, we can
    /// change this method to return a context-aware result.
    ///
    /// \sa SetProxyPrim(), GetProxyPrimRel()
    USDGEOM_API
    UsdPrim ComputeProxyPrim(UsdPrim *renderPrim=NULL) const;

    /// Convenience function for authoring the \em renderProxy rel on this
    /// prim to target the given \p proxy prim.
    ///
    /// To facilitate authoring on sparse or unloaded stages, we do not
    /// perform any validation of this prim's purpose or the type or
    /// purpoes of the specified prim.
    ///
    /// \sa ComputeProxyPrim(), GetProxyPrimRel()
    USDGEOM_API
    bool SetProxyPrim(const UsdPrim &proxy) const;
    
    /// \overload that takes any UsdSchemaBase-derived object
    USDGEOM_API
    bool SetProxyPrim(const UsdSchemaBase &proxy) const;
    
    /// Compute the bound of this prim in world space, at the specified
    /// \p time, and for the specified purposes.
    ///
    /// The bound of the prim is computed, including the transform (if any)
    /// authored on the node itself, and then transformed to world space.
    ///
    /// It is an error to not specify any purposes, which will result in the
    /// return of an empty box.
    ///
    /// <b>If you need to compute bounds for multiple prims on a stage, it
    /// will be much, much more efficient to instantiate a UsdGeomBBoxCache
    /// and query it directly;  doing so will reuse sub-computations shared 
    /// by the prims.</b>
    USDGEOM_API
    GfBBox3d ComputeWorldBound(UsdTimeCode const& time,
                               TfToken const &purpose1=TfToken(),
                               TfToken const &purpose2=TfToken(),
                               TfToken const &purpose3=TfToken(),
                               TfToken const &purpose4=TfToken()) const;


    /// Compute the bound of this prim in local space, at the specified
    /// \p time, and for the specified purposes.
    ///
    /// The bound of the prim is computed, including the transform (if any)
    /// authored on the node itself.
    ///
    /// It is an error to not specify any purposes, which will result in the
    /// return of an empty box.
    ///
    /// <b>If you need to compute bounds for multiple prims on a stage, it
    /// will be much, much more efficient to instantiate a UsdGeomBBoxCache
    /// and query it directly;  doing so will reuse sub-computations shared 
    /// by the prims.</b>
    USDGEOM_API
    GfBBox3d ComputeLocalBound(UsdTimeCode const& time,
                               TfToken const &purpose1=TfToken(),
                               TfToken const &purpose2=TfToken(),
                               TfToken const &purpose3=TfToken(),
                               TfToken const &purpose4=TfToken()) const;

    /// Compute the untransformed bound of this prim, at the specified
    /// \p time, and for the specified purposes.
    ///
    /// The bound of the prim is computed in its object space, ignoring
    /// any transforms authored on or above the prim.
    ///
    /// It is an error to not specify any purposes, which will result in the
    /// return of an empty box.
    ///
    /// <b>If you need to compute bounds for multiple prims on a stage, it
    /// will be much, much more efficient to instantiate a UsdGeomBBoxCache
    /// and query it directly;  doing so will reuse sub-computations shared 
    /// by the prims.</b>
    USDGEOM_API
    GfBBox3d ComputeUntransformedBound(UsdTimeCode const& time,
                                      TfToken const &purpose1=TfToken(),
                                      TfToken const &purpose2=TfToken(),
                                      TfToken const &purpose3=TfToken(),
                                      TfToken const &purpose4=TfToken()) const;

    /// Compute the transformation matrix for this prim at the given time,
    /// including the transform authored on the Prim itself, if present.
    ///
    /// <b>If you need to compute the transform for multiple prims on a
    /// stage, it will be much, much more efficient to instantiate a
    /// UsdGeomXformCache and query it directly; doing so will reuse
    /// sub-computations shared by the prims.</b>
    USDGEOM_API
    GfMatrix4d ComputeLocalToWorldTransform(UsdTimeCode const &time) const;

    /// Compute the transformation matrix for this prim at the given time,
    /// \em NOT including the transform authored on the prim itself.
    ///
    /// <b>If you need to compute the transform for multiple prims on a
    /// stage, it will be much, much more efficient to instantiate a
    /// UsdGeomXformCache and query it directly; doing so will reuse
    /// sub-computations shared by the prims.</b>
    USDGEOM_API
    GfMatrix4d ComputeParentToWorldTransform(UsdTimeCode const &time) const;

    /// @}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
