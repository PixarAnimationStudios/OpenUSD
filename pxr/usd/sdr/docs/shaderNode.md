# SdrShaderNode {#Sdr_ShaderNode}

SdrShaderNode represents a dataflow-connectable computation, intended to be a 
node in a computational network. It has the following core fields.

| Field             | Description                                                                                  |
|:------------------|:---------------------------------------------------------------------------------------------|
| identifier        | Concise string including what the node does, type specialization, and versioning information |
| name              | Concise string including what the node does, and type specialization                         |
| function          | Concise string consisting of what the node does                                              |
| version           | SdrVersion that indicates the node's version                                                 |
| shadingSystem     | Describes a system that usually has its own standard, language, and/or runtime               |
| properties        | Represents computational inputs and outputs (`SdrShaderProperty`)                            |
| definitionURI     | Asset providing the definition of the node, determined during \ref Sdr_Registry_Discovery |
| implementationURI | Asset providing implementation of the node's computation, determined during \ref Sdr_Registry_Parse |

SdrShaderNode is uniquely identified by a combination of `identifier` and `shadingSystem`.
As a consequence of the assigned meanings above, this means a node can alternately be
uniquely identified by `name`, `version`, and `shadingSystem`. Convenience API on SdrRegistry
allows for identifying nodes using various combinations of these fields.

Note that in some cases, `definitionURI` and `implementationURI` may be the same.


## Metadata {#Sdr_ShaderNode_Metadata}
SdrShaderNodeMetadata contains named metadata for SdrShaderNode and has support
for custom user-defined metadata.

Below is a table of the most frequently used named metadata for easy reference.
Descriptions below mirror the documentation on SdrShaderNodeMetadata itself.
These named metadata items are
<a href="https://github.com/PixarAnimationStudios/OpenUSD/blob/2f88bd53bd2690998c3d7507d24e8d213068deb1/pxr/usd/sdr/shaderNodeMetadata.h#L25-L38">
tokenized here</a>. In code to reference, for example, the "label" metadata key,
use `SdrNodeMetadata->Label`.

| Key                | Description                                                                                                        |
|:-------------------|:-------------------------------------------------------------------------------------------------------------------|
| label              | UI hint for a user-friendly string representation                                                                  |
| help               | Help message for this node                                                                                         |
| domain             | Defines groups for the broadest system categories, "rendering" by default                                          |
| subdomain          | Defines domain subsystems; e.g. "shading", "filtering", "lighting" in domain "rendering"                           |
| context            | Describes usage group in a subdomain; e.g. "pattern", "surface", "bxdf" in subdomain "shading"                     |
| role               | Provides finer granularity for contexts that contain many nodes                                                    |
| targetRenderer     | Describes that a node was designed for (but not necessarily limited to) a specific renderer                        |
| collections        | Provides a way to arbitrarily group nodes in ways that aren't captured by other metadata                           |
| implementationName | Implementation source's name for the shader, needed by some runtimes                                               |
| openPages          | Pages that should be opened/expanded by default                                                                    |
| pagesShownIf       | Per-page `shownIf` expressions; see SdrShaderProperty::GetShownIf                                                  |
| primvars           | Primvars used by this node                                                                                         |

Note that the following metadata items and core fields form a hierarchy
useful for logically grouping nodes for UI display (see helper function SdrShaderNodeQueryUtils::GroupQueryResults).
- domain
- subdomain
- context
- role
- function
- name
- identifier

In general, metadata can be authored either by shader writers or parser plugin writers.
Choice of when to author these items depends on the desired workflow.

## Style Guidelines {#Sdr_ShaderNode_StyleGuidelines}

While SdrShaderNode does allow usages that don't match the descriptions
above, adherence to these soft standards helps with consistent organization.

### Identifier/Name/Function
`identifier`, `name`, and `function` should follow the below format with
components separated by underscores. 

\code
identifier: <function>_<typeSpecialization>_<version>
name:       <function>_<typeSpecialization>
function:   <function>
\endcode

`<function>` describes what the node does. `<version>` is either in form
`<major>_<minor>` or `<major>`. `<typeSpecialization>` represents the type of
the node and has the following recommendations.
- The type specialization representation should attempt to maximize meaning while
  disambiguating all nodes with the same function. This rule supersedes the below
  rules. Use your best judgement.
- Prefer a representation of the shader's output type.
- Use lowercase sdr type representations if possible (see \ref Sdr_ShaderProperty_Types).
- If the type is a fixed-size container, use the type representation and
  concatenate the fixed size e.g. "float3" or "color4".
- If the type is a container whose size is <b>not</b> fixed, use "vector" and concatenate
  the contained type with camel case. See example 2.
- If multiple type specifications are required to disambiguate a function, concatenate
  them with underscores. Prefer output types but use input types as necessary.
- If at any point a brief word or phrase can describe one or more type representations
  while disambiguating this shader from other shaders, consider preferring the brief.
  See example 3.

| Field       | Example 1         | Example 2                | Example 3                      |
|:------------|:------------------|:-------------------------|:-------------------------------|
| identifier  | add_float_3       | position_vectorFloat_1_9 | convert_float4_surfaceShader_1 |
| name        | add_float         | position_vectorFloat     | convert_float4_surfaceShader   |
| function    | add               | position                 | convert                        |