# Overview

UsdProfiles provides infrastructure for capability-based profile compatibility
queries in USD scenes. It answers questions like: "does this prim satisfy the
requirements of profile X?" and "which USD capabilities does this prim use?"

UsdProfiles has two main components:

- **UsdProfileRegistry** — a singleton that loads capability metadata from
  plugins and exposes the capability DAG for querying.
- **UsdProfilesClaimsAPI** — a prim-level applied schema that records which
  capabilities a prim uses and which profiles it has been declared compatible
  with.

(usdProfiles_capability_dag)=
## The Capability DAG

Capabilities are named features of the USD format, identified by reverse-domain
tokens such as `usd`, `usd.geom.mesh`, or `usd.shading.mtlx`. They are
organized as a directed acyclic graph (DAG) where an edge from capability A to
capability B means "A depends on B" — a prim that uses A implicitly relies on
everything B requires.

Edges can be marked **deprecated**, indicating that a path through that edge
represents an older, superseded route. This lets the registry report three
distinct reachability states for a given ancestor:

- `ValidPath` — reachable via at least one non-deprecated path.
- `Deprecated` — only reachable via deprecated paths.
- `DeprecationConflict` — reachable via both deprecated and non-deprecated
  paths.

### Registering Capabilities via Plugins

Capabilities are registered through `plugInfo.json` entries. The registry
discovers and merges all registered capability data at startup:

```{code-block} json
{
    "Plugins": [{
        "Info": {
            "Profiles": {
                "Capabilities": {
                    "usd": {
                        "name": "USD",
                        "docstring": "Root USD capability",
                        "style": "core",
                        "subgraph": "Foundation"
                    },
                    "usd.geom.mesh": {
                        "predecessors": ["usd"],
                        "name": "Polygon Mesh",
                        "docstring": "Polygonal mesh geometry",
                        "style": "core"
                    },
                    "studio.vfx.26.08": {
                        "predecessors": [
                            "usd.geom.mesh",
                            { "name": "studio.vfx.25.11", "deprecated": true }
                        ],
                        "name": "VFX Profile 26.08",
                        "docstring": "August 2026 VFX pipeline profile",
                        "style": "profile",
                        "isProfile": true
                    }
                },
                "CapabilityStyles": {
                    "core":    "fill:#cce5ff,stroke:#004085",
                    "profile": "fill:#d4edda,stroke:#155724"
                }
            }
        },
        "Name": "MyStudioCapabilities",
        "Type": "resource"
    }]
}
```

Every capability graph must have exactly one root node named `usd`.

### Profiles

A **profile** is a capability node tagged with `"isProfile": true`. Profiles
represent named, coherent sets of capabilities for a specific platform or
pipeline target, such as a versioned studio release target or a platform
conformance level.

Profiles participate in the DAG the same way as any other capability. Expressing
profile supersession uses a deprecated edge from the new profile to the old one:

```{code-block} json
"studio.vfx.26.08": {
    "predecessors": [
        "usd.geom.mesh",
        { "name": "studio.vfx.25.11", "deprecated": true }
    ],
    "isProfile": true
}
```

This declares that `studio.vfx.26.08` requires `usd.geom.mesh` directly (valid
path) and that `studio.vfx.25.11` is now a superseded predecessor (deprecated
path). Querying `HasPredecessor("studio.vfx.26.08", "usd.geom.mesh")` returns
`DeprecationConflict` because two paths exist — one direct and one through the
deprecated `studio.vfx.25.11`.

### Capability Versioning

Capabilities can be versioned with a `_vN` suffix (e.g. `usd.physics_v2`).
`ResolveCapability()` automatically selects the highest registered versioned
form of an unversioned name, so callers can pass `"usd.physics"` and
transparently receive results for `"usd.physics_v2"`. `CoversCapabilities()`
and `HasPredecessor()` both resolve tokens before querying the graph.

(usdProfiles_using_claimsapi)=
## Using ClaimsAPI

### Recording Capability Usages

When a DCC tool saves a prim that uses specific USD features, it records the
capabilities used and a degradation class for each:

```{code-block} python
from pxr import Usd, UsdProfiles

stage = Usd.Stage.CreateNew("shot.usda")
prim  = stage.DefinePrim("/Char/Body", "Mesh")
api   = UsdProfiles.ClaimsAPI.Apply(prim)

api.SetCapabilityUsage("usd.geom.mesh",      "hard")
api.SetCapabilityUsage("usd.shading.mtlx",   "hard")
api.SetCapabilityUsage("usd.geom.hairAndFur", "soft")
```

| Degradation class | Meaning |
|---|---|
| `hard` | Load-bearing; a consumer lacking this capability produces incorrect results. |
| `soft` | The prim degrades gracefully if the capability is absent. |
| `enhancement` | Improves quality but its absence is acceptable. |

### Declaring Profile Compatibility

A pipeline conformance step declares which profiles the prim satisfies. A
fully compatible declaration (no exceptions) is an unqualified assertion:

```{code-block} python
api.SetProfileCompatible("studio.vfx.26.08")
```

If certain capabilities are present but known to fail audit, list them as
exceptions — a signed degradation declaration:

```{code-block} python
api.SetProfileCompatibleWithExceptions(
    "studio.mobile.v1",
    ["usd.geom.hairAndFur"])
```

### Querying Compatibility

`IsCompatibleWith()` performs the full compatibility check. It calls
`UsdProfileRegistry::CoversCapabilities()` with the stored capability usages
as the required set and the stored exception list for that profile:

```{code-block} python
PR = UsdProfiles.ProfileRegistry
QS = PR.QueryStatus

status, results = api.IsCompatibleWith("studio.vfx.26.08")

if status == QS.ValidPath:
    print("fully compatible")
elif status == QS.Deprecated:
    print("compatible via a deprecated path only")
elif status == QS.DeprecationConflict:
    print("compatible via both deprecated and non-deprecated paths")
elif status == QS.NoPath:
    print("not compatible, or profile not declared")

# per-capability detail
for r in results:
    print(f"  {r.capability}: {r.status}")
```

`IsCompatibleWith()` returns `QueryStatus.NoPath` immediately if the profile
has not been declared compatible via `SetProfileCompatible` or
`SetProfileCompatibleWithExceptions`, without consulting the registry.

```{admonition} Querying Without a Stage
:class: note

`UsdProfileRegistry` static methods (`HasPredecessor`, `CoversCapabilities`,
etc.) can be called without a stage. They query only the global capability
DAG, making them suitable for tooling and validation pipelines that do not
have a scene loaded.
```

(usdProfiles_data_layout)=
## Data Layout

All ClaimsAPI data is stored in the prim's `customData` dictionary under the
key `profilesInfo`, using two sub-dictionaries:

```{code-block} usda
def Mesh "Body" (
    prepend apiSchemas = ["ClaimsAPI"]
)
{
    # mesh attributes ...
}
```

Reading the raw dictionary back:

```{code-block} python
info = api.GetProfilesInfo()
# {
#   "capabilityUsages": {
#       "usd.geom.mesh":      "hard",
#       "usd.shading.mtlx":   "hard",
#       "usd.geom.hairAndFur": "soft"
#   },
#   "profileCompatibility": {
#       "studio.vfx.26.08": [],
#       "studio.mobile.v1": ["usd.geom.hairAndFur"]
#   }
# }
```

`GetProfilesInfo()` / `SetProfilesInfo()` expose the full dictionary for bulk
access. The typed accessors (`GetCapabilityUsages`, `GetCompatibleProfiles`,
etc.) read and write the same storage.
