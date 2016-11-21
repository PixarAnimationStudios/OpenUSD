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
#ifndef USD_STAGE_H
#define USD_STAGE_H


#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/work/dispatcher.h"

#include <boost/mpl/assert.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/spin_rw_mutex.h>

#include <string>
#include <unordered_map>
#include <utility>

class ArResolverContext;
class GfInterval;
class SdfAbstractDataValue;
class Usd_ClipCache;
class Usd_InstanceCache;
class Usd_InstanceChanges;
class Usd_InterpolatorBase;
class UsdResolveInfo;
class Usd_Resolver;
class UsdTreeIterator;

SDF_DECLARE_HANDLES(SdfLayer);

/// \class UsdStage
///
/// The outermost container for scene description, which owns and presents
/// composed prims as a scenegraph, following the composition recipe
/// recursively described in its associated "root layer".
///
/// USD derives its persistent-storage scalability by combining and reusing
/// simple compositions into richer aggregates using referencing and layering
/// with sparse overrides.  Ultimately, every composition (i.e. "scene") is
/// identifiable by its root layer, i.e. the <tt>.usd</tt> file, and a scene
/// is instantiated in an application on a UsdStage that presents a composed
/// view of the scene's root layer.  Each simple composition referenced into
/// a larger composition could be presented on its own UsdStage, at the same
/// (or not) time that it is participating in the larger composition on its
/// own UsdStage; all of the underlying layers will be shared by the two
/// stages, while each maintains its own scenegraph of composed prims.
///
/// A UsdStage has sole ownership over the UsdPrim 's with which it is populated,
/// and retains \em shared ownership (with other stages and direct clients of
/// SdfLayer's, via the Sdf_LayerRegistry that underlies all SdfLayer creation
/// methods) of layers.  It provides roughly five categories of API that
/// address different aspects of scene management:
///
/// - \ref Usd_lifetimeManagement "Stage lifetime management" methods for
/// constructing and initially populating a UsdStage from an existing layer
/// file, or one that will be created as a result, in memory or on the 
/// filesystem.
/// - \ref Usd_workingSetManagement "Load/unload working set management" methods
/// that allow you to specify which \ref Usd_Payloads "payloads" should be
/// included and excluded from the stage's composition.
/// - \ref Usd_variantManagement "Variant management" methods to manage
/// policy for which variant to use when composing prims that provide
/// a named variant set, but do not specify a selection.
/// - \ref Usd_primManagement "Prim access, creation, and mutation" methods
/// that allow you to find, create, or remove a prim identified by a path on
/// the stage.  This group also provides methods for efficiently traversing the
/// prims on the stage.
/// - \ref Usd_layerManagement "Layers and EditTargets" methods provide access
/// to the layers in the stage's <em>root LayerStack</em> (i.e. the root layer
/// and all of its recursive sublayers), and the ability to set a UsdEditTarget
/// into which all subsequent mutations to objects associated with the stage
/// (e.g. prims, properties, etc) will go.
/// - \ref Usd_stageSerialization "Serialization" methods for "flattening" a
/// composition (to varying degrees), and exporting a completely flattened
/// view of the stage to a string or file.  These methods can be very useful
/// for targetted asset optimization and debugging, though care should be 
/// exercized with large scenes, as flattening defeats some of the benefits of
/// referenced scene description, and may produce very large results, 
/// especially in file formats that do not support data de-duplication, like
/// the usda ASCII format!
///
/// \section Usd_SessionLayer Stage Session Layers
///
/// Each UsdStage can possess an optional "session layer".  The purpose of
/// a session layer is to hold ephemeral edits that modify a UsdStage's contents
/// or behavior in a way that is useful to the client, but should not be
/// considered as permanent mutations to be recorded upon export.  A very 
/// common use of session layers is to make variant selections, to pick a
/// specific LOD or shading variation, for example.  The session layer is
/// also freqenuently used to perform interactive vising/invsning of geometry 
/// and assets in the scene.   A session layer, if present, contributes to a 
/// UsdStage's identity, for purposes of stage-caching, etc.
///
class UsdStage : public TfRefBase, public TfWeakBase {
public:

    // --------------------------------------------------------------------- //
    /// \anchor Usd_lifetimeManagement
    /// \name Lifetime Management
    /// @{
    // --------------------------------------------------------------------- //

    /// Create a new stage with root layer \p identifier, destroying
    /// potentially existing files with that identifer; it is considered an
    /// error if an existing, open layer is present with this identifier.
    ///
    /// \sa SdfLayer::CreateNew()
    ///
    /// Invoking an overload that does not take a \p sessionLayer argument will
    /// create a stage with an anonymous in-memory session layer.  To create a
    /// stage without a session layer, pass TfNullPtr (or None in python) as the
    /// \p sessionLayer argument.
    ///
    /// Note that the \p pathResolverContext passed here will apply to all path
    /// resolutions for this stage, regardless of what other context may be
    /// bound at resolve time. If no context is passed in here, Usd will create
    /// one by calling \sa ArResolver::CreateDefaultContextForAsset with the
    /// root layer's repository path if the layer has one, otherwise its real 
    /// path.
	USD_API static UsdStageRefPtr
    CreateNew(const std::string& identifier);
    /// \overload
	USD_API static UsdStageRefPtr
    CreateNew(const std::string& identifier,
              const SdfLayerHandle& sessionLayer);
    /// \overload
	USD_API static UsdStageRefPtr
    CreateNew(const std::string& identifier,
              const SdfLayerHandle& sessionLayer,
              const ArResolverContext& pathResolverContext);
    /// \overload
	USD_API static UsdStageRefPtr
    CreateNew(const std::string& identifier,
              const ArResolverContext& pathResolverContext);

    /// Creates a new stage only in memory, analogous to creating an
    /// anonymous SdfLayer.
    ///
    /// Note that the \p pathResolverContext passed here will apply to all path
    /// resolutions for this stage, regardless of what other context may be
    /// bound at resolve time. If no context is passed in here, Usd will create
    /// one by calling \sa ArResolver::CreateDefaultContext.
    ///
    /// Invoking an overload that does not take a \p sessionLayer argument will
    /// create a stage with an anonymous in-memory session layer.  To create a
    /// stage without a session layer, pass TfNullPtr (or None in python) as the
    /// \p sessionLayer argument.
	USD_API static UsdStageRefPtr
    CreateInMemory();
    /// \overload
	USD_API static UsdStageRefPtr
    CreateInMemory(const std::string& identifier);
    /// \overload
	USD_API static UsdStageRefPtr
    CreateInMemory(const std::string& identifier,
                   const ArResolverContext& pathResolverContext);
    /// \overload
	USD_API static UsdStageRefPtr
    CreateInMemory(const std::string& identifier,
                   const SdfLayerHandle &sessionLayer);
    /// \overload
	USD_API static UsdStageRefPtr
    CreateInMemory(const std::string& identifier,
                   const SdfLayerHandle &sessionLayer,
                   const ArResolverContext& pathResolverContext);

    /// \enum InitialLoadSet
    ///
    /// Specifies the initial set of prims to load when opening a UsdStage.
    ///
    enum InitialLoadSet
    {
        LoadAll, ///< Load all loadable prims
        LoadNone ///< Load no loadable prims
    };

    /// Attempts to find an existing stage in a cache if UsdStageCacheContext
    /// objects exist on the stack. Failing that, creates a new stage and
    /// recursively loads all data within and referenced by the layer found at
    /// \p filePath, which must be a file that already exists.
    ///
    /// The initial set of prims to load on the stage can be specified
    /// using the \p load parameter. \sa UsdStage::InitialLoadSet.
    ///
    /// Note that the \p pathResolverContext passed here will apply to all path
    /// resolutions for this stage, regardless of what other context may be
    /// bound at resolve time. If no context is passed in here, Usd will create
    /// one by calling \sa ArResolver::CreateDefaultContextForAsset with the
    /// root layer's repository path if the layer has one, otherwise its real 
    /// path.
	USD_API static UsdStageRefPtr
    Open(const std::string& filePath, 
         InitialLoadSet load = LoadAll);
    /// \overload
	USD_API static UsdStageRefPtr
    Open(const std::string& filePath,
         const ArResolverContext& pathResolverContext,
         InitialLoadSet load = LoadAll);

    /// Open a stage rooted at \p rootLayer.
    ///
    /// Attempt to find a stage that matches the passed arguments in a
    /// UsdStageCache if UsdStageCacheContext objects exist on the calling
    /// stack.  If a matching stage is found, return that stage.  Otherwise,
    /// create a new stage rooted at \p rootLayer.
    ///
    /// Invoking an overload that does not take a \p sessionLayer argument will
    /// create a stage with an anonymous in-memory session layer.  To create a
    /// stage without a session layer, pass TfNullPtr (or None in python) as the
    /// \p sessionLayer argument.
    ///
    /// The initial set of prims to load on the stage can be specified
    /// using the \p load parameter. \sa UsdStage::InitialLoadSet.
    ///
    /// Note that the \p pathResolverContext passed here will apply to all path
    /// resolutions for this stage, regardless of what other context may be
    /// bound at resolve time. If no context is passed in here, Usd will create
    /// one by calling \sa ArResolver::CreateDefaultContextForAsset with the
    /// root layer's repository path if the layer has one, otherwise its real 
    /// path.
    ///
    /// When searching for a matching stage in bound UsdStageCache s, only the
    /// provided arguments matter for cache lookup.  For example, if only a root
    /// layer (or a root layer file path) is provided, the first stage found in
    /// any cache that has that root layer is returned.  So, for example if you
    /// require that the stage have no session layer, you must explicitly
    /// specify TfNullPtr (or None in python) for the sessionLayer argument.
	USD_API static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         InitialLoadSet load=LoadAll);
    /// \overload
	USD_API static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         const SdfLayerHandle& sessionLayer,
         InitialLoadSet load=LoadAll);
    /// \overload
	USD_API static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         const ArResolverContext& pathResolverContext,
         InitialLoadSet load=LoadAll);
    /// \overload
	USD_API static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         const SdfLayerHandle& sessionLayer,
         const ArResolverContext& pathResolverContext,
         InitialLoadSet load=LoadAll);

	USD_API virtual ~UsdStage();

	USD_API void Close();

    /// Calls SdfLayer::Reload on all layers contributing to this stage,
    /// except session layers and sublayers of session layers.
    ///
    /// This includes non-session sublayers, references and payloads.
    /// Note that reloading anonymous layers clears their content, so
    /// invoking Reload() on a stage constructed via CreateInMemory()
    /// will clear its root layer.
	USD_API void Reload();

    /// Indicates whether the specified file is supported by UsdStage.
    ///
    /// This function is a cheap way to determine whether a
    /// file might be open-able with UsdStage::Open. It is
    /// purely based on the given \p filePath and does not
    /// open the file or perform analysis on the contents.
    /// As such, UsdStage::Open may still fail even if this
    /// function returns true.
	USD_API static bool
    IsSupportedFile(const std::string& filePath);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_variantManagement
    /// \name Variant Management
    ///
    /// These methods provide control over the policy to use when composing
    /// prims that specify a variant set but do not specify a selection.
    ///
    /// For example, a model might provide a "shadingComplexity" variant
    /// set, providing multiple levels of detail or fidelity in shading.
    /// Some sites of use might specify a particular selection, but others
    /// may want to use a standardized default.  There are two ways to
    /// specify a default.
    ///
    /// The first is to declare a list of preferences in plugInfo.json
    /// metadata on a plugin using this structure:
    ///
    /// \code{.json}
    ///     "UsdVariantFallbacks": {    # top level key
    ///         "shadingComplexity": [  # example variant set
    ///             "full",             # example fallback #1
    ///             "light"             # example fallback #2
    ///         ]
    ///     },
    /// \endcode
    ///
    /// This example ensures that we will get the full shadingComplexity
    /// for any prim with that variant set that doesn't otherwise specify
    /// a selection.
    ///
    /// The plugin metadata is discovered and applied before the first
    /// UsdStage is constructed in a given process.  It can be defined
    /// in any plugin.  However, if multiple plugins express contrary
    /// lists for the same named variant set, the result is undefined.
    /// 
    /// The plugin metadata approach is useful for ensuring that default
    /// sensible behavior applies across a pipeline without requiring
    /// explicit proper configuration in every script, binary, etc.
    /// There may be times when you want to override this behavior in a
    /// particular script -- for example, a pipeline script that knows
    /// it wants to entirely ignore shading in order to minimize
    /// processing time -- which motivates the second approach.
    ///
    /// SetGlobalVariantFallbacks() provides a way to override, for
    /// the entire process, which fallbacks to use in subsequently
    /// constructed UsdStage instances.
    ///
    /// @{

    /// Get the global variant fallback preferences used in new UsdStages.
	USD_API static PcpVariantFallbackMap GetGlobalVariantFallbacks();

    /// Set the global variant fallback preferences used in new
    /// UsdStages. This overrides any defaults configured in plugin
    /// metadata, and only affects stages created after this call.
    ///
    /// \note This does not affect existing UsdStages.
	USD_API static void
    SetGlobalVariantFallbacks(const PcpVariantFallbackMap &fallbacks);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_workingSetManagement
    /// \name Working Set Management
    ///
    /// The following rules apply to all Load/Unload methods:
    ///
    ///     - Loading an already loaded prim is legal, though may result in
    ///       some recomposition cost. Similarly, unloading an unloaded prim
    ///       is legal.
    ///     - Specifying a path that does not target a prim is legal as long it
    ///       has at least one ancestor in the scene graph (not including the
    ///       absolute root). If the given path has no ancestors, it is an
    ///       error.
    ///     - Loading an inactive prim is an error.
    ///     - Loading a master prim is an error. However, note that loading
    ///       a prim in a master is legal.
    /// @{
    // --------------------------------------------------------------------- //

    /// Load the prim at \p path along with all descendants, ancestors
    /// and dependencies (relationship targets).
    ///
    /// If an instance prim is encountered during this operation, this
    /// function will also load prims in the instance's master. In other
    /// words, loading a single instance may affect other instances because
    /// it changes the load state of prims in the shared master. However, 
    /// loading a single instance will never cause other instances to be
    /// loaded as well.
    ///
    /// See the rules under
    /// \ref Usd_workingSetManagement "Working Set Management" for a discussion
    /// of what paths are considered valid.
	USD_API UsdPrim
    Load(const SdfPath& path=SdfPath::AbsoluteRootPath());

    /// Unload the prim and its descendants specified by \p path.
    ///
    /// If an instance prim is encountered during this operation, this
    /// function will also unload prims in the instance's master. In other
    /// words, unloading a single instance may affect other instances because
    /// it changes the load state of prims in the shared master. However, 
    /// unloading a single instance will never cause other instances to be
    /// unloaded as well.
    ///
    /// See the rules under
    /// \ref Usd_workingSetManagement "Working Set Management" for a discussion
    /// of what paths are considered valid.
	USD_API void
    Unload(const SdfPath& path=SdfPath::AbsoluteRootPath());

    /// Unloads and loads the given path sets; the effect is as if the
    /// unload set were processed first followed by the load set.
    ///
    /// This is equivalent to calling UsdStage::Unload for each item in the
    /// unloadSet followed by UsdStage::Load for each item in the loadSet,
    /// however this method is more efficient as all operations are committed
    /// in a single batch.
    ///
    /// See the rules under
    /// \ref Usd_workingSetManagement "Working Set Management" for a discussion
    /// of what paths are considered valid.
	USD_API 
    void LoadAndUnload(const SdfPathSet &loadSet, const SdfPathSet &unloadSet);

    /// Returns a set of all loaded paths.
    ///
    /// The paths returned are both those that have been explicitly loaded and
    /// those that were loaded as a result of dependencies, ancestors or
    /// descendants of explicitly loaded paths.
    ///
    /// This method does not return paths to inactive prims.
	USD_API SdfPathSet GetLoadSet();

    /// Returns an SdfPathSet of all paths that can be loaded.
    ///
    /// Note that this method does not return paths to inactive prims as they
    /// cannot be loaded.
    ///
    /// The set returned includes loaded and unloaded paths. To determine the
    /// set of unloaded paths, one can diff this set with the current load set,
    /// for example:
    /// \code
    /// SdfPathSet loaded = stage->GetLoadSet(),
    ///            all = stage->FindLoadable(),
    ///            result;
    /// std::set_difference(loaded.begin(), loaded.end(),
    ///                     all.begin(), all.end(),
    ///                     std::inserter(result, result.end()));
    /// \endcode
	USD_API SdfPathSet FindLoadable(
                         const SdfPath& rootPath = SdfPath::AbsoluteRootPath());

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_primManagement
    /// \name Prim Access, Creation and Mutation
    /// All of the methods in this group that accept a prim path as argument
    /// require paths in the namespace of the stage's root layer, \em regardless
    /// of what the currently active UsdEditTarget is set to.  In other words,
    /// a UsdStage always presents a composed view of its scene, and all
    /// prim operations are specified in the composed namespace.
    /// @{
    // --------------------------------------------------------------------- //

    /// Return the stage's "pseudo-root" prim, whose name is defined by Usd.
    ///
    /// The stage's named root prims are namespace children of this prim,
    /// which exists to make the namespace hierarchy a tree instead of a
    /// forest.  This simplifies algorithms that want to traverse all prims.
    ///
    /// A UsdStage always has a pseudo-root prim, unless there was an error
    /// opening or creating the stage, in which case this method returns
    /// an invalid UsdPrim.
	USD_API UsdPrim GetPseudoRoot() const;

    /// Return the root UsdPrim on this stage whose name is the root layer's
    /// defaultPrim metadata's value.  Return an invalid prim if there is no
    /// such prim or if the root layer's defaultPrim metadata is unset or is not
    /// a valid prim name.  Note that this function only examines this stage's
    /// rootLayer.  It does not consider sublayers of the rootLayer.  See also
    /// SdfLayer::GetDefaultPrim().
	USD_API UsdPrim GetDefaultPrim() const;

    /// Set the default prim layer metadata in this stage's root layer.  This is
    /// shorthand for:
    /// \code
    /// stage->GetRootLayer()->SetDefaultPrim(prim.GetName());
    /// \endcode
    /// Note that this function always authors to the stage's root layer.  To
    /// author to a different layer, use the SdfLayer::SetDefaultPrim() API.
	USD_API void SetDefaultPrim(const UsdPrim &prim);
    
    /// Clear the default prim layer metadata in this stage's root layer.  This
    /// is shorthand for:
    /// \code
    /// stage->GetRootLayer()->ClearDefaultPrim();
    /// \endcode
    /// Note that this function always authors to the stage's root layer.  To
    /// author to a different layer, use the SdfLayer::SetDefaultPrim() API.
	USD_API void ClearDefaultPrim();

    /// Return true if this stage's root layer has an authored opinion for the
    /// default prim layer metadata.  This is shorthand for:
    /// \code
    /// stage->GetRootLayer()->HasDefaultPrim();
    /// \endcode
    /// Note that this function only consults the stage's root layer.  To
    /// consult a different layer, use the SdfLayer::HasDefaultPrim() API.
	USD_API bool HasDefaultPrim() const;

    /// Return the already extant UsdPrim at \p path, or an invalid UsdPrim if
    /// none exists.
    ///
    /// Unlike OverridePrim() and DefinePrim(), this method will never author
    /// scene description, and therefore is safe to use as a "reader" in the Usd
    /// multi-threading model.
	USD_API UsdPrim GetPrimAtPath(const SdfPath &path) const;

private:
    // Return the primData object at \p path.
    Usd_PrimDataConstPtr _GetPrimDataAtPath(const SdfPath &path) const;
    Usd_PrimDataPtr _GetPrimDataAtPath(const SdfPath &path);

    // A helper function for LoadAndUnload to aggregate notification data
    void _LoadAndUnload(const SdfPathSet&, const SdfPathSet&, 
                        SdfPathSet*, SdfPathSet*);

public:

    /// Traverse the active, loaded, defined, non-abstract prims on this stage
    /// depth-first.
    ///
    /// Traverse() returns a UsdTreeIterator , which allows low-latency
    /// traversal, with the ability to prune subtrees from traversal.  It
    /// is python iterable, so in its simplest form, one can do:
    ///
    /// \code{.py}
    /// for prim in stage.Traverse():
    ///     print prim.GetPath()
    /// \endcode
    ///
    /// If either a pre-and-post-order traversal or a traversal rooted at a
    /// particular prim is desired, construct a UsdTreeIterator directly.
    ///
    /// This is equivalent to UsdTreeIterator::Stage() . 
	USD_API UsdTreeIterator Traverse();

    /// \overload
    /// Traverse the prims on this stage subject to \p predicate.
    ///
    /// This is equivalent to UsdTreeIterator::Stage() .
	USD_API 
	UsdTreeIterator Traverse(const Usd_PrimFlagsPredicate &predicate);

    /// Traverse all the prims on this stage depth-first.
    ///
    /// \sa Traverse()
    /// \sa UsdTreeIterator::Stage()
	USD_API UsdTreeIterator TraverseAll();

    /// Attempt to ensure a \a UsdPrim at \p path exists on this stage.
    ///
    /// If a prim already exists at \p path, return it.  Otherwise author
    /// \a SdfPrimSpecs with \a specifier == \a SdfSpecifierOver and empty
    /// \a typeName at the current EditTarget to create this prim and any
    /// nonexistent ancestors, then return it.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// If an ancestor of \p path identifies an \a inactive prim, author scene
    /// description as described above but return an invalid prim, since the
    /// resulting prim is descendant to an inactive prim.
    ///
	USD_API UsdPrim OverridePrim(const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim at \p path is defined (according to
    /// UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim at \p path is already defined on this stage and \p typeName is
    /// empty or equal to the existing prim's typeName, return that prim.
    /// Otherwise author an \a SdfPrimSpec with \a specifier ==
    /// \a SdfSpecifierDef and \p typeName for the the prim at \p path at the
    /// current EditTarget.  Author \a SdfPrimSpec s with \p specifier ==
    /// \a SdfSpecifierDef and empty typeName at the current EditTarget for any
    /// nonexistent, or existing but not \a Defined ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace or one of the ancestors of \p path is inactive on the 
    /// UsdStage), issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// match the supplied \p typeName, in case a stronger typeName opinion
    /// overrides the opinion at the current EditTarget.
    ///
	USD_API 
    UsdPrim DefinePrim(const SdfPath &path,
                       const TfToken &typeName=TfToken());

    /// Author an \a SdfPrimSpec with \a specifier == \a SdfSpecifierClass for
    /// the class at root prim path \p path at the current EditTarget.  The
    /// current EditTarget must have UsdEditTarget::IsLocalLayer() == true.
    ///
    /// The given \a path must be an absolute, root prim path that does not
    /// contain any variant selections.
    ///
    /// If a defined (UsdPrim::IsDefined()) non-class prim already exists at
    /// \p path, issue an error and return an invalid UsdPrim.
    ///
    /// If it is impossible to author the necessary PrimSpec, issue an error
    /// and return an invalid \a UsdPrim.
	USD_API UsdPrim CreateClassPrim(const SdfPath &rootPrimPath);

    /// Remove all scene description for the given \p path and its subtree
    /// <em>in the current UsdEditTarget</em>.
    ///
    /// This method does not do what you might initially think!  Calling this
    /// function will not necessarily cause the UsdPrim at \p path on this
    /// stage to disappear.  Completely eradicating a prim from a composition
    /// can be an involved process, involving edits to many contributing layers,
    /// some of which (in many circumstances) will not be editable by a client.
    /// This method is a surgical instrument that \em can be used iteratively
    /// to effect complete removal of a prim and its subtree from namespace,
    /// assuming the proper permissions are acquired, but more commonly it
    /// is used to perform layer-level operations; e.g.: ensuring that a given
    /// layer (as expressed by a UsdEditTarget) provides no opinions for a
    /// prim and its subtree.
    ///
    /// Generally, if your eye is attracted to this method, you probably want
    /// to instead use UsdPrim::SetActive(false) , which will provide the
    /// \ref Usd_ActiveInactive "composed effect" of removing the prim and
    /// its subtree from the composition, without actually removing any
    /// scene description, which as a bonus, means that the effect is 
    /// reversible at a later time!
	USD_API bool RemovePrim(const SdfPath& path);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_layerManagement
    /// \name Layers and EditTargets
    /// @{
    // --------------------------------------------------------------------- //

    /// Return this stage's root session layer.
	USD_API SdfLayerHandle GetSessionLayer() const;

    /// Return this stage's root layer.
	USD_API SdfLayerHandle GetRootLayer() const;

    /// Return the path resolver context for all path resolution during
    /// composition of this stage. Useful for external clients that want to
    /// resolve paths with the same context as this stage, or create new
    /// stages with the same context.
	USD_API ArResolverContext GetPathResolverContext() const;

    /// Resolve the given identifier using this stage's 
    /// ArResolverContext and the layer of its GetEditTarget()
    /// as an anchor for relative references (e.g. \@./siblingFile.usd\@).   
    ///
    /// \return a non-empty string containing either the same
    /// identifier that was passed in (if the identifier refers to an
    /// already-opened layer or an "anonymous", in-memory layer), or a resolved
    /// layer filepath.  If the identifier was not resolvable, return the
    /// empty string.
	USD_API std::string
    ResolveIdentifierToEditTarget(std::string const &identifier) const;

    /// Return this stage's local layers in strong-to-weak order.  If
    /// \a includeSessionLayers is true, return the linearized strong-to-weak
    /// sublayers rooted at the stage's session layer followed by the linearized
    /// strong-to-weak sublayers rooted at this stage's root layer.  If
    /// \a includeSessionLayers is false, omit the sublayers rooted at this
    /// stage's session layer.
	USD_API 
    SdfLayerHandleVector GetLayerStack(bool includeSessionLayers=true) const;

    /// Return a vector of all of the layers \em currently consumed by this
    /// stage, as determined by the composition arcs that were traversed to
    /// compose and populate the stage.
    ///
    /// The list of consumed layers will change with the stage's load-set and
    /// variant selections, so the return value should be considered only
    /// a snapshot.  The return value will include the stage's session layer,
    /// if it has one.
	USD_API SdfLayerHandleVector GetUsedLayers() const;

    /// Return true if \a layer is one of the layers in this stage's local,
    /// root layerStack.
	USD_API bool HasLocalLayer(const SdfLayerHandle &layer) const;
    
    /// Return the stage's EditTarget.
	USD_API const UsdEditTarget &GetEditTarget() const;

    /// Set the stage's EditTarget.  If \a editTarget.IsLocalLayer(), check to
    /// see if it's a layer in this stage's local LayerStack.  If not, issue an
    /// error and do nothing.  If \a editTarget is invalid, issue an error
    /// and do nothing.  If \a editTarget differs from the stage's current
    /// EditTarget, set the EditTarget and send
    /// UsdNotice::StageChangedEditTarget.  Otherwise do nothing.
	USD_API void SetEditTarget(const UsdEditTarget &editTarget);

    /// Mute the layer identified by \p layerIdentifier.  Muted layers are
    /// ignored by the stage; they do not participate in value resolution
    /// or composition and do not appear in any LayerStack.  If the root 
    /// layer of a reference or payload LayerStack is muted, the behavior 
    /// is as if the muted layer did not exist, which means a composition 
    /// error will be generated.
    ///
    /// A canonical identifier for \p layerIdentifier will be
    /// computed using ArResolver::ComputeRepositoryPath.  Any layer 
    /// encountered during composition with the same repository path will
    /// be considered muted and ignored.  Relative paths will be assumed 
    /// to be relative to the cache's root layer.  Search paths are immediately 
    /// resolved and the result is used for computing the canonical path.
    ///
    /// Note that muting a layer will cause this stage to release all
    /// references to that layer.  If no other client is holding on to
    /// references to that layer, it will be unloaded.  In this case, if 
    /// there are unsaved edits to the muted layer, those edits are lost.  
    /// Since anonymous layers are not serialized, muting an anonymous
    /// layer will cause that layer and its contents to be lost in this
    /// case.
    ///
    /// Muting a layer that has not been used by this stage is not an error.
    /// If that layer is encountered later, muting will take effect and that
    /// layer will be ignored.  
    ///
    /// The root layer of this stage may not be muted; attempting to do so
    /// will generate a coding error.
	USD_API void MuteLayer(const std::string &layerIdentifier);

    /// Unmute the layer identified by \p layerIdentifier if it had
    /// previously been muted.
	USD_API void UnmuteLayer(const std::string &layerIdentifier);

    /// Mute and unmute the layers identified in \p muteLayers and
    /// \p unmuteLayers.  
    ///
    /// This is equivalent to calling UsdStage::UnmuteLayer for each layer 
    /// in \p unmuteLayers followed by UsdStage::MuteLayer for each layer 
    /// in \p muteLayers, however this method is more efficient as all
    /// operations are committed in a single batch.
	USD_API 
    void MuteAndUnmuteLayers(const std::vector<std::string> &muteLayers,
                             const std::vector<std::string> &unmuteLayers);

    /// Returns a vector of all layers that have been muted on this stage.
	USD_API const std::vector<std::string>& GetMutedLayers() const;

    /// Returns true if the layer specified by \p layerIdentifier is
    /// muted in this cache, false otherwise.  See documentation on
    /// MuteLayer for details on how \p layerIdentifier is compared to the 
    /// layers that have been muted.
	USD_API bool IsLayerMuted(const std::string& layerIdentifier) const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_stageSerialization
    /// \name Flatten & Export Utilities
    /// @{
    // --------------------------------------------------------------------- //
    // Future Work:
    //    * Flatten sub-trees or individual prims
    //    * Allow flattening of local LayerStack
    //    * Move Flatten into a free-function to ensure it doesn't rely on
    //      Stage internals.

    /// Writes out the composite scene as a single flattened layer into
    /// \a filename.
    ///
    /// If addSourceFileComment is true, a comment in the output layer
    /// will mention the input layer it was generated from.
    ///
    /// See UsdStage::Flatten for details of the flattening transformation.
	USD_API 
    bool Export(const std::string &filename,
                bool addSourceFileComment=true,
                const SdfLayer::FileFormatArguments &args = 
                    SdfLayer::FileFormatArguments()) const;

    /// Writes the composite scene as a flattened Usd text
    /// representation into the given \a string.
    ///
    /// If addSourceFileComment is true, a comment in the output layer
    /// will mention the input layer it was generated from.
    ///
    /// See UsdStage::Flatten for details of the flattening transformation.
	USD_API 
    bool ExportToString(std::string *result,
                        bool addSourceFileComment=true) const;

    /// Returns a single, anonymous, merged layer for this composite
    /// scene.
    ///
    /// Specifically, this function removes **most** composition metadata and
    /// authors the resolved values for each object directly into the flattend
    /// layer.
    ///
    /// All VariantSets are removed and only the currently selected variants
    /// will be present in the resulting layer.
    ///
    /// Class prims will still exist, however all inherits arcs will have
    /// been removed and the inherited data will be copied onto each child
    /// object. Composition arcs authored on the class itself will be flattend
    /// into the class.
    ///
    /// Flatten preserves \ref Usd_ExplicitInstancing "stage-level instancing"
    /// by creating indepedent roots for each master currently composed on
    /// this stage, and adding a single internal reference arc on each 
    /// instance prim to its corresponding master.
    ///
    /// Time samples across sublayer offsets will will have the time offset and
    /// scale applied to each time index.
    ///
    /// Finally, any deactivated prims will be pruned from the result.
    ///
	USD_API SdfLayerRefPtr Flatten(bool addSourceFileComment=true) const;
    /// @}

public:
    // --------------------------------------------------------------------- //
    /// \anchor Usd_stageMetadata
    /// \name Stage Metadata
    /// Stage metadata applies to the entire contents of the stage, and is
    /// recorded only in the stage's root or primary session-layer.  Most of
    /// the other, specific metadata methods on UsdStage are defined in terms
    /// of these generic methods.
    /// @{
    // --------------------------------------------------------------------- //

    /// Return in \p value an authored or fallback value (if one was defined
    /// for the given metadatum) for Stage metadatum \p key.  Order of
    /// resolution is session layer, followed by root layer, else fallback to
    /// the SdfSchema.
    ///
    /// \return true if we succesfully retrieved a value of the requested type;
    /// false if \p key is not allowed as layer metadata or no value was found.
    /// Generates a coding error if we retrieved a stored value of a type other
    /// than the requested type
    ///
    /// \sa \ref Usd_OM_Metadata
    template <class T>
    bool GetMetadata(const TfToken &key, T *value) const;
    /// \overload
	USD_API bool GetMetadata(const TfToken &key, VtValue *value) const;

    /// Returns true if the \a key has a meaningful value, that is, if
    /// GetMetadata() will provide a value, either because it was authored
    /// or because the Stage metadata was defined with a meaningful fallback 
    /// value.
    ///
    /// Returns false if \p key is not allowed as layer metadata.
	USD_API bool HasMetadata(const TfToken &key) const;

    /// Returns \c true if the \a key has an authored value, \c false if no
    /// value was authored or the only value available is the SdfSchema's
    /// metadata fallback.
    ///
    /// \note If a value for a metadatum \em not legal to author on layers 
    /// is present in the root or session layer (which could happen through
    /// hand-editing or use of certain low-level API's), this method will
    /// still return \c false.
	USD_API bool HasAuthoredMetadata(const TfToken &key) const;

    /// Set the value of Stage metadatum \p key to \p value, if the stage's
    /// current UsdEditTarget is the root or session layer.
    ///
    /// If the current EditTarget is any other layer, raise a coding error.
    /// \return true if authoring was successful, false otherwise.
    /// Generates a coding error if \p key is not allowed as layer metadata.
    ///
    /// \sa \ref Usd_OM_Metadata
    template<typename T>
    bool SetMetadata(const TfToken &key, const T &value) const;
    /// \overload
	USD_API bool SetMetadata(const TfToken &key, const VtValue &value) const;

    /// Clear the value of stage metadatum \p key, if the stage's
    /// current UsdEditTarget is the root or session layer.
    ///
    /// If the current EditTarget is any other layer, raise a coding error.
    /// \return true if authoring was successful, false otherwise.
    /// Generates a coding error if \p key is not allowed as layer metadata.
    ///
    /// \sa \ref Usd_OM_Metadata
	USD_API bool ClearMetadata(const TfToken &key) const;

    /// Resolve the requested dictionary sub-element \p keyPath of
    /// dictionary-valued metadatum named \p key, returning the resolved
    /// value.
    ///
    /// If you know you need just a small number of elements from a dictionary,
    /// accessing them element-wise using this method can be much less
    /// expensive than fetching the entire dictionary with GetMetadata(key).
    ///
    /// \return true if we succesfully retrieved a value of the requested type;
    /// false if \p key is not allowed as layer metadata or no value was found.
    /// Generates a coding error if we retrieved a stored value of a type other
    /// than the requested type
    ///
    /// The \p keyPath is a ':'-separated path addressing an element
    /// in subdictionaries.  If \p keyPath is empty, returns an empty VtValue.
    template<typename T>
    bool GetMetadataByDictKey(const TfToken& key, const TfToken &keyPath, 
                              T* value) const;
    /// overload
	USD_API bool GetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, VtValue *value) const;

    /// Return true if there exists any authored or fallback opinion for
    /// \p key and \p keyPath.
    ///
    /// The \p keyPath is a ':'-separated path identifying a value in
    /// subdictionaries stored in the metadata field at \p key.  If
    /// \p keyPath is empty, returns \c false.
    ///
    /// Returns false if \p key is not allowed as layer metadata.
    ///
    /// \sa \ref Usd_Dictionary_Type
	USD_API bool HasMetadataDictKey(
        const TfToken& key, const TfToken &keyPath) const;

    /// Return true if there exists any authored opinion (excluding
    /// fallbacks) for \p key and \p keyPath.  
    ///
    /// The \p keyPath is a ':'-separated path identifying a value in
    /// subdictionaries stored in the metadata field at \p key.  If 
    /// \p keyPath is empty, returns \c false.
    ///
    /// \sa \ref Usd_Dictionary_Type
	USD_API bool HasAuthoredMetadataDictKey(
        const TfToken& key, const TfToken &keyPath) const;

    /// Author \p value to the field identified by \p key and \p keyPath
    /// at the current EditTarget.
    ///
    /// The \p keyPath is a ':'-separated path identifying a value in
    /// subdictionaries stored in the metadata field at \p key.  If 
    /// \p keyPath is empty, no action is taken.
    ///
    /// \return true if the value is authored successfully, false otherwise.
    /// Generates a coding error if \p key is not allowed as layer metadata.
    ///
    /// \sa \ref Usd_Dictionary_Type
    template<typename T>
    bool SetMetadataByDictKey(const TfToken& key, const TfToken &keyPath, 
                              const T& value) const;
    /// \overload
	USD_API bool SetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, const VtValue& value) const;

    /// Clear any authored value identified by \p key and \p keyPath
    /// at the current EditTarget.
    ///
    /// The \p keyPath is a ':'-separated path identifying a path in
    /// subdictionaries stored in the metadata field at \p key.  If
    /// \p keyPath is empty, no action is taken.
    ///
    /// \return true if the value is cleared successfully, false otherwise.
    /// Generates a coding error if \p key is not allowed as layer metadata.
    ///
    /// \sa \ref Usd_Dictionary_Type
	USD_API bool ClearMetadataByDictKey(
        const TfToken& key, const TfToken& keyPath) const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_timeCodeAPI
    /// \name TimeCode API
    /// Methods for managing the Stage's active timeSample range, time units,
    /// and intended rate of playback.  See \ref Usd_OM_UsdTimeCode for more
    /// on time and TimeCodes in USD.
    /// @{
    // --------------------------------------------------------------------- //
    /// Returns the stage's start timeCode. If the stage has an associated
    /// session layer with a start timeCode opinion, this value is returned. 
    /// Otherwise, the start timeCode opinion from the root layer is returned.
	USD_API double GetStartTimeCode() const;

    /// Sets the stage's start timeCode. 
    /// 
    /// The start timeCode is set in the current EditTarget, if it is the root 
    /// layer of the stage or the session layer associated with the stage. If 
    /// the current EditTarget is neither, a warning is issued and the start 
    /// timeCode is not set.
	USD_API void SetStartTimeCode(double);

    /// Returns the stage's end timeCode. If the stage has an associated
    /// session layer with an end timeCode opinion, this value is returned. 
    /// Otherwise, the end timeCode opinion from the root layer is returned.
	USD_API double GetEndTimeCode() const;

    /// Sets the stage's end timeCode. 
    /// 
    /// The end timeCode is set in the current EditTarget, if it is the root 
    /// layer of the stage or the session layer associated with the stage. If 
    /// the current EditTarget is neither, a warning is issued and the end 
    /// timeCode is not set.
	USD_API void SetEndTimeCode(double);

    /// Returns true if the stage has both start and end timeCodes 
    /// authored in the session layer or the root layer of the stage.
	USD_API bool HasAuthoredTimeCodeRange() const;

    /// Returns the stage's timeCodesPerSecond value.
    /// 
    /// The timeCodesPerSecond value scales the time ordinate for the samples
    /// contained in the stage to seconds. If timeCodesPerSecond is 24, then a 
    /// sample at time ordinate 24 should be viewed exactly one second after the 
    /// sample at time ordinate 0. 
    ///
    /// The default value of timeCodesPerSecond is 24.
	USD_API double GetTimeCodesPerSecond() const;

    /// Sets the stage's timeCodesPerSecond value.
    ///
    /// The timeCodesPerSecond value is set in the current EditTarget, if it 
    /// is the root layer of the stage or the session layer associated with the 
    /// stage. If the current EditTarget is neither, a warning is issued and no 
    /// value is set.
    ///
    /// \sa GetTimeCodesPerSecond()
	USD_API void SetTimeCodesPerSecond(double timeCodesPerSecond) const;

    /// Returns the stage's framesPerSecond value.
    /// 
    /// This makes an advisory statement about how the contained data can be 
    /// most usefully consumed and presented.  It's primarily an indication of 
    /// the expected playback rate for the data, but a timeline editing tool 
    /// might also want to use this to decide how to scale and label its 
    /// timeline.  
    ///
    /// The default value of framesPerSecond is 24.
	USD_API double GetFramesPerSecond() const;
    
    /// Sets the stage's framesPerSecond value.
    /// 
    /// The framesPerSecond value is set in the current EditTarget, if it 
    /// is the root layer of the stage or the session layer associated with the 
    /// stage. If the current EditTarget is neither, a warning is issued and no 
    /// value is set.
    /// 
    /// \sa GetFramesPerSecond()
	USD_API void SetFramesPerSecond(double framesPerSecond) const;
    
    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_interpolation
    /// \name Attribute Value Interpolation
    /// Controls the interpolation behavior when retrieving attribute
    /// values.  The default behavior is linear interpolation.
    /// See \ref Usd_AttributeInterpolation for more details.
    /// @{
    // --------------------------------------------------------------------- //

    /// Sets the interpolation type used during value resolution
    /// for all attributes on this stage.  Changing this will cause a
    /// UsdNotice::StageContentsChanged notice to be sent, as values at
    /// times where no samples are authored may have changed.
	USD_API void SetInterpolationType(UsdInterpolationType interpolationType);

    /// Returns the interpolation type used during value resolution
    /// for all attributes on this stage.
	USD_API UsdInterpolationType GetInterpolationType() const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_instancing
    /// \name Instancing
    /// See \ref Usd_Instancing for more details.
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns all master prims.
	USD_API std::vector<UsdPrim> GetMasters() const;

    /// @}

private:
    struct _IncludeNewlyDiscoveredPayloadsPredicate;

    enum _IncludePayloadsRule {
        _IncludeAllDiscoveredPayloads,
        _IncludeNoDiscoveredPayloads,
        _IncludeNewPayloadsIfAncestorWasIncluded
    };

    // --------------------------------------------------------------------- //
    // Stage Construction & Initialization
    // --------------------------------------------------------------------- //

    UsdStage(const SdfLayerRefPtr& rootLayer,
             const SdfLayerRefPtr& sessionLayer,
             const ArResolverContext& pathResolverContext,
             InitialLoadSet load);

    // Helper for Open() overloads -- searches and publishes to bound caches.
    template <class... Args>
    static UsdStageRefPtr _OpenImpl(InitialLoadSet load, Args const &... args);

    // Common ref ptr initialization, called by public, static constructors.
    //
    // This method will either return a valid refptr (if the stage is correctly
    // initialized) or it will return a null ref pointer, deleting the
    // raw stage pointer in the process.
    static UsdStageRefPtr
    _InstantiateStage(const SdfLayerRefPtr &rootLayer,
                      const SdfLayerRefPtr &sessionLayer,
                      const ArResolverContext &pathResolverContext,
                      InitialLoadSet load);

    // --------------------------------------------------------------------- //
    // Spec Existence & Definition Helpers
    // --------------------------------------------------------------------- //
    using _MasterToFlattenedPathMap 
        = std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;

    void _CopyMetadata(const UsdObject &source,
                       const SdfSpecHandle& dest) const;
    
    void _CopyProperty(const UsdProperty &prop,
                       const SdfPrimSpecHandle& dest,
                       const _MasterToFlattenedPathMap 
                            &masterToFlattened) const;

    void _CopyMasterPrim(const UsdPrim &masterPrim,
                         const SdfLayerHandle &destinationLyer,
                         const _MasterToFlattenedPathMap 
                            &masterToFlattened) const;

    void _FlattenPrim(const UsdPrim &usdPrim,
                      const SdfLayerHandle &layer,
                      const SdfPath &path,
                      const _MasterToFlattenedPathMap &masterToFlattened) const;

    SdfPropertySpecHandleVector
    _GetPropertyStack(const UsdProperty &prop, UsdTimeCode time) const;

    SdfPropertySpecHandle
    _GetPropertyDefinition(const UsdPrim &prim, const TfToken &propName) const;

    SdfPropertySpecHandle
    _GetPropertyDefinition(const UsdProperty &prop) const;

    template <class PropType>
    SdfHandle<PropType>
    _GetPropertyDefinition(const UsdProperty &prop) const;

    SdfAttributeSpecHandle
    _GetAttributeDefinition(const UsdAttribute &attr) const;

    SdfRelationshipSpecHandle
    _GetRelationshipDefinition(const UsdRelationship &rel) const;

    SdfPrimSpecHandle
    _CreatePrimSpecForEditing(const SdfPath& path);

    template <class PropType>
    SdfHandle<PropType>
    _CreatePropertySpecForEditing(const UsdProperty &prop);

    SdfPropertySpecHandle
    _CreatePropertySpecForEditing(const UsdProperty &prop);

    SdfAttributeSpecHandle
    _CreateAttributeSpecForEditing(const UsdAttribute &attr);

    SdfRelationshipSpecHandle
    _CreateRelationshipSpecForEditing(const UsdRelationship &rel);

    bool _RemoveProperty(const SdfPath& path);

    // --------------------------------------------------------------------- //
    // Value & Metadata Authoring
    // --------------------------------------------------------------------- //

    bool _SetValue(UsdTimeCode time, const UsdAttribute &attr,
                   const VtValue &newValue);
    bool _SetValue(UsdTimeCode time, const UsdAttribute &attr,
                   const SdfAbstractDataConstValue &newValue);
    template <class T>
    bool _SetValueImpl(UsdTimeCode time, const UsdAttribute &attr, const T& value);

    bool _ClearValue(UsdTimeCode time, const UsdAttribute &attr);

    bool _SetMetadata(const UsdObject &obj, const TfToken& fieldName,
                      const TfToken &keyPath, const VtValue &newValue);
    bool _SetMetadata(const UsdObject &obj, const TfToken& fieldName,
                      const TfToken &keyPath,
                      const SdfAbstractDataConstValue &newValue);
    template <class T>
    bool _SetMetadataImpl(const UsdObject &obj, const TfToken& fieldName,
                          const TfToken &keyPath, const T &value);

    bool _ClearMetadata(const UsdObject &obj, const TfToken& fieldName,
                        const TfToken &keyPath=TfToken());

    // --------------------------------------------------------------------- //
    // Misc Internal Helpers
    // --------------------------------------------------------------------- //

    // Pcp helpers.
    PcpCache const *_GetPcpCache() const { return _cache.get(); }
    PcpCache *_GetPcpCache() { return _cache.get(); }

    // Returns the PrimIndex, using the read-only PcpCache API. We expect prims
    // to be composed during initial stage composition, so this method should
    // not be used in that context.
    const PcpPrimIndex* _GetPcpPrimIndex(const SdfPath& primPath) const;

    // Helper to report pcp errors.
    void _ReportPcpErrors(const PcpErrorVector &errors,
                          const std::string &context) const;
    void _ReportErrors(const PcpErrorVector &errors,
                       const std::vector<std::string>& otherErrors,
                       const std::string &context) const;

    // --------------------------------------------------------------------- //
    // Scenegraph Composition & Change Processing
    // --------------------------------------------------------------------- //

    // Compose the prim indexes in the subtrees rooted at the paths in 
    // \p primIndexPaths.  If \p instanceChanges is given, returns
    // changes to masters and instances due to the discovery of new instances
    // during composition.
    void _ComposePrimIndexesInParallel(
        const std::vector<SdfPath>& primIndexPaths,
        _IncludePayloadsRule includeRule,
        const std::string& context,
        Usd_InstanceChanges* instanceChanges = NULL);

    // Recompose the subtree rooted at \p prim: compose its type, flags, and
    // list of children, then invoke _ComposeSubtree on all its children.
    void _ComposeSubtree(
        Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
        const SdfPath &primIndexPath = SdfPath());
    void _ComposeSubtreeImpl(
        Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
        const SdfPath &primIndexPath = SdfPath());
    void _ComposeSubtreeInParallel(Usd_PrimDataPtr prim);
    void _ComposeSubtreesInParallel(
        const std::vector<Usd_PrimDataPtr> &prims,
        const std::vector<SdfPath> *primIndexPaths = NULL);

    // Compose subtree rooted at \p prim under \p parent.  This function
    // ensures that the appropriate prim index is specified for \p prim if
    // \p parent is in a master.
    void _ComposeChildSubtree(Usd_PrimDataPtr prim, 
                              Usd_PrimDataConstPtr parent);

    // Compose \p prim's list of children and make any modifications necessary
    // to its _children member and the stage's _primMap, including possibly
    // instantiating new prims, or destroying existing subtrees of prims.  The
    // any newly created prims *do not* have their prim index, type, flags, or
    // children composed.
    //
    // Compose only \p prim's direct children if recurse=false.  Otherwise
    // recompose every descendent of \p prim.  Callers that pass recurse=false
    // should invoke _ComposeSubtree on any newly created prims to ensure caches
    // are correctly populated.
    void _ComposeChildren(Usd_PrimDataPtr prim, bool recurse);

    // Instantiate a prim instance.  There must not already be an instance
    // at \p primPath.
    Usd_PrimDataPtr _InstantiatePrim(const SdfPath &primPath);

    // For \p prim and all of its descendants, remove from _primMap and empty
    // their _children vectors.
    void _DestroyPrim(Usd_PrimDataPtr prim);

    // Destroy the prim subtrees rooted at each path in \p paths. \p paths may
    // not contain any path that is a descendent of another path in \p paths.
    void _DestroyPrimsInParallel(const std::vector<SdfPath>& paths);

    // Invoke _DestroyPrim() on all of \p prim's direct children.
    void _DestroyDescendents(Usd_PrimDataPtr prim);

    // Returns true if the object at the given path is elided from the
    // stage due to it being a child of an instance prim.
    bool _IsObjectElidedFromStage(const SdfPath& path) const;

    // If the given prim is an instance, returns the corresponding 
    // master prim.  Otherwise, returns an invalid prim.
    Usd_PrimDataConstPtr _GetMasterForInstance(Usd_PrimDataConstPtr p) const;

    // Returns the path of the Usd prim using the prim index at the given path.
    SdfPath _GetPrimPathUsingPrimIndexAtPath(const SdfPath& primIndexPath) const;

    // Update stage contents in response to changes in scene description.
    void _HandleLayersDidChange(const SdfNotice::LayersDidChangeSentPerLayer &);

    // Remove scene description for the prim at \p fullPath in the current edit
    // target.
    bool _RemovePrim(const SdfPath& fullPath);

    SdfPrimSpecHandle _GetPrimSpec(const SdfPath& fullPath);

    // Find and return the defining spec type for the property spec at the given
    // path, or SdfSpecTypeUnknown if none exists.  The defining spec type is
    // either the builtin definition's spec type, if the indicated property is
    // builtin, otherwise it's the strongest authored spec's type if one exists,
    // otherwise it's SdfSpecTypeUnknown.
    SdfSpecType _GetDefiningSpecType(const UsdPrim &prim,
                                     const TfToken &propName) const;

    // Helper to apply Pcp changes and recompose the scenegraph accordingly,
    // ignoring deactivatedPaths and given an optional initial set of paths to
    // recompose.
    void _Recompose(const PcpChanges &changes,
                    SdfPathSet *initialPathsToRecompose);

    // Helper for _Recompose to find the subtrees that need to be
    // fully recomposed and to recompose the name children of the
    // parents of these subtrees. Note that [start, finish) must be a
    // sorted range of paths with no descendent paths.
    template <class Iter>
    void _ComputeSubtreesToRecompose(Iter start, Iter finish,
                                     std::vector<Usd_PrimDataPtr>* recompose);

    // Helper for _Recompose to remove master subtrees in \p subtreesToRecompose
    // that would be composed when an instance subtree in the same container
    // is composed.
    template <class PrimIndexPathMap>
    void _RemoveMasterSubtreesSubsumedByInstances(
        std::vector<Usd_PrimDataPtr>* subtreesToRecompose,
        const PrimIndexPathMap& primPathToSourceIndexPathMap) const;

    // return true if the path is valid for load/unload operations.
    // This method will emit errors when invalid paths are encountered.
    bool _IsValidForLoadUnload(const SdfPath& path) const;

    // Discover all payloads in a given subtree, adding the path of each
    // discovered prim index to the \p primIndexPaths set. If specified,
    // the corresponding UsdPrim path will be added to the \p usdPrimPaths
    // set. The root path will be considered for inclusion in the result set.
    //
    // Note that some payloads may not be discoverable in until an ancestral
    // payload has been included. UsdStage::LoadAndUnload takes this into
    // account.
    void _DiscoverPayloads(const SdfPath& rootPath,
                           SdfPathSet* primIndexPaths,
                           bool unloadedOnly = false,
                           SdfPathSet* usdPrimPaths = nullptr) const;

    void
    _DiscoverPayloadsInternal(
        UsdPrim const &prim,
        tbb::concurrent_vector<SdfPath> *primIndexPaths,
        bool unloadedOnly,
        tbb::concurrent_vector<SdfPath> *usdPrimPaths,
        tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash> *seenMasterPrimPaths
        ) const;

    // Discover all ancestral payloads above a given root, adding the path
    // of each discovered prim index to the \p result set. The root path
    // itself will not be included in the result.
    void _DiscoverAncestorPayloads(const SdfPath& rootPath,
                                   SdfPathSet* result,
                                   bool unloadedOnly = false) const;

    // ===================================================================== //
    //                          VALUE RESOLUTION                             //
    // ===================================================================== //
    // --------------------------------------------------------------------- //
    // Specialized Value Resolution
    // --------------------------------------------------------------------- //

    // Specifier composition is special.  See comments in .cpp in
    // _ComposeSpecifier. This method returns either the authored specifier or
    // the fallback value registered in Sdf.
    SdfSpecifier _GetSpecifier(const UsdPrim &prim) const;
    SdfSpecifier _GetSpecifier(Usd_PrimDataConstPtr primData) const;

    // Custom is true if it is true anywhere in the stack.
    bool _IsCustom(const UsdProperty &prop) const;

    // Variability is determined by the weakest opinion in the stack.
    SdfVariability _GetVariability(const UsdProperty &prop) const;

    // Helper functions for resolving asset paths during value resolution.
    void _MakeResolvedAssetPaths(UsdTimeCode time, const UsdAttribute &attr,
                                 SdfAssetPath *assetPaths,
                                 size_t numAssetPaths) const;

    void _MakeResolvedAssetPaths(UsdTimeCode time, const UsdAttribute &attr,
                                 VtValue *value) const;

    // --------------------------------------------------------------------- //
    // Metadata Resolution
    // --------------------------------------------------------------------- //
    bool _GetMetadata(const UsdObject &obj,
                      const TfToken& fieldName,
                      const TfToken &keyPath,
                      bool useFallbacks,
                      VtValue* result) const;

    bool _GetMetadata(const UsdObject &obj,
                      const TfToken& fieldName,
                      const TfToken &keyPath,
                      bool useFallbacks,
                      SdfAbstractDataValue* result) const;

    template <class T>
    bool _GetMetadataImpl(const UsdObject &obj, const TfToken& fieldName, 
                          T* value) const;

    template <class Composer>
    void _GetAttrTypeImpl(const UsdAttribute &attr,
                          const TfToken &fieldName,
                          bool useFallbacks,
                          Composer *composer) const;

    template <class Composer>
    void _GetAttrVariabilityImpl(const UsdAttribute &attr,
                                 bool useFallbacks,
                                 Composer *composer) const;

    template <class Composer>
    void _GetPropCustomImpl(const UsdProperty &prop,
                            bool useFallbacks,
                            Composer *composer) const;

    template <class Composer>
    void _GetPrimTypeNameImpl(const UsdPrim &prim,
                              bool useFallbacks,
                              Composer *composer) const;

    template <class Composer>
    bool _GetPrimSpecifierImpl(Usd_PrimDataConstPtr primData,
                               bool useFallbacks, Composer *composer) const;

    template <class ListOpType, class Composer>
    bool _GetListOpMetadataImpl(const UsdObject &obj,
                                const TfToken &fieldName,
                                bool useFallbacks,
                                Usd_Resolver *resolver,
                                Composer *composer) const;

    template <class Composer>
    bool _GetSpecialMetadataImpl(const UsdObject &obj,
                                 const TfToken &fieldName,
                                 const TfToken &keyPath,
                                 bool useFallbacks,
                                 Composer *composer) const;
    template <class Composer>
    bool _GetMetadataImpl(const UsdObject &obj,
                          const TfToken& fieldName,
                          const TfToken& keyPath,
                          bool includeFallbacks,
                          Composer *composer) const;

    template <class Composer>
    bool _GetGeneralMetadataImpl(const UsdObject &obj,
                                 const TfToken& fieldName,
                                 const TfToken& keyPath,
                                 bool includeFallbacks,
                                 Composer *composer) const;

    template <class Composer>
    bool _ComposeGeneralMetadataImpl(const UsdObject &obj,
                                     const TfToken& fieldName,
                                     const TfToken& keyPath,
                                     bool includeFallbacks,
                                     Usd_Resolver* resolver,
                                     Composer *composer) const;

    // NOTE: The "authoredOnly" flag is not yet in use, but when we have
    // support for prim-based metadata fallbacks, they should be ignored when
    // this flag is set to true.
    bool _HasMetadata(const UsdObject &obj, const TfToken& fieldName,
                      const TfToken &keyPath, bool useFallbacks) const;

    TfTokenVector
    _ListMetadataFields(const UsdObject &obj, bool useFallbacks) const;

    void _GetAllMetadata(const UsdObject &obj,
                         bool useFallbacks,
                         UsdMetadataValueMap* result) const;

    template <class Composer>
    bool
    _GetFallbackMetadataImpl(const UsdObject &obj,
                             const TfToken &fieldName,
                             const TfToken &keyPath,
                             Composer *composer) const;

    template <class T>
    bool _GetFallbackMetadata(const UsdObject &obj, const TfToken& fieldName,
                              const TfToken &keyPath, T* result) const;

    // --------------------------------------------------------------------- //
    // Default & TimeSample Resolution
    // --------------------------------------------------------------------- //

    void _GetResolveInfo(const UsdAttribute &attr, 
                         UsdResolveInfo *resolveInfo,
                         const UsdTimeCode *time = nullptr) const;

    template <class T> struct _ExtraResolveInfo;

    template <class T>
    void _GetResolveInfo(const UsdAttribute &attr, 
                         UsdResolveInfo *resolveInfo,
                         const UsdTimeCode *time = nullptr,
                         _ExtraResolveInfo<T> *extraInfo = nullptr) const;

    template <class T> struct _ResolveInfoResolver;
    struct _PropertyStackResolver;

    template <class Resolver>
    void _GetResolvedValueImpl(const UsdProperty &prop,
                               Resolver *resolver,
                               const UsdTimeCode *time = nullptr) const;

    bool _GetValue(UsdTimeCode time, const UsdAttribute &attr, 
                   VtValue* result) const;

    template <class T>
    bool _GetValue(UsdTimeCode time, const UsdAttribute &attr,
                   T* result) const;

    template <class T>
    bool _GetValueImpl(UsdTimeCode time, const UsdAttribute &attr, 
                       Usd_InterpolatorBase* interpolator,
                       T* value) const;

    SdfLayerRefPtr
    _GetLayerWithStrongestValue(
        UsdTimeCode time, const UsdAttribute &attr) const;



    USD_API
    bool _GetValueFromResolveInfo(const UsdResolveInfo &info,
                                  UsdTimeCode time, const UsdAttribute &attr,
                                  VtValue* result) const;

    template <class T>
    USD_API
    bool _GetValueFromResolveInfo(const UsdResolveInfo &info,
                                  UsdTimeCode time, const UsdAttribute &attr,
                                  T* result) const;

    template <class T>
    bool _GetValueFromResolveInfoImpl(const UsdResolveInfo &info,
                                      UsdTimeCode time, const UsdAttribute &attr,
                                      Usd_InterpolatorBase* interpolator,
                                      T* value) const;

    // --------------------------------------------------------------------- //
    // Specialized Time Sample I/O
    // --------------------------------------------------------------------- //

    // Returns the TSM, transformed for the current offset & scale.
    SdfTimeSampleMap _GetTimeSampleMap(const UsdAttribute &attr) const;

    // Same as _GetTimeSampleMap but report whether or not timesamples exist.
    bool _GetTimeSampleMap(const UsdAttribute &attr,
                           SdfTimeSampleMap *out) const;

    /// Gets the set of time samples authored for a given attribute 
    /// within the \p interval. The interval may have any combination 
    /// of open/infinite and closed/finite endpoints; it may not have 
    /// open/finite endpoints, however, this restriction may be lifted 
    /// in the future.
    /// Returns false on an error.
    bool _GetTimeSamplesInInterval(const UsdAttribute &attr,
                                   const GfInterval& interval,
                                   std::vector<double>* times) const;

    bool _GetTimeSamplesInIntervalFromResolveInfo(
                                   const UsdResolveInfo &info,
                                   const UsdAttribute &attr,
                                   const GfInterval& interval,
                                   std::vector<double>* times) const;

    size_t _GetNumTimeSamples(const UsdAttribute &attr) const;

    size_t _GetNumTimeSamplesFromResolveInfo(const UsdResolveInfo &info,
                                           const UsdAttribute &attr) const;

    /// Gets the bracketing times around a desiredTime. Only false on error
    /// or if no value exists (default or timeSamples). See
    /// UsdAttribute::GetBracketingTimeSamples for details.
    bool _GetBracketingTimeSamples(const UsdAttribute &attr,
                                   double desiredTime,
                                   bool authoredOnly,
                                   double* lower,
                                   double* upper,
                                   bool* hasSamples) const;

    bool _GetBracketingTimeSamplesFromResolveInfo(const UsdResolveInfo &info,
                                                  const UsdAttribute &attr,
                                                  double desiredTime,
                                                  bool authoredOnly,
                                                  double* lower,
                                                  double* upper,
                                                  bool* hasSamples) const;

    bool _ValueMightBeTimeVarying(const UsdAttribute &attr) const;

    bool _ValueMightBeTimeVaryingFromResolveInfo(const UsdResolveInfo &info,
                                                 const UsdAttribute &attr) const;

    void _RegisterPerLayerNotices();

private:

    // The 'pseudo root' prim.
    Usd_PrimDataPtr _pseudoRoot;

    // The stage's root layer.
    SdfLayerRefPtr _rootLayer;

    // Every UsdStage has an implicit, in-memory session layer.
    // This is to allow for runtime overrides such as variant selections.
    SdfLayerRefPtr _sessionLayer;

    // The stage's EditTarget.
    UsdEditTarget _editTarget;

    boost::scoped_ptr<PcpCache> _cache;
    boost::scoped_ptr<Usd_ClipCache> _clipCache;
    boost::scoped_ptr<Usd_InstanceCache> _instanceCache;

    // A map from Path to Prim, for fast random access.
    typedef TfHashMap<
        SdfPath, Usd_PrimDataIPtr, SdfPath::Hash> PathToNodeMap;
    PathToNodeMap _primMap;
    mutable boost::optional<tbb::spin_rw_mutex> _primMapMutex;

    // The interpolation type used for all attributes on the stage.
    UsdInterpolationType _interpolationType;

    typedef std::vector<
        std::pair<SdfLayerHandle, TfNotice::Key> > _LayerAndNoticeKeyVec;
    _LayerAndNoticeKeyVec _layersAndNoticeKeys;
    size_t _lastChangeSerialNumber;

    boost::optional<WorkDispatcher> _dispatcher;

    // To provide useful aggregation of malloc stats, we bill everything
    // for this stage - from all access points - to this tag.
    std::string _mallocTagID;

    // The state used when instantiating the stage.
    const InitialLoadSet _initialLoadSet;
    
    bool _isClosingStage;
    
    friend class UsdAttribute;
    friend class UsdAttributeQuery;
    friend class UsdInherits;
    friend class UsdObject;
    friend class UsdPrim;
    friend class UsdProperty;
    friend class UsdReferences;
    friend class UsdRelationship;
    friend class UsdSpecializes;
    friend class UsdVariantSet;
    friend class UsdVariantSets;
    friend class Usd_PrimData;
    friend class Usd_StageOpenRequest;
};

template<typename T>
bool
UsdStage::GetMetadata(const TfToken& key, T* value) const
{
    VtValue result;
    if (not GetMetadata(key, &result)){
        return false;
    }

    if (result.IsHolding<T>()){
        *value = result.UncheckedGet<T>();
        return true;
    } else {
        TF_CODING_ERROR("Requested type %s for stage metadatum %s does not"
                        " match retrieved type %s",
                        ArchGetDemangled<T>().c_str(),
                        key.GetText(),
                        result.GetTypeName().c_str());
        return false;
    }
}

template<typename T>
bool 
UsdStage::SetMetadata(const TfToken& key, const T& value) const
{
    VtValue in(value);
    return SetMetadata(key, in);
}

template<typename T>
bool
UsdStage::GetMetadataByDictKey(const TfToken& key, const TfToken &keyPath, 
                               T* value) const
{
    VtValue result;
    if (not GetMetadataByDictKey(key, keyPath, &result)){
        return false;
    }

    if (result.IsHolding<T>()){
        *value = result.UncheckedGet<T>();
        return true;
    } else {
        TF_CODING_ERROR("Requested type %s for stage metadatum %s[%s] does not"
                        " match retrieved type %s",
                        ArchGetDemangled<T>().c_str(),
                        key.GetText(),
                        keyPath.GetText(),
                        result.GetTypeName().c_str());
        return false;
    }
}

template<typename T>
bool 
UsdStage::SetMetadataByDictKey(const TfToken& key, const TfToken &keyPath, 
                               const T& value) const
{
    VtValue in(value);
    return SetMetadataByDictKey(key, keyPath, in);
}


#endif //USD_STAGE_H

