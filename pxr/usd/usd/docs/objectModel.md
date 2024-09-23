# Object Model and How the Classes Work Together {#Usd_Page_ObjectModel}

## SdfLayer: Shared Data Files {#Usd_OM_SdfLayer}

An SdfLayer provides the interface to a persistent (in a file) or in-memory
only (via "anonymous" layers) container of scene description.  The scene
description contained in a layer consists of prims, attributes,
relationships, user-metadata on all of the above, and composition operators
that specify how the contained scene description should be composed with
scene description in other files.

If working directly with an SdfLayer, clients should be aware that Sdf
maintains an internal registry of layers that clients have requested to be
opened via SdfLayer::FindOrOpen(), SdfLayer::CreateNew() or
SdfLayer::CreateAnonymous().  The registry holds only weak pointers
(SdfLayerHandle) to the layers it caches, so it is the client's
responsibility to retain the strong SdfLayerRefPtr that the above methods
return, if they expect the layer to persist.  UsdStage takes care of this for
the layer for which the stage was opened, and all layers reached during the
process of population the stage by traversing composition arcs; the set of 
"reached" layers may be different for different variant selections on the
stage, different activation opinions, and different load-states.

\note <b>USDeeper: Layer-related plugins</b> Sdf also defines the
SdfFileFormat plugin mechanism that provides a file-extension-based
extensible registry of plugins that can either generate a layer's worth of
scene description procedurally, or translate a different file format into
USD's data model and relevant schemas "on the fly".  Sdf also uses the Ar Asset
Resolution plugin API to resolve layer identifiers to external (e.g. file) 
assets.  The Ar plugin API lets USD clients provide customized behavior for 
resolving "asset identifiers" and querying asset metadata.

## UsdStage: Composed View of an SdfLayer {#Usd_OM_UsdStage}

As described in its class documentation, a UsdStage is the interface to a
specific SdfLayer (known as its \em rootLayer), interpreting the data it
contains through the composition rules provided by Pcp.  Pcp informs the
UsdStage about which UsdPrim s should be populated on the stage, and provides
PcpPrimIndex objects, per-prim, which allow usd to perform efficient 
\ref Usd_ValueResolution "value resolution".

The primary client-facing aspect and purpose of a UsdStage is that it creates
and maintains (as new scene description is authored or mutated) a scenegraph
of UsdPrim s that enables efficient scene traversal, data extraction and
authoring.  A UsdStage can contain any number of "root level" prims, each of
which represents a different tree/graph (which may be related to each other
via composition operators or \ref Usd_OM_UsdRelationship "relationships").
To facilitate traversals that visit all root prims, every UsdStage has an
un-named "pseudo-root" prim that is the parent of all root prims; it can be
accessed via UsdStage::GetPseudoRoot().

An important property of the stage is that it always presents the accurate,
fully-composed view of the data in its underlying SdfLayer s.  This has
impact on \ref Usd_Page_AuthoringEditing , and implies that a UsdStage may
perform a potentially substantial amount of work in "recomposing" a scene in
response to certain kinds of authoring operations, namely the authoring of
composition operators.  For example, when one adds a reference to a prim
using UsdReferences, the stage on which the prim sits will immediately pull
in the scene topology information from the referenced SdfLayer, and
repopulate the affected parts of the stage.  Something similar occurs even
when one changes a variant selection on an existing UsdVariantSet.

\note <b>USDeeper: Editing Layers Updates Stage.</b> UsdStage recomposes in
response to mutations of SdfLayer 's that are composed into the stage.  Thus,
a UsdStage will remain accurate even if one uses the lower-level Sdf API's to
mutate layers, rather than the USD object API's.

### UsdStage Lifetime and Management {#Usd_OM_UsdStage_Management}

SdfLayer and UsdStage are the only objects in USD whose lifetime matters. 
SdfLayer is the true data container in USD, and if a layer is destroyed before
its contents are explicitly serialized (SdfLayer::Save(), SdfLayer::Export()),
then data may be lost.  If a UsdStage is destroyed, it will drop its retention
of all of the SdfLayer s it composes; if that results in a layer-with-changes'
refcount to drop to zero, the layer will be destroyed.

Like SdfLayer, UsdStage is also managed by strong and weak pointers; all
stage-creation methods return a UsdStageRefPtr for the client to retain.
UsdStages \em can also be managed in a registry, known as a UsdStageCache.
 Unlike the SdfLayer registry, however:

\li There isn't a core, global singleton registry - clients can make as many 
UsdStageCache registries as they need.  UsdUtils does provide a singleton
that clients can opt to use, if a single, known registry is appropriate.  This
is useful in scenarios where multiple, collaborating subsystems in a process
each need to access data directly from USD, but have estabished API that cannot
be perturbed to pass a UsdStagePtr back and forth.

\li UsdStageCache \em does retain a strong reference to each of the stages it
collects, and the cache can be explicitly cleared.


## UsdPrim: Nestable Namespace Containers {#Usd_OM_UsdPrim}

UsdPrim is the primary object used to interact with composed scene description,
and has the largest API of any of the core objects.  A UsdPrim represents a 
\b unique "namespace location" in a hierarchical composition on a UsdStage.  
Each UsdPrim can contain \ref Usd_OM_UsdProperty "properties" and child 
UsdPrim's, which is what allows us to build hierarchies.  If the following
usd example were opened on a UsdStage:
\code{.unparsed}
#usda 1.0

def "World"
{
    def "Sets"
    {
    }

    over "Fx"
    {
    }
}
\endcode
then we would be able to access the UsdPrim's on the stage, like so:
\code
// SdfPath identifiers can be constructed most efficiently by using SdfPath
// API to build up the path incrementally; however, they can always also 
// be constructed from a full string representation; we demonstrate both forms.
SdfPath worldPath = SdfPath("/World");
UsdPrim world = stage->GetPrimAtPath(worldPath);
UsdPrim sets  = stage->GetPrimAtPath(worldPath.AppendChild(TfToken("Sets")));
UsdPrim fx    = stage->GetPrimAtPath(worldPath.AppendChild(TfToken("Fx")));
\endcode

\note <b>USDeeper: def vs over?</b> You may have noticed in the example above
that the prims \</World\> and \</World/Sets\> were declared as \b def, while
\</World/Fx\> was declared as \b over.  "def" and "over" are two of the three
possible "prim specifiers" that inform USD of the intended purpose of the
data authored for a prim in a particular layer.  For a deeper explanation,
please see \ref Usd_PrimSpecifiers .

### Retaining and Using UsdPrims Safely {#Usd_OM_UsdPrim_Retention}

\todo contents/caching, then , etc

<!-- Not ready yet, so commenting out for now.
### UsdPrim is a Proxy for Composed PrimSpecs {#Usd_OM_UsdPrim_Proxy}

Let us consider an extension of the example above that adds a (intra-layer)
reference to the \</World/Fx\>  prim:

\code{.unparsed}
#usda 1.0

def "World"
{
    def "Sets"
    {
    }

    over "Fx" (
        references = </Effects>
    )
    {
    }
}

def "Effects"
{
    def "Effect_1"
    {
    }

    def "Effect_2"
    {
    }
}
\endcode
-->

## UsdProperty: Common Interface for Attributes and Relationships {#Usd_OM_UsdProperty}

## UsdAttribute: Typed, Sampled, Data  {#Usd_OM_UsdAttribute}

### Time and Timing in USD {#Usd_OM_UsdTimeCode}

## UsdRelationship: Targetting Namespace Objects {#Usd_OM_UsdRelationship}

## General Metadata in USD {#Usd_OM_Metadata}

All of the objects we have described so far, SdfLayer, UsdStage, UsdPrim, and
both subclasses of UsdProperty, can possess \em metadata.  In USD, metadata 
serves several critical roles for describing object behavior and meaning, and
is defined by the following properties:

\li *Metadata is strongly typed*, and can possess a fallback value. Both of
these pieces of information can be retrieved, using the metadatum's name,
from the SdfSchema singleton.  Metadata can have any of the types described
in \ref Usd_Page_Datatypes.

\li *Metadata is extensible.* The implication of the SdfSchema providing type
information about metadata is that all metadata must be registered with
USD/Sdf.  The SdfSchema itself registers a set of metadata meaningful to the
Sdf data model, but any module discoverable by USD's \ref plug_page_front "plugin system"
can extend the known set of metadata, as described in the \ref sdf_plugin_metadata
documentation.  Core USD, and most of the higher-level schema modules use 
this mechanism to good effect.

\li *Metadata is unvarying.* Although metadata values can be overridden in any
layer just as UsdAttribute values can be, \em within a given layer, a metadatum
can have only a single value - i.e. it cannot be sampled over time.  Removing
time as an axis of variation allows metadata to be evaluated and stored more
efficiently than attribute values, and also means that metadata authored in
\ref Usd_Page_ValueClips "Value Clips" will be ignored by USD.

\li *Core metadata resolution rules vary.* 
\ref Usd_ValueResolution "Attribute Value Resolution" is fixed for all
attributes, core and custom.  Certain of the "core metadata", however, have
value resolution behavior other than "strongest opinion wins."  A class of
metadata used to specify composition behavior (as well as relationships) subscribe
to \ref Usd_OM_ListOps "list-editing composition".  Other metadata, like
attribute \em typeName, adhere to "weakest opinion wins".  The prim metadata
\em specifier has a highly specialized resolution behavior owing to the way
in which overs, defs, and classes combine.  *Value resolution behavior may
not be changed for extension metadata,* however - all extension metadata will
be resolved based on its datatype: strongest wins for primitive datatypes,
element-wise strongest wins for dictionaries.

## Schema and Prim Definition Registry {#Usd_OM_SchemaDefinitionRegistry}

The singleton class UsdSchemaRegistry exists to provide access to all available 
schemas. It queries plugins to find all registered schema types and generates
prim definitions from the processed generatedSchema.usda files
(generated when a schema.usda file is processed by \em usdGenSchema).

A prim definition, provided by the UsdPrimDefinition class, is an encapsulation 
of the built-in data that is imparted on a prim by the schemas for the prim's 
complete type signature. The built-in data accessible from a prim definition 
includes the list of built-in properties, an SdfSpec defining each property, 
the list of built-in metadata fields, and fallback values for attributes and 
metadata fields.

The schema registry creates and provides access to prim definitions for each 
individual "IsA" and applied API schema. It also provides API to build a 
composite prim definition for a combination of an "IsA" type with a list of 
applied API schemas. The prim definition that a prim uses is determined by the 
combination of its type name and the list of any applied API schemas applied to 
the prim and is generally what we are referring to when we talk about a 
UsdPrim's "prim definition".

## Fallback Prim Types {#Usd_OM_FallbackPrimTypes}

When you create a new "IsA" schema to use as a prim type, there may be an 
expectation that stages containing prims of your new type will be opened using 
a version of USD that does not have the new schema. You may want to provide 
these other, typically older, versions of USD with one or more reasonable 
alternative prim types to use instead of your type when its schema is not 
available. We provide the following mechanism for this.

You can specify an array of \em fallbackTypes tokens as 
\ref Usd_PerClassProperties "customData" for your class in the schema.usda. 
Schema generation will process this list of fallback types and add it to the 
dictionary of all prim fallback types that the 
\ref Usd_OM_SchemaDefinitionRegistry "schema registry" provides.

To provide the currently registered prim type fallbacks to a version of USD
that does not have some of these schemas, they must be recorded into any stages
that may want to be opened in one of these versions. At any point before saving 
or exporting a stage, this can be done by calling the function 
UsdStage::WriteFallbackPrimTypes to write the schema registry's dictionary of 
fallback prim types to the stage's root layer metadata.

When a stage is opened and a prim with an unrecognized type name is 
encountered, the stage's fallback prim types metadata is consulted. If the
unrecognized type has a fallback types list in the metadata, all prims with
the unrecognized type name will be treated as having the effective schema type
of the first recognized type in the list.

\sa UsdPrim::GetPrimTypeInfo, UsdPrim::IsA, UsdSchemaRegistry::GetFallbackPrimTypes

## Composition Operator Interfaces: UsdReferences, UsdInherits, UsdVariantSets {#Usd_OM_OtherObjects}

### ListOps and List Editing {#Usd_OM_ListOps}
