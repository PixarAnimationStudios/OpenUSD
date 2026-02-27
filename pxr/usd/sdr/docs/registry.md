# SdrRegistry {#Sdr_Registry}

SdrRegistry is a singleton that provides methods to find shader
information. Shader information populates into the registry via two stages
that are customizable via plugins.

1. Discovery finds shader definition locations and produces \ref SdrShaderNodeDiscoveryResult
"SdrShaderNodeDiscoveryResults".
2. Parsing reads shader definitions and produces \ref SdrShaderNode "SdrShaderNodes".

Discovery runs at registry initialization time, and parsing runs on demand.

The entire flow looks something like this:

- Alice wants to introduce a new shadingSystem "elm" that doesn't have existing
  plugins, so she writes new discovery and parser plugins. She chooses
  discoveryType "tree" to associate the new plugins with each other.
- During initialization, the registry finds all discovery and parser plugins.
  Then, it runs all discovery plugins and stores the discovery results.
- Alice requests a shader node with identifier "foo" and shadingSystem "elm"
  from the registry.
- A `SdrShaderNodeDiscoveryResult` with identifier "foo" and shadingSystem
  "elm" exists in the registry. This result has discoveryType "tree".
  The registry finds the parser plugin associated with discoveryType "tree" and
  runs it to parse a shader node from the discovery result.
- A parsed `SdrShaderNode` is returned to Alice, after which she can load
  its source into her runtime for execution.

Note that each node is parsed only once. If the node is requested again, the
cached parsed node is returned.

## Discovery {#Sdr_Registry_Discovery}

At registry initialization, the registry discovers shaders by running discovery plugins.

A filesystem-based discovery plugin `_SdrFilesystemDiscoveryPlugin` is active
by default. This plugin is configurable via environment variables indicating
paths and file extensions to search. However, we recommend that renderers or
shading systems adopting Sdr provide their own discovery plugins to avoid
coupling configurations for multiple parser plugins.

Users may implement custom discovery plugins to find filesystem-based nodes
(a typical case), search a database, a cloud service, or any other system
that might contain shaders. If shaders are scattered across multiple systems,
a discovery plugin can be created for each.

To build your own discovery plugin, please take a look at the following
references:

- Documentation of base class `SdrDiscoveryPlugin`
- Best practices for SdrShaderNode `identifier`, `name`, and `function` (see \ref Sdr_ShaderNode_StyleGuidelines)
- <a href = "https://github.com/PixarAnimationStudios/OpenUSD/blob/2f88bd53bd2690998c3d7507d24e8d213068deb1/pxr/usd/usdMtlx/discovery.cpp#L163">
  Example UsdMtlx discovery plugin implementation
  </a>

## Parsing {#Sdr_Registry_Parse}

Once the registry knows about shader nodes via discovery plugins, parser
plugins parse nodes upon request and provide the registry with the resulting information.
This includes input and output property information, node metadata, and other
information that usually can only be gleaned by reading source implementation
files.

To build your own parser plugin, please take a look at the following
references:

- Documentation of base class `SdrParserPlugin`
- Best practices for `SdrShaderNode` fields and metadata (see \ref Sdr_ShaderNode_Metadata)
- <a href = "https://github.com/PixarAnimationStudios/OpenUSD/blob/2f88bd53bd2690998c3d7507d24e8d213068deb1/pxr/usd/usdMtlx/parser.cpp#L556">
  Example UsdMtlx parser plugin implementation
  </a>

## Query API {#Sdr_Registry_Queries}

The registry ships with a query API whose components are
`SdrShaderNodeQuery` and `SdrShaderNodeQueryResult`. Additional convenience query
functions are provided by `shaderNodeQueryUtils.h`.
