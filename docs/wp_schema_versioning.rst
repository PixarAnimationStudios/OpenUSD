========================
Schema Versioning in USD
========================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

Copyright |copy| 2022, Pixar Animation Studios,  *version 1.5*

.. contents:: :local:

############
Introduction
############

Pixar has been executing changes to schemas in the USD project nearly since
the project's inception, as our understanding of and requirements for core
schemas has evolved.  However, these changes have been developed and deployed
with the "contained ecosystem" perspective that we can afford as an animation
studio, namely, that:

#. New versions of the software containing the changes will be available to
   all consumers of affected assets
#. Any assets that are considered "active" can relatively easily be
   regenerated using the new version of the software 

These assumptions, combined with bespoke backwards-compatibility code for
consuming "old style" data until all necessary assets have been regenerated,
have allowed us to make substantive changes without altering schema identity,
i.e. without explicitly versioning any schemas.  However, as USD has grown
and been adopted in numerous industries and consumer products, these
assumptions obviously cannot hold.  Further, a key consideration for some USD
adopters is the ability to write future software that can robustly consume
USD assets authored ten or more years ago.  For these reasons, we must now
provide a means of encoding **in USD documents**, the schema versions used to
encode data.



**************************************
Challenges to Schema Versioning in USD
**************************************

There are (at least) four challenges to providing robust and useful
versioning support in USD itself, due to USD's inherent characteristics and
requirements.

Composition Makes Prim Version a Difficult Query
================================================

Due to the nature of composition, scene description authored at different
times, in different packages, in different USD variants can all be swirled
together to create a single "composed" UsdPrim.  Even if we were to record
the schema version used to create **every primSpec in every layer**, including
overs, what could we possibly do if versions in different collaborating
primSpecs disagreed?  It will likely be necessary to compromise and assume
each UsdPrim to have a single version, computed through value resolution;
this will, however, introduce complexities for possible "schema upgrading"
support, mentioned below.

Multi-level Authoring API's May Complicate Versioned Scene Description
======================================================================

Imagine that we address the previous challenge in the most straightforward
way, by associating a version tag (e.g. metadata) with "the strongest
defining primSpec" for typed schemas.  The generated
:cpp:`SchemaClass::Define()` methods provide an opportune and logical place
to add a version tag immediately upon defining the prim.  But lower-level
methods exist and are used for defining prims, such as
:usdcpp:`UsdStage::DefinePrim`, as well as the much lower-level
:usdcpp:`SdfPrimSpec::New`, and they know nothing of schemas or versions,
so anyone using these lower-level API's must remember to record schema
version when defining a prim, which seems brittle.

Prims Can Possess Numerous API Schemas
======================================

Any API schema applied to a prim may also be versioned over time, which means
that the straightforward "version tag" approach is insufficient: versions
must either be stored as a dictionary on each prim, or be turned into a
per-schema attribute, adding greater weight and cost.

Impact of Versioning on USD Speed and Scalability
=================================================

One of the things we value most highly about USD is the speed with which we
can open a Stage on a very large scene and be ready to start reading data out
of it in multiple threads.  A UsdStage intentionally limits the number of
pieces of data it needs to read in order to "compose" a UsdPrim while opening
a Stage, or loading or mutating parts of the stage, because each piece of
data must be independently resolved through the composed prim's full "prim
index". Adding a new metadatum (e.g. "schemaVersion") to be read on each of
potentially millions of composed prims may cause a noticeable performance
regression, but would be critical to providing support for versioning in the
UsdSchemaRegistry (i.e. so that the :usdcpp:`UsdPrimTypeInfo` and
:usdcpp:`UsdPrimDefinition` that the registry creates are version-specific).


##################################
Proposal for Per-Schema Versioning
##################################

This covers per-schema versioning which allows us to create a new version of
an individual Typed or applied API schema while continuing to support scene
description that has been created using a prior version of that schema. This
is the baseline of what we expect to cover to support versioning. In section
`Possible Code Generation Changes to Support Versioning <#schemaversioning-codegen-support>`_, we describe additional steps we can take to make client
interaction with versioned schemas easier.

*********************************
Version Representation in Schemas
*********************************

The schema version will be embedded in the schema identifier itself as a suffix, e.g. “SphereLight_2” is version 2 of the SphereLight schema.

- This matches the way we express versions in Sdr shader definitions already.
- Only major versions will be supported for now. Support for minor versions
  could always be added in the future if there is a compelling reason for it.
- Version 0 of a schema will be represented without a version suffix. For
  example “SphereLight” is version 0 of SphereLight; “SphereLight_0” will not
  be allowed as a schema identifier. All current existing schemas will
  automatically be their own version 0.

We considered encoding a schema’s version as a separate attribute or metadata
field on a prim instead of embedding it in the prim’s type name.  As
discussed above, there are many disadvantages to this that made us decide
against that approach; in further detail:

- The separate attribute/metadata value would need to be resolved separately
  from the type name in order to get the full identifier of the schema which
  has negative performance implications over just being able to resolve the
  type name. 
- The separate version would be subject to unintended consequences from
  composition in that overs could change the version without knowing what
  schema type they apply to.
- Applied API schemas are authored as a listOp of schema identifier tokens. A
  representation of versions for each API schema would be necessary if the
  version is not embedded in the schema identifier and is also prone to
  composition causing issues.

Schema Types with Versioned Schema Names
========================================

We will use the following nomenclature to describe the different types and
type names associated with a schema type.

- **Schema Identifier** - This is the name of the schema as it appears in the
  schema.usda that defines the schema and is what a prim’s type name must be
  set to in order to associate it with that schema (for Typed schemas; for
  API schemas this is the name added to the apiSchemas metadata to apply the
  API to the prim). The schema identifier is where the schema version is
  embedded and will be unique per schema type and version.
- **Schema Type** - This is the :usdcpp:`TfType` that represents each schema
  definition. Every version of a schema will have its own unique TfType,
  e.g. if the usdLux schema defined “SphereLight”, “SphereLight_1”, and
  “SphereLight_2”, they would have the schema types of UsdLuxSphereLight,
  UsdLuxSphereLight_1, and UsdLuxSphereLight_2 respectively. 
- **Schema Family** - This is the name that represents all versions of what
  would be considered the same schema. This will be the same as the schema
  identifier of version 0 of the schema, i.e. the schema name with no version
  suffix. So with the SphereLight example above, “SphereLight”,
  “SphereLight_1”, and “SphereLight_2” would all have the schema family of
  “SphereLight”.  But, moving forward, GUI that communicates high-level "prim
  type information" to users may want to query for schema family rather than
  schema identifier (currently returned by UsdPrim::GetTypeName() et al). 


The TfTypes for schema definitions will be generated exactly as they are
now. The UsdSchemaRegistry will be on the hook for parsing the schema family
and versions from the schema types and identifiers to provide the API for
reasoning about the versions of schema types. 

C++ Schema Classes
==================

Just as we generate a separate TfType for each version of a schema, we also
will generate a separate C++ class for each schema version (that isn’t
codeless). In the baseline implementation, these schema classes are not
interrelated, i.e. they’re generated as if they are just separate
schemas. `Below <#schemaversioning-codegen-support>`_, we explain the options
we’ve considered for a cohesive API among versioned schema classes of the
same schema family. 

Schema Inheritance
==================

Since schema inheritance is specified via schema identifier and versions are
embedded into the schema identifier, each version of a schema definition
inherits from a specific version of the base schema definition it inherits
from. Thus if a base schema class is upgraded to a new version, all schemas
that inherit from it must be upgraded to a new version that inherits from the
base schema’s new version. Depending on the base schema, adding a new version
could require new versions of dozens if not hundreds of other schemas. Thus,
**tremendous care must be taken when deciding whether a base schema should be
versioned**. 

We also will not allow a Typed schema to inherit from another version of 
the schema from the same family. For example, if we were to add a new version
of the Sphere schema as "Sphere_1", Sphere_1 would *not* be allowed to inherit
the Sphere schema itself. This is to prevent the situation of a prim with 
TypeName "Sphere_1" being both a Sphere and Sphere_1 and therefore version 0
and version 1 at the same time. This inheritance restriction will be enforced
by usdGenSchema.

Explicit Built-in API Schemas (not auto-apply)
==============================================

As with schema inheritance, built-in API schemas are included in other
schemas by schema identifier (e.g. as `LightAPI is a built-in of MeshLightAPI
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/pxr/usd/usdLux/schema.usda#L223-L235>`_)
so they will always be built-in with a specific version. If an API schema
that is included as built-in API schema for one or more other schemas is
upgraded to a new version, then all of those schemas that include the API
schema as built-in would need to be updated to include the new version,
likely by creating new versions of each of the dependent schemas. In this
way, adding a new version of an API schema used as a built-in API schema can
have the same potential breadth of impact as versioning an inherited base
schema type and the same care must be taken when deciding whether these API
schemas should be versioned. 


API Schema Version Conflicts
============================

Any given prim can possess exactly one version of a Typed schema, since a
composed USD prim has a single TypeName (with the schema inheritance 
restriction we've already mentioned).  However, a prim can have many API schemas
applied, and therefore can end up with more than one version *of the same API
schema family* applied to it in scene description. Precisely because a prim
cannot be made to be two versions of a Typed schema at the same time, a
composed prim definition will not be allowed to contain multiple versions of
the same applied API schema family. This restriction applies regardless of
how the API schemas are applied, be it any combination of authored API
schemas on prim or built-in/auto-apply API schemas already included for prim
definitions registered in the schema registry. 

We will prevent the inclusion of multiple versions of an API schema at
authoring time wherever possible (like during schema generation and the
ApplyAPI methods). However, it’s impossible to prevent all occurrences this
way (e.g. when the apiSchemas metadata is authored manually, composed from
separately-authored layers, or not all schemas have been fully updated after
an API schema version bump). So, in the cases where we do encounter more than
one version of an applied API schema in a prim definition, the first version
of the API schema encountered (i.e. the strongest API schema) will win and
all other versions of that same API schema family will be ignored by the
UsdPrimDefinition. 

In the case where the inclusion of an API schema would cause a version 
conflict and the API is included because it is a built-in of another API schema,
the whole of the API schema that includes it is considered a version conflict 
and is rejected from the prim definition. For example, the existing LightAPI in
UsdLux includes the built-in API schemas CollectionAPI:lightLink and 
CollectionAPI:shadowLink. Let's say that we've also added a new version of 
CollectionAPI, namely CollectionAPI_1. If a prim, had version 1 of CollectionAPI,
"CollectionAPI_1:lightLink" applied, and then LightAPI applied (in that
strength order), **LightAPI and both of its included schemas, CollectionAPI:lightLink
and CollectionAPI:shadowLink would be rejected** because its included 
CollectionAPI:lightLink would cause a version conflict.

Although not addressed in detail here, we do note that allowing multiple
versions of an API schema to be applied to the same prim might facilitate
interchange between DCCs that are running different versions of USD. We
believe that a preferable approach with fewer side-effects for accomplishing
this same goal might be to extend the existing feature of `fallback prim types
<https://openusd.org/release/api/_usd__page__object_model.html#Usd_OM_FallbackPrimTypes>`_
to apply to authored API schemas (currently the feature is only defined for
concrete prim type names) and to automatically determine fallback types for
schema versions that are authored but don’t exist in the USD version. But
this is out of scope for this proposal. 

Multiple-Apply API Schemas
==========================

Multiple apply API schemas, like :usdcpp:`UsdCollectionAPI`, which provide a
“schema template” that can be applied multiple times with different instance
names will also be versionable in the same way as single apply API
schemas. Similarly to single apply API schemas, we will prevent the inclusion
of multiple versions of the same multiple apply API schema family **for the
same instance name**, however different instance names can use different
versions of the multiple apply schema. 

So, for example, if we add a new version "CollectionAPI_1" of the
“CollectionAPI” schema family, a prim could define its applied API schemas
as: 

.. code-block:: usda

     apiSchemas = [“CollectionAPI_1:foo”, “CollectionAPI:bar”]

and both API schemas would be applied with the “foo” instance using version 1
and the “bar” instance as version 0. However, if the API schemas were: 

.. code-block:: usda

     apiSchemas = [“CollectionAPI_1:foo”, “CollectionAPI:foo”]

then only version 1 of the schema would be applied with the “foo” instance
since we can’t have the two versions of a schema using the same instance name
and as stated above we will just choose the strongest one. 


***************
Schema Registry
***************

Since we will have a unique TfType for each version of schema definition, the
:usdcpp:`UsdSchemaRegistry` is already prepared to handle the mapping of
versioned schema type to versioned schema definition without any
changes. However, the schema registry will be tasked with providing the API
for reasoning about the family of schemas and its version.

We'll provide a new structure for holding all the relevant information about 
each schema type:

.. code-block:: cpp

   struct SchemaInfo {
       TfType type;
       TfToken identifier;
       TfToken family;
       UsdSchemaVersion version;
       UsdSchemaKind kind;
   };

We'll provide API for getting the :cpp:`SchemaInfo` from type, identifier, 
family and version, or templated C++ schema class:

- :cpp:`const SchemaInfo *FindSchemaInfo(TfType schemaType)`
- :cpp:`const SchemaInfo *FindSchemaInfo(TfToken schemaIdentifier)`
- :cpp:`const SchemaInfo *FindSchemaInfo(TfToken schemaFamily, UsdSchemaVersion version)`
- :cpp:`template <class SchemaType> const SchemaInfo *FindSchemaInfo()`

We'll also provide API for getting the schema info for all or a subset of all 
versions of a schemas in a schema family:

- :cpp:`const std::vector<const SchemaInfo *> &FindSchemaInfosInFamily(TfToken schemaFamily)`
- :cpp:`std::vector<const SchemaInfo *> FindSchemaInfosInFamily(TfToken schemaFamily, UsdSchemaVersion version, VersionPolicy versionPolicy)`

The latter of these two functions takes the additional - :cpp:`VersionPolicy` 
enum parameter which, along with the provided version, specifies which subset of
the schema family's versioned schemas should be returned. The VersionPolicy enum
options will be:

- :cpp:`All` - Return all schemas in the schema family regardless of version.
- :cpp:`GreaterThan` - Return any schema in the schema family whose version is
  strictly greater than the input schema version.
- :cpp:`GreaterThanOrEqual` - Return any schema in the schema family whose
  version is the same or greater than the input schema version.
- :cpp:`LessThan` - Return any schema in the schema family whose version is
  strictly less than the input schema version.
- :cpp:`LessThanOrEqual` - Return any schema in the schema family whose version
  is the same or less than the input schema version.

**************************
UsdPrim Schema-related API
**************************

Individual UsdPrims will still have a specific type name and list of applied
API schema names that are bundled together in the UsdPrimTypeInfo for the
prim. Since type names and API schema names will have the schema version
embedded, the resulting UsdPrimTypeInfo will automatically have the version
information about the schemas that make up the prim’s type. 

UsdPrim has :cpp:`IsA` methods for querying about what Typed schema type it
is and :cpp:`HasAPI` methods for querying whether a particular API schema is
applied. All of these methods either take a TfType parameter representing the
schema type or are templated on the C++ schema class type. By default, these
functions will continue to work as they do today, returning true only if they
find a match for the given schema type. In other words, they will not
automatically be aware of how schema types may be versions within a schema
family. 

However, additional related functions, namely :cpp:`IsInFamily`
and :cpp:`HasAPIInFamily`, will be added that are schema family and version
aware. These two functions will take the same :cpp:`VersionPolicy` enum 
parameter that will be added for the schema registry in order to specify
which schema types to check for based on the schema family and version of the
schema type provided.

For example, if the “Sphere” schema has 3 versions, “Sphere” “Sphere_1” and
“Sphere_2”, with the TfTypes :cpp:`UsdGeomSphere`, :cpp:`UsdGeomSphere_1`,
and :cpp:`UsdGeomSphere_2` respectively then we can have the following
:cpp:`IsA`/:cpp:`IsInFamily` queries: 

- :cpp:`IsA<UsdGeomSphere_1>()` returns true for “Sphere_1” only
- :cpp:`IsInFamily<UsdGeomSphere_1>(All)` returns true for “Sphere”,
  “Sphere_1”, and “Sphere_2” 
- :cpp:`IsInFamily<UsdGeomSphere_1>(GreaterThanOrEqual)` returns true for
  “Sphere_1” and “Sphere_2” 
- :cpp:`IsInFamily<UsdGeomSphere_1>(LessThan)` returns true for “Sphere”
  only

A similar example can be created for an API schema and 
:cpp:`HasAPI`/:cpp:`HasAPIInFamily`, e.g if UsdGeom’s “VisibilityAPI” has
“VisilbiltyAPI_1” and “VisibilityAPI_2” we’d similarly be able to have the
following queries: 

- :cpp:`HasAPI<UsdGeomVisibilityAPI_1>()` returns true for “VisibilityAPI_1”
  only
- :cpp:`HasAPIInFamily<UsdGeomVisibilityAPI_1>(All)` returns true for
  “VisibilityAPI”, “VisibilityAPI_1”, and “VisibilityAPI_2” 
- :cpp:`HasAPIInFamily<UsdGeomVisibilityAPI_1>(GreaterThanOrEqual)` 
  returns true for “VisibilityAPI_1” and “VisibilityAPI_2” 
- :cpp:`HasAPIInFamily<UsdGeomVisibilityAPI_1>(LessThan)` returns true for
  “VisibilityAPI” only

Additionally, since API schemas may be multiple apply, which therefore must
be applied with an instance name, we’ll have the following semantics for
:cpp:`HasAPI`/:cpp:`HasAPIInFamily` with multiple apply schema types. If
for example “CollectionAPI” has versioned schemas “CollectionAPI_1” and
“CollectionAPI_2”: 

- :cpp:`HasAPI<UsdCollectionAPI_1>()` returns true for any instance of
  “CollectionAPI_1” but not instances of “CollectionAPI” or “CollectionAPI_2” 
- :cpp:`HasAPI<UsdCollectionAPI_1>(“foo”)` returns true for only the
  specific API schema instance “Collection_1:foo” 
- :cpp:`HasAPIInFamily<UsdCollectionAPI_1>(All)` returns true for any instance
  of any of “CollectionAPI”, “CollectionAPI_1”, and “CollectionAPI_2” 
- :cpp:`HasAPIInFamily<UsdCollectionAPI_1>(“foo”, All)` returns true for only
  “foo” instances of any version of “CollectionAPI”, so “CollectionAPI:foo”,
  “CollectionAPI_1:foo”, and “CollectionAPI_2:foo” only 

While the existing methods on UsdPrim for querying a prim’s schema
associations through a TfType or C++ class could technically be sufficient
after providing the overloads proposed above, it would be useful to be able
to call these same methods providing a schema family and version or just a
schema identifier token. So we propose adding additional overloads for 
:cpp:`IsA` and :cpp:`HasAPI` to do so: 

- :cpp:`IsA(TfToken schemaIdentifer)`
- :cpp:`IsA(TfToken schemaFamily, UsdSchemaVersion version)`
- :cpp:`HasAPI(TfToken schemaIdentifer)` 
- :cpp:`HasAPI(TfToken schemaFamily, UsdSchemaVersion version)`
- :cpp:`HasAPI(TfToken schemaIdentifer, TfToken instanceName)`
- :cpp:`HasAPI(TfToken schemaFamily, UsdSchemaVersion version, TfToken instanceName)`

And as logically follows, these similar overloads for :cpp:`IsInFamily` and 
:cpp:`HasAPIInFamily`

- :cpp:`IsInFamily(TfToken schemaIdentifer, VersionPolicy versionPolicy)`
- :cpp:`IsInFamily(TfToken schemaFamily)`
- :cpp:`IsInFamily(TfToken schemaFamily, UsdSchemaVersion version, VersionPolicy versionPolicy)`
- :cpp:`HasAPIInFamily(TfToken schemaIdentifer, VersionPolicy versionPolicy)` 
- :cpp:`HasAPIInFamily(TfToken schemaFamily)`
- :cpp:`HasAPIInFamily(TfToken schemaFamily, UsdSchemaVersion version, VersionPolicy versionPolicy)`
- :cpp:`HasAPIInFamily(TfToken schemaIdentifer, VersionPolicy versionPolicy, TfToken instanceName)`
- :cpp:`HasAPIInFamily(TfToken schemaFamily, TfToken  instanceName)`
- :cpp:`HasAPIInFamily(TfToken schemaFamily, UsdSchemaVersion version, VersionPolicy versionPolicy, TfToken  instanceName)`

Example queries with schema identifier:

- :cpp:`IsA(“Sphere”)` returns true for “Sphere” only
- :cpp:`IsA(“Sphere_1”)` returns true for “Sphere_1” only
- :cpp:`IsInFamily(“Sphere_1”, All)` returns true for “Sphere”, “Sphere_1”, 
  and “Sphere_2”

Example queries with family and version:

- :cpp:`IsA(“Sphere”, 0)` returns true for “Sphere” only
- :cpp:`IsA(“Sphere”, 1)` returns true for “Sphere_1” only
- :cpp:`IsInFamily(“Sphere”, 1, GreaterThanOrEqual)` returns true for 
  “Sphere_1” and “Sphere_2” 

We expect that there will be use cases where it is useful to check if a prim
is a schema or has an API schema of a certain family and get the particular
version of the compatible schema directly. Instead of overloading IsInFamily
and HasAPIInFamily further, we propose adding the following new functions to
UsdPrim to get the version if a compatible schema is present on a prim: 

- :cpp:`bool GetVersionIfIsInFamily(TfToken schemaFamily, UsdSchemaVersion *version)`
- :cpp:`bool GetVersionIfHasAPIInFamily(TfToken schemaFamily, UsdSchemaVersion *version)`
- :cpp:`bool GetVersionIfHasAPIInFamily(TfToken schemaFamily, TfToken instanceName, UsdSchemaVersion *version)`

These functions are the equivalent of calling IsInFamily or 
HasAPIInFamily with a schema family name and versionPolicy = All with the
addition of parsing and outputting the version from the prim's compatible schema
type if the function would return true. 

UsdPrim also has the ApplyAPI, CanApplyAPI, and RemoveAPI methods for editing
the ordered set of API schemas applied to a prim. Like HasAPI, these methods
also either take a TfType parameter representing the schema type, or are
templated on the C++ schema class type. So, like HasAPI, these will have
additional overloads introduced that take a schema identifier and take a
schema family and version:

- :cpp:`ApplyAPI(TfToken schemaIdentifer)`
- :cpp:`ApplyAPI(TfToken schemaFamily, UsdVersion version)`
- :cpp:`ApplyAPI(TfToken schemaIdentifer, TfToken &instanceName)`
- :cpp:`ApplyAPI(TfToken schemaFamily, UsdSchemaVersion version, TfToken instanceName)`

- :cpp:`CanApplyAPI(TfToken schemaIdentifer, std::string *whyNot = nullptr)`
- :cpp:`CanApplyAPI(TfToken schemaFamily, UsdVersion version, std::string *whyNot = nullptr)`
- :cpp:`CanApplyAPI(TfToken schemaIdentifer, TfToken &instanceName, std::string *whyNot = nullptr)`
- :cpp:`CanApplyAPI(TfToken schemaFamily, UsdSchemaVersion version, TfToken instanceName, std::string *whyNot = nullptr)`

- :cpp:`RemoveAPI(TfToken schemaIdentifer)`
- :cpp:`RemoveAPI(TfToken schemaFamily, UsdVersion version)`
- :cpp:`RemoveAPI(TfToken schemaIdentifer, TfToken &instanceName)`
- :cpp:`RemoveAPI(TfToken schemaFamily, UsdSchemaVersion version, TfToken instanceName)`


In addition to the new overloads, there are additional version-aware updates
that could be made to CanApplyAPI and RemoveAPI that seem sensible, but we do 
not plan, at this time, to implement them. A possible update to CanApplyAPI
would be to check for an existing applied schema of the same family but
different version in the prim’s current applied API schemas, and return false
if one is found (see `API Schema Version Conflicts <#api-schema-version-conflicts>`_). 
A possible update to RemoveAPI would be to provide an overload that allows the same
enum value as HasAPIInFamily in order to remove any or all versions in a schema
family or for all versions greater than or less than a version within the schema
family. While these changes could be useful, we are deferring consideration
of these particular changes to maintain the current simplicity of these
functions, as they do not even consider the prim’s fully composed API schemas
list in their current implementation.

*****************************************
Considerations for Auto-apply API Schemas
*****************************************

In this section, we call out the *possible* full support we could provide
for auto-apply API schema versioning. However, we only plan to implement the
necessities right now, outlined in *Case 2* below. I.e., the support we will
provide for auto-apply API schemas is that we will ensure that if more than one
version of an auto-applied API schema are applied to the same schema, the 
**latest version** of the auto-applied API schema will always be the version applied.

The full set of behaviors we could have for supporting versioning in auto-apply
API schemas are best explained through an example. Right now, we have an API schema
called PxrMeshLightAPI, which exists in the USD RenderMan plugin, to extend
RenderMan properties to MeshLightAPI and VolumeLightAPI in usdLux. Thus
`PxrMeshLightAPI is set to auto-apply to MeshLightAPI and VolumeLightAPI
<https://github.com/PixarAnimationStudios/OpenUSD/blob/v22.05/third_party/renderman-24/plugin/usdRiPxr/schema.usda#L1649-L1651>`_
and becomes a built-in API of those schemas at runtime. Here, we run this
schema, PxrMeshLIghtAPI, through a hypothetical versioning scenario to
explain the behaviors we need to support. 

**Case 1:** MeshLightAPI is version-upgraded to create MeshLightAPI_1.

In this case, we still want PxrMeshLightAPI to auto-apply to both versions of
MeshLightAPI, but we don’t want to have to update PxrMeshLightAPI every time
a schema it auto-applies to is versioned just to keep it from falling
off. Thus, schemas that are set up to auto-apply to another schema will
auto-apply to ALL schemas of the indicated version or later. 

**Case 2:** After upgrading MeshLightAPI to MeshLightAPI_1, we find that
PxrMeshLightAPI needs to be upgraded to PxrMeshLightAPI_1 to correctly extend
MeshLightAPI_1. 

In this case we create a new version of the auto-apply schema,
PxrMeshLightAPI_1, and set it to auto-apply to MeshLightAPI_1. But because of
the rule established in case 1, the old PxrMeshLightAPI would still
auto-apply to both MeshLightAPI and MeshLightAPI_1, resulting in
MeshLightAPI_1 having both versions, PxrMeshLightAPI_1 and PxrMeshLightAPI,
applied! To prevent this conflict, which was clearly not the
schema-maintainer’s intent, we use another rule: if more than one version of
the same schema tries to auto-apply to the same schema, **only the latest
version of the auto-applied schema is applied.** Following this rule, we’d
end up with PxrMeshLightAPI applying to MeshLightAPI and PxrMeshLightAPI_1
applying to MeshLightAPI_1. 

Note that this case's rule would be enforced automatically by the rules laid
out in the API Schema Conflicts section if the latest version of the auto
apply schema were to be considered as applied first relative to its other
versions. 

**Case 3:** VolumeLightAPI is now version upgraded to VolumeLightAPI_1, and
starting with VolumeLightAPI_1, it no longer makes sense for any version of
PxrMeshLightAPI to auto-apply to it (maybe instead we want a new
PxrVolumeLightAPI to auto-apply to VolumeLightAPI_1). 

Though not specifically called out in case 2, PxrMeshLightAPI_1 is already
set to auto-apply only to MeshLightAPI_1 (VolumeLightAPI is not included),
but the exclusion of VolumeLightAPI doesn’t stop the original PxrMeshLightAPI
from continuing to auto-apply to VolumeLightAPI version 0 and later. While we
do want the version zero of PxrMeshLightAPI to continue to apply to the
version zero of VolumeLightAPI, we do not want it applying to the new
VolumeLightAPI_1. Thus, we need to introduce a mechanism for **specifying
auto-apply blocks** that we can use to indicate that no version of
PxrMeshLightAPI should apply to VolumeLIghtAPI_1 or any of its later
versions. These auto-apply blocks will be specifiable either directly in
PxrMeshLightAPI_1’s schema definition directly or through extra plugInfo in
the auto-apply schema’s library. 


#############################
Risks, Questions, Limitations
#############################

- There may be a performance cost to the IsA and HasAPI methods related to
  checking for any version of the schema type. 
- Should the new overloads of IsA, HasA, ApplyAPI described in `UsdPrim
  Schema-related API <#usdprim-schema-related-api>`_ use new function names
  instead of the existing TfType param functions for additional clarity?
- We have not discussed "schema upgrading", as we do not believe it is a
  problem amenable to automatically code-generated solutions.  We leave it an
  open issue for future consideration.
- Per-schema versioning cannot capture the nuance of **all** kinds of schema
  changes.  For example, changing an API schema from non-applied to applied
  (which we are doing for :usdcpp:`UsdShadeCoordSysAPI`) does not benefit
  from schema versioning, because there is no scene description describing
  the "previous version".  In other words, retaining backwards compatibility
  would mean **never** being able to reliably ask the question
  :cpp:`HasAPI<NewlyAppliedSchemaAPI>(All)`.  We believe that providing
  efficient backwards compatibility for such changes would require
  "domain-level versioning", where, for example, every layer would record the
  version(s) of each schema domain used to create scene description, and the
  stage would integrate and present those (min, max) per-domain versions,
  allowing custom code to resort to expensive queries (e.g. looking for
  particular or any authored properties of an API schema to answer
  :cpp:`HasAPI()`) only when dealing with assets known to be "old".  However,
  in addition to being difficult to maintain, this approach presents
  difficulties for efficient change-processing, therefore we choose, for now,
  to accept this limitation rather than propose domain-level versioning.
- There may be an additional coding cost for clients when adopting new
  versions of schemas while retaining support for older versions. While we
  have considered many approaches for attempting to alleviate this cost, by
  providing API in the generated C++ classes that could handle multiple
  versions of the same schema, all of these approaches brought up new
  workflow issues that led us to conclude that “doing nothing” (at least in
  the automated class generation sense) would be the best approach for now.
  We describe some of the options we considered in section `Possible Code
  Generation Changes to Support Versioning <#schemaversioning-codegen-support>`_


################################
Guidelines for Schema Versioning
################################

The primary purpose of schema versioning is to maintain behavioral
compatibility with existing assets. It is not meant to be used solely to
maintain compatibility with existing code,  although it is certainly a
secondary goal to maintain code compatibility wherever we can, after
versioning a schema. 

***********************
Criteria for Versioning
***********************

A schema should be versioned if the following criteria are met:

#. A prim that conforms to the current version of the schema would behave
   differently when consumed by any existing downstream components
   (rendering, import into other DCCs, etc.) after the schema is updated. 
#. Preserving the behavior of assets written to conform to the current
   version of the schema for downstream components is necessary after the
   schema is updated. 
#. The benefits of preserving compatibility with the current schema version
   outweighs the impact of adding/maintaining a new version of the schema. 

*******************************
Do not Version a Schema When...
*******************************

the change is not expected to cause behavioral changes to a schema and
therefore should not require versioning, such as: 

- Adding a new attribute to a schema with a fallback value that should have
  no effect. For example, if we were to add a “tipRadius” attribute to
  UsdGeom’s Cone with fallback value of 0. Without this attributes Cone’s
  already use an effective “tipRadius” of 0 so no existing Cones should
  change. 
- Removing an attribute that has never been used or is no longer supported by
  any downstream clients. 
- Adding, removing, or renaming an attribute that serves just as an
  info/documentation field, like a “comments” or a “notes” attribute. 
- Changing certain metadata on properties that are informational or
  organizational only such as displayName, displayGroup, doc, and hidden. 
- Updating a token attribute’s allowedTokens field to  add a new allowed token. 
- Adding/removing a built-in API in a way that would not immediately change
  behavior. For example adding ShadowAPI to DistantLight but also overriding
  inputs:shadow:enable to false in the DistantLight itself to keep shadows
  disabled by default. 
- Promoting a non-applied API schema into an applied schema, since, as
  described above, the non-applied schema had no scene description typeName,
  therefore we have no tools to reason about the old vs new type, and
  versioning is pointless.  

***************************
Do Version a Schema When...
***************************

- Adding a new attribute to a schema with a fallback value that could affect
  existing prims of that type. For example, if we were to add a “tipRadius”
  attribute to UsdGeom’s Cylinder with any fallback value. Existing Cylinders
  without this attribute could end up undesirably tapered if their radius
  attribute value doesn’t happen to match the “tipRadius” fallback. 
- Removing an attribute that is used by imaging for a prim, like if we were
  to remove the “size” attribute from UsdGeom’s Cube. 
- Renaming an attribute that is used by imaging for a prim, like if we were
  to rename the “radius” attribute in Sphere to “size”. 
- Changing the type of an existing attribute, for example, if we were to
  change Sphere’s “radius” attribute from a float type to a double type. We
  expect to know the type of an attribute’s value at compile time, so even
  though the attribute types seem compatible, they won’t be in most use
  cases. 
- Changing the fallback value for an attribute that would have an impact,
  like if we were to change the fallback for “size” in Cube from 2 to 1 
- Making a property metadata change that would have a behavioral
  effect. Metadata examples include “interpolation” or “connectability” 
- Making a change to a token attribute’s allowedTokens field other than
  adding a new allowed token, i.e. removing or renaming an allowed
  token. Removing a possibly used value from allowedTokens will typically
  indicate that that value should no longer be supported in downstream
  clients. 
- Adding or removing a built-in API that would immediately change the
  behavior of that prim type. For example adding ShadowAPI as a built-in to
  UsdLux’s DistantLight would now cause all existing DistantLights to cast
  shadows since shadows are enabled by default when ShadowAPI is included. 
- Changing the base-schema from which a schema derives; even if such a change
  were not to result in the addition or removal of any properties, it would
  change the results of IsA/HasAPI queries on the new version. 


###############################
Pixar Examples, Past and Future
###############################

*********************
UsdLux Connectability
*********************

The following is an example of a change we made that **could have benefitted
from schema versioning**, but we did not because of the effort required to
design and implement versioning, and the timeline we had for making the
changes to UsdLux. 

In a prior release, as part of an update to our UsdLux library, we changed
all existing concrete lights to be UsdShade connectable. This involved
converting all attributes on light schemas in UsdLux to have the “inputs:”
prefix to indicate their connectability, so “intensity” became
“inputs:intensity”, “exposure” became “inputs:exposure”, etc. We were able to
maintain some code support via schema generation by making sure that the C++
schema class accessors stayed the same, e.g. GetIntensityAttr is still the
generated function that returns “inputs:intensity” instead of
“intensity”. However, in UsdImaging we immediately updated the light adapters
to accept the new prefixed attributes, immediately cutting off support for
the old attributes and therefore light created with the original schema. 

This change would have benefitted from versioning as we could have deployed
different imaging adapters for the old and new lights keyed off the versioned
schema that would have allowed them to remain compatible with the old and new
attribute names. And while this change would have involved versioning a base
class, namely the subsequently-deprecated UsdLuxLight, it would have been
manageable since only around ten UsdLuxLight-derived lights existed, and the
required changes were fairly mechanical. However, even if we had already
possessed a schema versioning mechanism, we still may have decided not to
version the light schemas, as we had not been using the UsdLux schemas yet in
production and may have tried to avoid starting with all of our lights
already version bumped. 

*****************
Light → LightAPI
*****************

The following is an example of a complex change that **did not require schema
versioning.** 

In a prior release we made another large structural change to the UsdLux
light schemas, in which we replaced the UsdLuxLight base class (which held
properties that were shared by all lights) with the UsdLuxLightAPI API schema
which was instead built-in to all the light schemas. Structurally, this
change was complex and involved the following changes: 

- We moved all the properties and functionality of the UsdLuxLight base
  schema into the UsdLuxLightAPI schema. 
- We replaced the UsdLuxLight base class with two new base schema classes,
  BoundableLightBase and NonboundableLightBase, which both included
  UsdLuxLightAPI as a built-in. 
- Every existing concrete UsdLux light was changed to inherit from the new
  bases; DistantLight and DomeLight in particular were switched to inherit
  from NobBoundableLightBase which meant they would no longer inherit from
  Boundable or have an extent attribute. 
- Every concrete UsdLux light added the light:shaderId attribute, whose value
  now indicates the shader to associate with the light, replacing the prior
  mechanism of inferring the light’s shader ID from its typeName. 

Despite the complexity of this change, it had no meaningful impact on the
behavior of any existing light assets for the following reasons: 

- The existing property names and fallback values for the concrete lights
  remained unchanged after all of these schema changes were done. 
- Even though DistantLight and DomeLight were no longer Boundable after this
  change, there was no real impact as no client code was expecting these
  lights to be boundable or have a meaningful extent in the first place. 
- The fallback for the new light:shaderId attribute on each light was set to
  be the same as the light’s type name, meaning that the communicated ID for
  the light shader of existing lights was still the same after the update. 

This change, of course,  required downstream code updates, especially in
places that relied on the existence of UsdLuxLight, but as long as client
code was updated with the new version of the USD schema, light assets would
behave equivalently in the old and new version of the software. This asset
compatibility is why this complex structural schema change would not have
required any versioning of the light schemas. 

***************************
Visibility to VisibilityAPI
***************************

The following is an example of a schema change we intend to make in 2023 that
would benefit from schema versioning, but **we would still choose not to use
versioning because the impact would be too extensive.**

We are working on a proposal to move the visibility-related properties, such
as “visibility” and “purpose”, from UsdGeomImageable into the
UsdGeomVisibilityAPI instead. Visibility opinions would then be expressed by
**applying** the UsdGeomVisibilityAPI to a prim and then authoring the
visibility opinions. With this schema adjustment, we expect that client code
that cares about visibility would be able to use the application of
VisibiltyAPI as a requirement for a prim to participate in visibility
computations, potentially avoiding more expensive attribute value resolves on
the visibility attributes when VisibilityAPI is not applied; the change also
allows visibility-related opinions to be expressed on prims that are not
currently Imageable, such as "typeless def" prims and UsdGeomSubsets. However
we have a strong desire to maintain backward compatibility with existing
assets where Imageable prims may hold visibility opinions without
VisibilityAPI applied. 

If we move the visibility attributes out of UsdGeomImageable without
versioning, we can still maintain backward compatibility by checking for the
existence of the attributes on Imageable prims even if the VisibilityAPI is
not applied. However, this defeats the point of avoiding a costly attribute
lookup when the prim does not have a VisibiltyAPI. If we versioned
UsdGeomImageable to UsdGeomImageable_1 before moving the visibility
attributes, we would be able to discern from the prim’s type whether we have
a UsdGeomImageable_1 and should check for VisibilityAPI, or whether we have
an older UsdGeomImageable, in which case we must always resolve the
visibility attribute. 

However, the impact of versioning a base class like UsdGeomImageable is
**massive**. Roughly one hundred schemas in the primary repository directly
or indirectly inherit from UsdGeomImageable, so version bumping
UsdGeomImageable to UsdGeomImageable_1 requires that we version bump all one
hundred of those derived schemas so that they can inherit from the new
Imageable_1. This also would require anyone with Imageable-derived schemas of
their own to version bump those too if they want them to be compatible with
the new Imageable. 

The impact of versioning such a low-level base schema leads us to believe
that the negatives outweigh the benefits of versioning in this case, and that
making a one-time breaking change here will benefit us in the long
run. Therefore we do not expect to use versioning even though it would
otherwise be a strong versioning candidate.  Instead, we will provide
backwards-compatibility for old assets in related UsdGeom and UsdImaging
computations for a sunsetting period, and also provide a fixup tool that will
apply the VisibilityAPI wherever :cpp:`visibility` or :cpp:`purpose` opinions
are expressed. 


.. _schemaversioning-codegen-support:

######################################################
Possible Code Generation Changes to Support Versioning
######################################################

One of our key design goals is to leverage the C++ compiler to help catch
client-side oversights in adapting to new versions of schemas, for regular,
code-generated schemas.  The main problem with this approach is that applying
it straightforwardly, as we have proposed, means code changes are always
required for clients to adopt new versions of schema families if they
leverage the C++ classes for the schemas - even if the version-change would
not truly change the client's handling of the data.  We considered many
approaches for providing "version unified" and "aliased" C++ classes for
schema versions, but they brought up new workflow issues that led us to
conclude that “doing nothing” (at least in the automated class generation
sense) would be the best approach. Here are quick summaries of the approaches
we have considered, for reference and discussion. 

Note that, even given our design goal of leveraging the compiler, we
recognize that for schemas that version a number of times, we may wish to
reduce code bloat by suppressing code-generation for "old versions".  Thus,
we do plan to enable "codeless-ness" on a per-schema/version basis (as of
22.11 codeless-ness is only specifiable at the domain-level). 

************************************************************
Base Schema Class is Always the Latest Version of the Family
************************************************************

- **Example**: Given schema identifiers “Sphere”, “Sphere_1”, and “Sphere_2”,
  usdGenSchema would generate the UsdGeomSphere class from the latest
  version, “Sphere_2”.  Thus clients adopting a new software release
  automatically target the latest version in that release, with no code
  changes; however, if the generated code has changed, compiler errors will
  direct attention appropriately. 
- Classes for older versions could be generated as UsdGeomSphere_1 and
  UsdGeomSphere_0 if desired. 
- **Problems with this approach**:

  - This provides no way of locking to the current latest version in your
    code if the latest version changes down the line. 
  - Also has the problem that the registered type for the latest version of
    the schema changes when a new version is added in order to free up the
    class type again, and the type for the latest version does not (of
    course) match the identifier.
  - Of particular note, the "original" version zero of the family will never
    have a matching identifier and type once an additional version is added. 

*****************************************************************************
Class Per Version with Typedef Mapping to “Current” or “Latest” Version-Class
*****************************************************************************

- **Example**: Given schemas “Sphere”, “Sphere_1”, and “Sphere_2”,
  usdGenSchema would generate the UsdGeomSphere_0, UsdGeomSphere_1, and
  UsdGeomSphere_2 classes for each schema version. The header file
  usdGeom/sphere.h would provide a typedef setting :cpp:`UsdGeomSphere =
  UsdGeomSphere_2`.
- **Problems with this approach**:

  - Typedefing to UsdGeomSphere_Latest or UsdGeomSphere_Current (instead of
    UsdGeomSphere) so that version 0 can always use UsdGeomSphere... **or** 
  - Retyping **all unversioned schemas** (yes, all schemas in existence) to have
    the “_0” prefix and immediately providing the latest version typedef for
    these. 
  - Still has the problem that the type of version 0 will change (from
    UsdGeomSphere to UsdGeomSphere_0) on first versioning of the
    schema. There are potential ways to solve this that bring in their own
    issues: 
  - The equivalent of this typedef would need to be implemented in python as
    well. 
  - Allowing clients to override the typedef in order to lock UsdGeomSphere
    to a different version than the latest would introduce additional
    complexity. 

************************************************************************
Single C++ Class That Provides API for ALL Versions of the Schema Family
************************************************************************

- **Example**: Given schemas “Sphere”, “Sphere_1”, and “Sphere_2”,
  usdGenSchema would generate the UsdGeomSphere class providing a union of
  accessors for properties from all three versions of the schema.  The
  "unified schema class" would have direct methods for querying the prim's
  version. 
- **Problems with this approach**:

  - There may be incompatibilities between versions of the schema that cannot
    be provided in the same class (e.g. the schema’s base class has changed) 
  - By combining all versions into one class it is very hard to tell which
    schema versions the API is compatible with, if properties change between
    versions. The documentation would provide such enumeration, but that is
    insufficient to keep code up to date between version changes.  Thus, this
    approach **violates our stated key design goal**.  Which leads us to: 

*************************************************************
"Compatible Cluster" Classes with Disambiguating Method Names
*************************************************************

- **Example**: Given schemas “Sphere”, “Sphere_1”, and “Sphere_2”,
  usdGenSchema would generate the UsdGeomSphere_0, UsdGeomSphere_1, and
  UsdGeomSphere_2 classes for each schema version. UsdGeomSphere_0 would have
  API for version 0 only. UsdGeomSphere_1 would have API compatible with
  versions 0 and 1. UsdGeomSphere_2 would have API compatible with all three
  versions.  The point, here, is that we would likely **suppress code
  generation** for the older schema classes, since the newer one will be
  usable to interact with all versions. 
- We still allow for the potential of "incompatible changes" between
  versions, such as changing base-classes, and will therefore provide
  mechanisms by which the schema.usda can specify what the *compatible*
  versions of the family are for each new version. 
- Methods would be named in a way that would indicate which versions of the
  schema they are compatible with, e.g.  

  - :cpp:`GetXXXAttr` - all versions
  - :cpp:`GetV1AndLaterAttr` - version 1 and later
  - :cpp:`GetV1AndEarlierAttr` - version 1 and earlier
- **Problems with this approach**:

  - Requires a runtime query to determine which version you are working with,
    i.e. cannot be inferred from the code unless we use explicit version
    checks. 
  - Client code cannot automatically keep up with the latest version unless we
    add the typedef system on top of this (which has the issues mentioned
    above) 
