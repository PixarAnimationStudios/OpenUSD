//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PROFILE_REGISTRY_H
#define PXR_USD_PROFILE_REGISTRY_H

/// \file usdProfiles/profileRegistry.h

#include "pxr/usd/usdProfiles/api.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/js/types.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdProfileRegistry
///
/// A singleton that manages profile capabilities and their relationships.
/// The profile registry collects capability information from plugins and
/// provides methods to query capabilities and their relationships.
///
/// \section profile_threadsafety UsdProfileRegistry Threadsafety
///
/// UsdProfileRegistry serves performance-critical clients that operate under the
/// STL threading model, and therefore itself follows that model in order
/// to avoid locking during queries.
///
class UsdProfileRegistry : public TfSingleton<UsdProfileRegistry>
{
    UsdProfileRegistry(const UsdProfileRegistry&) = delete;
    UsdProfileRegistry& operator=(const UsdProfileRegistry&) = delete;
    public:
    /// Result of a predecessor reachability query.
    /// \see HasPredecessor
    enum class QueryStatus {
        /// No path exists from \p capability to the candidate.
        NoPath,
        /// A non-deprecated path exists.
        ValidPath,
        /// All paths to the candidate pass through at least one deprecated edge.
        Deprecated,
        /// Both deprecated and non-deprecated paths exist.
        DeprecationConflict,
        /// The capability is reachable but was explicitly excluded by the caller
        /// via the \p excepted parameter of \c CoversCapabilities.
        /// Treated as \c NoPath for aggregate coverage purposes, but reported
        /// distinctly so callers can distinguish "not present" from "present but
        /// intentionally excluded".
        Excepted,
        /// A cycle was detected in the graph (indicates malformed data).
        CycleFound,
    };

    /// Test whether \p capability is known to the registry.
    USDPROFILES_API static bool HasCapability(const TfToken& capability);

    /// Return true if \p capability is tagged as a profile node
    /// (i.e., declared with \c "isProfile": true in plugInfo.json).
    /// Returns false for unknown capabilities.
    /// \sa HasCapability
    USDPROFILES_API static bool IsProfile(const TfToken& capability);

    /// Return metadata associated with \p capability as a \c VtDictionary with
    /// keys \c name, \c docstring, \c style, and \c subgraph.
    /// Returns an empty dictionary if the capability is unknown.
    /// \sa HasCapability
    USDPROFILES_API
    static VtDictionary GetCapabilityMetadata(const TfToken& capability);

    /// Per-capability query result
    struct CapabilityResult {
        TfToken capability;
        QueryStatus status;
    };

    /// Return all direct predecessors (incoming edges) of \p capability.
    /// Each entry carries the predecessor token and \c ValidPath or
    /// \c Deprecated reflecting whether that edge is deprecated.
    /// Returns an empty vector if \p capability is unknown or has no predecessors.
    /// \sa HasCapability
    USDPROFILES_API static
    std::vector<CapabilityResult> GetPredecessors(const TfToken& capability);

    /// Return the full transitive closure of all ancestors of \p capability.
    /// Each entry carries the ancestor token and the best \c QueryStatus of
    /// any path from \p capability to that ancestor (\c ValidPath,
    /// \c Deprecated, or \c DeprecationConflict).
    /// Does not include \p capability itself.
    /// Returns an empty vector if \p capability is unknown or has no predecessors.
    /// \sa HasCapability
    USDPROFILES_API static
    std::vector<CapabilityResult> GetTransitivePredecessors(const TfToken& capability);

    /// Parse the \c _vN version suffix from a capability token.
    /// Returns the base name and integer version, where an unversioned
    /// capability (e.g. \c usd.physics) returns version 0.
    /// Examples:
    /// \li \c usd.physics      → {usd.physics, 0}
    /// \li \c usd.physics_v2   → {usd.physics, 2}
    /// \li \c yoyo.foo_v10     → {yoyo.foo, 10}
    USDPROFILES_API static 
    std::pair<TfToken, int> ParseCapabilityVersion(const TfToken& capability);

    /// Return the preferred registered capability for the given token,
    /// applying the versioning precedence rules from §6 of the Profiles spec:
    /// - A versioned form (\c _vN) is preferred over the unversioned base name.
    /// - Among multiple versioned forms, the highest N wins.
    ///
    /// If \p capability is already the preferred form (or no versioned siblings
    /// are registered), returns \p capability unchanged.
    /// Returns an empty \c TfToken if neither \p capability nor any versioned
    /// sibling is registered.
    USDPROFILES_API static TfToken ResolveCapability(const TfToken& capability);

    /// Return whether \p capabilityA has \p capabilityB anywhere in its
    /// predecessor DAG (i.e., A transitively depends on B).
    /// Both tokens are resolved via \c ResolveCapability before the DAG query.
    /// Returns \c NoPath if either resolved capability is unknown or if A == B.
    /// \sa HasCapability
    USDPROFILES_API static QueryStatus HasPredecessor(const TfToken& capabilityA,
                                                      const TfToken& capabilityB);

    /// Return whether \p perspective transitively reaches every capability in
    /// \p required, and with what deprecation status.
    ///
    /// \p perspective and each token in \p required are resolved via
    /// \c ResolveCapability before the DAG query, so unversioned names
    /// automatically match the highest registered versioned form.
    ///
    /// \p excepted is an optional caller-supplied set of capabilities to
    /// exclude from coverage. A required capability whose resolved token
    /// appears in \p excepted is reported with status \c Excepted rather than
    /// its DAG-derived status, and is treated as \c NoPath for the aggregate.
    /// This allows callers to express "profile X is satisfied under these
    /// known exclusions" without modifying the registry.
    ///
    /// The aggregate \c QueryStatus precedence is:
    /// \c NoPath > \c DeprecationConflict > \c Excepted > \c Deprecated > \c ValidPath.
    /// \c CycleFound is treated as \c NoPath in the aggregate.
    ///
    /// If \p results is non-null it is populated with one entry per required
    /// capability. Each entry's \c capability field holds the \e resolved token
    /// (which may differ from the input if a higher versioned form was found),
    /// and \c status holds that capability's individual \c QueryStatus.
    /// Entries are in the same order as \p required.
    ///
    /// Returns \c NoPath (with an empty \p results) if \p perspective is
    /// unknown after resolution, or if \p required is empty.
    /// \sa HasCapability
    USDPROFILES_API static QueryStatus CoversCapabilities(
                                    const TfToken& perspective,
                                    const std::vector<TfToken>& required,
                                    const std::vector<TfToken>& excepted = {},
                                    std::vector<CapabilityResult>* results = nullptr);

    /// Return an unordered vector of all capabilities known to the registry.
    USDPROFILES_API static std::vector<TfToken> GetAllCapabilities();

    /// Return an unordered vector of all profiles known to the registry.
    /// Profiles are capabilities declared with \c "isProfile": true
    /// in plugInfo.json.
    USDPROFILES_API static std::vector<TfToken> GetAllProfiles();

    // Style, Subgraph, DocString, and DisplayName are all meant for 
    // visualization purposes. Styles can be used to give a capability a distinct
    // appearance, subgraph can be used to group capabilities into a box.

    /// Get the style token for a given capability
    USDPROFILES_API static TfToken GetStyleForCapability(const TfToken& capability);

    /// Get the subgraph name for a given capability
    USDPROFILES_API static TfToken GetSubgraphForCapability(const TfToken& capability);

    /// Get docstring for a given capability
    USDPROFILES_API static std::string GetDocString(const TfToken& capability);

    /// Get display name for a given capability
    USDPROFILES_API static std::string GetDisplayName(const TfToken& capability);

    /// Get all capability styles
    USDPROFILES_API static std::map<TfToken, std::string> GetCapabilityStyles();

private:
    friend class Usd_ProfilesRegistryTestAccess;
    friend class TfSingleton<UsdProfileRegistry>;

    UsdProfileRegistry();
    ~UsdProfileRegistry();
    
    /// Return the single \c UsdProfileRegistry instance.
    static UsdProfileRegistry& GetInstance();

    void _RegisterFromPlugins();

    /// Shared helper: parse Capabilities, CapabilityStyles, and
    /// Subgraphs from a "Profiles" JsObject into the graph.
    /// Called by both _RegisterFromPlugins and _TestLoadFromFile.
    bool _LoadCapabilitiesFromProfileData(const JsObject& profileData);
    
    class _CapabilityGraph;
    std::unique_ptr<_CapabilityGraph> _capabilityGraph;

    // Store styles and groups defined in plugins
    std::map<TfToken, std::string> _capabilityStyles;
};

USDPROFILES_API_TEMPLATE_CLASS(TfSingleton<UsdProfileRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PROFILE_REGISTRY_H

