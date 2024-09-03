# Creating New Schema Classes with usdGenSchema {#Usd_Page_GeneratingSchemas}

USD provides a code generator script 'usdGenSchema' for creating new schema 
classes. The script is driven by a USD layer (typically named schema.usda) and 
generates the necessary C++ classes and supporting Python code for all the 
schema classes defined in it.

This USD layer must meet the following prerequisites in order for generated code 
to compile and work with USD Core successfully.

  - Must specify the libraryName as layer metadata.
  - Schema typenames must be unique across all libraries.
  - Attribute names and tokens should be camelCased valid C/python identifiers,
    with the allowance that ':' signifies property namespacing. However if 
    useLiteralIdentifier metadata is provided in the schema.usda, then when 
    creating public tokens for names, token-values, etc, code gen will try to 
    use the tokens as-is substituting underscores where necessary to produce a 
    valid identifier. Any invalid identifier will be updated using 
    TfMakeValidIdentifier. Also note that as per established convention schema 
    tokens, library tokens, relationship names and property namespace prefix 
    tokens follow the use literal identifier pattern. See 
    \ref Usd_OM_UsdProperty and \ref Usd_NameSpacedPropertiesInCodeGen for more 
    information.
  - usd/schema.usda must exist in the LayerStack, not necessarily as a direct 
    subLayer. 
  - If the names, default values and token values are not valid C/python
    identifiers, then during schema gen we try to update these such that:
      - If the first element of the token is a digit, an '_' is prefixed before
      running it through TfMakeValidIdentifier.
      - All invalid tokens are updated to replace any illegal element with an
      '_'.
      - Do note that since all special characters are replaced with an '_',
      tokens like "foo#bar" and "foo$bar", will result in the same identifier
      "foo_bar", hence violating the One-to-One mapping of token identifiers
      mentioned in \ref Usd_NameSpacedPropertiesInCodeGen below.

See pxr/usd/lib/usdGeom/schema.usda for an example.

## IsA Vs. API Schemas {#Usd_IsAVsAPISchemas}

Schema classes can be classified into the following two types:

 - <b>API schema</b> - An API schema provides an interface to a 
   prim's qualities, but does not specify a typeName for the underlying prim. 
   The prim's qualities include its inheritance structure, attributes, 
   relationships etc. Since it cannot provide a typeName, an API schema is 
   considered to be non-concrete. As a convention, the C++/python class name for 
   an API schema must end with "API".  In core USD, UsdModelAPI is an example of 
   an API schema; UsdRiMaterialAPI is an example from our RenderMan schema 
   module, which adds/extracts RenderMan-specific shading information from a 
   generic UsdShadeMaterial-typed prim.  Also by convention (with which 
   usdGenSchema can help), the properties that "belong" to an API schema are 
   typically namespaced with the base-name of the schema, camelCased. For 
   example, UsdRiMaterialAPI::CreateSurfaceAttr() will create an attribute named 
   outputs:ri:surface.
   API schemas are classified into the following two sub-types: 
        - <b>Non-applied API schemas</b> - If an API schema only provides an 
        interface to certain core bits of metadata (like UsdModelAPI, which sets 
        model kind and UsdClipsAPI, which sets value-clips related metadata) or 
        if there is no use of recording the application of the API schema on a 
        prim (for the purpose of interchange), we make it a non-applied API 
        schema. Examples of non-applied API schemas include UsdModelAPI, 
        UsdClipsAPI, UsdShadeConnectableAPI and UsdGeomPrimvarsAPI. Typically, 
        non-applied API schemas can apply to any prim-type (eg, UsdClipsAPI) or 
        to a known fixed set of prim types, like in the case of 
        UsdShadeConnectableAPI which is only applicable to types that implement
        connectability behaviors such as shaders and lights.
        - <b>Applied API Schemas</b> - If there is a need to record and discover 
        whether an API schema has been applied to a prim, we make it an applied 
        API schema. An applied schema will impart its properties as additional 
        built-in properties on the prim. A public Apply() method is 
        auto-generated for applied API schemas. Once an API schema has been 
        applied to a prim, prim.HasAPI<APISchemaType>() will return 
        true. An applied API schema must be applied to a prim via a call to the 
        Apply() method, for the schema object to evaluate to true when 
        converted to a bool using the explicit bool conversion operator.
        Examples of applied API schemas include UsdCollectionAPI, 
        UsdGeomModelAPI and UsdGeomMotionAPI. All applied API schemas must 
        inherit from UsdAPISchemaBase directly and cannot inherit from other
        schemas. However, applied API schemas can include other applied API
        schemas as \ref Usd_BuiltinAPISchemas "built-ins". Applied API schemas
        are further classified into the following two categories:
            - <b>Single-Apply API Schemas</b> - Applied API schemas that can 
            only be applied as a single instance to a prim. 
            Examples of single-apply API schemas include UsdGeomModelAPI and 
            UsdGeomMotionAPI. 
            - <b>Multiple-Apply API Schemas</b> - Applied API schemas that can 
            be applied multiple times on the same prim with different instance 
            names. For example, UsdCollectionAPI, which must be applied once 
            per collection owned by a prim. Properties instantiated by this 
            API schema are prefixed with the namespace prefix of the schema 
            followed by the instance name. There is specific metadata one 
            authors in schema.usda to identify the prefix for multi-apply 
            schemas, and the properties for a multi-apply schema should 
            be listed without any prefix.

 - <b>IsA schema</b> - An IsA schema can impart a typeName to a prim in
    addition to providing an interface to a prim's qualities. Every IsA
    schema must derive from the core class UsdTyped, which is the base class
    for all typed schemas. Furthermore, an IsA schema can be concrete or
    non-concrete. An IsA schema will be concrete (or instantiable) if its
    schema declaration provides both a name for the schema (in quotes) and a
    typeName in the schema.usda file in which it is defined. A non-concrete
    (abstract) IsA schema provides only a name for the schema, and hence
    cannot be instantiated; non-concrete schemas exist to serve as
    super-classes for related sets of concrete IsA schemas. UsdGeomImageable
    is an example of a non-concrete IsA schema. UsdGeomScope is an example of
    a concrete, typed IsA schema.

\note "Instantiable," in this context, means instantiable as a typed prim in
scene description.  The generated classes for all schemas, be they API,
concrete-typed or non-concrete-typed are always instantiable in C++ or python
as interface objects through which one can interact with UsdPrim objects.

The definitions of both IsA schemas and applied API schemas are published, at 
runtime, into an introspectable 
\ref Usd_OM_SchemaDefinitionRegistry "schema definition registry", which
produces the \ref UsdPrimDefinition "prim definitions" consulted by 
core USD when performing property value resolution (i.e. retrieving a property's 
value at a given UsdTimeCode). This allows IsA and applied API schemas to 
provide \em fallback \em values for their properties, that is, a value
that the property will possess, even when none has been authored. Because a
prim can only have one typeName, a prim can "be" (IsA) at most a single type, 
but can host data for any number of API schemas. This combination of IsA and 
applied API schemas constitutes the prim's complete type signature and is used 
to construct a single \ref UsdPrimDefinition "prim definition" that provides 
all the built-in properties and fallback values for the prim.

The author of an API schema has to decide on the type of API schema at the time 
of its creation by setting token-valued customData entry 'apiSchemaType' in the 
schema definition. It can be set to one of 'nonApplied', 'singleApply' or 
'multipleApply'. When unspecified, the fallback apiSchemaType for an API schema 
is 'singleApply'. An API schema can only inherit from another compatible API 
schema with matching customData["apiSchemaType"] or from "/APISchemaBase" 
directly. This is enforced by the schema code generation script 'usdGenSchema'. 

API schemas and non-concrete typed schemas must not provide a typeName in their 
class declaration in a schema.usda file. 

See \ref Usd_ExampleSchema for examples of each type of schema class.

## Built-in applied API schemas {#Usd_BuiltinAPISchemas}

In some cases an IsA schema may want to have certain API schemas always applied 
to prims of its schema type. To avoid having to manually apply these API schemas
to all instances of these prim types we provide a few ways to specify built-in 
applied API schemas for IsA schema types. When a prim of an IsA schema type 
with built-in applied API schemas is created, all properties from the API 
schemas will also be built-in properties of the prim. The list of built-in API 
schemas can be queried from the prim type's 
\ref UsdPrimDefinition::GetAppliedAPISchemas "prim definition" and the prim will
also return true from UsdPrim::HasAPI for all of its built-in API schemas.

Furthermore, for some applied API schema types, we may want the application of 
the API schema to additionally apply one or more other API schemas. We also 
provide, through the same mechanisms as we have for IsA schemas, the ability to 
specify built-in applied API schemas for both single-apply and multiple-apply 
API schema types. When an API schema with built-in API schemas is applied to a
prim (or is included as a built-in for another type), all of its built-in API
schemas are also applied. These built-ins nest, so an API schema can include
another built-in API schema which itself includes yet another built-in API
schema and all these will be applied when the top level API schema is applied.

The built-ins of both IsA and single-apply API schemas may only contain the
names of single-apply API schemas and named instances of multiple-apply API
schemas (such as "ExampleSingleApplyAPI" and "MultiApplyAPI:foo").

For multiple-apply schemas, since they must always be applied to a prim using an
instance name, their built-in schemas must be able to be applied to a prim
using the same instance name (or a suffixed version of it) as the schema 
they're built in to. Thus, the schema names listed as built-ins for
multiple-apply API schemas must only refer to other multiple-apply API schemas
and can be specified in two ways:

1. By a non-instanced multiple-apply API schema name, e.g. "OtherMultiApplyAPI".
   This indicates an "inheritance-like" built-in as applying an instance of
   the multiple-apply API schema will always also apply an instance of 
   built-in API schema with the same instance name.

2. By a full instance name of a multiple-apply API schema, 
   e.g. "OtherMultiApplyAPI:someName".
   This indicates an "encapsulated instance" built-in since it provides an
   instance name suffix to append to the applied instance name to use for the
   built-in API schema's instance name. For example, if we had a multiple-apply
   API that included the built-in "OtherMultiApplyAPI:someName" and were to 
   apply it using the instance name "foo", then, OtherMultiApplyAPI would also 
   be applied using the suffixed instance name "foo:someName"

### Schema defined built-in APIs {#Usd_SchemaDefinedBuiltinAPIs}

The first and most straightforward way an IsA or applied API schema author
can specify which applied API schemas it wants built-in is by prepending them 
to the \em apiSchemas field in their schema definition like so:

\code
# Example of a concrete, typed (IsA) schema with built-in applied API schemas
class MyCustomPrim "MyCustomPrim" (
    inherits = </Typed>

    # These listed applied API schemas will be built-in to this schema type.
    prepend apiSchemas = ["ExampleSingleApplyAPI", "ExampleMultiApplyAPI:foo"]
)  {
    # Example of a fallback value override for a theoretical property defined in 
    # the API schema instance "ExampleMultiApplyAPI:foo"
    bool exampleMulti:foo:boolAttr = true (
        customData = {
            bool apiSchemaOverride = true
        }
    }
}

# Example of a single-apply API schema with built-in applied API schemas
class "MyCustomAPI" (
    inherits = </APISchemaBase>

    # These listed applied API schemas will be built-in to this schema type and
    # will always be applied when this schema is applied.
    prepend apiSchemas = ["ExampleSingleApplyAPI", "ExampleMultiApplyAPI:foo"]
)  {
    # Example of a fallback value override for a theoretical property defined in 
    # the API schema instance "ExampleMultiApplyAPI:foo". This schema will be
    # stronger when applied than its built-in API schemas and this default value
    # will be used for the added property.
    bool exampleMulti:foo:boolAttr = true (
        customData = {
            bool apiSchemaOverride = true
        }
    }
}

# Example of a multiple-apply API schema with built-in applied API schemas
class "MyCustomMultiApplyAPI" (
    inherits = </APISchemaBase>
    customData = {
        token apiSchemaType = "multipleApply"
        token propertyNamespacePrefix  = "myCustomProp"
    }

    # These listed applied API schemas will be built-in to this schema type and
    # will always be applied when this schema is applied.
    # Applying MyCustomMultiApplyAPI with instance name like "bar" will apply 
    # the following multiple-apply schema instances:
    #   MyCustomMultiApplyAPI:bar
    #   ExampleMultiApplyAPI:bar
    #   OtherMultiApplyAPI:bar:foo
    prepend apiSchemas = ["ExampleMultiApplyAPI", "OtherMultiApplyAPI:foo"]
)  {
    # Example attribute. It will have the full name 
    # myCustomProp:<instanceName>:boolAttr when applied.
    bool boolAttr = true (
        doc = "Multiple apply schema attribute example"
    )
}

\endcode

The listed API schemas will be built-in to the declared schema type. For IsA
schemas, derived types of the schema will inherit its built-in API schemas
(applied API schemas cannot inherit from each other). The \em apiSchemas field 
must always be specified with \em prepend as we only allow a schema to add new 
built-in API schemas that are stronger than any that may be inherited from its 
parent schema. Any schema can specify overrides to properties that would come 
from one of its built-in API schemas by declaring the property itself. This 
should be done carefully to avoid breaking conformance with the built-in schema
itself (like changing the property's type) and is typically used for changing
the default value for the property.
\sa \ref Usd_APISchemaStrengthOrdering

### Auto applied API schemas {#Usd_AutoAppliedAPISchemas}

In addition to schemas being able to declare built-in applied API schemas,
an applied API schema can specify that it should be automatically applied to 
any number of IsA or single-apply API schemas. The primary use case for this is 
when a schema author wants to extend an existing schema with properties from 
their own applied API schema without altering the existing schema itself. 
Specifying the API schema to auto apply can accomplish this goal without having 
to manually apply the API to every prim instance of a schema type (or to every
prim instance with a specific API schema applied). A concrete example would be a 
renderer providing an API schema for its own specific render properties that can
then be automatically applied to UsdLux light typed prims when the renderer and 
its API schema are available.

An author of an API schema can specify the IsA and/or single-apply API schemas 
to which it wants to be automatically applied through the 
\em apiSchemaAutoApplyTo field in the API schema definition's customData like 
so:

\code
# Example of an applied API schema specified to auto apply to both a concrete
# and an abstract IsA schema as well as another single-apply API schema.
class "MyCustomAPI" (
    inherits = </APISchemaBase>
    customData = {
        # These listed IsA and API schemas add this API schema to their built-in 
        # API schemas.
        token[] apiSchemaAutoApplyTo = [
           "MyCustomConcretePrim", "MyCustomAbstractPrim", "MyCustomAPI"]
    }
)  {
    int autoApplyApiAttr = 1
}
\endcode

The specified schema names in the \em apiSchemaAutoApplyTo field can refer to 
another single-apply API schema and to both abstract and concrete IsA schema 
types. For the IsA schema types, the applied schema will be added to the 
built-in schemas for the listed types as well as for any derived schemas of the 
listed types. Note that this customData field is only supported for single apply
API schemas as multiple apply API schemas cannot be applied without an instance 
name.

### Plugin defined auto applied API schemas {#Usd_PluginAutoAppliedAPISchemas}

Lastly, it is also possible for plugins to specify additional built-in API 
schemas for other schema types outside of schema generation. This is useful if a
client desires schema types to have built-in API schemas but doesn't want to 
force this upon all clients by changing the generated schemas themselves. An 
example use case for this would be a client pipeline that wants an available UI 
related API schema applied to all shader prim types as this is not something all
users of these schemas may necessarily want.
 
A plugin author can specify their additional built-in API schemas in their 
plugin's plugInfo.json file using the \em AutoApplyAPISchemas metadata like so:

\code
{
    "Plugins": [
        {
            "Info": {
                "AutoApplyAPISchemas" : {
                    "ExistingSingleApplyAPI": {
                        "apiSchemaAutoApplyTo": [
                            "ExistingPrimType"
                        ]
                    },
                    "ExistingMultiApplyAPI:foo": {
                        "apiSchemaAutoApplyTo": [
                            "ExistingPrimType", "ExistingSingleApplyAPI"
                        ]
                    }
                }
            }
        }
    ]
}
\endcode

The metadata format is a dictionary of API schema names to a list of the schemas
the plugin wants them to be applied to as a built-in API. Like with the schema 
generated \ref Usd_AutoAppliedAPISchemas "auto apply schemas", the API schemas 
will also be applied to any derived schema types of any listed IsA schemas. 
Multiple apply schemas (in addition to single apply) can also be auto applied 
using this plugin metadata as long as the fully qualified instance name for the 
mulitple apply schema is provided.

### Property conflicts, composition, and API schema strength ordering {#Usd_APISchemaStrengthOrdering}

Given that we encourage (or even require in the case of multiple-apply schemas)
the use of namespace prefixes for API schema property names, we expect most
properties defined in API schemas will be unique to the schema. However, there
will always be the possibility that properties with the same name may be defined
in multiple API schemas applied to the same prim. To account for this, there is
a strength ordering for applied API schemas that determines which property
definition "wins" when more than one API schema defines a property of the same
name.

In defining schema strength ordering, it is helpful to talk about a schema order
where **the first schema is the strongest and the last schema is the
weakest**. The strength order of schemas is as follows:

The strongest opinion for all aspects of a built-in property in the IsA schema's
\ref UsdPrimDefinition "prim definition". I.e., the IsA schema comes first in
the schema order.

The strength order of applied API schemas is determined by adding them to the
schema order, after the IsA schema, as follows:

1. Any \ref Usd_SchemaDefinedBuiltinAPIs "built-in schemas" that are defined in
   `apiSchemas` metadata are added to the schema order, immediately after the
   schema that introduces them, in the order they appear in the composed
   `apiSchemas` field.

2. Any auto applied API schemas (whether defined in the \ref
   Usd_AutoAppliedAPISchemas "API schema's definition" or in a \ref
   Usd_PluginAutoAppliedAPISchemas "plugin's metadata") come immediately after
   the schema to which they apply. When more than one API schema is auto applied
   to the same schema type, they are sorted in reverse dictionary order by
   schema name before they are appended to the schema order.  Reverse dictionary
   ordering ensures that the newest version of a versioned schema is the
   strongest if multiple versions are applied. Otherwise, this ordering is
   arbitrary but deterministic as there is no provided way to indicate strength
   ordering between auto applied API schemas.

3. Authored API schemas, API schemas that are applied to a prim instance in
   scene description, are added last.

This applied API strength order is invoked recursively, in a depth first
expansion, because **all schema types can introduce built-in schemas and can
cause additional auto applied schemas to be applied**.

### API schema property underrides {#Usd_APISchemaPropertyUnderride}

For properties that are defined by more than one of the included schemas, fields
for which the strongest property definition does *not* express an opinion,
**may** be composed from weaker API schema definitions (i.e., the weaker
definition may act as an _API schema underride_) if the following conditions are
met:
- The property types of the stronger and weaker property definitions
  match. Specifically, the property definitions must both be attributes or both
  be relationships, and if they are attributes, their type names must be exactly
  the same.
- The field is in the set of _allowed composable fields_, which currently
  includes only _default_ and _hidden_. We may expand the set of allowed
  composable fields in the future.

### API schema property overrides {#Usd_APISchemaPropertyOverride}

Sometimes when an IsA or an API schema includes another API schema as a
built-in, it may also want to alter one or more of the built-in schema's
properties to give the property a more reasonable fallback value or change some
other metadata on the property like its documentation or its _allowedTokens_. 
But we don't want the schema to have to fully redefine the property when it only
wants to sparsely override the metadata it cares about. In order to enable this
behavior we allow a schema to explicitly tag a property as an _API schema
override_.

A schema property that is declared as an API schema override does not define 
the property in that schema. Instead it behaves as an _over_ for a property
with the same name that may be defined by any of its included built-in API
schemas. This is different than the standard behavior when included API schemas
define the same property (without specifying an override) as in that case, the
strongest schema's property wins and completely stomps over the same property
from any weaker schema (as specified in \ref Usd_APISchemaStrengthOrdering).

API schema overrides can be declared for both IsA and applied API schemas in the
source schema files through adding 
\code bool apiSchemaOverride = true \endcode
to the \ref Usd_CustomizingPerProperty "customData of the property". When the 
\ref UsdSchemaRegistry "schema registry" is populated, any properties declared
as API schema overrides will be composed over the defined property from the
schema's built-in API schemas if a property with that name is indeed defined in
one of the built-in schemas. If no property with the name exists in the included
API schemas, the API schema override property is ignored.

An example of a common use case for this are schemas that include instances of 
\ref UsdCollectionAPI "CollectionAPI" and want to only change the fallback for
\ref UsdCollectionAPI::GetIncludeRootAttr "includeRoot" or 
\ref UsdCollectionAPI::GetExpansionRuleAttr "expansionRule", e.g.:
\code
class "MyCustomAPI" (
    prepend apiSchemas = ["CollectionAPI:myColl"]
) {
    # Override CollectionAPI:myColl to set the fallback for includeRoot to true
    # instead of using CollectionAPI's normal fallback of false.
    uniform bool collection:myColl:includeRoot = 1 (
        customData = {
            bool apiSchemaOverride = true
        }
    )
}
\endcode

Because we treat auto-apply API schemas as an extension of the built-in API
schemas included in a schema, API schema override properties can also be used
to sparsely override properties defined in auto-apply API schemas. Since 
auto-apply schemas will typically be defined in plugins, API schema override 
properties are a convenient way to allow a schema to override a property that it
expects to come from an auto-apply API schema without defining the property 
itself when the auto-apply API schema is not present (like when the auto-apply
API schema's plugin isn't included).

There are some limitations to how API schema override properties can be used:
1. Any property declared as an API schema override **must have the exact same 
   type** of the property it overrides. The property type consists of
   whether the property is an attribute or a relationship and, if the property
   is an attribute, the type name (int, float, color3f, etc.) of the attribute. 
   Any API schema property overrides that do not conform to the underlying 
   property type are ignored. Note that the variability of an API schema override
   property is allowed to be different than the defined property but the
   override's variability will be ignored.

2. API schema property overrides are currently only resolved when generating the
   static \ref UsdPrimDefinition "prim definitions" for registered IsA and
   applied API schema types in the schema registry. "Dangling overrides" are not
   retained which means they cannot be used by an IsA schema (or by builtin or
   auto-apply schemas included by an IsA schema) to provide overrides to
   properties from authored API schemas applied in scene description.

3. API schema property overrides only apply to the properties defined by API 
   schemas directly or indirectly included by the schema defining the override
   itself. What this means is that if you have a schema "A" that includes two 
   built-in API schemas "B" and "C", property overrides declared in "A" will 
   affect properties defined in both "B" and "C" (as well as properties defined 
   in any API schemas "B" and "C" include). However, property overrides declared
   in "B" will **not** affect any of the properties defined in "C" (and schemas 
   included by "C") and vice versa.

Note that API schema property overrides are specific to overriding properties
from built-in API schemas and do not apply to overriding properties via typed
schema inheritance. This is because schema properties always sparsely compose
via class inheritance during schema generation so no override specification is
necessary.

## Schema Code Generation {#Usd_SchemaCodeGeneration}

Simply run the ```usdGenSchema``` command to generate code in the current 
directory for the schema classes defined in the file named 'schema.usda'. 

The code generator uses \c jinja2 templates that are installed with USD.
build.  The default schema class templates that ship with USD include the 
following files:

  - <b>schemaClass.h, schemaClass.cpp, wrapSchemaClass.cpp</b>:
    One set for each class found in schema.usda

  - <b>%tokens.h, tokens.cpp, wrapTokens.cpp</b>:
    Contains static TfTokens for use with all schema in the library.

  - <b>plugInfo.json</b>: 
    Every Pxr module that contains plugins has one of these.  We add a 
    declaration for every generated schema class into this file so that 
    USD core can discover all plugin prim types cheaply and robustly.

  - <b>%api.h</b>: 
    Boilerplate macro definitions for exporting symbols on various platforms.

In addition to the files in schemata and tokens related files, the following 
files are edited by the script:

  - <b>generatedSchema.usda</b>: 
    Processed form of schema definitions that will be consumed at runtime by 
    USD core.

\parblock
\note usdGenSchema will update existing files in the current directory 
if it detects any differences with the code it generates. Make sure these 
files are editable before running usdGenSchema
\endparblock

\parblock
\note usdGenSchema does not update the CMakeLists.txt and module.cpp files, 
even if they are editable.  If you have added a new class(es), you must 
add them to these files yourself.  
\endparblock

Various command-line options are available for customizing the code generation 
process. Run ```usdGenSchema --help``` for more details.

## Namespaced Properties in Code Generation {#Usd_NameSpacedPropertiesInCodeGen}

usdGenSchema also supports the use of namespaced properties for code generation.  
For example, float  foo:bar will generate UsdMyClass::GetFooBarAttr() and 
UsdTokens->fooBar (with a value of "foo:bar").  usdGenSchema will raise 
exceptions to avoid naming collisions in the schema API and enforces a 
One-to-One mapping of token identifiers to token values, as shown below. Note
that irrespective of useLiteralIdentifier value, property names with ':' are
always camelCased for the defined convention.

\code
class MyClass "MyClass" {
    # Generates UsdMyClass::GetFooBarAttr() and UsdTokens->fooBar with value 
    # "fooBar"
    float fooBar

    # ERROR: Naming collision in both API and tokens. Generates 
    # UsdMyClass::GetFooBarAttr() and UsdTokens->fooBar with value "foo:bar"
    float foo:bar

    # ERROR: Naming collision in tokens. Generates UsdMyClass::GetMyTokenAttr() 
    # and UsdTokens->fooBar with value "foo-bar"
    token myToken = "foo-bar" (allowedTokens = ["foo-bar"])    
}
\endcode 

## Global Schema Properties {#Usd_GlobalSchemaProperties}

Each schema.usda file can contain a \em GLOBAL section like the following to
provide customizations that apply to all the classes in the module:

\code
over "GLOBAL" (
    customData = {
        string libraryName       = "pxUsdGeom"
        string libraryPath       = "folder/pxUsdGeom"
        string libraryPrefix     = "PxUsdGeom"
        string tokensPrefix      = "PxUsdGeom"
        dictionary libraryTokens = {
            dictionary libraryToken1 = {}
            dictionary libraryToken2 = {
                string value = "/non-identifier-tokenValue!"
                string doc = """doc for libraryToken2"""
            }
        }
        bool skipCodeGeneration = true
    }
)
{
\endcode
 
Here's a short description of each datum in the global customData dictionary:

  - \b libraryName - The name of the module into which the schema-generated files 
    will be installed. \b Required!

  - \b libraryPath - The partial path with which to prefix '#include' statements 
    of generated files for this module. For external (non-Pixar) plugins, we 
    recommend setting libraryPath to ".", so that the headers inside src 
    directory are included in the generated files. \b Required unless 
    skipCodeGeneration is true!

  - \b libraryPrefix - The prefix for all generated schema classes in the module.  
    If not specified, falls back to ProperCase(libraryName)

  - \b tokensPrefix - The prefix to use for the tokens class, if it needs to be 
    different from libraryPrefix 

  - \b libraryTokens - a place to declare tokens meaningful to the module.
    These tokens will be included in the module's static tokens. If 
    provided, the "doc" string will be included in the tokens documentation.
    If provided, the "value" string will be assigned as the token's value; 
    otherwise, the token's value will be its identifier (as a TfToken).
 
  - \anchor codeless_schema \b skipCodeGeneration - Flag that, when set to 
    true, tells schema generation that it should not generate any C++ code for 
    the classes in this module. We call a schema without any corresponding C++ 
    classes a "codeless schema". Note that if skipCodeGeneration is set to true 
    for a schema module, then any other schema module that includes it will 
    also not generate any code. This is because we don't want to generate C++ 
    code for classes that would possibly inherit from another C++ class that 
    doesn't exist.

  - \b useLiteralIdentifier - when set for a library all tokens (default values,
 	allowedTokens and property names) will try to use literal names as-is, instead
 	of camelCasing these, which is the default behavior. For invalid tokens
 	defined in schema.usda, usdGenSchema will use TfMakeValidIdentifier to make
 	valid tokens. Note that for property names containing ':', representing
 	namespace prefixes, usdGenSchema will continue to use camelCase as mentioned
 	in \ref Usd_NameSpacedPropertiesInCodeGen, example "namespacePrefix:attrName"
 	will be camelCased to "namespacePrefixAttrName" irrespective of
 	useLiteralIdentifier being set or not.
 
## Codeless Schemas {#Codeless_Schemas}

By default ```usdGenSchema``` generates C++ and Python code, providing
appropriate APIs. Clients also have an option of not generating any code by
setting the \em skipCodeGeneration metadata to True for a given schema, hence 
generating **only generatedSchema.usda and plugInfo.json** which are the only 
essential products for runtime schema registration.

Since codeless schemas do not provide any code, clients do not need to recompile
USD to use or update these schemas. This "dynamic" nature of codeless schemas 
is the primary motivation behind using codeless schemas. 
Also, for the same reasons, clients will have to use basic USD level APIs to 
query prims and attributes associated with the codeless schemas. This also means
codeless schemas concept can not be used for any schema which requires custom
code.

The usdRiPxr schema domain that ships with USD is an example of a codeless 
schema domain. These are usd schemas generated using renderman shader 
definitions (args files) using the Sdr library. ```usdgenschemafromsdr``` is the
utility which generates schema.usda, generatedSchema.usda and plugInfo.json for
the usdRiPxr schema domain.

## Customizing Per-Class Properties {#Usd_PerClassProperties}

\code
class PxHairman "PxHairman" (
    customData = {
        string className = "Hairman"
        string fileName = "_hairman"
        string extraIncludes = """
#include "pxr/usd/usdGeom/primvar.h"
"""
        dictionary extraPlugInfo = {
            string customString = "metadata"
            bool customBool = true
            int customInt = 0
            dictionary customDict = {
                string customNestedString = "nested"
            }
        }
        dictionary schemaTokens = {
            dictionary schemaToken1 = {}
            dictionary schemaToken2 = {
                string value = "/non-identifier-tokenValue!"
                string doc = """doc for schemaToken2"""
            }
        }
        token[] fallbackTypes = ["PrimTypeName1", "PrimTypeName2"]
    }
)
{
}
\endcode

Here's a short description of each datum in the per-class customData dictionary:

  - \b className - If the USD prim typeName for the schema must be different than 
    the un-prefixed class name,  then use className customData to provide the 
    class name for the C++ (which will be prefixed) and python schema classes. 

  - \b fileName - if specified, will be the base name for the .h and .cpp generated 
    files.  If not specified, base name falls back to  CamelCase(className)

  - \b extraIncludes - if specified, will add extra include paths for files 
    required by the "custom" section of this class only. Note that this will 
    add includes to the generated header file for the class. If includes 
    are only needed in the generated implementation file(.cpp), 
    one can instead add the header includes to the custom 
    section of the cpp file specifically.

  - \b extraPlugInfo - if specified, the (key, value) pairs in this dictionary 
    will be added as additional metadata for this class in the library's 
    plugInfo.json file. The values in this dictionary must be numeric types,
    strings, booleans, or dictionaries containing these types.
    Examples of such extraPlugInfo include
    _providesUsdShadeConnectableAPIBehavior_, _isUsdShadeContainer_,
    _requiresUsdShadeEncapsulation_ -- these can be used to configure
    UsdShadeConnectableAPIBehavior for Typed or API schemas.

  - \b schemaTokens - a place to declare tokens meaningful to the schema.
    These tokens will be included in the module's static tokens. See docs
    above for the libraryTokens entry in the global customData dictionary
    for details about the contents of this dictionary.

  - \b apiSchemaType - must only be specified for an API schema. Defaults to 
    the token 'singleApply', to indicate a single-apply API schema. 
    Can be set to 'nonApplied', to create a non-applied API schema or to 
    "multipleApply" to create a multiple-apply API schema.

  - \b propertyNamespacePrefix - must only be specified on multiple apply API 
    schemas which have properties. This token, alongside the instance name, 
    will be inserted as a prefix to all properties created by this 
    multiple-apply API schema.

  - \b apiSchemaAutoApplyTo - must only be specified on single apply API 
    schemas. This is a token array value that lists the names of typed schemas 
    and other single apply API schemas that this API schema will be 
    automatically applied to when the listed typed schemas are processed by the 
    \ref Usd_OM_SchemaDefinitionRegistry "schema definition registry."

  - \b apiSchemaCanOnlyApplyTo - must only be specified on single and multiple 
    apply API schemas. This is a token array value that, when specified, is used
    by UsdPrim::CanApplyAPI and the schema's generated CanApply function to 
    limit the prims to which this API schema can be validly applied to only the 
    schema types whose names are in this list.
    
  - \b apiSchemaAllowedInstanceNames - must only be specified on multiple apply 
    API schemas. This is a token array value that, when specified, is used by 
    UsdPrim::CanApplyAPI and the schema's generated CanApply function to limit 
    the instance names that are valid to use when applying this API schema to 
    only the instance names included in this list.

  - \b apiSchemaInstances - must only be specified on multiple apply API 
    schemas. This is a dictionary value that allows additional custom data 
    to be specified for specific instance names of this multiple apply API 
    schema. This dictionary can map an instance name to another dictionary value
    containing the custom data fields that will only apply to that instance 
    name. Currently only \em apiSchemaCanOnlyApplyTo custom data can be 
    specified in these per instance name dictionaries and will take precedence
    over the \em apiSchemaCanOnlyApplyTo custom data (specified for the entire 
    schema type) for that instance name when present. The GridCrittersAPI
    in the \ref Usd_ExampleSchema "examples section" demonstrates how this can
    be used.

  - \b fallbackTypes - must only be specified for a concrete typed schema. 
    This is a token array value used to specify the preferred fallback schema 
    types that can be used instead when this schema isn't present.
    This data is used by UsdStage::WriteFallbackPrimTypes to record, on a stage,
    the \ref Usd_OM_FallbackPrimTypes "fallback prim types" metadata that USD 
    versions which lack this schema will use to choose a suitable alternative 
    schema type.

## Customizing Per-Property  {#Usd_CustomizingPerProperty}

\code
ColorFloat[] primvars:displayColor (
    customData = {
        string apiName = "displayColor"
    }
)
\endcode
 
Here's a short description of each datum in the per-property customData 
dictionary:

  - \b apiName - Schema properties may define an 'apiName' in customData to 
    override the default generated accessor API. For example, the above spec 
    produces GetDisplayColorAttr instead of GetPrimVarsDisplayColorAttr as the 
    attribute accessor.  As a special case, if 'apiName' is set to the
    empty string, then no accessor API will be generated.
    Note: The actual name of the property as defined on the prim is still
    primvars:displayColor.
  - \b apiGetImplementation - Optionally control schema gen behavior for a property's 
    'Get' implementation. Valid values:
    - \e generated - Generate default header and implementation (Fallback value if unspecified).
    - \e custom - Generate default header ONLY. User must supply the implementation
    Custom should be used sparingly, primarily as a tool for API migration
    Given the performance expectations of Get, it is NOT appropriate to 
    add complicated validation logic in this method.
  - \b apiSchemaOverride - If this boolean value is set to true, it indicates
    this property is an 
    \ref Usd_APISchemaPropertyOverride "API schema override". Additionally, no
    accessor API will be generated for this property, as if the property had
    set 'apiName' to the empty string. 

## Example Schema Classes {#Usd_ExampleSchema}

\code
#usda 1.0
(
    """ This file describes an example schemata used for code generation using 
        usdGenSchema. 
    """
    subLayers = [
        # This is mainly needed for definition of UsdTyped.
        @usd/schema.usda@
    ]
)

over "GLOBAL" (
    customData = {
        string libraryName = "myLib"
        string libraryPath = "componentName/myLib"
        dictionary libraryTokens = {
            dictionary sampleToken = {
                string doc = "Documentation for sample token."
            }
        }
    }
) {

}

# Example of a non-concrete IsA schema
# Note that non-concrete IsA schemas cannot specify a typeName in the class
# declaration, but they are allowed to provide fallback values for attributes.
class "MyBaseCustomPrim" (
    doc = """Defines a non-instantiable (non-concrete) typed schema that derives
             from MyCustomPrim. Derived schema classes can inherit from this 
             schema to add (for e.g.) geometric properties."""
    
    # IsA schemas should derive from </Typed> or a Schema that derives from
    # Typed.
    #
    # API schemas need not specify inherits. usdGenSchema sets the parent
    # class for such schemas to UsdSchemaBase.
    inherits = </Typed>
    customData = {
        string className = "MyBasePrim"
    }
) {
    # Some base attributes common to all derived schemas
    uniform double uniformScale = 1.0 (
        doc = "A double valued uniform attribute representing scale."
    )

    float3 rotation = (0, 0, 0) (
        doc = "A varying 3D vector in floating-pt precision representing rotation."
    )

    double3 translation = (0, 0, 0) (
        doc = "A varying double valued 3D vector representing translation."
    )
}

# Example of a concrete, typed (IsA) schema
class MyCustomPrim "MyCustomPrim" (
    doc = """Defines a custom typed (IsA) schema prim"""

    inherits = </MyBaseCustomPrim>

    customData = {
        string className = "MyPrim"
        customData = {
            string extraIncludes = """
                #include "pxr/base/gf/bbox3d.h"
                #include "pxr/usd/usdGeom/primvar.h" """
        }
    }
)  {
    # Attributes with fallback values.
    asset filePath = @/path/to/default/file@ (
        doc = """An asset path valued attribute that points to a file on disk."""
    )

    uniform token axis = "X" (
        allowedTokens = ["X", "Y", "Z"]
        doc = """A token valued attribute representing an axis."""
    )

    matrix4d transform = ((1,0,0,0), (0,1,0,0), (0,0,1,0), (0,0,0,1)) (
        doc = """Double-precision transformation matrix, which should encode
                 the entire local transformation for a prim.""")
    )

    # Attributes with no fallback values.
    point3f[] points (
        doc = """An attribute representing a list of points in 3D space."""
    )

    string[] strArray (
        doc = """A string array valued attribute."""
    )

    string str (
        doc = """An int valued attribute."""
    )

    # Relationships
    rel target (
        doc = """A relationship called target that could point to another prim
                or a property"""
    )
}

# Example API schema that provides an interface for manipulating a specific 
# set of attributes on a prim.
# 
# API schemas can "declare" and provide access to properties defined by
# collections of other IsA and API schemas, gathered into one API for 
# convenience.  They can also (more commonly, in our use, thus far), define
# their own properties with their own fallbacks.  In this capacity,
# the convention is to namespace each property with the API's name (camelCased),
# for easy identification, as well as to help prevent built-in properties from 
# API schemas from unintentionally overriding built-in properties of the IsA 
# schema when applied.  
# For example...
class "MyParamsAPI" (
    inherits = </APISchemaBase>
    customData = {
        token apiSchemaType = "singleApply"

        # This is an example of specifying typed schemas that this single apply
        # schema will be automatically applied to.
        token[] apiSchemaAutoApplyTo = ["MyCustomPrim", 
                                        "OutsidePluginCustomPrim"]

        # This is an example of specifying typed schemas that this single apply
        # API can only be applied to. Specifying this means that the generated 
        # MyParamsAPI::CanApply(prim) and prim.CanApplyAPI<MyParamsAPI>()
        # will only return true if the prim is one of these listed types.
        token[] apiSchemaCanOnlyApplyTo = ["MyCustomPrim", 
                                           "OutsidePluginCustomPrim", 
                                           "AnotherCustomPrim"]
    }
)
{
    double myParams:size (
        customData = {
            string apiName = "size"
        }
        doc = "double specifying the size."
    )

    uniform int myParams:numSamples (
        customData = {
            string apiName = "numSamples"
        }
        doc = "Uniform int specifying the number of samples."
    )

    double3 myParams:offset = (0, 0, 0) (
        customData = {
            string apiName = "offset"
        }
        doc = "3D offset."
    )
    
    # By default, all properties of IsA and API schemas are considered
    # "builtin", i.e. not \ref UsdProperty::IsCustom() "custom".  However,
    # one can force a schema property to be considered custom by explicitly
    # declaring it to be so.
    custom string info
}

# Example multiple-apply API that gives an interface to create instances of 
# "critters" on a prim.
class "GridCrittersAPI" (
    inherits = </APISchemaBase>
    customData = {
        token apiSchemaType = "multipleApply"
        token propertyNamespacePrefix  = "critter"

        # This is an example of all the ways we can specify how the schema's 
        # generated CanApply and UsdPrim::CanApplyAPI behave for this multiple
        # apply schema. The following customData specifies that:
        # 1. GridCrittersAPI::CanApply(prim, instanceName) will return false
        #    if instanceName is anything but "insect" or "rodent" because of
        #    apiSchemaAllowedInstanceNames.
        # 2. GridCrittersAPI::CanApply(prim, "insect") will only return true
        #    if prim.IsA<MyCustomPrim>() because this is specified in 
        #    apiSchemaInstances
        # 3. GridCrittersAPI::CanApply(prim, "rodent") will only return true
        #    if prim.IsA<MyCustomPrim>() or prim.IsA<AnotherCustomPrim>() 
        #    because this is specified in apiSchemaCanOnlyApplyTo and there is
        #    no override in apiSchemaInstances
        token[] apiSchemaAllowedInstanceNames = ["insect", "rodent"]
        token[] apiSchemaCanOnlyApplyTo = ["MyCustomPrim", 
                                           "AnotherCustomPrim"]
        dictionary apiSchemaInstances = {
            dictionary insect = {
                token[] apiSchemaCanOnlyApplyTo = ["MyCustomPrim"]
            }
        }
    }
)
{
    # this will be instantiated as "critter:<instance name>:xform"
    matrix4d xform = ( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) )
    # this will be instantiated as "critter:<instance name>:color"
    color4f color
}
\endcode

See \ref Usd_Page_Datatypes for the list of all data types provided by Sdf.

## Adding Custom Code To Generated Schemas {#Usd_GeneratedSchemaCustomCode}

Custom code written after the "// --(BEGIN CUSTOM CODE)--" delimiter in the 
generated schema files will be preserved between successive usdGenSchema runs. 
Typically, this will include additional API you may want to provide on your
schema classes.

## Impact on Interchange of Creating and Extending Schemas {#Usd_SchemaExtensionPhilosophy}

Coming soon!
