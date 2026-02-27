# SdrShaderProperty {#Sdr_ShaderProperty}

SdrShaderProperty represents an input or output on SdrShaderNode. It has the following core fields.

| Field             | Description                                                                |
|:------------------|:---------------------------------------------------------------------------|
| name              | Name of the property                                                       |
| type              | Sdr type of the property                                                   |
| isOutput          | When true, the property is an output. When false, the property is an input |

All other fields are optional and thus spiritually belong more to metadata than to core fields.

Connectability of properties to other properties is a core feature described by methods
`SdrShaderProperty::CanConnectTo` and `SdrShaderProperty::IsConnectable`.

## Typing in Sdr {#Sdr_ShaderProperty_Types}
SdrShaderProperty deals with two typing systems.
- <a href="https://github.com/PixarAnimationStudios/OpenUSD/blob/2f88bd53bd2690998c3d7507d24e8d213068deb1/pxr/usd/sdr/shaderProperty.h#L29-L42">
  Sdr types</a>
- <a href="https://github.com/PixarAnimationStudios/OpenUSD/blob/2f88bd53bd2690998c3d7507d24e8d213068deb1/pxr/usd/sdf/types.h#L536-L576">
  Sdf types</a>

At construction, SdrShaderProperty resolves given information to a logical Sdr type. Its corresponding Sdf type
is automatically calculated upon invocation of SdrShaderProperty::GetTypeAsSdfType. Metadata-based
controls may modify the resulting Sdr and Sdf types; these are discussed in the following sections.
- \ref Sdr_ShaderProperty_SdrType
- \ref Sdr_ShaderProperty_SdfType

## Metadata {#Sdr_ShaderProperty_Metadata}

SdrShaderPropertyMetadata contains named metadata for SdrShaderProperty and has support for
custom user-defined metadata.

Below is a table of the most frequently used named metadata for easy reference.
Descriptions below mirror the documentation on SdrShaderPropertyMetadata itself.
These named metadata items are
<a href="https://github.com/PixarAnimationStudios/OpenUSD/blob/2f88bd53bd2690998c3d7507d24e8d213068deb1/pxr/usd/sdr/shaderPropertyMetadata.h#L22-L45">
tokenized here</a>. In code to reference, for example, the "label" metadata key,
use `SdrNodeMetadata->Label`.

| Key                  | Location |Description                                                                                               |
|:---------------------|:---------|:---------------------------------------------------------------------------------------------------------|
| label                | metadata |UI hint for a user-friendly string representation                                                         |
| help                 | metadata |Help message for this property                                                                            |
| widget               | metadata |UI hint describing an appropriate widget, e.g. "number", "slider"                                         |
| page                 | metadata |Page group this property appears on, if any. Nested pages should be delimited by ":"                      |
| shownIf              | metadata |Expression evaluating to true when this property should be shown. See SdrShaderProperty::GetShownIf       |
| hints                | field    |Other UI hints associated with this property                                                              |
| options              | field    |Specifies the finite pre-determined values this property is meant to have                                 |
| defaultValue         | field    |Sdr-typed default value for this property                                                                 |
| connectable          | metadata |Whether this property can connect to other properties. True by default.                                   |
| validConnectionTypes | metadata |List of valid connection types; doesn't affect SdrShaderProperty::CanConnectTo                            |
| arraySize            | field    |Array size                                                                                                |
| tupleSize            | metadata |Tuple size. This is independent of array size and describes an array's "column count"                     |
| isDynamicArray       | metadata |Indicates whether this property's array type is dynamically sized                                         |
| isAssetIdentifier    | metadata |When true, the computed Sdf type is SdfValueTypeNames->Asset or AssetArray                                |
| renderType           | metadata |Provides more info for Sdr types without analogous Sdf types (see \ref Sdr_ShaderProperty_SpecialTypes)   |
| role                 | metadata |When "none", a base Sdr type replaces the ingested Sdr type ("float3" replaces "color3")                  |
| sdrUsdDefinitionType | metadata |SdfValueTypeName that overrides any computed Sdf type for this property                                   |
| defaultInput         | metadata |Whether the value held by this property is the default input for the node                                 |
| colorspace           | metadata |Colorspace for this property                                                                              |

In general, metadata can be authored either by shader writers or parser plugin writers.
Choice of when to author these items depends on the desired workflow.

### Sdr Type Resolution {#Sdr_ShaderProperty_SdrType}
SdrShaderProperty's logical type is described by the constructor arguments `type`
(Sdr type) and `arraySize`, and the metadata item `isDynamicArray`.

During construction, if the metadata `role` set to "none", the `type` and `arraySize` fields
are transformed to base types according to
<a href="https://github.com/PixarAnimationStudios/OpenUSD/blob/0fff6d96b28de6ecf44a428271b912169a3d8986/pxr/usd/sdr/shaderProperty.cpp#L110-L138">
this map
</a>. For example, "float3" replaces "color3".

### Sdf Type Conversion {#Sdr_ShaderProperty_SdfType}
An Sdf type is represented by SdfValueTypeName. When triggered by a call to
`SdrShaderProperty::GetTypeAsSdfType`, the conversion process from the
Sdr typing system to SdfValueTypeName takes into account multiple data items
comprising the logical Sdr type described above in \ref Sdr_ShaderProperty_SdrType.

Normally, SdfValueTypeName conversion is performed according to 
<a href="https://github.com/PixarAnimationStudios/OpenUSD/blob/0fff6d96b28de6ecf44a428271b912169a3d8986/pxr/usd/sdr/shaderProperty.cpp#L37-L68">
these maps</a>.
However, the following metadata items alter this conversion process.
- `isAssetIdentifier` should be injected if the property is logically an asset.
  This metadata item guides the type mapping to choose SdfValueTypeNames->Asset
  over SdfValueTypeNames->String when the base Sdr type is string.
- `sdrUsdDefinitionType` explicitly specifies an SdfValueTypeName. This
  direct specification replaces the regular conversion process. This metadata
  value must be an explicit TfType value alias (see SdfValueTypeName::GetAliasesAsTokens)
  associated with the SdfValueTypeName -- this generally matches .usda type
  declarations. Examples follow.
  - "bool" represents `SdfValueTypeNames->Bool`
  - "bool[]" represents `SdfValueTypeNames->BoolArray`

  Note that Sdr doesn't have a "bool" Sdr type, so a logically "bool"-typed
  property must have an Sdr type of int and use an `sdrUsdDefinitionType` of "bool".

### Special Types {#Sdr_ShaderProperty_SpecialTypes}

Three types are specific to Sdr and have no corresponding SdfValueTypeName.

- `struct` -  Struct types
- `terminal` - Certain properties represent 'terminal' ports
  (ie. surface, displacement, volume, etc)
- `vstruct` - An abstract struct type that can connect to any
  other 'vstruct', and connections are only made if a matching vstruct member
  is found.

These special types have no reasonable corresponding SdfValueTypeName,
so we map them to `SdfValueTypeName->Token`, which is typically reserved
for an Unknown type.

For these cases, traditional methods of accessing type information will give incomplete info.
For example: `SdrShaderProperty::GetType` returns a "struct" token without indicating what _kind_
of struct it represents, and `SdrShaderProperty::GetTypeAsSdfType().GetSdfType`
returns `SdfValueTypeNames->Token`, which is even more ambiguous than "struct".

Therefore to provide more complete type info, the `renderType` metadata
should be authored when creating a SdrShaderProperty with one of the above special
types.
- On a Struct, the metadata should be `renderType = struct structName`,
  where structName is the actual struct's typename
- On a Terminal, the metadata should be `renderType = terminal terminalName`
  where terminalName is the actual kind of terminal
- On a Vstruct, the metadata should be `renderType = vstruct`, since there is
  no further specific type name for a vstruct

A user can find the original struct type or terminal type of one of these properties
by examining the 'renderType' metadata on the property directly.

## Type Behavior Examples

~~~~~~~~~~~~~{.usd}
shader TestOSLShader
(
    color foo = color(1.0, 1.0 1.0),
    token bar = "baz",
    token surface = ""
    [[
        string renderType = "terminal surface"
    ]]
)
~~~~~~~~~~~~~
In the above example, the properties would be translated as follows:

Property  |SdrPropertyType            |SdfValueTypeName
:-------- |:------------------------- |:----------------
foo       |SdrPropertyTypes->Color    |SdfValueTypeNames->Color
bar       |SdrPropertyTypes->String   |SdfValueTypeNames->String
surface   |SdrPropertyTypes->Terminal |SdfValueTypeNames->Token