//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_STAGE_H
#define PXR_USD_USD_STAGE_H

/// \file usd/stage.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stageLoadRules.h"
#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/primFlags.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/usd/ar/ar.h"
#include "pxr/usd/ar/notice.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/work/dispatcher.h"

#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/spin_rw_mutex.h>

#include <functional>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class ArResolverContext;
class GfInterval;
class SdfAbstractDataValue;
class TsSpline;
class Usd_AssetPathContext;
class Usd_ClipCache;
class Usd_InstanceCache;
class Usd_InstanceChanges;
class Usd_InterpolatorBase;
class Usd_Resolver;
class UsdResolveInfo;
class UsdResolveTarget;
class UsdPrim;
class UsdPrimRange;

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
/// for targeted asset optimization and debugging, though care should be
/// exercized with large scenes, as flattening defeats some of the benefits of
/// referenced scene description, and may produce very large results, 
/// especially in file formats that do not support data de-duplication, like
/// the usda text format!
///
/// \section Usd_SessionLayer Stage Session Layers
///
/// Each UsdStage can possess an optional "session layer".  The purpose of
/// a session layer is to hold ephemeral edits that modify a UsdStage's contents
/// or behavior in a way that is useful to the client, but should not be
/// considered as permanent mutations to be recorded upon export.  A very 
/// common use of session layers is to make variant selections, to pick a
/// specific LOD or shading variation, for example.  The session layer is
/// also frequently used to override the visibility of geometry 
/// and assets in the scene.  A session layer, if present, contributes to a 
/// UsdStage's identity, for purposes of stage-caching, etc. 
///
/// To edit content in a session layer, get the layer's edit target using 
/// stage->GetEditTargetForLocalLayer(stage->GetSessionLayer()) and set that
/// target in the stage by calling SetEditTarget() or creating a UsdEditContext.
///
class UsdStage : public TfRefBase, public TfWeakBase {
public:

    // --------------------------------------------------------------------- //
    /// \anchor Usd_lifetimeManagement
    /// \name Lifetime Management
    /// @{
    // --------------------------------------------------------------------- //
    
    /// \enum InitialLoadSet
    ///
    /// Specifies the initial set of prims to load when opening a UsdStage.
    ///
    enum InitialLoadSet
    {
        LoadAll, ///< Load all loadable prims
        LoadNone ///< Load no loadable prims
    };

    /// Create a new stage with root layer \p identifier, destroying
    /// potentially existing files with that identifier; it is considered an
    /// error if an existing, open layer is present with this identifier.
    ///
    /// \sa SdfLayer::CreateNew()
    ///
    /// Invoking an overload that does not take a \p sessionLayer argument will
    /// create a stage with an anonymous in-memory session layer.  To create a
    /// stage without a session layer, pass TfNullPtr (or None in python) as the
    /// \p sessionLayer argument.
    //
    /// The initial set of prims to load on the stage can be specified
    /// using the \p load parameter. \sa UsdStage::InitialLoadSet.
    ///
    /// If \p pathResolverContext is provided it will be bound when creating the
    /// root layer at \p identifier and whenever asset path resolution is done
    /// for this stage, regardless of what other context may be bound at that
    /// time. Otherwise Usd will create the root layer with no context bound,
    /// then create a context for all future asset path resolution for the stage
    /// by calling ArResolver::CreateDefaultContextForAsset with the root
    /// layer's repository path if the layer has one, otherwise its resolved
    /// path.
    USD_API
    static UsdStageRefPtr
    CreateNew(const std::string& identifier, 
              InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    CreateNew(const std::string& identifier,
              const SdfLayerHandle& sessionLayer,
              InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    CreateNew(const std::string& identifier,
              const SdfLayerHandle& sessionLayer,
              const ArResolverContext& pathResolverContext,
              InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    CreateNew(const std::string& identifier,
              const ArResolverContext& pathResolverContext,
              InitialLoadSet load = LoadAll);

    /// Creates a new stage only in memory, analogous to creating an
    /// anonymous SdfLayer.
    ///
    /// If \p pathResolverContext is provided it will be bound when creating the
    /// root layer at \p identifier and whenever asset path resolution is done
    /// for this stage, regardless of what other context may be bound at that
    /// time. Otherwise Usd will create the root layer with no context bound,
    /// then create a context for all future asset path resolution for the stage
    /// by calling ArResolver::CreateDefaultContext.
    ///
    /// The initial set of prims to load on the stage can be specified
    /// using the \p load parameter. \sa UsdStage::InitialLoadSet.
    ///
    /// Invoking an overload that does not take a \p sessionLayer argument will
    /// create a stage with an anonymous in-memory session layer.  To create a
    /// stage without a session layer, pass TfNullPtr (or None in python) as the
    /// \p sessionLayer argument.
    USD_API
    static UsdStageRefPtr
    CreateInMemory(InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    CreateInMemory(const std::string& identifier,
                   InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    CreateInMemory(const std::string& identifier,
                   const ArResolverContext& pathResolverContext,
                   InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    CreateInMemory(const std::string& identifier,
                   const SdfLayerHandle &sessionLayer,
                   InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    CreateInMemory(const std::string& identifier,
                   const SdfLayerHandle &sessionLayer,
                   const ArResolverContext& pathResolverContext,
                   InitialLoadSet load = LoadAll);

    /// Attempt to find a matching existing stage in a cache if
    /// UsdStageCacheContext objects exist on the stack. Failing that, create a
    /// new stage and recursively compose prims defined within and referenced by
    /// the layer at \p filePath, which must already exist.
    ///
    /// The initial set of prims to load on the stage can be specified
    /// using the \p load parameter. \sa UsdStage::InitialLoadSet.
    ///
    /// If \p pathResolverContext is provided it will be bound when opening the
    /// root layer at \p filePath and whenever asset path resolution is done for
    /// this stage, regardless of what other context may be bound at that
    /// time. Otherwise Usd will open the root layer with no context bound, then
    /// create a context for all future asset path resolution for the stage by
    /// calling ArResolver::CreateDefaultContextForAsset with the layer's
    /// repository path if the layer has one, otherwise its resolved path.
    USD_API
    static UsdStageRefPtr
    Open(const std::string& filePath, InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    Open(const std::string& filePath,
         const ArResolverContext& pathResolverContext,
         InitialLoadSet load = LoadAll);

    /// Create a new stage and recursively compose prims defined within and
    /// referenced by the layer at \p filePath which must already exist, subject
    /// to \p mask.
    /// 
    /// These OpenMasked() methods do not automatically consult or populate
    /// UsdStageCache s.
    ///
    /// The initial set of prims to load on the stage can be specified
    /// using the \p load parameter. \sa UsdStage::InitialLoadSet.
    ///
    /// If \p pathResolverContext is provided it will be bound when opening the
    /// root layer at \p filePath and whenever asset path resolution is done for
    /// this stage, regardless of what other context may be bound at that
    /// time. Otherwise Usd will open the root layer with no context bound, then
    /// create a context for all future asset path resolution for the stage by
    /// calling ArResolver::CreateDefaultContextForAsset with the layer's
    /// repository path if the layer has one, otherwise its resolved path.
    USD_API
    static UsdStageRefPtr
    OpenMasked(const std::string &filePath,
               UsdStagePopulationMask const &mask,
               InitialLoadSet load = LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    OpenMasked(const std::string &filePath,
               const ArResolverContext &pathResolverContext,
               UsdStagePopulationMask const &mask,
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
    /// If \p pathResolverContext is provided it will be bound when whenever
    /// asset path resolution is done for this stage, regardless of what other
    /// context may be bound at that time. Otherwise Usd will create a context
    /// for all future asset path resolution for the stage by calling
    /// ArResolver::CreateDefaultContextForAsset with the layer's repository
    /// path if the layer has one, otherwise its resolved path.
    ///
    /// When searching for a matching stage in bound UsdStageCache s, only the
    /// provided arguments matter for cache lookup.  For example, if only a root
    /// layer (or a root layer file path) is provided, the first stage found in
    /// any cache that has that root layer is returned.  So, for example if you
    /// require that the stage have no session layer, you must explicitly
    /// specify TfNullPtr (or None in python) for the sessionLayer argument.
    USD_API
    static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         InitialLoadSet load=LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         const SdfLayerHandle& sessionLayer,
         InitialLoadSet load=LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         const ArResolverContext& pathResolverContext,
         InitialLoadSet load=LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    Open(const SdfLayerHandle& rootLayer,
         const SdfLayerHandle& sessionLayer,
         const ArResolverContext& pathResolverContext,
         InitialLoadSet load=LoadAll);

    /// Open a stage rooted at \p rootLayer and with limited population subject
    /// to \p mask.
    ///
    /// These OpenMasked() methods do not automatically consult or populate
    /// UsdStageCache s.
    ///
    /// Invoking an overload that does not take a \p sessionLayer argument will
    /// create a stage with an anonymous in-memory session layer.  To create a
    /// stage without a session layer, pass TfNullPtr (or None in python) as the
    /// \p sessionLayer argument.
    ///
    /// The initial set of prims to load on the stage can be specified
    /// using the \p load parameter. \sa UsdStage::InitialLoadSet.
    ///
    /// If \p pathResolverContext is provided it will be bound when whenever
    /// asset path resolution is done for this stage, regardless of what other
    /// context may be bound at that time. Otherwise Usd will create a context
    /// for all future asset path resolution for the stage by calling
    /// ArResolver::CreateDefaultContextForAsset with the layer's repository
    /// path if the layer has one, otherwise its resolved path.
    USD_API
    static UsdStageRefPtr
    OpenMasked(const SdfLayerHandle& rootLayer,
               const UsdStagePopulationMask &mask,
               InitialLoadSet load=LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    OpenMasked(const SdfLayerHandle& rootLayer,
               const SdfLayerHandle& sessionLayer,
               const UsdStagePopulationMask &mask,
               InitialLoadSet load=LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    OpenMasked(const SdfLayerHandle& rootLayer,
               const ArResolverContext& pathResolverContext,
               const UsdStagePopulationMask &mask,
               InitialLoadSet load=LoadAll);
    /// \overload
    USD_API
    static UsdStageRefPtr
    OpenMasked(const SdfLayerHandle& rootLayer,
               const SdfLayerHandle& sessionLayer,
               const ArResolverContext& pathResolverContext,
               const UsdStagePopulationMask &mask,
               InitialLoadSet load=LoadAll);
    
    USD_API
    virtual ~UsdStage();

    /// Calls SdfLayer::Reload on all layers contributing to this stage,
    /// except session layers and sublayers of session layers.
    ///
    /// This includes non-session sublayers, references and payloads.
    /// Note that reloading anonymous layers clears their content, so
    /// invoking Reload() on a stage constructed via CreateInMemory()
    /// will clear its root layer.
    ///
    /// \note This method is considered a mutation, which has potentially
    /// global effect!  Unlike the various Load() methods whose actions
    /// affect only **this stage**, Reload() may cause layers to change their
    /// contents, and because layers are global resources shared by
    /// potentially many Stages, calling Reload() on one stage may result in
    /// a mutation to any number of stages.  In general, unless you are
    /// highly confident your stage is the only consumer of its layers, you
    /// should only call Reload() when you are assured no other threads may
    /// be reading from any Stages.
    USD_API
    void Reload();

    /// Indicates whether the specified file is supported by UsdStage.
    ///
    /// This function is a cheap way to determine whether a
    /// file might be open-able with UsdStage::Open. It is
    /// purely based on the given \p filePath and does not
    /// open the file or perform analysis on the contents.
    /// As such, UsdStage::Open may still fail even if this
    /// function returns true.
    USD_API
    static bool
    IsSupportedFile(const std::string& filePath);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_layerSerialization
    /// \name Layer Serialization
    ///
    /// Functions for saving changes to layers that contribute opinions to
    /// this stage.  Layers may also be saved by calling SdfLayer::Save or
    /// exported to a new file by calling SdfLayer::Export.
    ///
    /// @{

    /// Calls SdfLayer::Save on all dirty layers contributing to this stage
    /// except session layers and sublayers of session layers.
    ///
    /// This function will emit a warning and skip each dirty anonymous
    /// layer it encounters, since anonymous layers cannot be saved with
    /// SdfLayer::Save. These layers must be manually exported by calling
    /// SdfLayer::Export.
    USD_API
    void Save();

    /// Calls SdfLayer::Save on all dirty session layers and sublayers of 
    /// session layers contributing to this stage.
    ///
    /// This function will emit a warning and skip each dirty anonymous
    /// layer it encounters, since anonymous layers cannot be saved with
    /// SdfLayer::Save. These layers must be manually exported by calling
    /// SdfLayer::Export.
    USD_API
    void SaveSessionLayers();

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_variantManagement
    /// \name Variant Management
    ///
    /// These methods provide control over the policy to use when composing
    /// prims that specify a variant set but do not specify a selection.
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
    /// This example ensures that we will get the "full" shadingComplexity
    /// for any prim with a shadingComplexity VariantSet that doesn't
    /// otherwise specify a selection, \em and has a "full" variant; if its
    /// shadingComplexity does not have a "full" variant, but \em does have
    /// a "light" variant, then the selection will be "light".  In other
    /// words, the entries in the "shadingComplexity" list in the plugInfo.json
    /// represent a priority-ordered list of fallback selections.
    ///
    /// The plugin metadata is discovered and applied before the first
    /// UsdStage is constructed in a given process.  It can be defined
    /// in any plugin.  However, if multiple plugins express contrary
    /// lists for the same named variant set, the result is undefined.
    /// 
    /// The plugin metadata approach is useful for ensuring that sensible
    /// default behavior applies across a pipeline without requiring
    /// every script and binary to explicitly configure every VariantSet
    /// that subscribes to fallback in the pipeline.
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
    USD_API
    static PcpVariantFallbackMap GetGlobalVariantFallbacks();

    /// Set the global variant fallback preferences used in new
    /// UsdStages. This overrides any fallbacks configured in plugin
    /// metadata, and only affects stages created after this call.
    ///
    /// \note This does not affect existing UsdStages.
    USD_API
    static void
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
    ///       has an ancestor present in the scene graph (other than the
    ///       absolute root). If the given path has no such ancestor, it is an
    ///       error.
    ///     - Specifying a path to an inactive prim is an error.
    ///     - Specifying a path to a prototype prim or a prim within a
    ///       prototype is an error.
    ///
    /// If an instance prim (or a path identifying a prim descendant to an
    /// instance) is encountered during a Load/Unload operation, these functions
    /// may cause instancing to change on the stage in order to ensure that no
    /// other instances are affected.  The load/unload rules that affect a given
    /// prim hierarchy are considered when determining which prims can be
    /// instanced together.  Instance sharing occurs when different instances
    /// have equivalent load rules.
    ///
    /// The GetLoadRules() and SetLoadRules() provide direct low-level access to
    /// the UsdStageLoadRules that govern payload inclusion on a stage.
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Modify this stage's load rules to load the prim at \p path, its
    /// ancestors, and all of its descendants if \p policy is
    /// UsdLoadWithDescendants.  If \p policy is UsdLoadWithoutDescendants, then
    /// payloads on descendant prims are not loaded.
    ///
    /// See \ref Usd_workingSetManagement "Working Set Management" for more
    /// information.
    USD_API
    UsdPrim Load(const SdfPath& path=SdfPath::AbsoluteRootPath(),
                 UsdLoadPolicy policy=UsdLoadWithDescendants);

    /// Modify this stage's load rules to unload the prim and its descendants
    /// specified by \p path.
    ///
    /// See \ref Usd_workingSetManagement "Working Set Management" for more
    /// information.
    USD_API
    void Unload(const SdfPath& path=SdfPath::AbsoluteRootPath());

    /// Unload and load the given path sets.  The effect is as if the unload set
    /// were processed first followed by the load set.
    ///
    /// This is equivalent to calling UsdStage::Unload for each item in the
    /// unloadSet followed by UsdStage::Load for each item in the loadSet,
    /// however this method is more efficient as all operations are committed in
    /// a single batch.  The \p policy argument is described in the
    /// documentation for Load().
    ///
    /// See \ref Usd_workingSetManagement "Working Set Management" for more
    /// information.
    USD_API
    void LoadAndUnload(const SdfPathSet &loadSet, const SdfPathSet &unloadSet,
                       UsdLoadPolicy policy=UsdLoadWithDescendants);

    /// Returns a set of all loaded paths.
    ///
    /// The paths returned are both those that have been explicitly loaded and
    /// those that were loaded as a result of dependencies, ancestors or
    /// descendants of explicitly loaded paths.
    ///
    /// This method does not return paths to inactive prims.
    ///
    /// See \ref Usd_workingSetManagement "Working Set Management" for more
    /// information.
    USD_API
    SdfPathSet GetLoadSet();

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
    ///
    /// See \ref Usd_workingSetManagement "Working Set Management" for more
    /// information.
    USD_API
    SdfPathSet FindLoadable(
        const SdfPath& rootPath = SdfPath::AbsoluteRootPath());

    /// Return the stage's current UsdStageLoadRules governing payload
    /// inclusion.
    ///
    /// See \ref Usd_workingSetManagement "Working Set Management" for more
    /// information.
    UsdStageLoadRules const &GetLoadRules() const {
        return _loadRules;
    }

    /// Set the UsdStageLoadRules to govern payload inclusion on this stage.
    /// This rebuilds the stage's entire prim hierarchy to follow \p rules.
    ///
    /// Note that subsequent calls to Load(), Unload(), LoadAndUnload() will
    /// modify this stages load rules as described in the documentation for
    /// those member functions.
    ///
    /// See \ref Usd_workingSetManagement "Working Set Management" for more
    /// information.
    USD_API
    void SetLoadRules(UsdStageLoadRules const &rules);

    /// Return this stage's population mask.
    UsdStagePopulationMask GetPopulationMask() const {
        return _populationMask;
    }

    /// Set this stage's population mask and recompose the stage.
    USD_API
    void SetPopulationMask(UsdStagePopulationMask const &mask);

    /// Expand this stage's population mask to include the targets of all
    /// relationships that pass \p relPred and connections to all attributes
    /// that pass \p attrPred recursively.  The attributes and relationships are
    /// those on all the prims found by traversing the stage according to \p
    /// traversalPredicate.  If \p relPred is null, include all relationship
    /// targets; if \p attrPred is null, include all connections.
    ///
    /// This function can be used, for example, to expand a population mask for
    /// a given prim to include bound materials, if those bound materials are
    /// expressed as relationships or attribute connections.
    ///
    /// See also UsdPrim::FindAllRelationshipTargetPaths() and
    /// UsdPrim::FindAllAttributeConnectionPaths().
    USD_API
    void ExpandPopulationMask(
        Usd_PrimFlagsPredicate const &traversalPredicate,
        std::function<bool (UsdRelationship const &)> const &relPred = nullptr,
        std::function<bool (UsdAttribute const &)> const &attrPred = nullptr);

    /// \overload
    /// This convenience overload invokes ExpandPopulationMask() with the
    /// UsdPrimDefaultPredicate traversal predicate.
    USD_API
    void ExpandPopulationMask(
        std::function<bool (UsdRelationship const &)> const &relPred = nullptr,
        std::function<bool (UsdAttribute const &)> const &attrPred = nullptr);

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
    USD_API
    UsdPrim GetPseudoRoot() const;

    /// Return the UsdPrim on this stage whose path is the root layer's
    /// defaultPrim metadata's value.  Return an invalid prim if there is no
    /// such prim or if the root layer's defaultPrim metadata is unset or is not
    /// a valid prim path.  Note that this function will return the prim on the 
    /// stage whose path is the root layer's GetDefaultPrimAsPath() if that path
    /// is not empty and a prim at that path exists on the stage. 
    /// See also SdfLayer::GetDefaultPrimAsPath().
    USD_API
    UsdPrim GetDefaultPrim() const;

    /// Set the default prim layer metadata in this stage's root layer. This
    /// is shorthand for:
    /// \code
    /// stage->GetRootLayer()->SetDefaultPrim(prim.GetName());
    /// \endcode
    /// If prim is a root prim, otherwise
    /// \code
    /// stage->GetRootLayer()->SetDefaultPrim(prim.GetPath().GetAsToken());
    /// \endcode
    /// Note that this function always authors to the stage's root layer.
    /// To author to a different layer, use the SdfLayer::SetDefaultPrim() API.
    USD_API
    void SetDefaultPrim(const UsdPrim &prim);
    
    /// Clear the default prim layer metadata in this stage's root layer.  This
    /// is shorthand for:
    /// \code
    /// stage->GetRootLayer()->ClearDefaultPrim();
    /// \endcode
    /// Note that this function always authors to the stage's root layer.  To
    /// author to a different layer, use the SdfLayer::SetDefaultPrim() API.
    USD_API
    void ClearDefaultPrim();

    /// Return true if this stage's root layer has an authored opinion for the
    /// default prim layer metadata.  This is shorthand for:
    /// \code
    /// stage->GetRootLayer()->HasDefaultPrim();
    /// \endcode
    /// Note that this function only consults the stage's root layer.  To
    /// consult a different layer, use the SdfLayer::HasDefaultPrim() API.
    USD_API
    bool HasDefaultPrim() const;

    /// Return the UsdPrim at \p path, or an invalid UsdPrim if none exists.
    /// 
    /// If \p path indicates a prim beneath an instance, returns an instance
    /// proxy prim if a prim exists at the corresponding path in that instance's
    /// prototype.
    ///
    /// Unlike OverridePrim() and DefinePrim(), this method will never author
    /// scene description, and therefore is safe to use as a "reader" in the Usd
    /// multi-threading model.
    USD_API
    UsdPrim GetPrimAtPath(const SdfPath &path) const;

    /// Return the UsdObject at \p path, or an invalid UsdObject if none exists.
    ///
    /// If \p path indicates a prim beneath an instance, returns an instance
    /// proxy prim if a prim exists at the corresponding path in that instance's
    /// prototype. If \p path indicates a property beneath a child of an
    /// instance, returns a property whose parent prim is an instance proxy
    /// prim.
    ///
    /// Example:
    ///
    /// \code
    ///if (UsdObject obj = stage->GetObjectAtPath(path)) {
    ///    if (UsdPrim prim = obj.As<UsdPrim>()) {
    ///        // Do things with prim
    ///    }
    ///    else if (UsdProperty prop = obj.As<UsdProperty>()) {
    ///        // Do things with property. We can also cast to
    ///        // UsdRelationship or UsdAttribute using this same pattern.
    ///    }
    ///}
    ///else {
    ///    // No object at specified path
    ///}
    /// \endcode
    USD_API
    UsdObject GetObjectAtPath(const SdfPath &path) const;

    /// Return the UsdProperty at \p path, or an invalid UsdProperty
    /// if none exists.
    ///
    /// This is equivalent to 
    /// \code{.cpp}
    /// stage.GetObjectAtPath(path).As<UsdProperty>();
    /// \endcode
    /// \sa GetObjectAtPath(const SdfPath&) const
    USD_API
    UsdProperty GetPropertyAtPath(const SdfPath &path) const;

    /// Return the UsdAttribute at \p path, or an invalid UsdAttribute
    /// if none exists.
    ///
    /// This is equivalent to 
    /// \code{.cpp}
    /// stage.GetObjectAtPath(path).As<UsdAttribute>();
    /// \endcode
    /// \sa GetObjectAtPath(const SdfPath&) const
    USD_API
    UsdAttribute GetAttributeAtPath(const SdfPath &path) const;

    /// Return the UsdAttribute at \p path, or an invalid UsdAttribute
    /// if none exists.
    ///
    /// This is equivalent to 
    /// \code{.cpp}
    /// stage.GetObjectAtPath(path).As<UsdRelationship>();
    /// \endcode
    /// \sa GetObjectAtPath(const SdfPath&) const
    USD_API
    UsdRelationship GetRelationshipAtPath(const SdfPath &path) const;
private:
    // Return the primData object at \p path.
    Usd_PrimDataConstPtr _GetPrimDataAtPath(const SdfPath &path) const;
    Usd_PrimDataPtr _GetPrimDataAtPath(const SdfPath &path);

    // Return the primData object at \p path.  If \p path indicates a prim
    // beneath an instance, return the primData object for the corresponding 
    // prim in the instance's prototype.
    Usd_PrimDataConstPtr 
    _GetPrimDataAtPathOrInPrototype(const SdfPath &path) const;

    /// See documentation on UsdPrim::GetInstances()
    std::vector<UsdPrim>
    _GetInstancesForPrototype(const UsdPrim& prototype) const;

public:

    /// Traverse the active, loaded, defined, non-abstract prims on this stage
    /// depth-first.
    ///
    /// Traverse() returns a UsdPrimRange , which allows low-latency
    /// traversal, with the ability to prune subtrees from traversal.  It
    /// is python iterable, so in its simplest form, one can do:
    ///
    /// \code{.py}
    /// for prim in stage.Traverse():
    ///     print prim.GetPath()
    /// \endcode
    ///
    /// If either a pre-and-post-order traversal or a traversal rooted at a
    /// particular prim is desired, construct a UsdPrimRange directly.
    ///
    /// You'll need to use the returned UsdPrimRange's iterator to perform 
    /// actions such as pruning subtrees. See the "Using Usd.PrimRange in 
    /// python" section in UsdPrimRange for more details and examples. 
    ///
    /// This is equivalent to UsdPrimRange::Stage() . 
    USD_API
    UsdPrimRange Traverse();

    /// \overload
    /// Traverse the prims on this stage subject to \p predicate.
    ///
    /// This is equivalent to UsdPrimRange::Stage() .
    USD_API
    UsdPrimRange Traverse(const Usd_PrimFlagsPredicate &predicate);

    /// Traverse all the prims on this stage depth-first.
    ///
    /// \sa Traverse()
    /// \sa UsdPrimRange::Stage()
    USD_API
    UsdPrimRange TraverseAll();

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
    USD_API
    UsdPrim OverridePrim(const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim at \p path is defined (according to
    /// UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim at \p path is already defined on this stage and \p typeName is
    /// empty or equal to the existing prim's typeName, return that prim.
    /// Otherwise author an \a SdfPrimSpec with \a specifier ==
    /// \a SdfSpecifierDef and \p typeName for the prim at \p path at the
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
    USD_API
    UsdPrim CreateClassPrim(const SdfPath &rootPrimPath);

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
    USD_API
    bool RemovePrim(const SdfPath& path);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_layerManagement
    /// \name Layers and EditTargets
    /// @{
    // --------------------------------------------------------------------- //

    /// Return this stage's root session layer.
    USD_API
    SdfLayerHandle GetSessionLayer() const;

    /// Return this stage's root layer.
    USD_API
    SdfLayerHandle GetRootLayer() const;

    /// Return the path resolver context for all path resolution during
    /// composition of this stage. Useful for external clients that want to
    /// resolve paths with the same context as this stage, or create new
    /// stages with the same context.
    USD_API
    ArResolverContext GetPathResolverContext() const;

    /// Resolve the given identifier using this stage's 
    /// ArResolverContext and the layer of its GetEditTarget()
    /// as an anchor for relative references (e.g. \@./siblingFile.usd\@).   
    ///
    /// \return a non-empty string containing either the same
    /// identifier that was passed in (if the identifier refers to an
    /// already-opened layer or an "anonymous", in-memory layer), or a resolved
    /// layer filepath.  If the identifier was not resolvable, return the
    /// empty string.
    USD_API
    std::string
    ResolveIdentifierToEditTarget(std::string const &identifier) const;

    /// Return a PcpErrorVector containing all composition errors encountered 
    /// when composing the prims and layer stacks on this stage.
    USD_API
    PcpErrorVector GetCompositionErrors() const;

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
    /// if it has one. If \a includeClipLayers is true, we will also include
    /// all of the layers that this stage has had to open so far to perform
    /// value resolution of attributes affected by 
    /// \ref Usd_Page_ValueClips "Value Clips"
    USD_API
    SdfLayerHandleVector GetUsedLayers(bool includeClipLayers=true) const;

    /// Return true if \a layer is one of the layers in this stage's local,
    /// root layerStack.
    USD_API
    bool HasLocalLayer(const SdfLayerHandle &layer) const;
    
    /// Return the stage's EditTarget.
    USD_API
    const UsdEditTarget &GetEditTarget() const;

    /// Return a UsdEditTarget for editing the layer at index \a i in the
    /// layer stack.  This edit target will incorporate any layer time
    /// offset that applies to the sublayer.
    USD_API
    UsdEditTarget GetEditTargetForLocalLayer(size_t i);

    /// Return a UsdEditTarget for editing the given local \a layer.
    /// If the given layer appears more than once in the layer stack,
    /// the time offset to the first occurrence will be used.
    USD_API
    UsdEditTarget GetEditTargetForLocalLayer(const SdfLayerHandle &layer);

    /// Set the stage's EditTarget.  If \a editTarget.IsLocalLayer(), check to
    /// see if it's a layer in this stage's local LayerStack.  If not, issue an
    /// error and do nothing.  If \a editTarget is invalid, issue an error
    /// and do nothing.  If \a editTarget differs from the stage's current
    /// EditTarget, set the EditTarget and send
    /// UsdNotice::StageChangedEditTarget.  Otherwise do nothing.
    USD_API
    void SetEditTarget(const UsdEditTarget &editTarget);

    /// Mute the layer identified by \p layerIdentifier.  Muted layers are
    /// ignored by the stage; they do not participate in value resolution
    /// or composition and do not appear in any LayerStack.  If the root 
    /// layer of a reference or payload LayerStack is muted, the behavior 
    /// is as if the muted layer did not exist, which means a composition 
    /// error will be generated.
    ///
    /// A canonical identifier for each layer in \p layersToMute will be
    /// computed using ArResolver::CreateIdentifier using the stage's root
    /// layer as the anchoring asset. Any layer encountered during composition
    /// with the same identifier will be considered muted and ignored.
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
    USD_API
    void MuteLayer(const std::string &layerIdentifier);

    /// Unmute the layer identified by \p layerIdentifier if it had
    /// previously been muted.
    USD_API
    void UnmuteLayer(const std::string &layerIdentifier);

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
    USD_API
    const std::vector<std::string>& GetMutedLayers() const;

    /// Returns true if the layer specified by \p layerIdentifier is
    /// muted in this cache, false otherwise.  See documentation on
    /// MuteLayer for details on how \p layerIdentifier is compared to the 
    /// layers that have been muted.
    USD_API
    bool IsLayerMuted(const std::string& layerIdentifier) const;

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
    /// authors the resolved values for each object directly into the flattened
    /// layer.
    ///
    /// All VariantSets are removed and only the currently selected variants
    /// will be present in the resulting layer.
    ///
    /// Class prims will still exist, however all inherits arcs will have
    /// been removed and the inherited data will be copied onto each child
    /// object. Composition arcs authored on the class itself will be flattened
    /// into the class.
    ///
    /// Flatten preserves 
    /// \ref Usd_Page_ScenegraphInstancing "scenegraph instancing" by creating 
    /// independent roots for each prototype currently composed on this stage,
    /// and adding a single internal reference arc on each instance prim to its 
    /// corresponding prototype.
    ///
    /// Time samples across sublayer offsets will will have the time offset and
    /// scale applied to each time index.
    ///
    /// Finally, any deactivated prims will be pruned from the result.
    ///
    USD_API
    SdfLayerRefPtr Flatten(bool addSourceFileComment=true) const;
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
    /// \return true if we successfully retrieved a value of the requested type;
    /// false if \p key is not allowed as layer metadata or no value was found.
    /// Generates a coding error if we retrieved a stored value of a type other
    /// than the requested type
    ///
    /// \sa \ref Usd_OM_Metadata
    template <class T>
    bool GetMetadata(const TfToken &key, T *value) const;
    /// \overload
    USD_API
    bool GetMetadata(const TfToken &key, VtValue *value) const;

    /// Returns true if the \a key has a meaningful value, that is, if
    /// GetMetadata() will provide a value, either because it was authored
    /// or because the Stage metadata was defined with a meaningful fallback 
    /// value.
    ///
    /// Returns false if \p key is not allowed as layer metadata.
    USD_API
    bool HasMetadata(const TfToken &key) const;

    /// Returns \c true if the \a key has an authored value, \c false if no
    /// value was authored or the only value available is the SdfSchema's
    /// metadata fallback.
    ///
    /// \note If a value for a metadatum \em not legal to author on layers 
    /// is present in the root or session layer (which could happen through
    /// hand-editing or use of certain low-level API's), this method will
    /// still return \c false.
    USD_API
    bool HasAuthoredMetadata(const TfToken &key) const;

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
    USD_API
    bool SetMetadata(const TfToken &key, const VtValue &value) const;

    /// Clear the value of stage metadatum \p key, if the stage's
    /// current UsdEditTarget is the root or session layer.
    ///
    /// If the current EditTarget is any other layer, raise a coding error.
    /// \return true if authoring was successful, false otherwise.
    /// Generates a coding error if \p key is not allowed as layer metadata.
    ///
    /// \sa \ref Usd_OM_Metadata
    USD_API
    bool ClearMetadata(const TfToken &key) const;

    /// Resolve the requested dictionary sub-element \p keyPath of
    /// dictionary-valued metadatum named \p key, returning the resolved
    /// value.
    ///
    /// If you know you need just a small number of elements from a dictionary,
    /// accessing them element-wise using this method can be much less
    /// expensive than fetching the entire dictionary with GetMetadata(key).
    ///
    /// \return true if we successfully retrieved a value of the requested type;
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
    USD_API
    bool GetMetadataByDictKey(
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
    USD_API
    bool HasMetadataDictKey(
        const TfToken& key, const TfToken &keyPath) const;

    /// Return true if there exists any authored opinion (excluding
    /// fallbacks) for \p key and \p keyPath.  
    ///
    /// The \p keyPath is a ':'-separated path identifying a value in
    /// subdictionaries stored in the metadata field at \p key.  If 
    /// \p keyPath is empty, returns \c false.
    ///
    /// \sa \ref Usd_Dictionary_Type
    USD_API
    bool HasAuthoredMetadataDictKey(
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
    USD_API
    bool SetMetadataByDictKey(
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
    USD_API
    bool ClearMetadataByDictKey(
        const TfToken& key, const TfToken& keyPath) const;

    /// Writes the fallback prim types defined in the schema registry to the 
    /// stage as dictionary valued fallback prim type metadata. If the stage 
    /// already has fallback prim type metadata, the fallback types from the 
    /// schema registry will be added to the existing metadata, only for types 
    /// that are already present in the dictionary, i.e. this won't overwrite 
    /// existing fallback entries.
    ///
    /// The current edit target determines whether the metadata is written to 
    /// the root layer or the session layer. If the edit target specifies 
    /// another layer besides these, this will produce an error.
    ///
    /// This function can be used at any point before calling Save or Export on 
    /// a stage to record the fallback types for the current schemas. This 
    /// allows another version of Usd to open this stage and treat prim types it
    /// doesn't recognize as a type it does recognize defined for it in this 
    /// metadata.
    /// 
    /// \sa \ref Usd_OM_FallbackPrimTypes UsdSchemaRegistry::GetFallbackPrimTypes
    USD_API
    void WriteFallbackPrimTypes();

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
    USD_API
    double GetStartTimeCode() const;

    /// Sets the stage's start timeCode. 
    /// 
    /// The start timeCode is set in the current EditTarget, if it is the root 
    /// layer of the stage or the session layer associated with the stage. If 
    /// the current EditTarget is neither, a warning is issued and the start 
    /// timeCode is not set.
    USD_API
    void SetStartTimeCode(double);

    /// Returns the stage's end timeCode. If the stage has an associated
    /// session layer with an end timeCode opinion, this value is returned. 
    /// Otherwise, the end timeCode opinion from the root layer is returned.
    USD_API
    double GetEndTimeCode() const;

    /// Sets the stage's end timeCode. 
    /// 
    /// The end timeCode is set in the current EditTarget, if it is the root 
    /// layer of the stage or the session layer associated with the stage. If 
    /// the current EditTarget is neither, a warning is issued and the end 
    /// timeCode is not set.
    USD_API
    void SetEndTimeCode(double);

    /// Returns true if the stage has both start and end timeCodes 
    /// authored in the session layer or the root layer of the stage.
    USD_API
    bool HasAuthoredTimeCodeRange() const;

    /// Returns the stage's timeCodesPerSecond value.
    /// 
    /// The timeCodesPerSecond value scales the time ordinate for the samples
    /// contained in the stage to seconds. If timeCodesPerSecond is 24, then a 
    /// sample at time ordinate 24 should be viewed exactly one second after the 
    /// sample at time ordinate 0.
    ///
    /// Like SdfLayer::GetTimeCodesPerSecond, this accessor uses a dynamic
    /// fallback to framesPerSecond.  The order of precedence is:
    ///
    /// \li timeCodesPerSecond from session layer
    /// \li timeCodesPerSecond from root layer
    /// \li framesPerSecond from session layer
    /// \li framesPerSecond from root layer
    /// \li fallback value of 24
    USD_API
    double GetTimeCodesPerSecond() const;

    /// Sets the stage's timeCodesPerSecond value.
    ///
    /// The timeCodesPerSecond value is set in the current EditTarget, if it 
    /// is the root layer of the stage or the session layer associated with the 
    /// stage. If the current EditTarget is neither, a warning is issued and no 
    /// value is set.
    ///
    /// \sa GetTimeCodesPerSecond()
    USD_API
    void SetTimeCodesPerSecond(double timeCodesPerSecond) const;

    /// Returns the stage's framesPerSecond value.
    /// 
    /// This makes an advisory statement about how the contained data can be 
    /// most usefully consumed and presented.  It's primarily an indication of 
    /// the expected playback rate for the data, but a timeline editing tool 
    /// might also want to use this to decide how to scale and label its 
    /// timeline.  
    ///
    /// The default value of framesPerSecond is 24.
    USD_API
    double GetFramesPerSecond() const;
    
    /// Sets the stage's framesPerSecond value.
    /// 
    /// The framesPerSecond value is set in the current EditTarget, if it 
    /// is the root layer of the stage or the session layer associated with the 
    /// stage. If the current EditTarget is neither, a warning is issued and no 
    /// value is set.
    /// 
    /// \sa GetFramesPerSecond()
    USD_API
    void SetFramesPerSecond(double framesPerSecond) const;
    
    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_ColorConfigurationAPI
    /// \name Color Configuration API
    ///
    /// Methods for authoring and querying the display color configuration 
    /// encoded in layer metadata. This color configuration information is
    /// stored as a convenience for use in pipeline tools and is unrelated
    /// to color space information associated with Usd attributes or textures.
    /// 
    /// Site-wide fallback values for the colorConfiguration and
    /// colorManagementSystem metadata can be set in the plugInfo.json file of 
    /// a plugin using this structure:
    /// 
    /// \code{.json}
    ///         "UsdColorConfigFallbacks": {
    ///             "colorConfiguration" = "https://path/to/color/config.ocio",
    ///             "colorManagementSystem" : "OpenColorIO"
    ///         }
    /// \endcode
    /// 
    /// @{
    // --------------------------------------------------------------------- //

    /// Sets the default color configuration to be used for querying color
    /// configuration metadata stored in a layer. This data is informational
    /// for use in pipeline tools.
    /// 
    /// \ref Usd_ColorConfigurationAPI "Color Configuration API"
    USD_API
    void SetColorConfiguration(const SdfAssetPath &colorConfig) const;

    /// Returns the default color configuration stored in layer metadata.
    /// 
    /// \ref Usd_ColorConfigurationAPI "Color Configuration API"
    USD_API
    SdfAssetPath GetColorConfiguration() const;

    /// Sets the name of the color management system used to interpret the 
    /// color configuration file pointed at by the colorConfiguration metadata.
    /// 
    /// \ref Usd_ColorConfigurationAPI "Color Configuration API"
    USD_API
    void SetColorManagementSystem(const TfToken &cms) const;

    /// Sets the name of the color management system to be used for loading 
    /// and interpreting the color configuration file.
    /// 
    /// \ref Usd_ColorConfigurationAPI "Color Configuration API"
    USD_API
    TfToken GetColorManagementSystem() const;

    /// Returns the global fallback values of 'colorConfiguration' and 
    /// 'colorManagementSystem'. These are set in the plugInfo.json file 
    /// of a plugin, but can be overridden by calling the static method 
    /// SetColorConfigFallbacks().
    /// 
    /// The python wrapping of this method returns a tuple containing 
    /// (colorConfiguration, colorManagementSystem).
    /// 
    /// \sa SetColorConfigFallbacks,
    /// \ref Usd_ColorConfigurationAPI "Color Configuration API"
    USD_API
    static void GetColorConfigFallbacks(SdfAssetPath *colorConfiguration,
                                        TfToken *colorManagementSystem);

    /// Sets the global fallback values of color configuration metadata which 
    /// includes the 'colorConfiguration' asset path and the name of the 
    /// color management system. This overrides any fallback values authored 
    /// in plugInfo files.
    /// 
    /// If the specified value of \p colorConfiguration or 
    /// \p colorManagementSystem is empty, then the corresponding fallback 
    /// value isn't set. In other words, for this call to have an effect, 
    /// at least one value must be non-empty. Additionally, these can't be
    /// reset to empty values.
    ///
    /// \sa GetColorConfigFallbacks()
    /// \ref Usd_ColorConfigurationAPI "Color Configuration API"
    USD_API
    static void
    SetColorConfigFallbacks(const SdfAssetPath &colorConfiguration, 
                            const TfToken &colorManagementSystem);

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
    USD_API
    void SetInterpolationType(UsdInterpolationType interpolationType);

    /// Returns the interpolation type used during value resolution
    /// for all attributes on this stage.
    USD_API
    UsdInterpolationType GetInterpolationType() const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor Usd_instancing
    /// \name Instancing
    /// See \ref Usd_Page_ScenegraphInstancing for more details.
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns all native instancing prototype prims.
    USD_API
    std::vector<UsdPrim> GetPrototypes() const;

    /// @}

private:
    struct _IncludePayloadsPredicate;

    // --------------------------------------------------------------------- //
    // Stage Construction & Initialization
    // --------------------------------------------------------------------- //

    UsdStage(const SdfLayerRefPtr& rootLayer,
             const SdfLayerRefPtr& sessionLayer,
             const ArResolverContext& pathResolverContext,
             const UsdStagePopulationMask& mask,
             InitialLoadSet load);

    // Helper for Open() overloads -- searches and publishes to bound caches.
    template <class... Args>
    static UsdStageRefPtr _OpenImpl(InitialLoadSet load, Args const &... args);

    // Releases resources used by this stage.
    void _Close();

    // Common ref ptr initialization, called by public, static constructors.
    //
    // This method will either return a valid refptr (if the stage is correctly
    // initialized) or it will return a null ref pointer, deleting the
    // raw stage pointer in the process.
    static UsdStageRefPtr
    _InstantiateStage(const SdfLayerRefPtr &rootLayer,
                      const SdfLayerRefPtr &sessionLayer,
                      const ArResolverContext &pathResolverContext,
                      const UsdStagePopulationMask &mask,
                      InitialLoadSet load);

    // --------------------------------------------------------------------- //
    // Spec Existence & Definition Helpers
    // --------------------------------------------------------------------- //

    SdfPropertySpecHandleVector
    _GetPropertyStack(const UsdProperty &prop, UsdTimeCode time) const;

    std::vector<std::pair<SdfPropertySpecHandle, SdfLayerOffset>> 
    _GetPropertyStackWithLayerOffsets(
        const UsdProperty &prop, UsdTimeCode time) const;

    static SdfPrimSpecHandleVector 
    _GetPrimStack(const UsdPrim &prim);

    static std::vector<std::pair<SdfPrimSpecHandle, SdfLayerOffset>> 
    _GetPrimStackWithLayerOffsets(const UsdPrim &prim);

    UsdPrimDefinition::Property
    _GetSchemaProperty(const UsdProperty &prop) const;

    UsdPrimDefinition::Attribute
    _GetSchemaAttribute(const UsdAttribute &attr) const;

    UsdPrimDefinition::Relationship
    _GetSchemaRelationship(const UsdRelationship &rel) const;

    SdfAttributeSpecHandle
    _CreateNewSpecFromSchemaAttribute(
        const UsdPrim &prim,
        const UsdPrimDefinition::Attribute &attrDef);

    SdfRelationshipSpecHandle
    _CreateNewSpecFromSchemaRelationship(
        const UsdPrim &prim,
        const UsdPrimDefinition::Relationship &relDef);

    template <class PropType> 
    SdfHandle<PropType>
    _CreateNewPropertySpecFromSchema(const UsdProperty &prop);

    SdfPrimSpecHandle
    _CreatePrimSpecForEditing(const UsdPrim& prim);

    template <class PropType>
    SdfHandle<PropType>
    _CreatePropertySpecForEditing(const UsdProperty &prop);

    SdfPropertySpecHandle
    _CreatePropertySpecForEditing(const UsdProperty &prop);

    SdfAttributeSpecHandle
    _CreateAttributeSpecForEditing(const UsdAttribute &attr);

    SdfRelationshipSpecHandle
    _CreateRelationshipSpecForEditing(const UsdRelationship &rel);

    // Check if the given path is valid to use with the prim creation API,
    // like DefinePrim. If it is valid, returns (true, GetPrimAtPath(path)).
    // Otherwise, returns (false, UsdPrim()).
    std::pair<bool, UsdPrim> 
    _IsValidPathForCreatingPrim(const SdfPath &path) const;

    // Validates that editing a specified prim is allowed. If editing is not
    // allowed, issues a coding error like "Cannot <operation> ..." and 
    // returns false. Otherwise, returns true.
    bool _ValidateEditPrim(const UsdPrim &prim, const char* operation) const;
    bool _ValidateEditPrimAtPath(const SdfPath &primPath, 
                                 const char* operation) const;

    UsdPrim _DefinePrim(const SdfPath &path, const TfToken &typeName);

    bool _RemoveProperty(const SdfPath& path);

    UsdProperty _FlattenProperty(const UsdProperty &srcProp,
                                 const UsdPrim &dstParent, 
                                 const TfToken &dstName);

    // --------------------------------------------------------------------- //
    // Value & Metadata Authoring
    // --------------------------------------------------------------------- //

    // Trait that allows us to call the correct versions of _SetValue and 
    // _SetMetadata for types whose values need to be mapped when written to
    // different edit targets.
    template <class T>
    struct _IsEditTargetMappable {
        static const bool value =
            std::is_same<T, SdfTimeCode>::value ||
            std::is_same<T, VtArray<SdfTimeCode>>::value ||
            std::is_same<T, SdfPathExpression>::value ||
            std::is_same<T, VtArray<SdfPathExpression>>::value ||
            std::is_same<T, SdfTimeSampleMap>::value ||
            std::is_same<T, TsSpline>::value ||
            std::is_same<T, VtDictionary>::value;
    };

    // Set value for types that don't need to be mapped for edit targets.
    template <class T>
    typename std::enable_if<!_IsEditTargetMappable<T>::value, bool>::type
    _SetValue(
        UsdTimeCode time, const UsdAttribute &attr, const T &newValue);

    // Set value for types that do need to be mapped for edit targets.
    template <class T>
    typename std::enable_if<_IsEditTargetMappable<T>::value, bool>::type
    _SetValue(
        UsdTimeCode time, const UsdAttribute &attr, const T &newValue);

    // Set value for dynamically typed VtValue. Will map the value across edit
    // targets if the held value type supports it.
    bool _SetValue(
        UsdTimeCode time, const UsdAttribute &attr, const VtValue &newValue);

    template <class T>
    bool _SetEditTargetMappedValue(
        UsdTimeCode time, const UsdAttribute &attr, const T &newValue);

    TfType _GetAttributeValueType(
        const UsdAttribute &attr) const;

    template <class T>
    bool _SetValueImpl(
        UsdTimeCode time, const UsdAttribute &attr, const T& value);

    bool _ClearValue(UsdTimeCode time, const UsdAttribute &attr);

    // Set metadata for types that don't need to be mapped across edit targets.
    template <class T>
    typename std::enable_if<!_IsEditTargetMappable<T>::value, bool>::type
    _SetMetadata(const UsdObject &object, const TfToken& key,
                 const TfToken &keyPath, const T& value);

    // Set metadata for types that do need to be mapped for edit targets.
    template <class T>
    typename std::enable_if<_IsEditTargetMappable<T>::value, bool>::type
    _SetMetadata(const UsdObject &object, const TfToken& key,
                 const TfToken &keyPath, const T& value);

    // Set metadata for dynamically typed VtValue. Will map the value across 
    // edit targets if the held value type supports it.
    USD_API
    bool _SetMetadata(const UsdObject &object,
                      const TfToken& key,
                      const TfToken &keyPath,
                      const VtValue& value);

    template <class T>
    bool _SetEditTargetMappedMetadata(
        const UsdObject &obj, const TfToken& fieldName,
        const TfToken &keyPath, const T &newValue);

    template <class T>
    bool _SetMetadataImpl(
        const UsdObject &obj, const TfToken& fieldName,
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
    // changes to prototypes and instances due to the discovery of new instances
    // during composition.
    void _ComposePrimIndexesInParallel(
        const std::vector<SdfPath>& primIndexPaths,
        const std::string& context,
        Usd_InstanceChanges* instanceChanges = nullptr);

    // Recompose the subtree rooted at \p prim: compose its type, flags, and
    // list of children, then invoke _ComposeSubtree on all its children.
    void _ComposeSubtree(
        Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
        UsdStagePopulationMask const *mask,
        const SdfPath &primIndexPath = SdfPath());
    void _ComposeSubtreeImpl(
        Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
        UsdStagePopulationMask const *mask,
        const SdfPath &primIndexPath = SdfPath());
    void _ComposeSubtreesInParallel(
        const std::vector<Usd_PrimDataPtr> &prims,
        const std::vector<SdfPath> *primIndexPaths = nullptr);

    // Composes the full prim type info for the prim based on its type name
    // and applied API schemas.
    void _ComposePrimTypeInfoImpl(Usd_PrimDataPtr prim);

    // Compose subtree rooted at \p prim under \p parent.  This function
    // ensures that the appropriate prim index is specified for \p prim if
    // \p parent is in a prototype.
    void _ComposeChildSubtree(Usd_PrimDataPtr prim, 
                              Usd_PrimDataConstPtr parent,
                              UsdStagePopulationMask const *mask);

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
    void _ComposeChildren(Usd_PrimDataPtr prim,
                          UsdStagePopulationMask const *mask, bool recurse);

    // Instantiate a prim instance.  There must not already be an instance
    // at \p primPath.
    Usd_PrimDataPtr _InstantiatePrim(const SdfPath &primPath);

    // Instantiate a prototype prim and sets its parent to pseudoroot.  
    // There must not already be a prototype at \p primPath.
    Usd_PrimDataPtr _InstantiatePrototypePrim(const SdfPath &primPath);

    // For \p prim and all of its descendants, remove from _primMap and empty
    // their _children vectors.
    void _DestroyPrim(Usd_PrimDataPtr prim);

    // Destroy the prim subtrees rooted at each path in \p paths. \p paths may
    // not contain any path that is a descendent of another path in \p paths.
    void _DestroyPrimsInParallel(const std::vector<SdfPath>& paths);

    // Invoke _DestroyPrim() on all of \p prim's direct children.
    void _DestroyDescendents(Usd_PrimDataPtr prim);

    // Returns true if the object at the given path is a descendant of
    // an instance prim, i.e. a prim beneath an instance prim, or a property
    // of a prim beneath an instance prim.
    bool _IsObjectDescendantOfInstance(const SdfPath& path) const;

    // If the given prim is an instance, returns the corresponding 
    // prototype prim.  Otherwise, returns an invalid prim.
    Usd_PrimDataConstPtr _GetPrototypeForInstance(Usd_PrimDataConstPtr p) const;

    // Returns the path of the Usd prim using the prim index at the given path.
    SdfPath _GetPrimPathUsingPrimIndexAtPath(const SdfPath& primIndexPath) const;

    // Responds to LayersDidChangeSentPerLayer event and update stage contents 
    // in response to changes in scene description.
    void _HandleLayersDidChange(const SdfNotice::LayersDidChangeSentPerLayer &);

    // Pushes changes through PCP to determine invalidation based on 
    // composition metadata.
    // Note: This method will not perform any processing or notification
    // Refer to _ProcessPendingChanges()
    void _ComputePendingChanges(const SdfLayerChangeListVec &);

    // Update stage contents in response to changes to the asset resolver.
    void _HandleResolverDidChange(const ArNotice::ResolverChanged &);

    // Process stage change information stored in _pendingChanges.
    // _pendingChanges will be set to nullptr by the end of the function.
    // This function will return true if UsdNotice::ObjectsChanged and 
    // UsdNotice::StageContentsChanged notices were sent during execution.
    bool _ProcessPendingChanges();

    // Remove scene description for the prim at \p fullPath in the current edit
    // target.
    bool _RemovePrim(const SdfPath& fullPath);

    SdfPrimSpecHandle _GetPrimSpec(const SdfPath& fullPath);

    // Find and return the defining spec type for the property spec at the given
    // path, or SdfSpecTypeUnknown if none exists.  The defining spec type is
    // either the builtin definition's spec type, if the indicated property is
    // builtin, otherwise it's the strongest authored spec's type if one exists,
    // otherwise it's SdfSpecTypeUnknown.
    SdfSpecType _GetDefiningSpecType(Usd_PrimDataConstPtr primData,
                                     const TfToken &propName) const;

    // Helper to apply Pcp changes and recompose the scenegraph accordingly,
    // given an optional initial set of paths to recompose.
    void _Recompose(const PcpChanges &changes);
    template <class T>
    void _Recompose(const PcpChanges &changes, T *pathsToRecompose);
    template <class T>
    void _RecomposePrims(T *pathsToRecompose);

    // Helper for _Recompose to find the subtrees that need to be
    // fully recomposed and to recompose the name children of the
    // parents of these subtrees. Note that [start, finish) must be a
    // sorted range of map iterators whose keys are paths with no descendent
    // paths. In C++20, consider using the ranges API to improve this.
    template <class Iter>
    void _ComputeSubtreesToRecompose(Iter start, Iter finish,
                                     std::vector<Usd_PrimDataPtr>* recompose);

    // return true if the path is valid for load/unload operations.
    // This method will emit errors when invalid paths are encountered.
    bool _IsValidForLoad(const SdfPath& path) const;
    bool _IsValidForUnload(const SdfPath& path) const;

    // Discover all payloads in a given subtree, adding the path of each
    // discovered prim index to the \p primIndexPaths set. If specified,
    // the corresponding UsdPrim path will be added to the \p usdPrimPaths
    // set. The root path will be considered for inclusion in the result set.
    //
    // Note that some payloads may not be discoverable in until an ancestral
    // payload has been included. UsdStage::LoadAndUnload takes this into
    // account.
    void _DiscoverPayloads(const SdfPath& rootPath,
                           UsdLoadPolicy policy,
                           SdfPathSet* primIndexPaths,
                           bool unloadedOnly = false,
                           SdfPathSet* usdPrimPaths = nullptr) const;

    // ===================================================================== //
    //                          VALUE RESOLUTION                             //
    // ===================================================================== //
    // --------------------------------------------------------------------- //
    // Specialized Value Resolution
    // --------------------------------------------------------------------- //

    // Helpers for resolving values for metadata fields requiring
    // special behaviors.
    static SdfSpecifier _GetSpecifier(Usd_PrimDataConstPtr primData);
    static TfToken _GetKind(Usd_PrimDataConstPtr primData);
    static bool _IsActive(Usd_PrimDataConstPtr primData);

    // Custom is true if it is true anywhere in the stack.
    bool _IsCustom(const UsdProperty &prop) const;

    // Variability is determined by the weakest opinion in the stack.
    SdfVariability _GetVariability(const UsdProperty &prop) const;

    // Helper functions for resolving asset paths during value resolution.
    void _MakeResolvedAssetPaths(UsdTimeCode time, const UsdAttribute &attr,
                                 SdfAssetPath *assetPaths,
                                 size_t numAssetPaths,
                                 bool anchorAssetPathsOnly = false) const;

    void _MakeResolvedAssetPathsValue(UsdTimeCode time, const UsdAttribute &attr,
                                      VtValue *value,
                                      bool anchorAssetPathsOnly = false) const;

    void _MakeResolvedTimeCodes(UsdTimeCode time, const UsdAttribute &attr,
                                SdfTimeCode *timeCodes,
                                size_t numTimeCodes) const;

    void _MakeResolvedPathExpressions(
        UsdTimeCode time, const UsdAttribute &attr,
        SdfPathExpression *pathExprs,
        size_t numPathExprs) const;

    void _MakeResolvedAttributeValue(UsdTimeCode time, const UsdAttribute &attr,
                                     VtValue *value) const;

    // --------------------------------------------------------------------- //
    // Metadata Resolution
    // --------------------------------------------------------------------- //

public:
    // Trait that allows us to call the correct version of _GetMetadata for 
    // types that require type specific value resolution as opposed to just
    // strongest opinion. These types also use type specific resolution 
    // in _GetValue.
    template <class T>
    struct _HasTypeSpecificResolution {
        static const bool value =
            std::is_same<T, SdfAssetPath>::value ||
            std::is_same<T, VtArray<SdfAssetPath>>::value ||
            std::is_same<T, SdfTimeCode>::value ||
            std::is_same<T, VtArray<SdfTimeCode>>::value ||
            std::is_same<T, SdfPathExpression>::value ||
            std::is_same<T, VtArray<SdfPathExpression>>::value ||
            std::is_same<T, SdfTimeSampleMap>::value ||
            std::is_same<T, TsSpline>::value ||
            std::is_same<T, VtDictionary>::value;
    };

private:
    // Get metadata for types that do not have type specific value resolution.
    template <class T>
    typename std::enable_if<!_HasTypeSpecificResolution<T>::value, bool>::type
    _GetMetadata(const UsdObject &obj,
                 const TfToken& fieldName,
                 const TfToken &keyPath,
                 bool useFallbacks,
                 T* result) const;

    // Get metadata for types that do have type specific value resolution.
    template <class T>
    typename std::enable_if<_HasTypeSpecificResolution<T>::value, bool>::type
    _GetMetadata(const UsdObject &obj,
                 const TfToken& fieldName,
                 const TfToken &keyPath,
                 bool useFallbacks,
                 T* result) const;

    // Get metadata as a dynamically typed VtValue. Will perform type specific
    // value resolution if the returned held type requires it.
    bool _GetMetadata(const UsdObject &obj,
                      const TfToken& fieldName,
                      const TfToken &keyPath,
                      bool useFallbacks,
                      VtValue* result) const;

    // Gets a metadata value using only strongest value resolution. It is 
    // assumed that result is holding a value that does not require type 
    // specific value resolution.
    USD_API
    bool _GetStrongestResolvedMetadata(const UsdObject &obj,
                                       const TfToken& fieldName,
                                       const TfToken &keyPath,
                                       bool useFallbacks,
                                       SdfAbstractDataValue* result) const;

    // Gets a metadata value with the type specific value resolution for the 
    // type applied. This is only implemented for types that 
    // _HasTypeSpecificResolution.
    template <class T>
    USD_API
    bool _GetTypeSpecificResolvedMetadata(const UsdObject &obj,
                                          const TfToken& fieldName,
                                          const TfToken &keyPath,
                                          bool useFallbacks,
                                          T* result) const;

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
    bool _GetSpecialPropMetadataImpl(const UsdObject &obj,
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

    // NOTE: The "authoredOnly" flag is not yet in use, but when we have
    // support for prim-based metadata fallbacks, they should be ignored when
    // this flag is set to true.
    bool _HasMetadata(const UsdObject &obj, const TfToken& fieldName,
                      const TfToken &keyPath, bool useFallbacks) const;

    TfTokenVector
    _ListMetadataFields(const UsdObject &obj, bool useFallbacks) const;

    void _GetAllMetadata(const UsdObject &obj,
                         bool useFallbacks,
                         UsdMetadataValueMap* result,
                         bool anchorAssetPathsOnly = false) const;

    // --------------------------------------------------------------------- //
    // Default & TimeSample Resolution
    // --------------------------------------------------------------------- //

    void _GetResolveInfo(const UsdAttribute &attr, 
                         UsdResolveInfo *resolveInfo,
                         const UsdTimeCode *time = nullptr) const;

    void _GetResolveInfoWithResolveTarget(
        const UsdAttribute &attr, 
        const UsdResolveTarget &resolveTarget,
        UsdResolveInfo *resolveInfo,
        const UsdTimeCode *time = nullptr) const;

    template <class T> struct _ExtraResolveInfo;

    // Gets the value resolve info for the given attribute. If time is provided,
    // the resolve info is evaluated for that specific time (which may be 
    // default). Otherwise, if time is null, the resolve info is evaluated for
    // "any numeric time" and will not populate values in extraInfo that 
    // require a specific time to be evaluated.
    template <class T>
    void _GetResolveInfo(const UsdAttribute &attr, 
                         UsdResolveInfo *resolveInfo,
                         const UsdTimeCode *time = nullptr,
                         _ExtraResolveInfo<T> *extraInfo = nullptr) const;

    // Gets the value resolve info for the given attribute using the given 
    // resolve target. If time is provided, the resolve info is evaluated for 
    // that specific time (which may be default). Otherwise, if time is null, 
    // the resolve info is evaluated for "any numeric time" and will not 
    // populate values in extraInfo that require a specific time to be 
    // evaluated.
    template <class T>
    void _GetResolveInfoWithResolveTarget(
        const UsdAttribute &attr, 
        const UsdResolveTarget &resolveTarget,
        UsdResolveInfo *resolveInfo,
        const UsdTimeCode *time = nullptr,
        _ExtraResolveInfo<T> *extraInfo = nullptr) const;

    // Shared implementation function for _GetResolveInfo and 
    // _GetResolveInfoWithResolveTarget. The only difference between how these
    // two functions behave is in how they create the Usd_Resolver used for 
    // iterating over nodes and layers, thus they provide this implementation
    // with the needed MakeUsdResolverFn to create the Usd_Resolver.
    template <class T, class MakeUsdResolverFn>
    void _GetResolveInfoImpl(const UsdAttribute &attr, 
                         UsdResolveInfo *resolveInfo,
                         const UsdTimeCode *time,
                         _ExtraResolveInfo<T> *extraInfo,
                         const MakeUsdResolverFn &makeUsdResolveFn) const;

    template <class T> struct _ResolveInfoResolver;
    struct _PropertyStackResolver;

    template <class Resolver, class MakeUsdResolverFn>
    void _GetResolvedValueAtDefaultImpl(
        const UsdProperty &prop,
        Resolver *resolver,
        const MakeUsdResolverFn &makeUsdResolverFn) const;

    template <class Resolver, class MakeUsdResolverFn>
    void _GetResolvedValueAtTimeImpl(
        const UsdProperty &prop,
        Resolver *resolver,
        const UsdTimeCode *time,
        const MakeUsdResolverFn &makeUsdResolverFn) const;

    bool _GetValue(UsdTimeCode time, const UsdAttribute &attr, 
                   VtValue* result) const;

    template <class T>
    bool _GetValue(UsdTimeCode time, const UsdAttribute &attr,
                   T* result) const;

    template <class T>
    bool _GetValueImpl(UsdTimeCode time, const UsdAttribute &attr, 
                       Usd_InterpolatorBase* interpolator,
                       T* value) const;

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

    template <class T>
    bool _GetDefaultValueFromResolveInfoImpl(const UsdResolveInfo &info,
                                             const UsdAttribute &attr,
                                             T* value) const;

    Usd_AssetPathContext
    _GetAssetPathContext(UsdTimeCode time, const UsdAttribute &attr) const;

    // --------------------------------------------------------------------- //
    // Specialized Time Sample I/O
    // --------------------------------------------------------------------- //

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
    void _RegisterResolverChangeNotice();

    // Helper to obtain a malloc tag string for this stage.
    inline char const *_GetMallocTagId() const;

private:
    class _PendingChanges;

    // Change block for use by the UsdNamespaceEditor to allow it to indicate
    // to its dependent stages what the expected namespace edits are when the
    // stages handles notices from the changes the namespace editor performs.
    // The stage uses this provide additional information about prim resyncs
    // related to namespace edits in the ObjectsChanged notice it sends.
    class _NamespaceEditsChangeBlock {
    public:
        // Info about an expected namespace edit change from UsdNamespaceEditor.
        // This includes the original pre-edit prim stack of the prim at the old
        // path which is used to determine if the prim at the new path has the
        // same composed contents after the edits as the prim had originally at
        // the old path before the edits.
        struct ExpectedNamespaceEditChange {
            SdfPath oldPath;
            SdfPath newPath;
            SdfPrimSpecHandleVector oldPrimStack;
        };
        using ExpectedNamespaceEditChangeVector = 
            std::vector<ExpectedNamespaceEditChange>;

        _NamespaceEditsChangeBlock(const UsdStagePtr &stage,
            ExpectedNamespaceEditChangeVector &&expectedChanges);
        _NamespaceEditsChangeBlock(_NamespaceEditsChangeBlock &&);
        ~_NamespaceEditsChangeBlock();

    private:
        UsdStagePtr _stage;
        std::unique_ptr<_PendingChanges> _localPendingChanges;
    };

    // The 'pseudo root' prim.
    Usd_PrimDataPtr _pseudoRoot;

    // The stage's root layer.
    SdfLayerRefPtr _rootLayer;

    // Every UsdStage has an implicit, in-memory session layer.
    // This is to allow for runtime overrides such as variant selections.
    SdfLayerRefPtr _sessionLayer;

    // The stage's EditTarget.
    UsdEditTarget _editTarget;
    bool _editTargetIsLocalLayer;

    std::unique_ptr<PcpCache> _cache;
    std::unique_ptr<Usd_ClipCache> _clipCache;
    std::unique_ptr<Usd_InstanceCache> _instanceCache;

    TfHashMap<TfToken, TfToken, TfHash> _invalidPrimTypeToFallbackMap;

    size_t _usedLayersRevision;

    // A concurrent map from Path to Prim, for fast random access.
    struct _TbbHashEq {
        inline bool equal(SdfPath const &l, SdfPath const &r) const {
            return l == r;
        }
        inline size_t hash(SdfPath const &path) const {
            return path.GetHash();
        }
    };
    using PathToNodeMap = tbb::concurrent_hash_map<
        SdfPath, Usd_PrimDataIPtr, _TbbHashEq>;
    PathToNodeMap _primMap;

    // The interpolation type used for all attributes on the stage.
    UsdInterpolationType _interpolationType;

    typedef std::vector<
        std::pair<SdfLayerHandle, TfNotice::Key> > _LayerAndNoticeKeyVec;
    _LayerAndNoticeKeyVec _layersAndNoticeKeys;
    size_t _lastChangeSerialNumber;

    TfNotice::Key _resolverChangeKey;

    // Data for pending change processing.
    _PendingChanges* _pendingChanges;

    std::optional<WorkDispatcher> _dispatcher;

    // To provide useful aggregation of malloc stats, we bill everything
    // for this stage - from all access points - to this tag.
    std::unique_ptr<std::string> _mallocTagID;

    // The state used when instantiating the stage.
    const InitialLoadSet _initialLoadSet;

    // The population mask that applies to this stage.
    UsdStagePopulationMask _populationMask;
    
    // The load rules that apply to this stage.
    UsdStageLoadRules _loadRules;
    
    bool _isClosingStage;
    bool _isWritingFallbackPrimTypes;

    friend class UsdAPISchemaBase;
    friend class UsdAttribute;
    friend class UsdAttributeQuery;
    friend class UsdEditTarget;
    friend class UsdInherits;
    friend class UsdNamespaceEditor;
    friend class UsdObject;
    friend class UsdPrim;
    friend class UsdProperty;
    friend class UsdRelationship;
    friend class UsdSpecializes;
    friend class UsdVariantSet;
    friend class UsdVariantSets;
    friend class Usd_AssetPathContext;
    friend class Usd_FlattenAccess;
    friend class Usd_PcpCacheAccess;
    friend class Usd_PrimData;
    friend class Usd_StageOpenRequest;
    friend class Usd_TypeQueryAccess;
    template <class T> friend struct Usd_AttrGetValueHelper;
    friend struct Usd_AttrGetUntypedValueHelper;
    template <class RefsOrPayloadsEditorType, class RefsOrPayloadsProxyType> 
        friend struct Usd_ListEditImpl;
};

// UsdObject's typed metadata query relies on this specialization being
// externally visible and exporting the primary template does not
// automatically export this specialization.
template <>
USD_API
bool
UsdStage::_GetTypeSpecificResolvedMetadata(const UsdObject &obj,
                                           const TfToken& fieldName,
                                           const TfToken &keyPath,
                                           bool useFallbacks,
                                           SdfTimeSampleMap* result) const;

template<typename T>
bool
UsdStage::GetMetadata(const TfToken& key, T* value) const
{
    VtValue result;
    if (!GetMetadata(key, &result)){
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
    if (!GetMetadataByDictKey(key, keyPath, &result)){
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

// Get metadata for types that do not have type specific value resolution.
template <class T>
typename std::enable_if<
    !UsdStage::_HasTypeSpecificResolution<T>::value, bool>::type
UsdStage::_GetMetadata(const UsdObject &obj,
                       const TfToken& fieldName,
                       const TfToken &keyPath,
                       bool useFallbacks,
                       T* result) const
{
    // Since these types don't have type specific value resolution, we can just 
    // get the strongest metadata value and be done.
    SdfAbstractDataTypedValue<T> out(result);
    return _GetStrongestResolvedMetadata(
        obj, fieldName, keyPath, useFallbacks, &out);
}

// Get metadata for types that do have type specific value resolution.
template <class T>
typename std::enable_if<
    UsdStage::_HasTypeSpecificResolution<T>::value, bool>::type
UsdStage::_GetMetadata(const UsdObject &obj,
                       const TfToken& fieldName,
                       const TfToken &keyPath,
                       bool useFallbacks,
                       T* result) const
{
    // Call the templated type specifice resolved metadata implementation that 
    // will only be implemented for types that support it.
    return _GetTypeSpecificResolvedMetadata(
        obj, fieldName, keyPath, useFallbacks, result);
}


// Set metadata for types that don't need to be mapped across edit targets.
template <class T>
typename std::enable_if<!UsdStage::_IsEditTargetMappable<T>::value, bool>::type
UsdStage::_SetMetadata(const UsdObject &object, const TfToken& key,
                       const TfToken &keyPath, const T& value)
{
    // Since we know that we don't need to map the value for edit targets, 
    // we can just type erase the value and set the metadata as is.
    SdfAbstractDataConstTypedValue<T> in(&value);
    return _SetMetadataImpl<SdfAbstractDataConstValue>(
        object, key, keyPath, in);
}

// Set metadata for types that do need to be mapped for edit targets.
template <class T>
typename std::enable_if<UsdStage::_IsEditTargetMappable<T>::value, bool>::type
UsdStage::_SetMetadata(const UsdObject &object, const TfToken& key,
                       const TfToken &keyPath, const T& value)
{
    return _SetEditTargetMappedMetadata(object, key, keyPath, value);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_STAGE_H

