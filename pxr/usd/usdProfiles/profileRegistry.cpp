//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/api.h"
#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/usd/usdProfiles/tokens.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/js/json.h"
#include "pxr/base/js/types.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <cctype>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(UsdProfileRegistry);
TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((PluginProfilesKey, "Profiles"))
    ((CapabilitiesKey, "Capabilities"))
    ((CapabilityStylesKey, "CapabilityStyles"))
    ((PredecessorsKey, "predecessors"))
    ((SubgraphKey, "subgraph"))
    ((StyleKey, "style"))
    ((NameKey, "name"))
    ((DocstringKey, "docstring"))
    ((IsProfileKey, "isProfile"))
    );

namespace {

/// \class Capability
///
/// Represents a single capability node within the capability DAG.
/// Maps directly to a plugInfo.json capability entry.
///
/// Names use reverse domain notation (e.g., "usd.geom.mesh").
/// Deprecation is expressed as an edge property (Edge::deprecated).
///
/// IsProfile indicates the node is tagged as a profile — a capability
/// that names a coherent set of capabilities for a specific use case
/// (e.g., a versioned OpenUSD release).
///
class Capability {
public:
    Capability()
        : _isProfile(false)
    {}

    Capability(const TfToken& id)
        : _id(id)
        , _name(id)
        , _isProfile(false)
    {}

    Capability(const TfToken& id,
                const TfToken& name,
                const std::string& docstring,
                bool isProfile = false)
        : _id(id)
        , _name(name)
        , _docstring(docstring)
        , _isProfile(isProfile)
    {}

    const TfToken& GetId() const { return _id; }
    const TfToken& GetName() const { return _name; }
    const TfToken& GetStyle() const { return _style; }
    const TfToken& GetSubgraph() const { return _subgraph; }
    const std::string& GetDocString() const { return _docstring; }
    bool IsProfile() const { return _isProfile; }

    void SetStyle(const TfToken& style) { _style = style; }
    void SetSubgraph(const TfToken& subgraph) { _subgraph = subgraph; }

private:
    TfToken _id;            // The id of the capability
    TfToken _name;          // The name of the capability
    TfToken _style;         // Style hint to be applied in visualization
    TfToken _subgraph;      // Grouping for this capability for visualization
    std::string _docstring; // Documentation
    bool _isProfile;        // True if capability is a profile
};

} // anon

class UsdProfileRegistry::_CapabilityGraph {
public:
    /// Construct empty capability graph
    _CapabilityGraph();
    ~_CapabilityGraph();

    // querying for a capability must be made from a perspective, as a 
    // capability may be deprecated, but a deprecation can only be known 
    // at a certain point in the graph. Simply querying for a capability will
    // not determine whether or not the capability is viable.
    UsdProfileRegistry::QueryStatus HasPredecessor(const TfToken& capability, 
                                                const TfToken& candidate) const;

    struct Edge {
        TfToken name;
        bool deprecated;
    };

    bool AddCapability(const TfToken& capId,
                       const std::vector<Edge>& incomingEdges,
                       const Capability& cap);

    void Clear();
    bool ValidateAcyclic(std::vector<std::string>* errors = nullptr) const;
    bool ValidateEdges(std::vector<std::string>* errors = nullptr) const;
    std::vector<TfToken> GetAllCapabilities() const;
    std::vector<TfToken> GetAllProfiles() const;
    std::optional<Capability> GetMetadata(const TfToken& cap) const;
    bool HasCapability(const TfToken& cap) const;

    UsdProfileRegistry::QueryStatus DepthFirstSearch(
                                        const TfToken& current,
                                        const TfToken& target,
                                        std::unordered_set<TfToken,
                                        TfToken::HashFunctor>& visited) const;

    std::vector<UsdProfileRegistry::CapabilityResult> 
        GetPredecessors(const TfToken& cap) const;

    std::vector<UsdProfileRegistry::CapabilityResult> 
        GetTransitivePredecessors(const TfToken& cap) const;

    bool ValidateRoot(std::vector<std::string>* errors = nullptr) const;

    // Single-pass DFS helper for GetTransitivePredecessors.
    void _DFSTransitive(
        const TfToken& node,
        bool pathDeprecated,
        std::unordered_map<TfToken, UsdProfileRegistry::QueryStatus,
                           TfToken::HashFunctor>& status,
        std::unordered_set<TfToken, TfToken::HashFunctor>& inPath) const;

    // The graph is represented as a set of nodes and their incoming edges.
    std::unordered_map<TfToken, 
                       std::vector<Edge>, TfToken::HashFunctor> incomingEdgeMap;

    // Metadata discovered from the initialization is tracked here.
    std::unordered_map<TfToken, Capability, TfToken::HashFunctor> metadata;
};

void UsdProfileRegistry::_CapabilityGraph::Clear() {
    incomingEdgeMap.clear();
    metadata.clear();
}

/// Add a single capability with its edges and metadata.
/// Returns false if the capability already exists (duplicate).
bool UsdProfileRegistry::_CapabilityGraph::AddCapability(
                    const TfToken& capId,
                    const std::vector<Edge>& incomingEdges,
                    const Capability& cap)
{
    if (incomingEdgeMap.find(capId) != incomingEdgeMap.end()) {
        TF_WARN("Duplicate capability '%s' - ignoring", capId.GetText());
        return false;
    }
    incomingEdgeMap[capId] = incomingEdges;
    metadata[capId] = cap;
    return true;
}

/// Validate that the graph is a DAG (no cycles).
/// Uses Kahn's algorithm (topological sort via in-degree counting).
/// Returns true if acyclic, false if cycles exist.
bool UsdProfileRegistry::_CapabilityGraph::ValidateAcyclic(
                    std::vector<std::string>* errors) const
{
    std::unordered_map<TfToken, int, TfToken::HashFunctor> incomingDegree;
    std::unordered_map<TfToken, std::vector<TfToken>, TfToken::HashFunctor> dependents;

    // Initialize all nodes and build forward adjacency
    for (const auto& [id, edges] : incomingEdgeMap) {
        // Ensure the node itself is tracked
        if (incomingDegree.find(id) == incomingDegree.end()) {
            incomingDegree[id] = 0;
        }
        
        // record the predecessor count for this node
        incomingDegree[id] = static_cast<int>(edges.size());
        
        for (const auto& edge : edges) {
            dependents[edge.name].push_back(id);
            // Ensure the predecessor is also tracked in incomingDegree
            if (incomingDegree.find(edge.name) == incomingDegree.end()) {
                incomingDegree[edge.name] = 0;
            }
        }
    }

    // Seed Kahn's algorithm with all zero-in-degree nodes
    std::vector<TfToken> queue;
    for (const auto& [id, degree] : incomingDegree) {
        if (degree == 0) {
            queue.push_back(id);
        }
    }

    // Traverse dependents, decrementing in-degree as we go.
    size_t processedCount = 0;
    while (!queue.empty()) {
        TfToken current = queue.back();
        queue.pop_back();
        processedCount++;

        auto depsIt = dependents.find(current);
        if (depsIt != dependents.end()) {
            for (const TfToken& dep : depsIt->second) {
                if (--incomingDegree[dep] == 0) {
                    queue.push_back(dep);
                }
            }
        }
    }

    // If there were less processed nodes than the number of nodes with
    // incoming edges, there was a cycle. Although the cycle could be
    // reported, a list of implicated nodes should be enough to debug with.
    if (processedCount != incomingDegree.size()) {
        if (errors) {
            std::vector<std::string> cycleNodes;
            for (const auto& [id, degree] : incomingDegree) {
                if (degree > 0) {
                    cycleNodes.push_back(id.GetString());
                }
            }
            errors->push_back(TfStringPrintf(
                "Graph contains a cycle involving nodes: %s",
                TfStringJoin(cycleNodes, ", ").c_str()));
        }
        return false;
    }
    return true;
}

/// Validate that all predecessor references point to existing capabilities.
bool UsdProfileRegistry::_CapabilityGraph::ValidateEdges(
                    std::vector<std::string>* errors) const
{
    bool valid = true;
    for (const auto& [id, edges] : incomingEdgeMap) {
        for (const auto& edge : edges) {
            if (incomingEdgeMap.find(edge.name) == incomingEdgeMap.end()) {
                if (errors) {
                    errors->push_back(TfStringPrintf(
                        "Capability '%s' has unknown predecessor '%s'",
                        id.GetText(), edge.name.GetText()));
                }
                valid = false;
            }
        }
    }
    return valid;
}

/// Validate that the graph has exactly one root (a node with no predecessors)
/// and that it is named "usd". Skipped for empty graphs.
/// Returns true if valid (or empty), false otherwise.
bool UsdProfileRegistry::_CapabilityGraph::ValidateRoot(
                    std::vector<std::string>* errors) const
{
    if (incomingEdgeMap.empty()) {
        return true;
    }

    std::vector<TfToken> roots;
    for (const auto& [id, edges] : incomingEdgeMap) {
        if (edges.empty()) {
            roots.push_back(id);
        }
    }

    static const TfToken usdRoot("usd");

    if (roots.size() != 1) {
        if (errors) {
            std::vector<std::string> rootNames;
            for (const TfToken& r : roots) {
                rootNames.push_back(r.GetString());
            }
            errors->push_back(TfStringPrintf(
                "Graph must have exactly one root node named 'usd', "
                "but found %zu root(s): %s",
                roots.size(),
                roots.empty() ? "(none)" :
                    TfStringJoin(rootNames, ", ").c_str()));
        }
        return false;
    }

    if (roots[0] != usdRoot) {
        if (errors) {
            errors->push_back(TfStringPrintf(
                "Graph root must be named 'usd', but found '%s'",
                roots[0].GetText()));
        }
        return false;
    }

    return true;
}

/// Get all capability IDs in the graph.
std::vector<TfToken> UsdProfileRegistry::_CapabilityGraph::GetAllCapabilities() const {
    std::vector<TfToken> result;
    result.reserve(incomingEdgeMap.size());
    for (const auto& [id, edges] : incomingEdgeMap) {
        result.push_back(id);
    }
    return result;
}

/// Get all capability IDs tagged as profiles (isProfile=true).
std::vector<TfToken> UsdProfileRegistry::_CapabilityGraph::GetAllProfiles() const {
    std::vector<TfToken> result;
    for (const auto& [id, cap] : metadata) {
        if (cap.IsProfile()) {
            result.push_back(id);
        }
    }
    return result;
}

std::optional<Capability> UsdProfileRegistry::_CapabilityGraph::GetMetadata(
                    const TfToken& cap) const {
    auto it = metadata.find(cap);
    if (it == metadata.end()) {
        return {};
    }
    return it->second;
}

bool UsdProfileRegistry::_CapabilityGraph::HasCapability(const TfToken& cap) const {
    return incomingEdgeMap.find(cap) != incomingEdgeMap.end();
}

// visited is the cycle detecting set for the recurrence.
UsdProfileRegistry::QueryStatus
UsdProfileRegistry::_CapabilityGraph::DepthFirstSearch(
                    const TfToken& current,
                    const TfToken& target,
                    std::unordered_set<TfToken,
                    TfToken::HashFunctor>& visited) const
{
    if (current == target) {
        return UsdProfileRegistry::QueryStatus::ValidPath; // from a to a is valid.
    }
    if (!visited.insert(current).second) {
        return UsdProfileRegistry::QueryStatus::CycleFound; // safety, reject a cycle
    }

    UsdProfileRegistry::QueryStatus result = UsdProfileRegistry::QueryStatus::NoPath;

    // iterate predecessors of current
    auto it = incomingEdgeMap.find(current);
    if (it == incomingEdgeMap.end()) {
        visited.erase(current);
        return UsdProfileRegistry::QueryStatus::NoPath;
    }
    for (const auto& edge : it->second) {
        UsdProfileRegistry::QueryStatus subGraphStatus = DepthFirstSearch(edge.name, target, visited);
        if (subGraphStatus == UsdProfileRegistry::QueryStatus::NoPath) {
            continue;
        }

        if (edge.deprecated) {
            subGraphStatus = UsdProfileRegistry::QueryStatus::Deprecated;
        }

        if (result == UsdProfileRegistry::QueryStatus::NoPath) {
            result = subGraphStatus;
        }
        else if (result != subGraphStatus) {
            result = UsdProfileRegistry::QueryStatus::DeprecationConflict;
        }
    }
    
    visited.erase(current);
    return result;
}

std::vector<UsdProfileRegistry::CapabilityResult>
UsdProfileRegistry::_CapabilityGraph::GetPredecessors(const TfToken& cap) const {
    std::vector<UsdProfileRegistry::CapabilityResult> result;
    auto it = incomingEdgeMap.find(cap);
    if (it != incomingEdgeMap.end()) {
        for (const auto& edge : it->second) {
            UsdProfileRegistry::QueryStatus status = edge.deprecated
                ? UsdProfileRegistry::QueryStatus::Deprecated
                : UsdProfileRegistry::QueryStatus::ValidPath;
            result.push_back({edge.name, status});
        }
    }
    return result;
}

/// Return the full transitive closure of all ancestors of \p cap, each with
/// the best QueryStatus of any path from \p cap to that ancestor.
/// Single-pass O(V+E): each node's status can escalate at most once
/// (ValidPath → DeprecationConflict or Deprecated → DeprecationConflict),
/// so each edge is traversed at most twice. Does not include \p cap itself.
std::vector<UsdProfileRegistry::CapabilityResult>
UsdProfileRegistry::_CapabilityGraph::GetTransitivePredecessors(const TfToken& cap) const {
    std::unordered_map<TfToken, UsdProfileRegistry::QueryStatus,
                       TfToken::HashFunctor> status;
    std::unordered_set<TfToken, TfToken::HashFunctor> inPath;

    auto seedIt = incomingEdgeMap.find(cap);
    if (seedIt != incomingEdgeMap.end()) {
        for (const auto& edge : seedIt->second) {
            _DFSTransitive(edge.name, edge.deprecated, status, inPath);
        }
    }

    std::vector<UsdProfileRegistry::CapabilityResult> result;
    result.reserve(status.size());
    for (const auto& [node, s] : status) {
        result.push_back({node, s});
    }
    return result;
}

void
UsdProfileRegistry::_CapabilityGraph::_DFSTransitive(
    const TfToken& node,
    bool pathDeprecated,
    std::unordered_map<TfToken, UsdProfileRegistry::QueryStatus,
                       TfToken::HashFunctor>& status,
    std::unordered_set<TfToken, TfToken::HashFunctor>& inPath) const
{
    using QS = UsdProfileRegistry::QueryStatus;

    if (!inPath.insert(node).second) {
        return;  // cycle in current DFS path
    }

    QS incoming = pathDeprecated ? QS::Deprecated : QS::ValidPath;
    auto [it, inserted] = status.try_emplace(node, incoming);

    bool propagate = inserted;
    if (!inserted) {
        QS prior = it->second;
        if (prior != QS::DeprecationConflict && prior != incoming) {
            // New path with different deprecation status: escalate and re-propagate
            // so descendants also receive the new context.
            it->second = QS::DeprecationConflict;
            propagate = true;
        }
        // If same status or already DeprecationConflict: no update, no re-propagation.
    }

    if (propagate) {
        auto edgesIt = incomingEdgeMap.find(node);
        if (edgesIt != incomingEdgeMap.end()) {
            for (const auto& edge : edgesIt->second) {
                _DFSTransitive(edge.name, pathDeprecated || edge.deprecated,
                               status, inPath);
            }
        }
    }

    inPath.erase(node);
}

UsdProfileRegistry::_CapabilityGraph::_CapabilityGraph() = default;
UsdProfileRegistry::_CapabilityGraph::~_CapabilityGraph() = default;

UsdProfileRegistry::QueryStatus
UsdProfileRegistry::_CapabilityGraph::HasPredecessor(const TfToken& capability,
                                const TfToken& target) const
{
    if (capability == target) {
        return UsdProfileRegistry::QueryStatus::NoPath; // reject self-cycles.
    }
    std::unordered_set<TfToken, TfToken::HashFunctor> visited;
    return DepthFirstSearch(capability, target, visited);
}

UsdProfileRegistry::UsdProfileRegistry()
: _capabilityGraph(new _CapabilityGraph())
{
    _RegisterFromPlugins();
}

UsdProfileRegistry::~UsdProfileRegistry() = default;

/* static */
UsdProfileRegistry&
UsdProfileRegistry::GetInstance()
{
    return TfSingleton<UsdProfileRegistry>::GetInstance();
}

/* static */
bool
UsdProfileRegistry::HasCapability(const TfToken& capability)
{
    return GetInstance()._capabilityGraph->HasCapability(capability);
}

/* static */
UsdProfileRegistry::QueryStatus
UsdProfileRegistry::CoversCapabilities(
    const TfToken& perspective,
    const std::vector<TfToken>& required,
    const std::vector<TfToken>& excepted,
    std::vector<CapabilityResult>* results)
{
    if (results) {
        results->clear();
    }

    if (required.empty()) {
        return QueryStatus::NoPath;
    }

    // Resolve perspective through the versioning rules.
    TfToken resolvedPerspective = ResolveCapability(perspective);
    if (resolvedPerspective.IsEmpty()) {
        return QueryStatus::NoPath;
    }

    // Build a resolved set of excepted tokens for O(1) lookup.
    std::unordered_set<TfToken, TfToken::HashFunctor> resolvedExcepted;
    for (const TfToken& e : excepted) {
        TfToken resolved = ResolveCapability(e);
        if (!resolved.IsEmpty()) {
            resolvedExcepted.insert(resolved);
        } else {
            resolvedExcepted.insert(e);
        }
    }

    UsdProfileRegistry& instance = UsdProfileRegistry::GetInstance();

    // Precedence: NoPath > DeprecationConflict > Excepted > Deprecated > ValidPath
    // CycleFound is treated as NoPath in the aggregate.
    // A single NoPath (or Excepted) makes the aggregate NoPath.
    QueryStatus aggregate = QueryStatus::ValidPath;
    bool anyNoPath = false;

    for (const TfToken& cap : required) {
        // Resolve each required capability — unversioned names match the
        // highest registered versioned form.
        TfToken resolvedCap = ResolveCapability(cap);
        const TfToken& reportToken = resolvedCap.IsEmpty() ? cap : resolvedCap;

        QueryStatus s;
        if (!resolvedCap.IsEmpty()
                && resolvedExcepted.count(resolvedCap)) {
            s = QueryStatus::Excepted;
        } else if (resolvedCap.IsEmpty()) {
            s = QueryStatus::NoPath;
        } else {
            s = instance._capabilityGraph->HasPredecessor(
                resolvedPerspective, resolvedCap);
        }

        if (results) {
            results->push_back({reportToken, s});
        }

        if (s == QueryStatus::NoPath || s == QueryStatus::CycleFound
                || s == QueryStatus::Excepted) {
            anyNoPath = true;
            // Keep accumulating so results is fully populated.
            continue;
        }

        // Escalate aggregate: DeprecationConflict > Deprecated > ValidPath
        if (s == QueryStatus::DeprecationConflict) {
            aggregate = QueryStatus::DeprecationConflict;
        } else if (s == QueryStatus::Deprecated
                   && aggregate != QueryStatus::DeprecationConflict) {
            aggregate = QueryStatus::Deprecated;
        }
    }

    if (anyNoPath) {
        return QueryStatus::NoPath;
    }
    return aggregate;
}

/* static */
bool
UsdProfileRegistry::IsProfile(const TfToken& capability)
{
    auto c = GetInstance()._capabilityGraph->GetMetadata(capability);
    return c && c->IsProfile();
}

/* static */
VtDictionary
UsdProfileRegistry::GetCapabilityMetadata(const TfToken& capability)
{
    VtDictionary result;
    auto c = GetInstance()._capabilityGraph->GetMetadata(capability);
    if (c) {
        result["name"]      = VtValue(c->GetName().GetString());
        result["docstring"] = VtValue(c->GetDocString());
        result["style"]     = VtValue(c->GetStyle().GetString());
        result["subgraph"]  = VtValue(c->GetSubgraph().GetString());
        result["isProfile"] = VtValue(c->IsProfile());
    }
    return result;
}

/* static */
std::vector<UsdProfileRegistry::CapabilityResult>
UsdProfileRegistry::GetPredecessors(const TfToken& capability)
{
    return GetInstance()._capabilityGraph->GetPredecessors(capability);
}

/* static */
std::vector<UsdProfileRegistry::CapabilityResult>
UsdProfileRegistry::GetTransitivePredecessors(const TfToken& capability)
{
    return GetInstance()._capabilityGraph->GetTransitivePredecessors(capability);
}

/* static */
std::pair<TfToken, int>
UsdProfileRegistry::ParseCapabilityVersion(const TfToken& capability)
{
    const std::string& s = capability.GetString();

    // Find the last occurrence of "_v" followed by one or more digits.
    // We scan backwards so "usd.foo_v2_v3" correctly finds "_v3".
    size_t pos = std::string::npos;
    size_t searchFrom = s.size();
    while (searchFrom > 1) {
        size_t found = s.rfind("_v", searchFrom - 1);
        if (found == std::string::npos) {
            break;
        }
        // Verify every character after "_v" is a digit and there is at least one.
        size_t digitStart = found + 2;
        if (digitStart < s.size()) {
            bool allDigits = true;
            for (size_t i = digitStart; i < s.size(); ++i) {
                if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
                    allDigits = false;
                    break;
                }
            }
            if (allDigits) {
                pos = found;
                break;
            }
        }
        searchFrom = found;
    }

    if (pos == std::string::npos) {
        return {capability, 0};
    }

    TfToken baseName(s.substr(0, pos));
    int version = std::stoi(s.substr(pos + 2));
    return {baseName, version};
}

/* static */
TfToken
UsdProfileRegistry::ResolveCapability(const TfToken& capability)
{
    UsdProfileRegistry& instance = UsdProfileRegistry::GetInstance();

    // Parse the input to get its base name.
    auto [inputBase, inputVersion] = ParseCapabilityVersion(capability);

    // Scan all registered capabilities for siblings sharing the same base name.
    // Track the best: versioned beats unversioned; highest version wins.
    TfToken best;
    int bestVersion = -1;  // -1 = nothing found yet

    for (const TfToken& cap : instance._capabilityGraph->GetAllCapabilities()) {
        auto [base, ver] = ParseCapabilityVersion(cap);
        if (base != inputBase) continue;

        if (bestVersion < 0) {
            // First candidate.
            best = cap;
            bestVersion = ver;
        } else if (ver > bestVersion) {
            // Higher version wins (versioned always > unversioned since ver=0
            // for unversioned and we only get here when bestVersion >= 0).
            best = cap;
            bestVersion = ver;
        }
    }

    if (bestVersion < 0) {
        // Nothing registered under this base name at all.
        return TfToken();
    }
    return best;
}

/* static */
UsdProfileRegistry::QueryStatus
UsdProfileRegistry::HasPredecessor(const TfToken& capabilityA, const TfToken& capabilityB)
{
    TfToken resolvedA = ResolveCapability(capabilityA);
    TfToken resolvedB = ResolveCapability(capabilityB);
    if (resolvedA.IsEmpty() || resolvedB.IsEmpty()) {
        return QueryStatus::NoPath;
    }
    UsdProfileRegistry& instance = UsdProfileRegistry::GetInstance();
    return instance._capabilityGraph->HasPredecessor(resolvedA, resolvedB);
}


/* static */
TfToken
UsdProfileRegistry::GetStyleForCapability(const TfToken& cap)
{
    auto c = GetInstance()._capabilityGraph->GetMetadata(cap);
    return c ? c->GetStyle() : TfToken{};
}

/* static */
TfToken
UsdProfileRegistry::GetSubgraphForCapability(const TfToken& cap)
{
    auto c = GetInstance()._capabilityGraph->GetMetadata(cap);
    return c ? c->GetSubgraph() : TfToken{};
}

/* static */
std::string
UsdProfileRegistry::GetDocString(const TfToken& cap)
{
    auto c = GetInstance()._capabilityGraph->GetMetadata(cap);
    return c ? c->GetDocString() : std::string{};
}

/* static */
std::string
UsdProfileRegistry::GetDisplayName(const TfToken& cap)
{
    auto c = GetInstance()._capabilityGraph->GetMetadata(cap);
    return c ? std::string{c->GetName()} : std::string{};
}

/* static */
std::map<TfToken, std::string>
UsdProfileRegistry::GetCapabilityStyles()
{
    return UsdProfileRegistry::GetInstance()._capabilityStyles;
}

/* static */
std::vector<TfToken>
UsdProfileRegistry::GetAllCapabilities()
{
    UsdProfileRegistry& instance = UsdProfileRegistry::GetInstance();
    return instance._capabilityGraph->GetAllCapabilities();
}

/* static */
std::vector<TfToken>
UsdProfileRegistry::GetAllProfiles()
{
    UsdProfileRegistry& instance = UsdProfileRegistry::GetInstance();
    return instance._capabilityGraph->GetAllProfiles();
}

// Helper function to make reading from dictionaries easier
static bool
_GetKey(const JsObject &dict, const std::string &key, JsObject *value)
{
    JsObject::const_iterator i = dict.find(key);
    if (i != dict.end() && i->second.IsObject()) {
        *value = i->second.GetJsObject();
        return true;
    }
    return false;
}

// Helper function to read a string from a dictionary
static std::string
_GetStringFromDict(const JsObject &dict, const std::string &key, 
                   const std::string &defaultValue = "")
{
    JsObject::const_iterator i = dict.find(key);
    if (i != dict.end() && i->second.IsString()) {
        return i->second.GetString();
    }
    return defaultValue;
}

bool
UsdProfileRegistry::_LoadCapabilitiesFromProfileData(
    const JsObject& profileData)
{
    // --- Merge styles and groups ---

    JsObject stylesData;
    if (_GetKey(profileData, _tokens->CapabilityStylesKey, &stylesData)) {
        for (const auto& stylePair : stylesData) {
            _capabilityStyles[TfToken(stylePair.first)] = stylePair.second.GetString();
        }
    }

    // --- Build capability graph ---

    JsObject capsData;
    if (!_GetKey(profileData, _tokens->CapabilitiesKey, &capsData)) {
        // No Capabilities section is valid (e.g., plugin only has styles)
        return true;
    }

    for (const auto& capPair : capsData) {
        TfToken capId(capPair.first);

        if (!capPair.second.IsObject()) {
            TF_WARN("Expected object for capability '%s'",
                     capId.GetText());
            continue;
        }

        const JsObject& capObj = capPair.second.GetJsObject();

        // Extract predecessors (incoming edges)
        // Supports hybrid format: string or object with deprecation
        std::vector<_CapabilityGraph::Edge> incomingEdges;
        auto predIt = capObj.find(_tokens->PredecessorsKey);
        if (predIt != capObj.end() && predIt->second.IsArray()) {
            const JsArray& predArray = predIt->second.GetJsArray();
            for (const JsValue& predValue : predArray) {
                _CapabilityGraph::Edge edge;
                edge.deprecated = false;

                if (predValue.IsString()) {
                    edge.name = TfToken(predValue.GetString());
                }
                else if (predValue.IsObject()) {
                    const JsObject& predObj =
                        predValue.GetJsObject();

                    auto nameIt = predObj.find("name");
                    if (nameIt != predObj.end()
                            && nameIt->second.IsString()) {
                        edge.name =
                            TfToken(nameIt->second.GetString());
                    } else {
                        TF_WARN("Predecessor object missing 'name'"
                                 " for capability '%s'",
                                 capPair.first.c_str());
                        continue;
                    }

                    auto deprecatedIt = predObj.find("deprecated");
                    if (deprecatedIt != predObj.end() && deprecatedIt->second.IsBool()) {
                        edge.deprecated = deprecatedIt->second.GetBool();
                    }
                }
                else {
                    TF_WARN("Invalid predecessor format for "
                             "capability '%s'",
                             capPair.first.c_str());
                    continue;
                }

                incomingEdges.push_back(std::move(edge));
            }
        }

        // Extract metadata fields
        std::string docstring = _GetStringFromDict(capObj, _tokens->DocstringKey);
        std::string displayName = _GetStringFromDict(capObj, _tokens->NameKey, 
                                                     capId.GetString());
        TfToken style(_GetStringFromDict(capObj, _tokens->StyleKey));
        TfToken subgraph(_GetStringFromDict(capObj, _tokens->SubgraphKey));

        bool isProfile = false;
        auto isProfileIt = capObj.find(_tokens->IsProfileKey);
        if (isProfileIt != capObj.end() && isProfileIt->second.IsBool()) {
            isProfile = isProfileIt->second.GetBool();
        }

        Capability cap(capId, TfToken(displayName), docstring,
                       isProfile);
        cap.SetStyle(style);
        cap.SetSubgraph(subgraph);

        _capabilityGraph->AddCapability(capId, incomingEdges, cap);
    }

    return true;
}

void
UsdProfileRegistry::_RegisterFromPlugins()
{
    // Following the KindRegistry pattern: iterate all plugins,
    // performing three logical passes:
    //   Pass 1+2 - Collect styles/groups and build graph (via shared helper)
    //   Pass 3   - Validate the graph (acyclic, edges resolve)

    const PlugPluginPtrVector& plugins =
        PlugRegistry::GetInstance().GetAllPlugins();

    for (auto plug : plugins) {
        const JsObject& plug_metadata = plug->GetMetadata();

        JsObject profileData;
        if (!_GetKey(plug_metadata, _tokens->PluginProfilesKey, &profileData)) {
            continue;
        }

        TfDiagnosticTrap trap;
        _LoadCapabilitiesFromProfileData(profileData);

        if (!trap.IsClean()) {
            std::vector<std::string> msgs;
            trap.ForEach([&](TfDiagnosticBase const& d) {
                msgs.push_back(d.GetCommentary());
            });
            trap.Clear();
            TF_WARN("Problems loading Profiles data from plugin '%s' "
                    "(%s): %s",
                    plug->GetName().c_str(),
                    plug->GetPath().c_str(),
                    TfStringJoin(msgs, "; ").c_str());
        }
    }

    // --- Pass 3: Validate the graph ---

    std::vector<std::string> errors;

    _capabilityGraph->ValidateEdges(&errors);
    _capabilityGraph->ValidateAcyclic(&errors);
    _capabilityGraph->ValidateRoot(&errors);

    for (const std::string& err : errors) {
        TF_WARN("%s", err.c_str());
    }
}

class Usd_ProfilesRegistryTestAccess {
public:
    static void Clear() {
        UsdProfileRegistry& instance = UsdProfileRegistry::GetInstance();
        instance._capabilityGraph->Clear();
        instance._capabilityStyles.clear();
    }

    static bool LoadFromFile(const std::string& filePath,
                             std::vector<std::string>* errors) {
    // Open and parse the JSON file
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        if (errors) {
            errors->push_back(
                TfStringPrintf("Failed to open file: %s", filePath.c_str()));
        }
        return false;
    }

    JsParseError parseError;
    JsValue root = JsParseStream(ifs, &parseError);
    if (root.IsNull()) {
        if (errors) {
            errors->push_back(
                TfStringPrintf("JSON parse error in '%s' at line %d col %d: %s",
                    filePath.c_str(),
                    parseError.line, parseError.column,
                    parseError.reason.c_str()));
        }
        return false;
    }

    if (!root.IsObject()) {
        if (errors) {
            errors->push_back(
                TfStringPrintf("Expected JSON object at root of '%s'",
                    filePath.c_str()));
        }
        return false;
    }

    const JsObject& rootObj = root.GetJsObject();

    // Clear existing state
    Clear();

    // Unwrap canonical plugInfo.json format:
    //   { "Plugins": [{ "Info": { "Profiles": {...} }, ... }, ...] }
    // Iterate each plugin entry, extract its Info dict, and look for Profiles.
    UsdProfileRegistry& instance = UsdProfileRegistry::GetInstance();

    auto pluginsIt = rootObj.find("Plugins");
    if (pluginsIt == rootObj.end() || !pluginsIt->second.IsArray()) {
        if (errors) {
            errors->push_back(TfStringPrintf(
                "Expected 'Plugins' array at root of '%s'",
                filePath.c_str()));
        }
        return false;
    }

    const JsArray& plugins = pluginsIt->second.GetJsArray();
    for (const JsValue& pluginValue : plugins) {
        if (!pluginValue.IsObject()) {
            continue;
        }
        const JsObject& pluginObj = pluginValue.GetJsObject();

        // Extract "Info" dict (same level as GetMetadata() returns)
        JsObject infoDict;
        if (!_GetKey(pluginObj, "Info", &infoDict)) {
            continue;
        }

        // Look for "Profiles" inside Info
        JsObject profileData;
        if (!_GetKey(infoDict, _tokens->PluginProfilesKey, &profileData)) {
            continue;
        }

        instance._LoadCapabilitiesFromProfileData(profileData);
    }

    // Validate the graph
    bool valid = true;

    std::vector<std::string> validationErrors;
    if (!instance._capabilityGraph->ValidateEdges(&validationErrors)) {
        valid = false;
    }
    if (!instance._capabilityGraph->ValidateAcyclic(&validationErrors)) {
        valid = false;
    }
    if (!instance._capabilityGraph->ValidateRoot(&validationErrors)) {
        valid = false;
    }

    if (errors) {
        errors->insert(errors->end(),
            validationErrors.begin(), validationErrors.end());
    }

    return valid;
}
};

USDPROFILES_API void
Usd_ProfilesRegistryTestClear()
{
    Usd_ProfilesRegistryTestAccess::Clear();
}

USDPROFILES_API bool
Usd_ProfilesRegistryTestLoadFromFile(
    const std::string& filePath,
    std::vector<std::string>* errors)
{
    return Usd_ProfilesRegistryTestAccess::LoadFromFile(filePath, errors);
}

PXR_NAMESPACE_CLOSE_SCOPE
