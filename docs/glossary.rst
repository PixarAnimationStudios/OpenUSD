======================
USD Terms and Concepts
======================

.. raw:: html

   <script>
      $(function() {
          // The old documentation site had anchors like #USDGlossary-APISchema,
          // but even if we specify explicit targets with the same name, Sphinx
          // will lower-case them in the generated HTML. So, for backwards
          // compatibility we try to transform any specified anchors.
          if (window.location.hash) {
              new_anchor = window.location.hash.toLowerCase().replace('/', '-')
              if ($(new_anchor).length) {
                  window.location.hash = new_anchor;
              }
          }
      });
   </script>

.. include:: rolesAndUtils.rst

.. include:: <isotech.txt>

USD introduces quite a few terms and concepts, some of which, unavoidably,
already have different meanings in other contexts, so it can be quite daunting
to make sense of all of our documentation and code until you have become fully
indoctrinated. This document attempts to define and explain all the major
concepts and behaviors referred to in other parts of the USD documentation.

.. contents::
   :local:
   :class: fourcolumn

----

.. _usdglossary-active-inactive:

Active / Inactive
*****************

*Activation* is a `metadatum <#usdglossary-metadata>`_/behavior of `prims
<#usdglossary-prim>`_ that models a "non destructive and reversible prim
deletion" from a `stage <#usdglossary-stage>`_ . Prims are active by default,
which means they and their active children will be `composed
<#usdglossary-composition>`_ and visited by `stage traversals
<#usdglossary-stagetraversal>`_. However, by making a prim *inactive* by calling
:usdcpp:`UsdPrim::SetActive(false) <UsdPrim::SetActive>`,
we prevent the prim itself from being visited by default traversals, and we
also prevent the prim's descendant prims from even being composed on the stage,
which makes *deactivation* a useful tool for pruning unneeded scene description
for scalability and complexity management.

In the following example, the prim at `path <#usdglossary-path>`_
:sdfpath:`/Parent` has been deactivated, so :sdfpath:`/Parent/Child1` and other
descendants will not be composed. However, :sdfpath:`/Parent`'s :usda:`active`
metadatum can be overridden to *true* in a stronger layer, which would cause
:sdfpath:`/Parent/Child1` and etc. to compose onto the stage.

.. code-block:: usda
   :caption: Example of a deactivated prim

   def Xform "Parent" (
       active = false
   )
   {
       def Mesh "Child1"
       {
       }
       # other siblings of "Child1" ...
   }

.. _usdglossary-apischema:

API Schema
**********

An API schema is a prim `Schema <#usdglossary-schema>`_ that serves as an
interface or *API* for authoring and extracting a set of related data, and *may*
also contribute to a `prim's definition <#usdglossary-primdefinition>`_. In
terms of the USD object model, an API schema is one that derives from
:usdcpp:`UsdAPISchemaBase`, but **not** from its subclass :usdcpp:`UsdTyped`.
That means :usdcpp:`UsdPrim::IsA<UsdModelAPI>() <UsdPrim::IsA>` will never
return true. In contrast to typed, "is a" schemas, think of API schemas as "has
a" schemas. There are three types of API schema: **non-applied, single-apply,
and multiple-apply.**

* **Non-applied API Schemas**

  Non-applied API schemas are, in some sense, the "weakest" API schemas, since
  **all** they provide is API for setting and fetching some related properties
  and/or metadata, and do not contribute to a prim's type or definition in any
  way. The :usdcpp:`UsdModelAPI` is an example of a non-applied schema whose
  purpose is to interact with a few pieces of prim metadata related to models
  and assets; there is no way to tell whether a :usdcpp:`UsdModelAPI` is
  *present* on a prim - instead one simply uses its methods to interrogate
  particular aspects.

  .. code-block:: python

     Usd.ModelAPI(prim).SetKind(Kind.Tokens.subcomponent)

  One might also create a non-applied schema to interact with some related (for
  your purposes) properties that exist on some number of other typed schemas,
  which may allow for more concise coding patterns.

* **Single and Multiple Apply Schemas**

  Applied schemas get recorded onto a prim in its :usda:`apiSchemas` metadata,
  which allows them to contribute to the prim's definition, adding builtin
  properties, and submitting to presence queries, using
  :cpp:`UsdPrim::HasAPI<UsdShadeMaterialBindingAPI>()`. The most common type of
  API schema is **single-apply,** which should be applied before using the
  schema's authoring APIs.  That is, whereas when we are authoring *typed* schemas
  we usually **assign the type** as we define the prim, before using the schema to
  author data, like:
  
  .. code-block:: python

     mesh = UsdGeom.Mesh.Define(stage, path)
     mesh.CreateSubdivisionSchemeAttr().Set(UsdGeom.Tokens.bilinear)

  for applied API schemas we instead **apply the type to an existing prim** before
  using the schema to author data, like:

  .. code-block:: python

     bindingAPI = UsdShade.MaterialBindingAPI.Apply(prim)
     bindingAPI.Bind(materialPrim)

  **Multiple-apply schemas** can be applied to a prim more than once, requiring an
  "instance name" to distinguish one application from another - the instance name
  will form part of the namespace in which the schema's properties will be
  authored. :usdcpp:`UsdCollectionAPI` is an example of a multiple-apply schema,
  which :usdcpp:`UsdShadeMaterialBindingAPI` uses to assign materials to
  *collections* of prims, allowing multiple collections to be located on a
  single prim.

Choose to create an applied API Schema when you have a group of related
properties, metadata, and possibly associated behaviors that may need to be
applied to **multiple different types of prims** . For example, if your
pipeline has a set of three attributes that get authored onto every `gprim
<#usdglossary-gprim>`_, and you want to have a robust schema for authoring and
extracting those attributes, you could create an applied API schema for
them. Why not instead subclass the `typed <#usdglossary-isaschema>`_
:usdcpp:`UsdGeomGprim` schema and add the attributes there? Because you would
then need to redefine all of the schema classes that derive from Gprim, thus
preventing you from taking advantage of built-in DCC support for the
:usdcpp:`UsdGeomGprim`-derived classes. API schemas provide for "mix in" data
organization.

API schemas can be generated using the `USD schema generation tools
<api/_usd__page__generating_schemas.html>`_, and extended with custom methods,
just as typed schemas can. We use schema generation even for non-applied API
schemas that have no builtin properties, to ensure consistency.

.. _usdglossary-assembly:

Assembly
********

In USD an assembly is a `kind <#usdglossary-kind>`_ of `Model
<#usdglossary-model>`_.  Assemblies are `group <#usdglossary-group>`_ models,
i.e. models that aggregate other models into meaningful collections. An assembly
may consist primarily of references to other models, and themselves be published
assets.

.. code-block:: usda
   :caption: An "assembly" asset, declared in scene description

   def Xform "Forest_set" (
       kind = "assembly"
   )
   {
       # Possibly deep namespace hierarchy of prims, with references to other assets
   }

**See also:**  `model hierarchy <#usdglossary-modelhierarchy>`_

.. _usdglossary-asset:

Asset
*****

*Asset* is a fairly common organizational concept in content-producing
pipelines. In the most generic terms in USD, an asset is something that can be
identified and located (via `asset resolution <#usdglossary-assetresolution>`_) 
with a string identifier. To facilitate operations such as asset dependency
analysis, USD defines a specialized string type, `asset
<api/_usd__page__datatypes.html>`__, so that
all metadata and attributes that refer to assets can be quickly and robustly
identified. In content pipelines, assets are sometimes a single file (e.g. a
UV texture), or a collection of files anchored by a single file that in turn
references others. A non-defining, but important quality of assets is that
they are generally published and version-controlled. As an aid to asset
management and analysis of composed scenes, USD provides an `AssetInfo
<#usdglossary-assetinfo>`_ schema. The text :filename:`.usda` format 
uses a special syntax for asset-valued strings to make them easily
differentiable from ordinary strings, using the "@" symbol instead of quotes to
delimit their values, or "@@@" as delimiter if the asset path itself contains
the "@" character. If an asset path must contain the string "@@@" then it
should be singly escaped, as "\\@@@". See `AssetInfo
<#usdglossary-assetinfo>`_ for examples of both forms.

.. _usdglossary-assetinfo:

AssetInfo
*********

*AssetInfo* is a metadata dictionary that can be applied to any `UsdPrim
<#usdglossary-prim>`_ or `UsdProperty <#usdglossary-property>`_ to convey
asset-identification-and-management-related information. Typically a `UsdStage
<#usdglossary-stage>`_ pulls in many assets through `composition arcs
<#usdglossary-compositionarcs>`_. When interacting with prims and properties
on the stage, however, the presence and location of the arcs and the identity
of the assets they target is, by design, fairly deeply hidden as an
implementation detail. We do provide low-level tools for examining the
arc-by-arc `index <#usdglossary-index>`_ for each prim, but it can be difficult
to reconstruct intent and asset identity from the arc structure
alone. **AssetInfo** provides a mechanism for identifying and locating assets
that survives composition (and even `stage flattening <#usdglossary-flatten>`_)
to allow clients to discover the `namespace location <#usdglossary-namespace>`_
at which an asset is introduced, and generally how to construct a reference to
the asset.

.. note:: 
   :usda:`assetInfo` authored in USD files is advisory data one supplies for
   client applications to use. **It is not consulted or consumed by the
   USD core in any way during Stage loading/composition.**

The :usdcpp:`AssetInfo dictionary <UsdObject::GetAssetInfo>` can contain any
fields a client or pipeline finds useful, but promotes four core fields, which
each have direct API:

    * **identifier** (:usda:`asset`)
      
      Provides the asset identifier one would use to target the asset with a
      composition arc.

    * **name** (:usda:`string`)

      Provides the name of the asset as it would be identified, for example, in
      an asset database query.

    * **payloadAssetDependencies** (:usda:`asset[]`)

      Provides what can be a substantial optimization for dynamic asset
      dependency analysis. If your asset build/publish process can pre-compute,
      at publish-time, the external asset dependencies contained inside the
      asset's payload(s), then shot/render-time dependency analysis (for asset
      isolation) need not load the payload.

    * **version** (:usda:`string`)

      Provides the version of the asset that was/would-be targeted. In some
      pipelines, it may make sense to inject an asset's revision control version
      into the published asset itself. In other pipelines, version is tracked in
      an external database, so the version assetInfo must be added in the
      referencing context when we add a reference to an asset.

A continuation of the example above that illustrates `assemblies
<#usdglossary-assembly>`_:

.. code-block:: usda
   :caption: AssetInfo on a published assembly asset

   def Xform "Forest_set" (
       assetInfo = {
           asset identifier = @Forest_set/usd/Forest_set.usd@
           string name = "Forest_set"
       }
       kind = "assembly"
   )
   {
       # Example of an asset that embeds the '@', and so must be delimited
       # with the "@@@" form
       asset primvars:texture = @@@body_decal.exr@v3@@@
   }

.. _usdglossary-assetresolution:

Asset Resolution
****************

*Asset resolution* is the process by which an `asset path <#usdglossary-asset>`_
is translated into the location of a consumable resource. USD provides a plugin
point for asset resolution, the :cpp:`ArResolver` interface, which clients can
implement to resolve assets discovered in a USD scene, using whatever logic and
external inputs they require. This plugin also lets clients provide USD with
an asset's data without fetching the data to disk.

If no asset resolver plugin is available, USD falls back to a default resolver
implementation that uses search paths to locate assets referenced via relative
paths. See the examples for `References <#usdglossary-references>`_.

.. _usdglossary-attribute:

Attribute
*********

*Attributes* are the most common type of `property <#usdglossary-property>`_
authored in most USD scenes. An attribute can take on exactly one of the legal
`attribute typeNames <api/_usd__page__datatypes.html>`_
USD provides, and can take on both a `default value <#usdglossary-defaultvalue>`_
**and** a value each at any number of `timeSamples <#usdglossary-timesample>`_.
`Resolving <#usdglossary-valueresolution>`_ an attribute at any given
`timeCode <#usdglossary-timecode>`_ will yield either a single value or no
value. Attributes resolve according to "strongest wins" rules, so all values
for any given attribute will be fetched from the strongest `PrimSpec
<#usdglossary-primspec>`_ that provides **either** a default value or
timeSamples. Note that this simple rule is somewhat more complicated in the
presence of authored `clips <#usdglossary-valueclips>`_. One interacts with
attributes through the :usdcpp:`UsdAttribute` API.

A simple example of an attribute that has both an authored default and two
timeSamples in the same primSpec:

.. code-block:: usda
   :caption: An attribute with both Default and TimeSamples

   def Sphere "BigBall" 
   {
       double radius = 100
       double radius.timeSamples = {
           1: 100,
           24: 500,
       }
   }   

.. _usdglossary-attributeblock:

Attribute Block
***************

Similarly to how `prims can be deactivated <#usdglossary-active-inactive>`_
through composing overriding opinions, the value that an attribute produces can
be **blocked** by an overriding opinion of :usda:`None`, which can be authored using
:usdcpp:`UsdAttribute::Block`.  A block itself can, of course be overridden by
an even stronger opinion. The following example extends the previous attribute
example, adding a :usda:`DefaultBall` prim that blocks the value of
:usda:`radius` it references from :usda:`BigBall`, causing :usda:`radius`'s
value to resolve back to `its fallback <#usdglossary-fallback>`_ at
:cpp:`UsdTimeCode` :mono:`t` (any blocked attribute that has no fallback will
report that it has no value when we invoke :usdcpp:`UsdAttribute::Get`):

.. code-block:: usda
   :caption: :usda:`DefaultBall` **Blocks** the radius values referenced from :usda:`BigBall`

   def Sphere "BigBall" 
   {
       double radius = 100
       double radius.timeSamples = {
           1: 100,
           24: 500,
       }
   }
   
   def "DefaultBall" (
       references = </BigBall>
   )
   {
       double radius = None
   }

In addition to completely blocking an attribute's value, sub time-ranges can be
separately blocked, by blocking individual time samples. Consider the following
examples:

**Example 1:**

.. code-block:: usda

   def Sphere "BigBall"
   { 
       double radius.timeSamples = { 
           101: 12, 
           102: None, 
       } 
   }

For the attribute :usda:`radius` on :usda:`BigBall`:

  * :python:`Usd.Attribute.Get(t)` will return 12 for :python:`Usd.TimeCode`
    :mono:`t` in (-|infin|, 102).

  * :python:`Usd.Attribute.Get(t)` will return :mono:`None` for
    :python:`Usd.TimeCode` :mono:`t` in [102, |infin|).

**Example 2:**

.. code-block:: usda

   def Sphere "BigBall"
   { 
       double radius.timeSamples = { 
           101: None, 
           102: 12, 
       } 
   }

For the attribute :usda:`radius` on :usda:`BigBall`:

  * :python:`Usd.Attribute.Get(t)` will return :mono:`None` for
    :python:`Usd.TimeCode` :mono:`t` in (-|infin|, 102).
 
  * :python:`Usd.Attribute.Get(t)` will return 12 for :python:`Usd.TimeCode`
    :mono:`t` in [102, |infin|).

Note that the per-timeSample-blocking ability does **not** allow us to sparsely
**override** timeSamples, i.e. in the following example:

.. code-block:: usda

   def Sphere "BigBall"
   { 
       double radius.timeSamples = { 
           101: 1, 
           102: 2, 
       } 
   } 
   
   def "DefaultBall" ( 
       references = </BigBall> 
   ) 
   { 
       double radius.timeSamples = { 
           101: None, 
       } 
   }

For the attribute :usda:`radius` on :usda:`DefaultBall`:

  * :python:`Usd.Attribute.Get(t)` will return :mono:`None` for
    :python:`Usd.TimeCode` :mono:`t` in (-|infin|, |infin|).

.. _usdglossary-attributeconnection:

Attribute Connection
********************

See `Connection <#usdglossary-connection>`_.

.. _usdglossary-attributevariability:

Attribute Variability
*********************

See `Variability <#usdglossary-variability>`_.

.. _usdglossary-changeprocessing:

Change Processing
*****************

*Change processing* is the action a :usdcpp:`UsdStage` takes in response to edits
of any of the layers that contribute to its composition. During change
processing, prims on the stage may be `re-indexed <#usdglossary-index>`_ or
disappear entirely, or new prims may come into being. The key things to
understand about change processing are:

#. It is always active. Whenever you use the :cpp:`Usd` or :cpp:`Sdf` APIs to
   mutate any layers that contribute to a :cpp:`UsdStage`'s composition, that
   stage will update itself immediately, in the same thread in which authoring
   was performed (though the change-processing itself may spawn worker threads),
   so that it always presents an accurate result to clients.

    ..

#. When a stage has completed a round of change processing, it will 
   :usdcpp:`send notification <UsdNotice>` to clients who have registered
   interest, so that they may keep themselves updated in light of authored
   changes to the stage.

   .. note::
      Clients listening to :usdcpp:`UsdStage` notifications should not update
      themselves immediately upon receiving a notice, but instead note what has
      become out-of-date (dirty), and defer updating until necessary; this helps
      minimize the amount of work an application needs to undertake when many
      edits are being performed rapidly.

    ..

#. For all of the layers that contribute to a given :usdcpp:`UsdStage`, **only 
   one thread at a time may be editing any of those layers, and no other thread
   can be reading from the stage while the editing and change processing are
   taking place.** This is because we do not want to weigh down read access to
   :usdcpp:`UsdStage` data with any kind of locking.

.. _usdglossary-class:

Class
*****

*Class* is one of the three possible `specifiers <#usdglossary-specifier>`_ a
`prim <#usdglossary-prim>`_ (and also a `primSpec <#usdglossary-primspec>`_) can
possess. A class prim and all of the prims nested inside it in `namespace
<#usdglossary-namespace>`_ will report that they are **abstract**, via
:usdcpp:`UsdPrim::IsAbstract`, which causes the prims to be skipped by stage and
child traversals, unless a client specifically asks to include abstract
prims. The most common use of classes is to create prims from which other prims
can `inherit <#usdglossary-inherits>`_. Classes can define/override metadata as
well as properties and nested prims. The following example contains a class
:usda:`_class_Ball` that provides a value for the radius attribute for any prim
that would `inherit <#usdglossary-inherits>`_ or `reference
<#usdglossary-references>`_ it. The :usda:`_class_` prefix is a Pixar convention
for naming classes, and is not a requirement.

.. code-block:: usda
   :caption: A "class" prim contains opinions meant to be inherited

   class "_class_Ball" {
       double radius = 50
   }

.. _usdglossary-clips:

Clips
*****

See `Value Clips <#usdglossary-valueclips>`_

.. _usdglossary-collection:

Collection
**********

Collections build on the ability of `relationships <#usdglossary-relationship>`_
to identify objects in a USD scene. Whereas a relationship is (resolves to, by
fetching its *Targets*) simply an ordered list of paths identifying other
objects in the scene, a Collection uses a pair of relationships and extra rules
to compactly encode potentially large sets of objects by identifying a set of
paths to hierarchically **include** in the Collection, and a separate set of
paths to hierarchically **exclude**. For example, the following Collection
identifies all of the prims comprising the two buildings, *except* for all the
prims organized under the :usda:`Floor13` prim in each.

.. code-block:: usda
   :caption: Collections refine a hierarchical set by including and excluding
             finer and finer branches

   over "World"
   {
       over "Buildings" (
           prepend apiSchemas = "CollectionAPI:luckyBuildings"
       )
       {
           uniform token collection:luckyBuildings:expansionRule = "expandPrims"
           rel collection:luckyBuildings:includes = [
               </World/Buildings/Skyscraper>,
               </World/Buildings/Pyramid>,
           ]
           rel collection:luckyBuildings:excludes = [
               </World/Buildings/Skyscraper/Floor13>,
               </World/Buildings/Pyramid/Floor13>,
           ]
       }
   }

We can see from the example that Collections in USD are expressed as "multiple
apply" `API Schemas <#usdglossary-apischema>`_, so that we can add as many
different collections as we want, to any prim already in the scene. The
:usda:`expansionRule` attribute specifies how the targets of the
:usda:`includes` relationship should be expanded; we can include all prims, all
prims and properties, or just the targeted objects themselves. In addition to
prims and properties, a Collection can include *another Collection,* which
allows us to hide internal details of assets when we publish them, as the
aggregate scenes that consume the assets can do things like binding materials
and illuminating the scene by targeting the asset's collections (which break
down the asset into meaningful groups) with other collections.

We create and query Collections using :usdcpp:`UsdCollectionAPI`.

For more details on Collections, including pattern-based collections that use
a path expression to determine collection membership, 
see :ref:`collections_and_patterns`.`

.. _usdglossary-component:

Component
*********

A *component* is a "leaf" `kind <#usdglossary-kind>`_ of `Model
<#usdglossary-model>`_. Components can contain `subcomponents
<#usdglossary-subcomponent>`_, but no other models. Components *can* reference
in other assets that are published as models, but they should override the
:usda:`kind` of the referenced prim to ":usda:`subcomponent`".

.. code-block:: usda
   :caption: A "component" asset, declared in scene description, overriding the
             kind of a "nested" asset reference

   def Xform "TreeSpruce" (
       kind = "component"
   )
   {
       # Geometry and shading prims that define a Spruce tree...
   
       def "Cone_1" (
           kind = "subcomponent"
           references = @Cones/PineConeA.usd@
       )
       {
       }
   }

**See also:** `model hierarchy <#usdglossary-modelhierarchy>`_

.. _usdglossary-composition:

Composition
***********

*Composition* is the process that assembles multiple `layers
<#usdglossary-layer>`_ together by the `composition arcs
<#usdglossary-compositionarcs>`_ that relate them to each other, resulting in a
`UsdStage <#usdglossary-stage>`_ scenegraph of `UsdPrims
<#usdglossary-prim>`_. Each :cpp:`UsdPrim` contains an `index
<#usdglossary-index>`_ that allows clients to subsequently extract "`resolved
values <#usdglossary-valueresolution>`_" for properties and metadata from the
relevant layers. Composition occurs when first opening a :cpp:`UsdStage`,
when `loading or unloading prims <#usdglossary-load-unload>`_ on the stage, and
when `layers that contribute to the stage are edited
<#usdglossary-changeprocessing>`_. One of the results of composition is `path
translation <#usdglossary-pathtranslation>`_ so that all the data in all layers
contributing to the scene are accessed by their "scene level" namespace
locations (paths).

We also sometimes refer to "a composition" or "a composed prim" or "a composed
scene", in which contexts we are referring to the *result* of performing
composition.

.. _usdglossary-compositionarcs:

Composition Arcs
****************

*Composition arcs* are the "operators" that allow USD to create rich
compositions of many layers containing mixes of "base" scene description and
overrides. The arcs are:

   * `subLayers <#usdglossary-sublayers>`_
   * `inherits <#usdglossary-inherits>`_
   * `variantSets <#usdglossary-variantset>`_
   * `references <#usdglossary-references>`_
   * `payloads <#usdglossary-payload>`_
   * `specializes <#usdglossary-specializes>`_ 

We refer to these operators as **arcs** because each one targets either a `layer
<#usdglossary-layer>`_, a `prim <#usdglossary-prim>`_, or a combination of layer
and prim, and when diagramming a `prim's index <#usdglossary-index>`_ to
understand how a scene or value is composed, the composition operators represent
the directional arcs that combine layers and `primSpecs
<#usdglossary-primspec>`_ into an ordered graph that we traverse when performing
`value resolution <#usdglossary-valueresolution>`_. Except for subLayers, all
composition arcs target a specific prim in a LayerStack, and allow the renaming
of that target prim as the result "flows" across the arc. USD performs all the
necessary `path translation <#usdglossary-pathtranslation>`_ to ensure that
stage-level queries always work in the stage-level namespace, regardless of how
many different and/or nested composition arcs and name-changes contributed to
the result.

Except for subLayers, all composition arcs are `list editable
<#usdglossary-listediting>`_, which means that each layer in a `layerStack
<#usdglossary-layerstack>`_ can sparsely prepend, append, remove, or reset
targets, which allows us to non-destructively edit the composition structure of
a scene as it evolves or passes through multiple stages of a pipeline. Following
is an example of list-edited references. The **resolved** value of references on
:sdfpath:`/MyPrim` that will be consumed during `composition
<#usdglossary-composition>`_ of :filename:`superLayer.usd` is a two-element
list: :usda:`[ @file1.usd@, @file3.usd@ ]`

.. code-block:: usda
   :caption: Contents of file :filename:`base.usd`

   #usda 1.0
   
   def "MyPrim" (
       references = [
           @file1.usd@,
           @file2.usd@ 
       ]
   )
   {
   }

.. code-block:: usda
   :caption: Contents of file :filename:`superLayer.usd`

   #usda 1.0
   (
       subLayers = [
           @./base.usd@
       ]
   )
   
   # Removes reference to file2.usd, while adding a reference to file3.usd at the end of the list 
   over "MyPrim" (
       delete references = [ @file2.usd@ ]
       append references = [ @file3.usd@ ]
   )
   {
   }

.. _usdglossary-connection:

Connection
**********

Connections are quite similar to `relationships <#usdglossary-relationship>`_
in that they are list-edited "scene pointers" that robustly identify other
scene objects. The key difference is that while relationships are typeless,
independent `properties <#usdglossary-property>`_, connections are instead a
sub-aspect of USD `attributes <#usdglossary-attribute>`_. Relationships serve
to establish dependencies between prims as a whole, or the dependency of a prim
as a whole upon a targeted property. But they lack the expressiveness to
encode, for example, complex, typed, dataflow networks such as shading
networks. In networks in which each node has multiple, value-typed inputs
and/or outputs, we can represent the nodes as prims, and the inputs and outputs
as attributes. Each attribute is strongly typed, and *connections* allow each
input to target an output attribute from which, in some downstream consuming
application, it may receive dataflow.

Connections empower USD to robustly encode dataflow networks of prims, but **USD
itself provides no dataflow behavior across connections.** Instead, USD
stipulates that `schemas <#usdglossary-schema>`_ can impose semantics on
connections, to be interpreted/implemented by the schema or higher-level
systems. We do so for two reasons:

    #. Consulting connections during `value resolution
       <#usdglossary-valueresolution>`_ :cpp:`(UsdAttribute::Get())` would
       necessarily slow down *all* attribute value resolution, whether the
       attribute possesses connections or not.

    #. Interchange is an important aspect of USD, and we are not currently
       willing to tackle the conformance problem of different dataflow semantics
       between different DCCs.

A basic example of connections, from the :doc:`tut_simple_shading`
tutorial, which demonstrates how USD's shading model uses input-to-output
connections to indicated render-time dataflow between shaders, and
output-to-output connections on Materials to bind shader outputs to important
computed results consumed by the renderer:

.. code-block:: usda

   def Material "boardMat"
   {
       token inputs:frame:stPrimvarName = "st"
       token outputs:surface.connect = </TexModel/boardMat/PBRShader.outputs:surface>
   
       def Shader "PBRShader"
       {
           uniform token info:id = "UsdPreviewSurface"
           color3f inputs:diffuseColor.connect = </TexModel/boardMat/diffuseTexture.outputs:rgb>
           float inputs:metallic = 0
           float inputs:roughness = 0.4
           token outputs:surface
       }
   
       def Shader "stReader"
       {
           uniform token info:id = "UsdPrimvarReader_float2"
           token inputs:varname.connect = </TexModel/boardMat.inputs:frame:stPrimvarName>
           float2 outputs:result
       }
   
       def Shader "diffuseTexture"
       {
           uniform token info:id = "UsdUVTexture"
           asset inputs:file = @USDLogoLrg.png@
           float2 inputs:st.connect = </TexModel/boardMat/stReader.outputs:result>
           float3 outputs:rgb
       }
   }

.. _usdglossary-cratefileformat:

Crate File Format
*****************

The *crate* file format is USD's own binary file format, whose file extension is
:filename:`.usdc`, and which is losslessly, bidirectionally convertible to the
:filename:`.usda` text format. The :filename:`.usd` file format is special, as
files with that extension can be **either** crate or text files; this
facilitates debugging as crate files can be converted to text in-place for
rapid iterative hand-editing without needing to change any referencing
layers. The primary differences between text and crate files (other than the
obvious human readability aspect) are:

    #. Text files must be read in their entirety, parsed, and stored in-memory
       as soon as they are opened, whereas crate files read in only a small
       index of the file's contents when they are opened, deferring access to
       big data until a client requests it specifically.

    #. Except for very small files (under a few hundred kilobytes), crate files
       will be much more compact than text files.

Crate was designed for low-latency and high-performance lazy queries, and we
believe it to be the best file format choice for storing scene description
consumed by USD. Some of its features include:

    * Aggressive, multi-level data deduplication yields small file sizes

    * Lockless data extraction for high-bandwidth multi-threaded reading

    * Access to files either by :cpp:`mmap()` or :cpp:`pread()`, trading VM
      pressure for file descriptor consumption and system call overhead. By
      default crate uses :cpp:`mmap()`, but the choice is configurable at
      runtime.

    * Low latency in "cold file-system cache" network access, as all data needed
      to open a crate file for USD's use is compacted into a contiguous footer
      section.

    * Editing crate files does not copy all data to a new file. Instead, it
      appends. Disused values consume disk space, so repeated editing may
      produce files larger than ideal. Use :ref:`toolset:usdcat` to rewrite
      files in their most compact form.

You can :doc:`convert between file formats using usdcat <tut_converting_between_layer_formats>`.

.. _usdglossary-def:

Def
***

*Def* is one of the three possible `specifiers <#usdglossary-specifier>`_ a
`prim <#usdglossary-prim>`_ (and also a `primSpec <#usdglossary-primspec>`_)
can possess. A *def* **defines** a prim, which, for most consumers of USD,
equates to the prim being present on the stage and available for
processing. Prims whose specifier resolves to `class <#usdglossary-class>`_
or `over <#usdglossary-over>`_ are actually present and composed on a stage,
but will not be visited by :usdcpp:`UsdPrim::GetChildren` or
:usdcpp:`UsdStage::Traverse`.  A *def* may, but need not declare a schema type
for the prim. For further information see the FAQ: :ref:`usdfaq:what's the
difference between an "over" and a "typeless def" ?`

The following example defines a prim :sdfpath:`/Ball` as belonging to the **Sphere**
schema, and sets its :usda:`radius` to 50.

.. code-block:: usda
   :caption: A "def" defines a prim

   def Sphere "Ball" {
       double radius = 50
   }

.. _usdglossary-defaultvalue:

Default Value
*************

Many assets consist entirely of a static (with respect to time) definition,
which really exists "outside time". When encoding such assets in a format that
only allows `timeSamples <#usdglossary-timesample>`_, one must choose a
"sentinel" time coordinate at which to record static data, and hope that no other
application uses that sentinel time for any other purpose. This can be fragile,
and also lead to the "static" definition becoming overshadowed and not easily
accessible when overridden in a stronger layer.

USD addresses this problem by providing a completely separate field for each
attribute, called its *default*. This field can be authored and resolved in
isolation of any authored timeSamples anywhere in an attribute's `index
<#usdglossary-index>`_, by using the universal, reserved sentinel
:usdcpp:`UsdTimeCode::Default` as the (implicit or explicit) time coordinate to
:usdcpp:`UsdAttribute::Get` and :usdcpp:`UsdAttribute::Set`.  When
`resolving an attribute's value <#usdglossary-valueresolution>`_ at a
non-Default time, defaults still participate, but within a given `primSpec
<#usdglossary-primspec>`_, an authored default is always weaker than authored
timeSamples. However, an authored default in a stronger layer/primSpec **is
stronger** than timeSamples authored in a weaker layer. In text USD layers, the
default value is the single value that can be assigned directly to an attribute
in the attribute declaration line:

.. code-block:: usda
   :caption: Overriding the default value of a Ball's radius

   over "Ball" {
       double radius = 50
   }

.. _usdglossary-directopinion:

Direct Opinion
**************

A *direct opinion* for some property or metadatum of a prim at path
:sdfpath:`/foo/bar/baz` in a `layerStack <#usdglossary-layerstack>`_ is one that
is authored "directly" on the `primSpec <#usdglossary-primspec>`_ at
:sdfpath:`/foo/bar/baz` **in contrast** to an indirect opinion on a different
primSpec that affects :sdfpath:`/foo/bar/baz` due to the effect of one or more
composition arcs. Two examples of where we find it useful to talk about direct
vs. indirect opinions are:

    #. `References <#usdglossary-references>`_.

       A *direct reference* is one authored on a prim itself, as opposed to an
       *ancestral reference* , authored on some ancestor of the prim. Ancestral
       references are weaker than direct references, so if the targets of both
       the direct and ancestral references contain opinions about the same
       property on the prim, the opinions of the direct reference will win. This
       "weaker ancestor" behavior is also true for direct vs ancestral `Payloads
       <#usdglossary-payload>`_, `VariantSets <#usdglossary-variantset>`_,
       `Inherits <#usdglossary-inherits>`_, and `Specializes
       <#usdglossary-specializes>`_ arcs.

    #. `Specializes arcs <#usdglossary-specializes>`_.

       When prim :sdfpath:`/root/derived` specializes prim :sdfpath:`/root/base`
       in a layer, *direct opinions* authored on the "derived" prim in any
       referencing context (that is, a layerStack that references the
       :sdfpath:`/root` prim in the original layer, **or** any layerStack that
       references the new layerStack, ad infinitum) will always be stronger than
       any opinions expressed directly on the "base" prim in any of the
       referencing contexts. See `specializes <#usdglossary-specializes>`_ for
       examples.

.. _usdglossary-edittarget:

EditTarget
**********

When authoring composed scene description, it is often desirable to edit the
targets of various `composition arcs <#usdglossary-compositionarcs>`_ *in
context* of the scene you are constructing, rather than needing to edit
individual layers in isolation. *Edit Targets*, embodied by the
:usdcpp:`UsdEditTarget` class, allow you to work with the composed stage and the
:usdcpp:`UsdPrims <UsdPrim>` it contains, while specifying which contributing
site in the stage's network of composition arcs should receive the opinions you
are about to author using the composed prim. You can think of Edit Targets as an
extension of the idea of "selecting a layer" in
Photoshop. :usdcpp:`UsdEditTarget` provides specific methods for selecting any
`sublayer <#usdglossary-sublayers>`_ in a stage's `root layerStack
<#usdglossary-rootlayerstack>`_, or the currently selected `variant
<#usdglossary-variant>`_ of any defined top-level or nested `variantSet
<#usdglossary-variantset>`_. In addition, You can create an EditTarget from any
"Node" in a prim's `PrimIndex <#usdglossary-index>`_, which allows you to target
`inherited classes <#usdglossary-inherits>`_, `reference targets
<#usdglossary-references>`_, etc.

.. _usdglossary-fallback:

Fallback
********

Many `IsA Schemas <#usdglossary-isaschema>`_ and applied `API Schemas
<#usdglossary-apischema>`_ define attributes that have an identifiable value
that makes sense in most situations. The :doc:`USD schema generation process
<tut_generating_new_schema>` allows a schema creator to specify such
values as a *fallback* that will be implicitly present on an attribute even when
no values have been authored for it. The presence of fallback values allows us
to keep our scene description sparse and compact, and allows for
self-documenting behavior. Fallbacks are deployed extensively in the `UsdGeom
schemas <api/usd_geom_page_front.html>`_, for example
:usdcpp:`UsdGeomImageable's visibility attribute
<UsdGeomImageable::GetVisibilityAttr>` has a fallback value of
:usda:`inherited`, and :usdcpp:`UsdGeomGprim's orientation attribute
<UsdGeomGprim::GetOrientationAttr>` has a fallback of :usda:`rightHanded`.

.. _usdglossary-flatten:

Flatten
*******

Even though USD derives great efficiencies from accessing data directly from the
`layers <#usdglossary-layer>`_ that a `stage <#usdglossary-stage>`_ composes
together as directed by the various `composition arcs
<#usdglossary-compositionarcs>`_ that weave the files together, it can sometimes
be useful to "bake down the composition" into a single layer that no longer
contains any composition arcs. A flattened stage is highly portable since its
single layer is self-contained, and in some cases, it *may* be more efficient to
compose and resolve, although this is definitely not a given. For example,
flattening a stage to an text USD layer will generally produce extremely large
files since assets that were referenced multiple times on a stage will be
uniquely baked out into their own namespaces, with all data duplicated; the
`crate file format <#usdglossary-cratefileformat>`_ will perform better in this
metric, at least, since it performs data deduplication. Regardless of file
format, the action of flattening a stage will generally be memory and
compute-intensive, and will not, at this time, benefit from multithreading.

To flatten a stage, use :usdcpp:`UsdStage::Flatten` or :ref:`toolset:usdcat`
with the :option:`--flatten` option.

To flatten individual `layer stacks <#usdglossary-layerstack>`_, use
:usdcpp:`UsdUtilsFlattenLayerStack` or :ref:`toolset:usdcat` with the
:option:`--flattenLayerStack` option.

.. _usdglossary-gprim:

Gprim
*****

*Gprim* comes from `Pixar's RenderMan <https://renderman.pixar.com>`_
terminology, and is a contraction for "Geometric primitive", which is to say,
any primitive whose imaging/rendering will directly cause something to be
drawn. Gprim is a first-class concept in USD, embodied in the class
:usdcpp:`UsdGeomGprim`. All Gprims possess the following qualities:

    * Gprims are `boundable
      <api/class_usd_geom_boundable.html#details>`_,
      and should always submit to computing an extent (even if it be an empty
      extent), and valid :cpp:`UsdGeom` scene description should **always
      provide an authored extent** on a gprim that captures its changing shape
      (if its shape is animated).

    * Gprims are directly `transformable
      <api/class_usd_geom_xformable.html#details>`_ ,
      which is the primary distinguishing factor between :usdcpp:`UsdGeomGprim`
      and Autodesk Maya's similar "shape" node type. Transformable gprims
      necessitate fewer prims overall in most 3D compositions, which aids
      scalability, since prim-count is one of the primary scaling factors for
      USD.

.. admonition:: Effectively structuring gprims in namespace
           
   Please observe the following two rules when laying out gprims in a namespace
   hierarchy:
   
   #. **Do not nest gprims in** `namespace <#usdglossary-namespace>`_.

      We consider it invalid to nest gprims under other gprims, and
      :ref:`usdchecker <toolset:usdchecker>` will issue warnings on scenes
      that contain this construct. This is because key features of USD apply
      hierarchically and are "pruning", such as `activation
      <#usdglossary-active-inactive>`_ and `visibility
      <#usdglossary-visibility>`_.  When an ancestor gprim is deactivated or
      made invisible, there is no possible way to make any descendant gprim
      active or visible.

       ..

   #. **Do not directly** `instance <#usdglossary-instancing>`_ **gprims.**

      Because the root-prims of instance prototypes possess no properties, it
      is pointless to instance a gprim directly. Renderers *must not*
      infer instanceability from an instance of a gprim prototype, because each
      instance is allowed to override any property defined originally on the
      referenced prototype root prim. One can use USD's instancing feature to
      create gprim-level instancing, but to do so requires adding an Xform
      parent to the gprim in the prototype, and making the instances reference
      the parent, rather than the gprim, directly.
   

.. _usdglossary-group:

Group
*****

In USD a *group* is a `kind <#usdglossary-kind>`_ of `Model
<#usdglossary-model>`_.  Groups are models that aggregate other models into
meaningful collections. Whereas the specialized group-model kind `assembly
<#usdglossary-assembly>`_ generally identifies group models that are published
`assets <#usdglossary-asset>`_, groups tend to be simple "inlined" model prims
defined inside and as part of assemblies. They are the "glue" that holds a
`model hierarchy <#usdglossary-modelhierarchy>`_ together.

.. code-block:: usda
   :caption: An "assembly" asset, that contains "group" models for organizing its sub-parts

   def Xform "Forest_set" (
       kind = "assembly"
   )
   {
       def Xform "Outskirts" (
           kind = "group"
       )
       {
           # More deeply nested groups, bottoming out at references to other assemblies and components
       }
   	
       def Xform "Glade" (
           kind = "group"
       )
       {
           # More deeply nested groups, bottoming out at references to other assemblies and components
       }
   }

**See also:**  `model hierarchy <#usdglossary-modelhierarchy>`_

.. _usdglossary-hydra:

Hydra
*****

*Hydra* is a modern rendering architecture optimized for handling very large
scenes and "change processing" (i.e. responding to authored or time-varying
changes to the scene inputs). It has three major components: the *scene
delegates* (which provide scene data), the *render index* (responsible for
change tracking and other scene management), and the *render delegates* (which
consume the scene data to produce an image). This flexible architecture allows
for easy integration within pipelines that have their own scene data, as well
as their own renderers. Pixar uses Hydra for asset preview in many of its
tools, including :program:`usdview` and the `Presto Animation System
<http://www.cartoonbrew.com/tech/watch-a-rare-demo-of-pixars-animation-system-presto-98099.html>`_.

If you're interested in developing with Hydra components, see the 
`Hydra Getting Started Guide <api/_page__hydra__getting__started__guide.html>`__
and other Hydra developer documentation in our `Developer Guides <api/_developer__guides.html>`__.

.. _usdglossary-index:

Index
*****

An *index*, also referred to as a :usdcpp:`PrimIndex <PcpPrimIndex>`,
is the result of `composition <#usdglossary-composition>`_, and the primary
piece of information we compute and cache for a composed `prim
<#usdglossary-prim>`_ on a `stage <#usdglossary-stage>`_. A prim's index
contains an ordered (from strongest to weakest) list of "Nodes", which are all
of the locations in `layers <#usdglossary-layer>`_ (also known as `primSpecs
<#usdglossary-primspec>`_) that contribute opinions to the data contained in the
composed prim, as well as an indication of how each location was woven into the
composite, i.e. what `composition arc <#usdglossary-compositionarcs>`_ was
traversed to discover it.

All of the queries on USD classes except for stage-level metadata rely on prim
indices to perform `value resolution <#usdglossary-valueresolution>`_. USD
also uses prim indices to compute `primStacks <#usdglossary-primstack>`_ for
debugging, and to construct `Edit Targets <#usdglossary-edittarget>`_.

.. _usdglossary-inherits:

Inherits
********

*Inherits* is a `composition arc <#usdglossary-compositionarcs>`_ that addresses
the problem of adding a single, non-destructive edit (override) that can affect
a whole *class* of distinct objects on a `stage <#usdglossary-stage>`_. Inherits
acts as a non-destructive "broadcast" operator that applies opinions authored
on one prim to every other prim that inherits the "source" prim; not only do
property opinions broadcast over inherits arcs - **all** scene description,
hierarchically from the source, inherits. Consider the following example:

.. code-block:: usda
   :caption: Trees.usd, demonstrating inherits

   #usda 1.0

   class Xform "_class_Tree"
   {
       def Mesh "Trunk"
       {
           color3f[] primvars:displayColor = [(.8, .8, .2)]
       }

       def Mesh "Leaves"
       {
           color3f[] primvars:displayColor = [(0, 1, 0)]
       }
   }

   def "TreeA" (
       inherits = </_class_Tree>
   )
   {
   }

   def "TreeB" (
       inherits = </_class_Tree>
   )
   {
       over "Leaves"
       {
           color3f[] primvars:displayColor = [(0.8, 1, 0)]
       }
   }

If you paste the example into a :filename:`.usda` file and view the composed
result in :program:`usdview`, you will see that :sdfpath:`/TreeA` and
:sdfpath:`/TreeB` both inherit the child prims and properties nested inside
:sdfpath:`/_class_Tree` You can also observe an important property of *inherits*
behavior in :sdfpath:`/TreeB/Leaves`, which is that inherited opinions are
weaker than `direct opinions <#usdglossary-directopinion>`_ on prims that
inherit the opinions. This allows us to always create exceptions to the
broadcast behavior of inherits, in the same `layerStack
<#usdglossary-layerstack>`_ in which we are broadcasting inherited overrides.

The `specifier <#usdglossary-specifier>`_ for :sdfpath:`/_class_Tree` is `class
<#usdglossary-class>`_. This is not a requirement. A prim can inherit from
any prim that is neither a descendant nor ancestor of itself, regardless of the
prim's specifier or type. However, when inherits are used as a "broadcast edit"
facilitator, we don't typically expect the prims into which we deposit the edits
to be processed directly when we (for example) render a scene - their purpose is
to convey edits to other prims, and may not even contain a complete definition
of any prim(s). Using a *class* specifier for these "edit holders" ensures that
standard stage traversals will skip the prims, and generally conveys intent that
the prim will be inherited by other prim(s).

A good way to understand inherits is to start by understanding `references
<#usdglossary-references>`_. In the above example, if you replace both
:usda:`"inherits = "` with :usda:`"references = "` and view the composition, 
the results will be indistinguishable from each other! Within a `layerStack
<#usdglossary-layerstack>`_ (and ignoring any interaction with `variantSets
<#usdglossary-variantset>`_ since VariantSets come between Inherits and
References in `LIVERPS <#usdglossary-livrpsstrengthordering>`_) inherits are
indistinguishable in effect from *local* references. **The key difference
between references and inherits** is that references fully encapsulate their
targets, and therefore "disappear" when composed through another layer of
referencing, whereas the relationship between inheritors and their inherits
target remains "live" through arbitrary levels of referencing. In other words,
when a prim *inherits* from another prim, it **subscribes itself and all
referencing contexts to changes made to the inherited prim.** You can see this
difference with the following example that uses the previous example as
:filename:`Trees.usd`:

.. code-block:: usda
   :caption: Forest.usd, demonstrating inherits propagation through references

   #usda 1.0
   
   # A new prim, of the same name as the original inherits target, providing new overrides 
   class "_class_Tree"
   {
       token size = "small"
   
       # It's autumn in California
       over "Leaves" 
       {
           color3f[] primvars:displayColor = [(1.0, .1, .1)]
       }
   }
   
   # TreeB_1 still inherits from _class_Tree because its referent does 
   def "TreeB_1" (
       references = @./Trees.usd@</TreeB>
   )
   {
   }

Viewing the `flattened <#usdglossary-flatten>`_ :filename:`Forest.usd`
you can observe that :sdfpath:`/TreeB_1` has both all the
structure inherited from :sdfpath:`/_class_Tree` in :filename:`Trees.usd`, but
also the :usda:`size` attribute it inherits from :sdfpath:`/_class_Tree` in its
own defining layer; as well, even though the referenced :sdfpath:`/TreeB` had
specified its own :usda:`primvars:displayColor` for its :usda:`Leaves` prim, the
reddish color override in :sdfpath:`/class_Tree` wins. Were you to change the
*inherits* to *references* in :filename:`Trees.usd`, :sdfpath:`/TreeB_1` would
not compose the :usda:`size` attribute and its :usda:`Leaves` would retain their
original color, and the only way to broadcast changes to all instances of the
original :sdfpath:`/_class_Tree` in :filename:`Forest.usd` would be to
destructively edit the :filename:`Trees.usd` file. There is a runtime cost to
keeping inherits live, so you may want to avoid proactively adding inherits
everywhere just in case you may want to "override all XXX". Deploy inherits
where they are likely to be useful; for example, at `asset
<#usdglossary-asset>`_ root-prims.

.. _usdglossary-instanceable:

Instanceable
************

*Instanceable* is a `metadatum <#usdglossary-metadata>`_ that declares
whether a given `prim <#usdglossary-prim>`_ should be considered as a
**candidate** for instancing, and is authored via
:usdcpp:`UsdPrim::SetInstanceable`. If :usda:`instanceable = true` on a prim,
then the prim will become an instance of an implicit prototype when composed
on a `stage <#usdglossary-stage>`_, if and only if the prim also contains one
or more direct `composition arcs. <#usdglossary-compositionarcs>`_ It does
not matter whether :usda:`instanceable` is authored in a referenced layer (on
the prim being referenced) or in the layer (or a super-layer) in which the
reference is authored: only the `composed value
<#usdglossary-valueresolution>`_ on the prim matters. See `Instancing
<#usdglossary-instancing>`_ for more information.

.. _usdglossary-instancing:

Instancing
**********

*Instancing* in USD is a feature that allows many instances of "the same" object
to share the same representation (composed prims) on a :cpp:`UsdStage`. In
exchange for this sharing of representation (which provides speed and memory
benefits both for the USD core and, generally, for clients processing the
:cpp:`UsdStage`), we give up the ability to uniquely override opinions on prims
beneath the "instance root", although it is possible to override opinions that
will affect **all** instances' views of the data. Instancing is controlled by
authored metadata, and can be overridden in stronger layers, so it is possible
to "break" an instance when necessary, if it must be uniquified.

**Background:** 

When you add a `reference <#usdglossary-references>`_, `inherits
<#usdglossary-inherits>`_, `specializes <#usdglossary-specializes>`_, or
`payload <#usdglossary-payload>`_ to a prim, the targeted scene description will
be composed onto the referencing stage, causing new prims to compose beneath the
anchoring prim, and allowing the referencing stage to override any of the
targeted prims or properties. See, for example, the `Trees.usd snippet in the
Inherits entry <#usdglossary-inherits>`_. Often we build large environments by
referencing in many copies of "simple" `assets <#usdglossary-asset>`_ into a
larger `assembly <#usdglossary-assembly>`_; we add quotes around "simple"
because it is a matter of perspective and scale: an office chair asset may
consist of hundreds of prims, for example. Although the asset files are shared
by a :cpp:`UsdStage` each time we add a new reference to any given asset, we
`compose <#usdglossary-compositionarcs>`_ a unique copy of all of the prims that
asset contains. This is a requirement to be able to non-destructively edit the
prims, and conversely for clients to see the unique overrides that may be
applied to each copy of the asset. However, since number of prims on a stage is
one of the primary factors that governs how USD scales, this cost can become
prohibitive as environment size grows, regardless of how many improvements we
make over time to the per-prim cost in USD.

**Pay for what you need:** 

Often, however, an environment will need to express very few overrides on the
assets it references, and the majority of the overrides it tends to need to
override bind naturally on the root prim of the asset. This observation provides
us with a means to apply a philosophy to which we try to adhere broadly in USD:
pay runtime cost only for the features you need to use. By making a reference
`instanceable <#usdglossary-instanceable>`_, we declare to USD that we will not
need to express any overrides on the prims beneath the reference anchor (and any
overrides already present in the referencing context will be ignored). In
response to finding an instanceable composition arc, a :cpp:`UsdStage` will
prune its prim composition at the instanceable prim, and make note of the
targeted layer(s) and the arcs used to target them, as an "instancing key". The
stage will create a *prototype* for each unique instancing key, composing fully -
*just once* - all of the prims that would otherwise appear redundantly under
each of the instances, and note the relationship between each instance and its
prototype. Default stage traversals terminate at instances (because instances are
not allowed to have prim children), and from any prim for which
:usdcpp:`UsdPrim::IsInstance` is true, a client can identify and process its 
prototype using :usdcpp:`UsdPrim::GetPrototype`.

This behavior can be described as **explicit instances, with implicit
prototypes:** clients are required to be **explicit** about what prims should be
instanced, so that it is not possible to inadvertently defeat instancing by
sublayering a new layer that (unintentionally) contains overrides beneath an
instanced prim in namespace, and so that we can very efficiently determine where
to apply instancing. Unlike "explicit prototype" schemes, which require (in USD
terminology) a prim/tree to be explicitly added on a stage as concrete prims
before adding instances using relationships, a :cpp:`UsdStage` manages the
creation of prototypes for you, as an **implicit result** of which instanceable
layers you have referenced; this can lead to greater sharing than "explicit
prototypes" because if instances of the same asset appear in more than one
referenced assembly in a scene, they will be identified as sharing the same
prototype, which is not true for explicit prototypes. Extending the
:filename:`Trees.usd` example, by making both :sdfpath:`/TreeA` and
:sdfpath:`/TreeB` instanceable, they will share the same composed :usda:`Trunk`
and :usda:`Leaves` prims inside the prototype created out of
:sdfpath:`/_class_Tree`. **Note that the override expressed on**
:sdfpath:`/TreeB/Leaves` **will be ignored**, because we have declared
:sdfpath:`/TreeB` to be instanceable. Like most features in USD, instanceability
can be overridden in stronger layers, so if a :filename:`Forest.usd` layer
referenced :sdfpath:`/TreeB` in :filename:`Trees.usd` and overrode
:usda:`instanceable = false`, then in that context, :sdfpath:`/TreeB` would get
back its own :usda:`Trunk` and :usda:`Leaves` children, with the override for
:usda:`displayColor` on its :usda:`Leaves` child prim.

.. code-block:: usda
   :caption: Instanceable Trees

   #usda 1.0
   
   class Xform "_class_Tree"
   {
       def Mesh "Trunk"
       {
           color3f[] primvars:displayColor = [(.8, .8, .2)]
       }

       def Mesh "Leaves"
       {
           color3f[] primvars:displayColor = [(0, 1, 0)]
       }
   }
   
   def "TreeA" (
       inherits = </_class_Tree>
       instanceable = true
   )
   {
   }
   
   def "TreeB" (
       inherits = </_class_Tree>
       instanceable = true
   )
   {
       over "Leaves" 
       {
           color3f[] primvars:displayColor = [(0.8, 1, 0)]
       }
   }

For more information on usage and examples, see `Scenegraph Instancing
<api/_usd__page__scenegraph_instancing.html>`_ in the USD Manual.

.. _usdglossary-interpolation:

Interpolation
*************

*Interpolation* appears in two different contexts in USD:

* **Temporal Interpolation** of values in attribute value resolution.

  By default, when :usdcpp:`UsdAttribute::Get` `resolves a
  value from timeSamples <#usdglossary-valueresolution>`_, if the value-type
  supports linear interpolation, the returned value will be linearly interpolated
  between the timeSamples that bracket the requested sample time. This behavior
  can be inhibited for all attributes on a stage by calling
  :usdcpp:`UsdStage::SetInterpolationType(UsdInterpolationTypeHeld)
  <UsdStage::SetInterpolationType>`, which will force all timeSamples to resolve
  with held interpolation.

  Linear interpolation is the default interpolation mode for timeSamples because
  composition arcs can apply time-scales and offsets to the data they reference,
  and therefore data that was originally smoothly sampled can easily become poorly
  filtered and sampled in a referencing context if value resolution preformed only
  point or held interpolation: it would become every client's responsibility to
  attempt to sample the data smoothly, which would be difficult given that the
  function that maps stage-time to the time of the layer in which the timeSamples
  were authored is not easily accessible.

* **Spatial Interpolation** of `Primvar <#usdglossary-primvar>`_ values across a
  `gprim <#usdglossary-gprim>`_. 

  Interpolation is also the name we give to the metadatum that describes how the
  value(s) in a primvar will interpolate over a `geometric primitive
  <#usdglossary-gprim>`_ when that primitive is subdivided for rendering. The
  interpretation of interpolation depends on the type of gprim; for example, on a
  :usda:`Mesh` primitive, a primitive can contain a single value to be held across
  the entire mesh, one value per-face, one value per-point to be interpolated
  either linearly or with the mesh's subdivision basis function, or one value per
  face-vertex. For more information, see :ref:`primvars`.

.. _usdglossary-isaschema:

IsA Schema
**********

An *IsA schema* is a prim `Schema <#usdglossary-schema>`_ that defines the
prim's role or purpose on the Stage. The IsA schema to which a prim subscribes
is determined by the authored value of its :usdcpp:`typeName
<UsdPrim::GetTypeName>` metadata, from which it follows that a prim can
subscribe to at most **one IsA schema** - unlike `API schemas
<#usdglossary-apischema>`_, to which a prim can subscribe many. In terms of the
USD object model, IsA schemas derive from the C++ class :usdcpp:`UsdTyped` and
derive their name from the fact that :usdcpp:`UsdPrim::IsA<SomeSchemaClass>()
<UsdPrim::IsA>` will return true for any prim whose *typeName* is or derives
from :cpp:`SomeSchemaClass`.

IsA schemas can be either abstract or concrete; :usdcpp:`UsdGeomImageable` is an
abstract IsA schema: many prims will answer true to
:cpp:`UsdPrim::IsA<UsdGeomImageable>()`, but there is no
:cpp:`UsdGeomImageable::Define()` method because you cannot create a prim of
type :usda:`Imageable`. :usdcpp:`UsdGeomMesh`, however, is a concrete IsA
schema, since it has a :cpp:`Define()` method and prims can possess the typeName
:usda:`Mesh`.

IsA schemas can provide `fallback <#usdglossary-fallback>`_ values for the
properties they define, which will be reflected at runtime in the `prim
definition <#usdglossary-primdefinition>`_.

IsA schemas can be generated using the `USD schema generation tools
<api/_usd__page__generating_schemas.html>`_, but they can also be created
manually.

.. _usdglossary-kind:

Kind
****

*Kind* is a reserved, prim-level `metadatum <#usdglossary-metadata>`_ whose
authored value is a simple string token, but whose interpretation is backed by
an extensible hierarchical typing-system managed by the :usdcpp:`KindRegistry` 
singleton. We use *kind* to **classify** `prims <#usdglossary-prim>`_ in USD
into a higher-level categorization than the prim's `schema type
<#usdglossary-isaschema>`_ provides, principally to assign roles according to
USD's organizational notion of "`Model Hierarchy
<#usdglossary-modelhierarchy>`_". Out of the box, USD's :usdcpp:`KindRegistry`
comes pre-loaded with a type hierarchy rooted with the base type of "model", as
well as an independent type "subcomponent", like so:

    * `model <#usdglossary-model>`_ - base class for all model kinds. **"model"
      is considered an abstract type and should not be assigned as any prim's
      kind.** 

    * `group <#usdglossary-group>`_ - models that simply group other
      models. See `Model Hierarchy <#usdglossary-modelhierarchy>`_ for why we
      require "namespace contiguity" of models

    * `assembly <#usdglossary-assembly>`_ - an important group model, often a
      published asset or reference to a published asset

    * `component <#usdglossary-component>`_ - a "leaf model" that can contain no
      other models

    * `subcomponent <#usdglossary-subcomponent>`_ - an identified, important
      "sub part" of a component model.

You can query a prim's kind using :usdcpp:`UsdModelAPI::GetKind`; 
several other queries on UsdPrim are derived from a prim's kind, such as the
:usdcpp:`UsdPrim::IsModel` and :usdcpp:`UsdPrim::IsGroup` queries.

.. _usdglossary-layer:

Layer
*****

A *Layer* is the atomic persistent container of scene description for USD. A
layer contains zero or more `PrimSpecs <#usdglossary-primspec>`_, that in turn
describe `Property <#usdglossary-property>`_ and `Metadata
<#usdglossary-metadata>`_ values. Each layer possesses an *identifier* that can
be used to construct references to the layer from other layers.

:usdcpp:`SdfLayer` provides both the "document model" for layers, and the
interface by which USD authors to and extracts data from layers. The
:usdcpp:`SdfLayer` interface serves data according to the USD
prim/property/metadata data model, but the actual encoding of data in the
backing file is quite flexible, thanks to the :usdcpp:`SdfFileFormat` plugin
interface. By implementing a sub-class of :usdcpp:`SdfFileFormat` and
associating it with a unique file extension for USD's consumption, we can enable
direct USD referencing of layers expressed as files of any format whose encoding
can reasonably be translated into USD. This is not only how USD supports direct
consumption of Alembic (:filename:`.abc`) files, but also how USD's native text
and `crate binary <#usdglossary-cratefileformat>`_ representations are
provisioned.

Data authored to layers by applications or scripts **will remain in memory
until** :usdcpp:`SdfLayer::Save` **is called on the layer**. If a program is
writing more data than fits in the program's memory allotment, we suggest:

    #. Using USD's native binary `crate format <#usdglossary-cratefileformat>`_
       (which is the default file format for files created with the
       :filename:`.usd` file extension)

        ..

    #. Calling :python:`layer.Save()` periodically: doing so will flush all of
       the heavy property-value data from memory into the file, while leaving
       the file open and available for continued writing of data. Only the crate
       binary format possesses this "flushability" property; USD's text
       representation can only be written out sequentially beginning-to-end, and
       cannot be digested lazily, therefore it cannot be authored incrementally
       and must always keep all of its data in-memory; other formats do not
       allow incremental saving because they must translate USD's encoding into
       their own format that does not itself allow incremental saving, like the
       :doc:`Alembic FileFormatPlugin <plugins_alembic>`.

Although layers exist first and foremost to define the persistent storage
representation of USD data, one can also create temporary, "in-memory" layers
for lightweight (in that there is no filesystem access required) USD data
storage, via :usdcpp:`UsdStage::CreateInMemory`.

.. _usdglossary-layeroffset:

Layer Offset
************

`Composition arcs <#usdglossary-compositionarcs>`_ such as `References
<#usdglossary-references>`_ and `SubLayers <#usdglossary-sublayers>`_ can
include an offset and scaling of time to be applied during attribute `value
resolution <#usdglossary-valueresolution>`_ for all data composed from the
target layer. We call this a *Layer Offset*, embodied in
:usdcpp:`SdfLayerOffset`.  Layer offsets compose, so that if **A** references
**B** with a time-scale of 2.0 and **B** references **C** with a time-scale of
3.5, then data resolved at **A** whose source is **C** will have a total
time-scale of 7.0 applied to it.

When an arc has both an offset and scale applied, the referenced animation is
**first scaled, then offset** as it is brought into the referencing layer. So,
in the following example, a `timeSample <#usdglossary-timesample>`_ at `timeCode
<#usdglossary-timecode>`_ 12 in the file :filename:`someAnimation.usd` will
appear at ((12 * 0.5) + 10) = **16** as resolved from the referencing
layer. Layer offsets cannot themselves vary over time. If a consuming context
requires variable retiming of referenced data, it can use the more powerful (and
somewhat more costly) `Value Clips <#usdglossary-valueclips>`_ feature.

.. code-block:: usda
   :caption: SubLayer offset/scale in usda

   #usda 1.0
   (
       subLayers = [
           @./someAnimation.usd@ (offset = 10; scale = 0.5)
       ]
   )

.. _usdglossary-layerstack:

LayerStack
**********

*LayerStacks* are the keystone to understanding `composition
<#usdglossary-composition>`_ in USD. The **definition** of a LayerStack is
simply:

    * **LayerStack:** The ordered set of layers resulting from the recursive
      gathering of all `SubLayers <#usdglossary-sublayers>`_ of a `Layer
      <#usdglossary-layer>`_, plus the layer itself as first and strongest.

The LayerStack is important to understanding composition for two reasons:

    #. **Composition Arcs target LayerStacks, not Layers.** 

       When a layer `references <#usdglossary-references>`_ (or `payloads
       <#usdglossary-payload>`_ or sub-layers) another layer, it is targeting
       (and therefore composing) not just the data in the single layer, but all
       the data (in strength-order) in the *LayerStack* rooted at the targeted
       layer.

    #. **LayerStacks provide the container through which references can be
       list-edited.** 

       Many of the composition arcs (as well as `relationships
       <#usdglossary-relationship>`_) describe not just a single target, but an
       *orderable list of targets*, that will be processed (in order) according
       to the type of the arc. References can be `list edited
       <#usdglossary-listediting>`_ among the layers of a LayerStack. This can
       be a powerful method of non-destructively changing the large-scale
       structure of a scene as it flows down the pipeline.

For example, we might have a generic version of a special effect added into a
scene at the sequence level:

.. code-block:: usda
   :caption: sequenceFX.usd, which adds a reference onto an asset that may be
             referenced in from a weaker layer

   #usda 1.0
   
   over "World"
   {
       over "Props"
       {
           over "Prop_145" (
               prepend references = @sequenceFX/turbulence.usd@
           )
           { 
           }
       }
   }

Now, at the shot-level, we have a :filename:`shotFX.usd` layer that participates
in the same LayerStack as :filename:`sequenceFX.usd` (because one of the shot
layers SubLayers in the :filename:`sequence.usd` layer, which in turn SubLayers
the above :filename:`sequenceFX.usd` layer). In this particular shot, we need to
replace the generic turbulence effect with a different one, which may have
completely different prims in it. Therefore it is not enough to just "add on" an
extra reference, because the prims from :filename:`turbulence.usd` will still
"shine through" - we must also **remove** the weaker reference, which we can do
via list editing.

.. code-block:: usda
   :caption: shotFX.usd, which replaces the sequence-level reference, while
             preserving any other references

   #usda 1.0
   
   over "World"
   {
       over "Props"
       {
           over "Prop_145" (
               prepend references = @./fx/shotEffect1.usd@
               delete references = @sequenceFX/turbulence.usd@
           )
           {
           }
       }
   }

When the shot is composed, the references on :sdfpath:`/World/Props/Prop_145`
will be combined using the list editing rules, and will resolve to a list that
includes :filename:`shotEffect1.usd,` but **not** :filename:`turbulence.usd.` In
this second example we have also shown that the operand of list-editing
operations can be a list that can contain multiple targets.

.. _usdglossary-listediting:

List Editing
************

*List editing* is a feature to which some array-valued data elements in USD can
subscribe, that allows the array-type element to be non-destructively, sparsely
modified across `composition arcs <#usdglossary-compositionarcs>`_ . It would
be very expensive and difficult to reason about list editable elements that are
also time-varying, so `Attributes <#usdglossary-attribute>`_ can never be list
editable. `Relationships <#usdglossary-relationship>`_, `References
<#usdglossary-references>`_, `Inherits <#usdglossary-inherits>`_,
`Specializes <#usdglossary-specializes>`_, `VariantSets
<#usdglossary-variantset>`_, and integer and string/token array `custom
metadata <#usdglossary-metadata>`_ can be list edited. When an element is list
editable, instead of only being able to assign an explicit value to it, you can
also, in any stronger layer:

    * **append** another value or values **to the back** of the resolved list;
      if the values already appear in the resolved list, they will be reshuffled
      to the back. An appended composition arc in a **stronger** layer of a
      LayerStack will therefore be **weaker** than all of the arcs *of the same
      type* appended from weaker layers, by default; however, the Usd API's for
      adding composition arcs give you some flexibility here.

        ..

    * **delete** a value or values from the resolved list. A "delete" statement
      can be speculative, that is, it is not an error to attempt to delete a
      value that is not present in the resolved list.

        ..

    * **prepend** another value or values **on the front** of the resolved list;
      if the values already appear in the resolved list, they will be shuffled
      to the front. A prepended composition arc in a **weaker** layer of a
      LayerStack will still be **stronger** than any arcs *of the same type*
      that are **appended** from **stronger** layers.

        ..

    * **reset to explicit** , which is an "unqualified" operation, as in
      :usda:`references = @myFile.usd@`. This causes the resolved list to be reset
      to the provided value or values, **ignoring all list ops from weaker
      layers**.

Although we refer to these operations as "list editing", and they operate on
array-valued data, it should be clear from the description of the operators that
list-edited elements *always resolve to a set* , that is, there will never be
any repetition of values in the resolved list, and it is a syntax error for the
same value to appear twice in the same operation in a layer. Also, in the usda
text syntax, any operation can assign either a single value *without* the
square-bracket list delimiters, or a sequence of values inside square brackets.

See `LayerStack <#usdglossary-layerstack>`_ for an example of list editing, as
applied to references. See also the FAQ on deleting items with list ops:
:ref:`usdfaq:when can you delete a reference (or other deletable thing)?`

.. _usdglossary-livrpsstrengthordering:

LIVERPS Strength Ordering
*************************

LIVERPS is an acronym for **Local, Inherits, VariantSets, Relocates, References, 
Payload, Specializes**, and is the fundamental rubric for understanding how 
:ref:`opinions <usdglossary-opinions>` and 
:ref:`namespace <usdglossary-namespace>` compose in USD. **LIVERPS** describes 
the strength ordering in which the various composition arcs combine, 
**within each** :ref:`LayerStack <usdglossary-layerstack>`. For example, when we 
are trying to determine the value of an :ref:`attribute <usdglossary-attribute>` 
or :ref:`metadatum <usdglossary-metadata>` on a stage at *path* that subscribes 
to the :ref:`value resolution <usdglossary-valueresolution>` policy that 
"strongest opinion wins" (which is all attributes and most metadata), we iterate 
through :ref:`PrimSpecs <usdglossary-primspec>` in the following order looking 
for an opinion for the requested datum:

    #. **Local**: 

       Iterate through all the layers in the local LayerStack
       looking for opinions on the PrimSpec at *path* in each layer - recall
       that according to the definition of LayerStack, this is where the effect
       of direct opinions in all :ref:`SubLayers <usdglossary-sublayers>` of the
       root layer of the LayerStack will be consulted. If no opinion is found,
       then...

    #. **Inherits**: 
       
       Resolve the :ref:`Inherits <usdglossary-inherits>` affecting
       the prim at *path*, and iterate through the resulting targets. For
       each target, **recursively apply** **LIVERP** **evaluation** on
       the targeted LayerStack - **Note that the "S" is not present** - we
       ignore Specializes arcs while recursing . If no opinion is found,
       then...

    #. **VariantSets**: 

       Apply the resolved variant selections to all
       :ref:`VariantSets <usdglossary-variantset>` that affect the PrimSpec at
       *path* in the LayerStack, and iterate through the selected 
       :ref:`Variants <usdglossary-variant>` on each VariantSet. For each 
       target, **recursively apply** **LIVERP** **evaluation** on the targeted
       LayerStack - **Note that the "S" is not present** - we ignore Specializes
       arcs while recursing. If no opinion is found, then...

    #. **r(E)locates**:

       Resolve any :ref:`Relocates <usdglossary-relocates>` target path 
       affecting the prim at *path*, and iterate through the resulting relocates 
       source. For each source, **recursively apply** **LIVERP** **evaluation** 
       on the source's remote LayerStack - **Note that the "S" is not present** 
       - we ignore Specializes arcs while recursing. If no opinion is found, 
       then...

    #. **References**:

       Resolve the :ref:`References <usdglossary-references>`
       affecting the prim at *path*, and iterate through the resulting
       targets. For each target, **recursively apply** **LIVERP** **evaluation**
       on the targeted LayerStack - **Note that the "S" is not present** - we
       ignore Specializes arcs while recursing. If no opinion is found, then...

    #. **Payload**: 

       Resolve the :ref:`Payload <usdglossary-payload>`
       arcs affecting the prim at *path*; if *path* has been **loaded on
       the stage,** iterate through the resulting targets just as we would
       references from step 4. If no opinion is found, then...

    #. **Specializes**: 

       Resolve the :ref:`Specializes <usdglossary-specializes>`
       affecting the prim at *path*, and iterate through the resulting
       targets, **recursively applying *full* LIVERPS evaluation** on each target
       prim. If no opinion is found, then...

    #. Indicate that we could find no authored opinion

We have omitted some details, such as how, for any composition arc in the above
recipe, we order arcs applied directly on the PrimSpec in relation to the same
kind of arc authored on an *ancestral* PrimSpec in the LayerStack - the short
answer is that "ancestral arcs" are weaker than "direct arcs" (see 
:ref:`usdglossary-directopinion`). Additionally, we skip over "S" during 
recursion for other arcs, as explained further in the entry for 
:ref:`Specializes <usdglossary-specializes>`. Performing every step as described 
above for each value lookup would be very costly. This is why we compute and 
cache an :ref:`Index <usdglossary-index>` for every prim on the Stage. The Index 
"pre-applies" the full algorithm to gather all PrimSpecs contributing opinions 
to a prim, caching the list in a recursive data structure that allows efficient 
processing whenever a value on the prim needs to be resolved.

The algorithm for computing the namespace of the stage (i.e. what prims are
present and where) are slightly more involved, but still follows the LIVERPS
recipe.

.. _usdglossary-load-unload:

Load / Unload
*************

The `Payload arc <#usdglossary-payload>`_ provides a "deferred reference"
behavior. Wherever a Stage contains at least one Payload (payloads can be
list-edited and chained), the client has the ability to **Load** (compose) all
the scene description targeted by the Payload, or to **Unload** the Payloads and
all their scene description, recomposing all prims beneath the payloaded-prim,
recursively unloading *their* payloads, should they possess any. We generally
associate payloads with "model assets", which provides us with payloads, and
therefore "load points" at all the leaves of the `Model Hierarchy
<#usdglossary-modelhierarchy>`_. This organization allows clients to craft
"working sets" of a Stage, fully composing only the parts of the scene needed
for a given task, saving time and memory for the operation.

For more information, see `Working Set Management
<api/class_usd_stage.html#Usd_workingSetManagement>`_ in the USD Manual.

.. _usdglossary-localize:

Localize
********

USD allows the construction of highly referenced and layered scenes that
assemble files from many different sources, which may resolve differently in
different contexts (for example, your `asset resolver
<#usdglossary-assetresolution>`_ may apply external state to select between
multiple versions of an asset). If one wishes to package up all of the needed
files for a given scene so that they are isolated from asset resolution behavior
and can be copied or shipped easily to another location, **without** the drastic
transformation that `flattening a stage <#usdglossary-flatten>`_ incurs, then
one must "localize" all of the scattered layers into a coherent tree of
files, which requires not only copying files, but also editing them to retarget
all of the `references <#usdglossary-references>`_, `payloads
<#usdglossary-payload>`_, and generic asset paths to target the copied files in
their new locations. :usdcpp:`UsdUtilsCreateNewUsdzPackage` does this for you,
although we have not yet exposed the ability to *just* localize yet, but we hope
to eventually.

.. _usdglossary-metadata:

Metadata
********

*Metadata* is the lightest-weight form of (name, value) scene description; it is
"light" because unlike attributes, metadata cannot be time-varying, and because
prims and properties can possess metadata, but metadata cannot itself have
metadata. Metadata are extensible, however adding a new, named piece of
metadata requires a change to a configuration file to do so, because the
software wants to know, definitively, what the datatype of the metadatum should
be. USD provides a special, dictionary-valued metadatum called :usda:`customData` 
that provides a place to put user metadata without needing to touch any
configuration files. For more information on the allowed types for metadata and
how to add new metadata to the system, please see `the discussion of metadata in
the API manual. <api/_usd__page__object_model.html#Usd_OM_Metadata>`_

.. _usdglossary-model:

Model
*****

*Model* is a scenegraph annotation ascribable to `prims <#usdglossary-prim>`_ by
setting their `kind <#usdglossary-kind>`_ metadata. 
We label certain prims as models to partition large scenegraphs into
more manageable pieces - there is a core "leaf model" kind, `component
<#usdglossary-component>`_, and two refinements of "models that aggregate
other models", `group <#usdglossary-group>`_, and `assembly
<#usdglossary-assembly>`_. Core :usdcpp:`UsdPrim` API can cheaply answer
questions like :usdcpp:`UsdPrim::IsModel` and :usdcpp:`UsdPrim::IsGroup`, and
"model-ness" is one of the criteria that a :usdcpp:`UsdPrimRange` can use to
traverse a stage. All model kinds are `extensible via site-customization
<api/kind_page_front.html#kind_extensions>`_, so that, for example, you can have
both "character" and "prop" component model kinds in your pipeline if that is a
useful distinction to make. See also `Model Hierarchy
<#usdglossary-modelhierarchy>`_.

.. _usdglossary-modelhierarchy:

Model Hierarchy
***************

*Model Hierarchy* builds on the concept of `model <#usdglossary-model>`_ in a
scenegraph to tackle the problem of discovering and defining a "table of
contents of important subtrees of prims" that can be enumerated and traversed
very efficiently. The model hierarchy defines a **contiguous** set of prims
descending from a root prim on a stage, **all of which are models**. Model
hierarchy is an index of the scene that is, strictly, a **prefix**, of the
entire scene. The member prims must adhere to the following three rules:

    #. Only `group model prims <#usdglossary-group>`_ can have other model
       children (`assembly <#usdglossary-assembly>`_ is a `kind of
       <#usdglossary-kind>`_ group)

        ..

    #. A prim can only be a model if its parent prim is also a (group) model -
       except for the root model prim.

        ..

    #. No prim should have the exact kind "model", because "model" is neither a
       singular component nor a plural group container - it is just the
       "abstract" commonality between components and groups.

This implies that `component <#usdglossary-component>`_ models cannot have model
children. It also implies that just because a prim has its `kind
<#usdglossary-kind>`_ metadata authored to "component", its
:usdcpp:`UsdPrim::IsModel` query will only return true if its parent
:usdcpp:`UsdPrim::IsGroup` also answers affirmatively.

We find model hierarchy to be a useful construct because the models in our
scenes align very closely with "referenced assets", and we build complex scenes
by referencing many assets together and overriding them. Reasoning about
referencing structure can get complicated very quickly and necessitate
introducing fragile conventions. However, reasoning about a model hierarchy is
much more straightforward, and when assets are published/packaged with model
kinds already established, the model hierarchy becomes mostly "self assembling".

.. _usdglossary-namespace:

Namespace
*********

*Namespace* is simply the term USD uses to describe the set of prim `paths
<#usdglossary-path>`_ that provide the identities for prims on a `Stage
<#usdglossary-stage>`_, or `PrimSpecs <#usdglossary-primspec>`_ in a `Layer
<#usdglossary-layer>`_. A Stage's namespace nominally consists of a "forest"
in graph theory, that is, any number of "root prims" that have (possibly empty)
trees beneath them. To facilitate traversal and processing of a Stage's
namespace of prims, each Stage possesses a `PseudoRoot
<#usdglossary-pseudoroot>`_ prim that is the parent of all authored root prims,
represented by the path :sdfpath:`/`.

.. _usdglossary-opinions:

Opinions
********

*Opinions* are the atomic elements that participate in `Value Resolution
<#usdglossary-valueresolution>`_ in USD. Each time you author a value for a
Metadatum, Attribute, or Relationship, you are expressing an *opinion* for that
object in a PrimSpec in a particular Layer. On a composed Stage, any object
may be affected by multiple opinions from different layers; the ordering of
these opinions is determined by the `LIVERPS strength ordering
<#usdglossary-livrpsstrengthordering>`_.

.. _usdglossary-over:

Over
****

*Over* is one of the three possible `specifiers <#usdglossary-specifier>`_ a
`prim <#usdglossary-prim>`_ (and also a `PrimSpec <#usdglossary-primspec>`_)
can possess. An *over* is the "weakest" of the three specifiers, in that it
does not change the resolved specifier of a prim in a LayerStack even when an
*over* appears in a stronger layer than a *def* or *class* for the same
PrimSpec. *Over* is short for "override" or "compose over", and its purpose is
just to provide a *speculative*, neutral prim container for overriding
opinions; we use the term "speculative" because if the over composes over a
defined prim, its opinions will contribute to the evaluation of the stage, but
if all the PrimSpecs contributing to a prim have the *over specifier*, then the
prim will not be visited by :usdcpp:`UsdPrim::GetChildren` or 
:usdcpp:`UsdStage::Traverse`.

When an application exports sparse overrides into a layer that sits on top of an
existing composition, it is common to see deep nesting of overs.

.. code-block:: usda
   :caption: An "over" provides speculative opinions

   #usda 1.0

   over "World"
   {
       over "Props"
       {
           over "LuxoBall"
           {
               double radius = 10
           }
       }
   }

.. _usdglossary-path:

Path
****

A *path* is a location in `namespace <#usdglossary-namespace>`_. In USD text
syntax (and documentation), paths are enclosed in angle-brackets, as found in
the authored targets for `references <#usdglossary-references>`_, `payloads
<#usdglossary-payload>`_, `inherits <#usdglossary-inherits>`_, `specializes
<#usdglossary-specializes>`_, and `relationships <#usdglossary-relationship>`_.
USD assigns paths to all elements of scene description other than metadata,
and the concrete embodiment of a path, :usdcpp:`SdfPath`, serves in the API as a
compact, thread-safe, key by which to fetch and store scene description, both
within a Layer, and composed on a Stage. The :usdcpp:`SdfPath` syntax allows for
recording paths to different kinds of scene description. For example:

    #. :sdfpath:`/Root/Child/Grandchild` represents an absolute prim path of
       three nested prims

    #. :sdfpath:`/Root/Child/Grandchild.visibility` names the property
       :usda:`visibility` on the prim :usda:`Grandchild`.

    #. :sdfpath:`/Root/Child/Grandchild{modelingVariant=withCargoRack}/GreatGrandchild`
       represents the child prim :usda:`GreatGrandchild` authored *inside* the
       `Variant <#usdglossary-variant>`_ ":usda:`withCargoRack` of `VariantSet
       <#usdglossary-variantset>`_ :usda:`modelingVariant`

.. code-block:: usda
   :caption: Scene description in a Layer corresponding to example paths

   #usda 1.0
   
   def "Root" 
   {
       def "Child"
       {
           def "GrandChild" (    # Corresponds to path #1 above
               add variantSets = [ "modelingVariant" ]
           {
               variantSet "modelingVariant" = {
                   "withCargoRack" {
                       def "GreatGrandchild"   # Corresponds to path #3 above
                       {
                       }
                   }
               }
   
               token visibility   # Corresponds to path #2 above
           }
       }
   }

.. _usdglossary-pathtranslation:

Path Translation
****************

All of USD's `Composition Arcs <#usdglossary-compositionarcs>`_ other than
`SubLayers <#usdglossary-sublayers>`_ allow a "prim name change" as the target
prim of the arc gets composed under the prim that added the composition arc. 
One of a `UsdStage's <#usdglossary-stage>`_ central responsibilities is applying
all of the necessary **path translation** required to allow its users to deal
almost exclusively in the "fully composed namespace" of the stage's root layer,
rather than needing to be concerned about what layer (with its own namespace)
provides the data we want to access or author. Path translation is applied
during such queries as :usdcpp:`finding a prim <UsdPrim::GetChildren>`
and :usdcpp:`fetching the targets <UsdRelationship::GetTargets>`
of a `relationship <#usdglossary-relationship>`_ or `connection
<#usdglossary-connection>`_, and *inverse* path translation is performed by the
active `Edit Target <#usdglossary-edittarget>`_ whenever you author to a stage.

Following is an example of the kind of path translation that happens in response
to adding references, demonstrating the effects on not only the stage's
namespace, but also on relationship targets. Given:

.. code-block:: usda
   :caption: asset.usd

   #usda 1.0
   (
       defaultPrim = "MyModel"
   )
   
   def Xform "MyModel"
   {
       rel gprims = [ </MyModel/Cube>, </MyModel/Sphere> ]
   
   
       def Cube "Cube"
       {
       }
   }

and an aggregating file that references :filename:`asset.usd`:

.. code-block:: usda
   :caption: assembly.usd

   #usda 1.0
   (
       defaultPrim = "MySet"
   )
   
   def Xform "MySet"
   {
       def Xform "Building_1" (
           references = @asset.usd@
       )
       {
       }
   
       def Xform "Building_2" (
           references = @asset.usd@
       )
       {
       }
   }

**Then**, if we add the "set" represented by :filename:`assembly.usd` into a
:filename:`shot.usd,` USD path translation operates at two levels (recursively),
translating :sdfpath:`/MyModel` to either :sdfpath:`/Building_1` or
:sdfpath:`/Building_2,` conextually, and translating :sdfpath:`/MySet` to
:sdfpath:`/WestVillage.` So if we :usdcpp:`query the targets
<UsdRelationship::GetTargets>` of the relationship
:sdfpath:`/World/WestVillage/Building_1.gprims`, we will get back:

   [ :sdfpath:`/World/WestVillage/Building_1/Cube`, :sdfpath:`/World/WestVillage/Building_1/Sphere` ]

If we note that there *is* no :usda:`Sphere` prim, and therefore want to
eliminate it from consideration at the shot-level using relationship
`list-editing <#usdglossary-listediting>`_, **we refer to it by its stage-level
path** , rather than its authored path, which is not even easy to determine
using the public Usd API's. Deleting the target in :filename:`shot.usd` might
look like this:

.. code-block:: usda
   :caption: shot.usd

   #usda 1.0
   (
       defaultPrim = "World"
   )
   
   def Xform "World"
   {
       def "WestVillage" (
           references = @assembly.usd@
       )
       {
           over "Building_1" 
           {
               delete rel gprims = </World/WestVillage/Building_1/Sphere>
           }
       }
   }

We mentioned above that path translation also operates in the opposite direction
when you use Edit Targets to send your relationship or connection edits across a
composition arc, because it follows that **every encoded path must be in the
namespace of the** `PrimSpec <#usdglossary-primspec>`_ **on which it is
recorded**.  For example, if we were working with the same :filename:`shot.usd`
Stage, and specified the **same path to delete**,
:sdfpath:`/World/WestVillage/Building_1/Sphere`, but have set the Stage's
EditTarget to :filename:`assembly.usd` across the reference authored on
:sdfpath:`/World/WestVillage/Building_1`, then the act of authoring will
transform the path into the *target* namespace, so the result would be an
:filename:`assembly.usd` that looks like:

.. code-block:: usda
   :caption: Modified assembly.usd with relationship deletion

   #usda 1.0
   (
       defaultPrim = "MySet"
   )
   
   
   def Xform "MySet"
   {
       def Xform "Building_1" (
           references = @asset.usd@
       )
       {
           delete rel gprims = </MySet/Building_1/Sphere>
       }
   
       def Xform "Building_2" (
           references = @asset.usd@
       )
       {
       }
   }

.. _usdglossary-payload:

Payload
*******

A *Payload* is a `composition arc <#usdglossary-compositionarcs>`_ that is a
special kind of a `Reference <#usdglossary-references>`_. It is different from
references primarily in two ways:

    * The targets of References are always consumed greedily by the `indexing
      algorithm <#usdglossary-index>`_ that is used to open and build a
      Stage. When a Stage is opened with
      :usdcpp:`UsdStage::InitialLoadSet::LoadNone <LoadNone>` specified, Payload
      arcs are recorded, but not traversed. This behavior allows clients to
      manually construct a "working set" that is a subset of the whole scene, by
      `loading <#usdglossary-load-unload>`_ just the bits of the scene that they
      require.

       ..

    * `Payloads are weaker than references
      <#usdglossary-livrpsstrengthordering>`_, so, for a particular prim within
      any given `LayerStack <#usdglossary-layerstack>`_, all direct references
      will be stronger than all direct payloads.

Although payloads can be authored on any prim in any layer, in Pixar's pipeline
we find it very useful to primarily add payloads to the root prims of
`component-model <#usdglossary-component>`_ assets. See :ref:`the performance
note on packaging assets with payloads <maxperf:Package assets with payloads>`.

.. _usdglossary-prim:

Prim
****

A *Prim* is the primary container object in USD: prims can contain (and order)
other prims, creating a "`namespace hierarchy <#usdglossary-namespace>`_" on a
`Stage <#usdglossary-stage>`_, and prims can also contain (and order)
`properties <#usdglossary-property>`_ that hold meaningful data. Prims, along
with their associated, computed `indices <#usdglossary-index>`_, are the only
persistent scenegraph objects that a Stage retains in memory, and the API for
interacting with prims is provided by the :usdcpp:`UsdPrim` class. Prims always
possess a resolved `Specifier <#usdglossary-specifier>`_ that determines the
prim's generic role on a stage, and a prim *may* possess a `schema
<#usdglossary-schema>`_ typeName that dictates what kind of data the prim
contains. Prims also provide the granularity at which we apply `scene-level
instancing <#usdglossary-instancing>`_, `load/unload behavior
<#usdglossary-load-unload>`_, and `deactivation
<#usdglossary-active-inactive>`_.

.. _usdglossary-primdefinition:

Prim Definition
***************

A *prim definition* is the set of built-in `properties <#usdglossary-property>`_
and `metadata <#usdglossary-metadata>`_ that a `prim <#usdglossary-prim>`_ gains
from a combination of the `IsA schema <#usdglossary-isaschema>`_ determined from
its typeName and its `applied API schemas <#usdglossary-apischema>`_. A prim's
prim definition is used to determine what properties and metadata the prim has
besides what is authored in its scene description. It also may provide `fallback
<#usdglossary-fallback>`_ values during property value or metadata value
resolution for the prim's built-in properties and metadata. The API for prim
definitions are provided by the :usdcpp:`UsdPrimDefinition` class.

.. _usdglossary-primspec:

PrimSpec
********

Each *composed* `Prim <#usdglossary-prim>`_ on a `Stage <#usdglossary-stage>`_ is
the result of potentially many *PrimSpecs* each contributing their own scene
description to a composite result. A PrimSpec can be thought of as an
"uncomposed prim in a layer". Similarly to a composed prim, a PrimSpec is a
container for `property <#usdglossary-property>`_ data and nested
PrimSpecs. Importantly, `composition arcs <#usdglossary-compositionarcs>`_ can
only be applied on PrimSpecs, and those arcs that specify targets are targeting
other PrimSpecs.

.. _usdglossary-primstack:

PrimStack
*********

A *PrimStack* is a list of `PrimSpecs <#usdglossary-primspec>`_ that contribute
`opinions <#usdglossary-opinions>`_ for a composed `prim's <#usdglossary-prim>`_
`metadata <#usdglossary-metadata>`_. This information is condensed from the
prim's `index <#usdglossary-index>`_, and made available through
:usdcpp:`UsdPrim::GetPrimStack`.

.. _usdglossary-primvar:

Primvar
*******

The name *primvar* comes from RenderMan, and stands for "primitive variable". A
primvar is a special attribute that a renderer associates with a geometric
primitive, and can vary (interpolate) the value of the attribute over the
surface/volume of the primitive. In USD, you create and retrieve primvars using
the :usdcpp:`UsdGeomImageable` schema, and interact with the special primvar
encoding using the :usdcpp:`UsdGeomPrimvar` schema.

There are two key aspects of Primvars:

    * Primvars define a value that can vary across the primitive on which they
      are defined, via `prescribed interpolation rules
      <api/class_usd_geom_primvar.html#Usd_InterpolationVals>`_.

       ..

    * Taken collectively on a prim, its Primvars describe "per-primitive 
      overrides" used to communicate variables to consumers, such as providing
      providing joint influences for UsdSkel schemas, or shader variables for
      renderers. In the shader use-case, different renderers may communicate the 
      variables to the shaders using different mechanisms over which USD has no 
      control; Primvars simply provide the classification that any renderer 
      should use to locate potential overrides.

For examples of primvars and primvar interpolation modes, see :ref:`primvars`.

.. _usdglossary-property:

Property
********

*Properties* are the other kind of namespace object in USD (Prims being the
first). Whereas prims provide the organization and indexing for a composed
scene, properties contain the "real data". There are two types of Property:
`Attribute <#usdglossary-attribute>`_ and `Relationship
<#usdglossary-relationship>`_. All properties can be ordered within their
containing Prim (and are otherwise enumerated in dictionary order) via
:usdcpp:`UsdPrim::SetPropertyOrder`, and can host 
`Metadata <#usdglossary-metadata>`_.

Sometimes it is desirable to be able to further group and organize a prim's
properties without introducing new child prim containers, either for locality
reasons, or to :ref:`keep the scene description lightweight <maxperf:What makes
a USD scene heavy/expensive?>` to that end, properties in USD can be
:usdcpp:`created inside nested namespaces <UsdPrim::CreateAttribute>`, and
:usdcpp:`enumerated by namespace <UsdPrim::GetPropertiesInNamespace>`. Here are
some examples of namespaced properties from usd schemas:

.. code-block:: usda

   #usda 1.0

   over MyMesh
   {
       rel material:binding = </ModelRoot/Materials/MetalMaterial>
       color3f[] primvars:displayColor = [ (.4, .2, .6) ]
   }

.. _usdglossary-propertyspec:

PropertySpec
************

Just as `PrimSpecs <#usdglossary-primspec>`_ contain data for a prim within a
layer, *PropertySpecs* contain the data for a property within a
layer. PropertySpecs are nested inside PrimSpecs; a PropertySpec can be as
simple as a statement of a property's existence (which, for `Attributes
<#usdglossary-attribute>`_, includes its typeName), or can contain values for
any piece of `metadata <#usdglossary-metadata>`_ authorable on properties,
including its value. For `Relationships <#usdglossary-relationship>`_, the value
a PropertySpec can contain is its *targets* , which is an
:usdcpp:`SdfListOp\<SdfPath> <SdfListOp>` For `Attributes
<#usdglossary-attribute>`_, each PropertySpec can contain two independent
values: a timeless `Default Value <#usdglossary-defaultvalue>`_, and a freely
varying, ordered collection of `TimeSamples <#usdglossary-timesample>`_.

.. _usdglossary-propertystack:

PropertyStack
*************

A *PropertyStack* is a list of `PropertySpecs <#usdglossary-propertyspec>`_ that
contribute a `default <#usdglossary-defaultvalue>`_ or `timeSample
<#usdglossary-timesample>`_ (for `Attributes <#usdglossary-attribute>`_) or
target (for `relationships <#usdglossary-relationship>`_), *or* any piece of
`metadata <#usdglossary-metadata>`_, for a given `property
<#usdglossary-property>`_. The information returned by
:usdcpp:`UsdProperty::GetPropertyStack` **should only be used for
debugging/diagnostic purposes, not for** `value resolution
<#usdglossary-valueresolution>`_, because:

    #. In the presence of `Value Clips <#usdglossary-valueclips>`_, an
       attribute's PropertyStack may need to be recomputed on each frame, which
       may be expensive.

        ..

    #. A PropertyStack does not contain the proper time-offsets that must be
       applied to the PrimSpecs to retrieve the correct timeSample when there
       are authored `Layer Offsets <#usdglossary-layeroffset>`_ on `references
       <#usdglossary-references>`_, `subLayers <#usdglossary-sublayers>`_, or
       `clips <#usdglossary-valueclips>`_.

If your goal is to optimize repeated value resolutions on attributes, retain a
:usdcpp:`UsdAttributeQuery` instead, which is designed for exactly this purpose.

.. _usdglossary-proxy:

Proxy
*****

*Proxy* is a highly overloaded term in computer graphics... but so were all the
alternatives we considered for the same concept in USD. "proxy" is one of the
possible `purpose <#usdglossary-purpose>`_ values a prim can possess in the
UsdGeom schemas. When we talk about "a proxy" for a model or part of a model,
we mean a prim (that may have a subtree) that has purpose **proxy** , and *is
paired* with a prim whose purpose is **render.** The idea behind this pairing
is that the **proxy** provides a set of `gprims <#usdglossary-gprim>`_ that are
lightweight to read and draw, and provide an idea of what the full **render**
geometry will look like, at much cheaper cost.

Why not just use a "level of detail" or "standin" `VariantSet
<#usdglossary-variantset>`_, rather than creating this special, different kind
of visibility setting? The answer is twofold:

    * Most clients of USD in our pipeline place a high value on bringing up a
      complex scene for inspection and introspection as quickly as possible

    * But, they also require access to the actual data that will be used for
      rendering, at all times.

Therefore, a VariantSet is not a very attractive option for solving this display
problem, because in order to draw the lightweight geometry, we would have
removed the possibility of inspecting the "render quality" data, because only
one `variant <#usdglossary-variant>`_ of a VariantSet can be composed at any
given time, for a particular prim on a `Stage <#usdglossary-stage>`_. It is a
fairly lightweight operation to instruct the renderer to ignore the proxies and
image the full render geometry, when that is required.

.. _usdglossary-pseudoroot:

PseudoRoot
**********

A Stage's :usdcpp:`PseudoRoot Prim <UsdStage::GetPseudoRoot>` is a contrivance
that allows every `UsdStage <#usdglossary-stage>`_ to contain a single
tree of `prims <#usdglossary-prim>`_ rather than a forest. See `Namespace
<#usdglossary-namespace>`_ for further details.

.. _usdglossary-purpose:

Purpose
*******

*Purpose* is a builtin attribute of the :usdcpp:`UsdGeomImageable`
schema, and is a concept we have found useful in our pipeline for classifying
geometry into categories that can each be independently included or excluded
from traversals of prims on a stage, such as rendering or bounding-box
computation traversals. In essence, it provides client-driven "visibility
categories" as gates on a scenegraph traversal.

The fallback purpose, ***default*** indicates that a prim has "no special
purpose" and should generally be included in all traversals. Subtrees rooted at
a prim with purpose ***render*** should generally only be included when
performing a "final quality" render. Subtrees rooted at a prim with purpose
***proxy*** should generally only be included when performing a lightweight
proxy render (such as OpenGL). Finally, subtrees rooted at a prim with purpose
***guide*** should generally only be included when an interactive application
has been explicitly asked to "show guides".

For a discussion of the motivation for *purpose*, see `Proxy
<#usdglossary-proxy>`_.

.. _usdglossary-references:

References
**********

After `SubLayers <#usdglossary-sublayers>`_, *References* are the next
most-basic and most-important `composition arc <#usdglossary-compositionarcs>`_.
Because a `PrimSpec <#usdglossary-primspec>`_ can apply an entire `list
<#usdglossary-listediting>`_ of References, References *can* be used to achieve
a similar kind of layering of data, when one knows exactly which prims need to
be layered (and with some differences in how the `participating opinions will be
resolved <#usdglossary-livrpsstrengthordering>`_).

But the primary use for References is to compose smaller units of scene
description into larger *aggregates* , building up a `namespace
<#usdglossary-namespace>`_ that includes the "encapsulated" result of composing
the scene description targeted by a reference. Following is a simple example of
referencing, with overrides.

We start with a trivial model asset, :usda:`Marble`. Note that, for brevity, we
are eliding some of the key data usually found in published assets (such as
`AssetInfo <#usdglossary-assetinfo>`_, shading of any kind, `Inherits
<#usdglossary-inherits>`_, a `Payload <#usdglossary-payload>`_, detailed model
substructure).

.. code-block:: usda
   :caption: Marble.usd, defines a single, green marble

   #usda 1.0
   (
       defaultPrim = "Marble"
   )
   
   def Xform "Marble" (
       kind = "component"
   )
   {
       def Sphere "marble_geom"
       {
           color3f[] primvars:displayColor = [ (0, 1, 0) ]
       }
   }

Now we want to create a collection of marbles, by referencing the :usda:`Marble`
asset multiple times, and overriding some of the referenced properties to make
each instance unique.

.. code-block:: usda
   :caption: MarbleCollection.usd, an assembly of referenced Marble assets

   #usda 1.0
    
   def Xform "MarbleCollection" (
       kind = "assembly"
   )
   {
       def "Marble_Green" (
   	       references = @Marble.usd@
   	   )
       {
           double3 xformOp:translate = (-10, 0, 0)
           uniform token[] xformOpOrder = [ "xformOp:translate" ]
       }
    
       def "Marble_Red" (
           references = @Marble.usd@
       )
       {
           double3 xformOp:translate = (5, 0, 0)
           uniform token[] xformOpOrder = [ "xformOp:translate" ]
   	
           over "marble_geom" 
           {
               color3f[] primvars:displayColor = [ (1, 0, 0) ]
           }
       }
   }

To understand the results, we'll examine the result of `flattening a Stage
<#usdglossary-flatten>`_ opened for :filename:`MarbleCollection.usd`, which
provides us with the same namespace and resolved property values that the
originating Stage would, with all of the composition arcs "baked out".

.. code-block:: usda
   :caption: FlattenedMarbleCollection.usd demonstrates how references combine namespaces

   #usda 1.0
    
   def Xform "MarbleCollection" (
       kind = "assembly"
   )
   {
       def Xform "Marble_Green" (
           kind = "component"
       )
       {
           double3 xformOp:translate = (-10, 0, 0)
           uniform token[] xformOpOrder = [ "xformOp:translate" ]
   
           def Sphere "marble_geom"
           {
               color3f[] primvars:displayColor = [ (0, 1, 0) ]
           }
       }
    
       def Xform "Marble_Red" (
           kind = "component"
       )
       {
           double3 xformOp:translate = (5, 0, 0)
           uniform token[] xformOpOrder = [ "xformOp:translate" ]
   	
           def Sphere "marble_geom" 
           {
               color3f[] primvars:displayColor = [ (1, 0, 0) ]
           }
       }
   }

**Things to note:**

    * In the composed namespace, the prim name :usda:`Marble` is gone, since the
      references allowed us to perform a prim name-change on the prim targeted
      by the reference. This is a key feature of references, since without it,
      we would be unable to reference the same asset more than once within any
      given prim scoping, because sibling prims must be uniquely named to form a
      proper namespace.

       ..

    * Even though the asset prim named :sdfpath:`/Marble/marble_geom` shows up
      twice in the flattened scene, which indicates that there were indeed two
      distinct prims on the Stage, when we opened a stage for the original
      :filename:`MarbleCollection.usd`, the file :filename:`Marble.usd` was only
      opened once and shared by both references. For deeper sharing of
      referenced assets, in which the prims themselves are also shared, see
      `Instancing <#usdglossary-instancing>`_ .

       ..

    * References can apply a `Layer Offset <#usdglossary-layeroffset>`_ to
      offset and scale the time-varying data contained in the referenced
      layer(s).

       ..

    * References can target any prim in a `LayerStack
      <#usdglossary-layerstack>`_, excepting ancestors of the prim containing
      the reference, if the reference is an *internal reference* targeting the
      same LayerStack in which the reference is authored. **When targeting
      sub-root prims, however, there is the potential for surprising behavior**
      unless you are aware of and understand the ramifications. One such
      ramification is that if the targeted sub-root prim has an ancestor prim
      that contains a `VariantSet <#usdglossary-variantset>`_, the referencer
      will have no ability to express a selection for that VariantSet. For a
      more complete discussion of the ramifications of referencing sub-root
      prims, see the :usdcpp:`UsdReferences class documentation
      <UsdReferences>`.

See `List Editing <#usdglossary-listediting>`_ for the rules by which
references can be combined within a `LayerStack <#usdglossary-layerstack>`_

.. _usdglossary-relationship:

Relationship
************

A *Relationship* is a "namespace pointer" that is robust in the face of
`composition arcs <#usdglossary-compositionarcs>`_, which means that when you
ask USD for a relationship's targets, USD will perform all the necessary
namespace-manipulations required to translate the **authored** target value into
the scene-level namespace. Relationships are used throughout the USD schemas;
perhaps most visibly in the :usdcpp:`UsdShadeMaterialBindingAPI schema's binding
of gprims to their associated Materials <UsdShadeMaterialBindingAPI>`.
Relationships can have multiple targets, as, for instance, the relationships in
a :usdcpp:`UsdCollectionAPI` target all of the objects that belong to the
named collection; therefore, relationships are `List Edited
<#usdglossary-listediting>`_.  Following is an example that demonstrates how a
relationship's targets must be remapped to provide useful pointers.

Let's enhance the asset example from the `References <#usdglossary-references>`_
entry to have a shading :usda:`Material` and binding:

.. code-block:: usda
   :caption: Marble.usd, with a bound Material

   #usda 1.0
   (
       defaultPrim = "Marble"
   )
   
   def Xform "Marble" (
       kind = "component"
   )
   {
       def Sphere "marble_geom"
       {
           rel material:binding = </Marble/GlassMaterial>
           color3f[] primvars:displayColor = [ (0, 1, 0) ]
       }
   
       def Material "GlassMaterial"
       {
           # Interface inputs, shading networks, etc.
       }
   }

Now, because each marble in the :filename:`MarbleCollection.usd` scene has its
own copy of the :usda:`GlassMaterial` prim, we expect that when we:

.. code-block:: python
   :caption: Resolving referenced relationships

   stage = Usd.Stage.Open("MarbleCollection.usd")
   greenMarbleGeom = stage.GetPrimAtPath("/Marble_Collection/Marble_Green/marble_geom")
   print(UsdShade.MaterialBindingAPI(greenMarbleGeom).GetDirectBindingRel().GetTargets())

we will get: 

   :sdfpath:`/MarbleCollection/Marble_Green/GlassMaterial` 

as the result, even though that was not the authored value in :filename:`Marble.usd`.

.. _usdglossary-relocates:

Relocates
*********

*Relocates* is a :ref:`composition arc <usdglossary-compositionarcs>` that maps 
a prim :ref:`path <usdglossary-path>` defined in a remote 
:ref:`LayerStack <usdglossary-layerstack>` (i.e. across a composition arc) to a 
new path location in the local namespace. 

Relocates are defined in layer metadata, as a list of source path to target
path mappings. Note that these paths can only be prim paths, not property paths.

.. code-block:: usda
    :caption: example relocates defined in layer metadata

    #usda 1.0
    (
        relocates = {
            </CharANewVersion/Clothing> : </CharACurrent/TestClothing>, 
            </EnvA/Trees> : </AlternateEnv/ParkA/Trees>
        }
    )    

Relocates let you rename or reparent prims in situations where you would not be 
able to edit the prims directly. Normally, prims with underlying PrimSpecs from 
composition arcs cannot be directly reparented. While it's possible to reparent 
the underlying "composition source" prims directly, such a change would be a 
destructive change that would affect all other instances that share that scene 
description. Relocates provides a way to *non-destructively* reparent or rename 
prims by specifying a mapping of source namespace paths to target namespace 
paths in the local namespace, ensuring that the source of the composition arc is 
not modified.

As an example, if you had layer :filename:`refLayer.usda` with the following 
prims:

.. code-block:: usda
    :caption: refLayer.usda

    def "PrimA" ()
    {
        def "PrimAChild" ()
        {
            uniform string testString = "test"
            float childValue = 3.5
        }
    }

In another layer, :filename:`main.usda`, "PrimA" is referenced:

.. code-block:: usda
    :caption: main.usda

    def "MainPrim" (
        prepend references = @refLayer.usda@</PrimA>
    )
    {
    }

You cannot directly rename or reparent :sdfpath:`/MainPrim/PrimAChild`. However, 
you can provide a relocates mapping (in :filename:`main.usda`) of 
:sdfpath:`MainPrim/PrimAChild` to another path in the local namespace:

.. code-block:: usda
    :caption: relocates added to main.usda

    #usda 1.0
    (
        relocates = {
            </MainPrim/PrimAChild> : </MainPrim/RenamedPrimAChild>
        }
    )

This renames :sdfpath:`/MainPrim/PrimAChild` to 
:sdfpath:`/MainPrim/RenamedPrimAChild` without affecting the 
:filename:`refLayer.usda` layer.

You could then add an override for :sdfpath:`/MainPrim/RenamedPrimAChild` in 
:filename:`main.usda`. Note that the override uses the *relocated path*:

.. code-block:: usda
    :caption: override added to main.usda

    def "MainPrim" (
        prepend references = @refLayer.usda@</PrimA>
    )
    {
        over RenamedPrimAChild 
        {
            float childValue = 5.2
        }
    }

The resulting stage composition will apply the override to the relocated prim
reference:

.. code-block:: usda
    :caption: flattened main.usda
 
    def "MainPrim"
    {
        def "RenamedPrimAChild"
        {
            float childValue = 5.2
            uniform string testString = "test"
        }
    }

**Things to note:**

    * You cannot relocate a root prim. In other words, the source path for a 
      relocates cannot be a root prim. This is because it's impossible for a 
      root prim to be introduced by an ancestral composition arc, and relocates
      can only used to map prims introduced via composition arcs. 

    * When a source path is relocated, that original source path is considered 
      *no longer valid in the current namespace*. Any local opinions authored
      on a source path will generate a "invalid option at relocation source
      path" error. See 
      :ref:`local opinions not allowed at source paths <usdglossary-relocates-source-invalid>` 
      below for more details.

    * Source and target paths must be complete scene paths. Paths with variant 
      selections (e.g. :sdfpath:`/Prim{var=sel}Child`) are not supported.  

    * Relocates that would create invalid or conflicting namespace paths are not 
      allowed, such as:

        * Relocating a prim to an existing ancestor: 
          :sdfpath:`Prim/Child/Grandchild` : :sdfpath:`Prim/Child` is not 
          allowed.

        * Relocating a prim to a descendant of the source path: 
          :sdfpath:`/Prim/Child` : :sdfpath:`/Prim/Child/Grandchild` is not 
          allowed.

        * Relocating the same source path to multiple targets, or relocating 
          multiple source paths to the same target.

        * Relocating a prim to the source path of a different relocate in the
          same namespace: 
          :sdfpath:`/Prim/Prim1` : :sdfpath:`/Prim/Prim2`, 
          :sdfpath:`/Prim/Prim2` : :sdfpath:`/Prim/Prim3` is not allowed. 
          "Transitive" relocates must be collapsed into the smallest relocation,
          e.g. for the previous example, 
          :sdfpath:`/Prim/Prim1` : :sdfpath:`/Prim/Prim3` should be used instead.
          This applies to relocates in the same LayerStack.

    * If a relocate has "ancestral relocates" (e.g. an ancestor prim that has 
      also been relocated), the relocate source path must use the ancestral 
      relocated path. For example, if you have :sdfpath:`/Root` referencing 
      :sdfpath:`/Ref`, and :sdfpath:`/Ref` also references :sdfpath:`/Ref2`, 
      if :sdfpath:`/Root/Ref` is relocated to :sdfpath:`/Root/RefRelocated`, 
      any additional relocate that would use :sdfpath:`/Root/Ref/Ref2` as a 
      source path must use the relocated path 
      :sdfpath:`/Root/RefRelocated/Ref2`.

With respect to 
:ref:`composition strength ordering <usdglossary-livrpsstrengthordering>`, 
relocates is stronger than :ref:`usdglossary-references`, but weaker than 
:ref:`usdglossary-variantset`. In the previous example, if we had an authored 
opinion at a relocates target location that used the :ref:`usdglossary-inherits` 
composition arc (which is stronger than relocates) we might have a 
:filename:`main.usda` layer that looks like the following: 

.. code-block:: usda
    :caption: main.usda with added class and inherits

    #usda 1.0
    (
        relocates = {
            </MainPrim/PrimAChild> : </MainPrim/RenamedPrimAChild>
        }
    )

    class "WorkClass"
    {
        float childValue = 20.5
        uniform string testString = "from WorkClass"
    }

    def "MainPrim" (
        prepend references = @refLayer.usda@</PrimA>
    )
    {
        def "RenamedPrimAChild"
        (
            inherits = </WorkClass>
        )
        {
        }
    }

When the stage is composed, the inherited opinions for 
:sdfpath:`MainPrim/RenamedPrimAChild` will be stronger:

.. code-block:: usda
    :caption: flattened main.usda with inherits and relocates applied

    def "MainPrim"
    {
        def "RenamedPrimAChild"
        {
            float childValue = 20.5
            uniform string testString = "from WorkClass"
        }
    }

**Relocates and inherits**

A relocated prim will still inherit the same opinions it would have had it not 
been relocated. This can result in some subtle composition behavior.

For example, we have a layer :filename:`model.usda` that defines 
:sdfpath:`/ClassA` and has prim :sdfpath:`/Model` that inherits from 
:sdfpath:`/ClassA`. It also has a relocate for :sdfpath:`/Model/Rig/LRig` to 
:sdfpath:`/Model/Anim/LAnim`:

.. code-block:: usda
    :caption: model.usda with ClassA and Model that inherits from ClassA

    #usda 1.0
    (
        relocates = {
            </Model/Rig/LRig>: </Model/Anim/LAnim>
        }    
    )

    class "ClassA"
    {
        def "Rig"
        {
            def "LRig"
            {
                uniform token modelClassALRig = "test"          
            }
        }

        def "Anim"
        {
            def "LAnim"
            {
                uniform token modelClassALAnim = "test"                     
            }
        }    
    }

    def "Model" (
        inherits = </ClassA>
    )
    {
    }

We reference :sdfpath:`/Model` in another layer, :filename:`root.usda`, which
also has a :sdfpath:`/ClassA` class:

.. code-block:: usda
    :caption: root.usda

    def "Model_1" (
        references = @./model.usda@</Model>
    )
    {
    }

    class "ClassA"
    {
        def "Rig"
        {
            def "LRig"
            {
                uniform token rootClassALRig = "test"
            }
        }

        def "Anim"
        {
            def "LAnim"
            {
                uniform token rootClassALAnim = "test"
            }
        }
    }  

If we load :filename:`root.usda` and inspect the flattened stage,
:sdfpath:`/Model/Rig/LRig` in :filename:`model.usda` has inherited from 
:sdfpath:`/ClassA/Rig/LRig` even though it was relocated to 
:sdfpath:`/Model/Anim/LAnim` in that layer, and does *not* inherit opinions from
:sdfpath:`/ClassA/Anim/LAnim`. However, note that :sdfpath:`/Model_1/Anim/LAnim`
in the :filename:`root.usda` layer does inherit from the layer's 
:sdfpath:`/ClassA/Anim/LAnim`.

.. code-block:: usda
    :caption: flattened root.usda

    def "Model_1"
    {
        def "Rig"
        {
        }

        def "Anim"
        {
            def "LAnim"
            {
                uniform token modelClassALRig = "test"
                uniform token rootClassALAnim = "test"
                uniform token rootClassALRig = "test"
            }
        }
    }


**Relocates and ancestral arcs during composition**

One aspect of relocates and composition is that relocates will *ignore* 
all ancestral arcs *except variant arcs* when we build the 
:ref:`PrimIndex <usdglossary-index>` for a prim. So, if you had a layer that
relocates a prim to be the child of a prim with an ancestral inherits arc:

.. code-block:: usda
    :caption: layer with ancestral inherits and relocates

    #usda 1.0
    (
        relocates = {
            </PrimA/Child>: </PrimWithInherits/Child>
        }    
    )

    def "ClassA"
    (
    )
    {
        def "Child"
        {
            uniform token testString = "from ClassA/Child"
            uniform token classAChildString = "test"
        }
    }

    def "RefPrim"
    (
    )
    {
        def "Child"
        {
            uniform token testString = "from RefPrim/Child"
            uniform token refPrimChildString = "test"
        }
    }

    def "PrimA"
    (
        prepend references = </RefPrim>
    )
    {
    }


    def "PrimWithInherits"
    (
        inherits = </ClassA>
    )
    {
    }

With the relocates for :sdfpath:`/PrimA/Child` to 
:sdfpath:`/PrimWithInherits/Child`, the ancestral opinions from 
:sdfpath:`/ClassA/Child` are ignored. 

.. code-block:: usda
    :caption: flattened PrimWithInherits

    def "PrimWithInherits"
    {
        def "Child"
        {
            uniform token refPrimChildString = "test"
            uniform token testString = "from RefPrim/Child"
        }
    }

However, as mentioned earlier, ancestral *variant* arcs will still compose with
relocates. If we introduce ancestral opinions from a variant in 
:sdfpath:`/PrimWithInherits` instead of using inherits:

.. code-block:: usda
    :caption: replace inherits with variantset in PrimWithInherits

    def "PrimWithInherits"
    (
        # Removed inherits of ClassA
        # Added variantSet and selection with authored Child opinions
        variants = {
            string varSet = "Set1"
        }
        prepend variantSets = "varSet"    
    )
    {
        variantSet "varSet" = {
            "Set1" ()
            {
                def "Child"
                {                
                    uniform token testString = "from varSet Child"
                    uniform token varChildString = "test"
                }
            }
            "Set2" ()
            {
            }
        }
    }

The ancestral opinions from the selected variant will be applied:

.. code-block:: usda
    :caption: flattened PrimWithInherits with ancestral variant opinions

    def "PrimWithInherits"
    {
        def "Child"
        {
            uniform token refPrimChildString = "test"
            uniform token testString = "from varSet Child"
            uniform token varChildString = "test"
        }
    }

See :ref:`composition strength ordering <usdglossary-livrpsstrengthordering>` 
for more details on relocates and composition.

**Local opinions not allowed at source paths**

.. _usdglossary-relocates-source-invalid:

When a source path is relocated, that original source path is considered 
no longer valid in the current namespace. So, if you relocated 
:sdfpath:`/PrimA/Child` to :sdfpath:`/PrimA/NewChild`, you cannot have any 
local opinions authored at :sdfpath:`/PrimA/Child`. This avoids ambiguity and
ensures there's exactly one location (the target path) in the namespace to 
express opinions about a given object.

.. _usdglossary-rootlayerstack:

Root LayerStack
***************

Every `Stage <#usdglossary-stage>`_ has a "root" `LayerStack
<#usdglossary-layerstack>`_, comprised of the LayerStack defined by the root
`Layer <#usdglossary-layer>`_ on which the Stage was opened, **appended** to the
LayerStack defined by the stage's `Session Layer <#usdglossary-sessionlayer>`_
(the method for enumerating the layers in a Stage's root LayerStack,
:usdcpp:`UsdStage::GetLayerStack`, provides the option of eliding the Session
Layer's contributions). The root LayerStack is special/important for two
reasons:

    #. The `prims <#usdglossary-prim>`_ declared in root layers are the only
       ones locatable using the same `paths <#usdglossary-path>`_ that identify
       composed prims on the Stage. Currently, an `Edit Target
       <#usdglossary-edittarget>`_ can only target `PrimSpecs
       <#usdglossary-primspec>`_ in the root LayerStack, although we hope to
       relax that restriction eventually.

        ..

    #. It is the layers of the root LayerStack that are the most useful in
       facilitating shared workflows using USD. Cooperating departments and/or
       artists can each manage their own layer(s) in the root LayerStack of an
       asset, sequence, or shot, and their work will combine in intuitive ways
       with that of other artists working in the same context in their own
       layers.

.. _usdglossary-schema:

Schema
******

USD defines a *schema* as an object whose purpose is to author and retrieve
structured data from some :usdcpp:`UsdObject`.  Most schemas found in the core
are "prim schemas", which are further refined into `IsA Schemas
<#usdglossary-isaschema>`_ and `API Schemas <#usdglossary-apischema>`_, for
which the USD distribution provides `tools for code generation
<api/_usd__page__generating_schemas.html>`_ to create your own schemas. However,
there are several examples of "property schemas", also, such as
:usdcpp:`UsdGeomPrimvar` and :usdcpp:`UsdShadeInput`. Schemas are
lightweight objects we create to wrap a :usdcpp:`UsdObject`, as and when needed,
to robustly interrogate and author scene description. We also use schema classes
to package type/schema-based **computations,** such as
:usdcpp:`UsdGeomImageable::ComputeVisibility`.

.. _usdglossary-sessionlayer:

Session Layer
*************

Each `UsdStage <#usdglossary-stage>`_ can be created with a *session layer* that
provides for "scratch space" to configure, override, and experiment with the
data contained in files backing the stage. If requested or provided at stage
creation-time, a session layer participates fully in the stage's `composition
<#usdglossary-composition>`_, as the strongest layer in the stage's `Root
LayerStack <#usdglossary-rootlayerstack>`_, and can possess its own `SubLayers
<#usdglossary-sublayers>`_. Session layers generally embody "application state",
and, if saved, would be saved as part of application state rather than as part 
of the data set they modify. :ref:`toolset:usdview` creates a sesssion layer, 
into which are targeted all `VariantSet <#usdglossary-variantset>`_ selections,
`vis/invis <#usdglossary-visibility>`_ opinions, and `activation/deactivation
<#usdglossary-active-inactive>`_ operations provided by the GUI. In keeping with
the view of session-layer as application state rather than asset data,
:usdcpp:`UsdStage::Save` **does not save its stage's session layer(s)**.

To edit content in a session layer, get the layer's edit target using 
:usdcpp:`stage->GetEditTargetForLocalLayer(stage->GetSessionLayer()) 
<UsdStage::GetEditTargetForLocalLayer>` and set that target in the stage by 
calling :usdcpp:`UsdStage::SetEditTarget` or creating a 
:usdcpp:`UsdEditContext`.

.. _usdglossary-specializes:

Specializes
***********

*Specializes* is a `composition arc <#usdglossary-compositionarcs>`_ that allows
a *specialized* prim to be continuously refined from a *base* prim, through
unlimited levels of referencing. A specialized prim will contain all of the
scene description contained in the base prim it specializes (including the
entire namespace hierarchy rooted at the base prim), but any opinion expressed
directly on the specialized prim will always be stronger than any opinion
expressed on the base prim, **in any referencing context**. In behavior,
specializes is very similar to `inherits <#usdglossary-inherits>`_, with the
key difference being the relative strength of referenced opinions vs
"inherited/specialized" opinions, which leads to very different uses for the
two.

Let us examine an example inspired by the first uses of the specializes arc at
Pixar: specializing materials in our shading schema. We wish to publish an
asset that contains several types of metallic materials, all of which should
maintain their basic "metalness", even as the definition of metal may change in
referencing contexts. For brevity, we focus on one particular specialization, a
corroded metal; we also leave out many of the schema details of how materials
and their shaders interact.

.. code-block:: usda
   :caption: Robot.usd

   #usda 1.0
   
   def Xform "Robot"
   {
       def Scope "Materials"
       {
           def Material "Metal"
           {
               # Interface inputs drive shader parameters of the encapsulated
               # network. We are not showing the connections, nor how we encode
               # that the child Shader "Surface" is the primary output for the
               # material.
               float inputs:diffuseGain = 0
               float inputs:specularRoughness = 0
   
               def Shader "Surface"
               {
                   asset info:id = @PxrSurface@
               }
           }
   
           def Material "CorrodedMetal" (
               specializes = </Robot/Materials/Metal>
           )
           {
               # specialize roughness...
               float inputs:specularRoughness = 0.2
   
               # Adding a pattern to drive Surface bump
               def Shader "Corrosion"
               {
                   asset info:id = @PxrOSL@
                   vector3f outputs:disp
               }
   
               over "Surface"
               {
                   # Override that would connect specularBump to Corrosion
                   # pattern's "outputs:disp" attribute
               }
           }
       }
   }

**Notes:**

    * The above example is not realistic at all regarding how you would actually
      design a :usda:`Metal` or :usda:`CorrodedMetal` material!

    * Specializes can target any prim **on the stage** that is neither an
      ancestor nor descendant of the specializing prim.

    * In the example above, replacing the *specializes* with `inherits
      <#usdglossary-inherits>`_ will produce the same composed result - try it!

In the :filename:`sameRobot.usd` asset, we might define many more materials,
some of which are "siblings" of :usda:`CorrodedMetal` in that they specialize
:usda:`Metal,` and some that further specialize :usda:`CorrodedMetal,` since
specialization is not limited to any depth. The unique behavior of specializes
only becomes evident, however, in a referencing context, so let us try one.

.. code-block:: usda
   :caption: RobotScene.usd

   #usda 1.0
   
   def Xform "World"
   {
       def Xform "Characters"
       {
           def "Rosie" (
               references = @./Robot.usd@</Robot>
           )
           {
               over "Materials"
               {
                   over "Metal"
                   {
                        float inputs:diffuseGain = 0.3
                        float inputs:specularRoughness = 0.1
                   }
               }
           }
       }
   }

If you examine the `flattened <#usdglossary-flatten>`_
:filename:`RobotScene.usd` you will see the effect of specializes on the
specialized :sdfpath:`/World/Characters/Rosie/Materials/CorrodedMetal` prim: we
overrode both :usda:`diffuseGain` and :usda:`specularRoughness` on the base
:usda:`Metal` material, but only the :usda:`diffuseGain` propagates onto
:sdfpath:`/World/Characters/Rosie/Materials/CorrodedMetal`, because
:usda:`specularRoughness` was already refined on the **referenced**
:sdfpath:`/Robot/Materials/CorrodedMetal` prim. This also demonstrates the
difference between specializes and `inherits <#usdglossary-inherits>`_: if you
change the :usda:`specializes` arc to :usda:`inherits` in Robot.usd and
recompose the scene, you will see that **both** :usda:`diffuseGain` and
:usda:`specularRoughness` propagate onto
:sdfpath:`/World/Characters/Rosie/Materials/CorrodedMetal`.

The *specializes* behavior is desirable in this context of building up many
unique refinements of something whose base properties we may want to continue to
update as assets travel down the pipeline, but **without** changing anything
that makes the refinements unique. What if we **do** want to broadcast an edit
on the :usda:`Metal` material to all of the :usda:`Materials` that specialize it? All we need
do is use *inherits* and *specializes* together: make each :usda:`Metal` itself and each
specialized material (such as :usda:`CorrodedMaterial`) inherit from a :sdfpath:`/_class_Metal`
prim. Now any overrides added in :sdfpath:`/_class_Metal` will propagate to each
specialized material regardless of whether they have expressed any opinion about
it in referenced layers, themselves.

.. _usdglossary-specifier:

Specifier
*********

Every `PrimSpec <#usdglossary-primspec>`_ possesses a *specifier*, which
conveys the author's (authoring tool's) intent for how the PrimSpec should be
consumed and interpreted in a composed scene. There are three possible values
for a prim's specifier in USD, defined by :usdcpp:`SdfSpecifier`:

    * `def <#usdglossary-def>`_ - a concrete, defined prim

    * `over <#usdglossary-over>`_ - a speculative override

    * `class <#usdglossary-class>`_ - prims from which other prims `inherit
      <#usdglossary-inherits>`_

A prim's **resolved specifier** on a UsdStage determines which kinds of
traversals (as defined by `"prim flags"
<api/prim_flags_8h.html#Usd_PrimFlags>`_) will visit
the prim. The most common, default traversals, which are meant to be used for
rendering and other common scenegraph processing, will visit only **defined**,
**non-abstract** prims.

.. _usdglossary-stage:

Stage
*****

A *stage* is the USD abstraction for a scenegraph derived from a root USD file,
and all of the referenced/layered files it `composes
<#usdglossary-compositionarcs>`_. A stage always presents the composed view of
the scene description that backs it.  The :usdcpp:`UsdStage` class embodies a
Stage, and caches in memory just enough information about the composed namespace
and backing files so that we can rapidly traverse the stage and perform
efficient `data queries <#usdglossary-valueresolution>`_, and allow prim,
property, and metadata editing targeted at any layer that contributes to the
Stage's composition. Mutated stage layers can be collectively saved to
backing-store using :usdcpp:`UsdStage::Save`.

.. _usdglossary-stagetraversal:

Stage Traversal
***************

Most consumption of USD data follows the pattern of traversing an open stage,
processing prims one at a time, or several in parallel. A :cpp:`UsdStage` allows
for both direct and subtree range-based traversals, each with "filtering"
according to a number of criteria.

    * **direct** 

      Given a `UsdPrim <#usdglossary-prim>`_ *p*, one can fetch its
      direct children on the Stage using `p.GetChild() <UsdPrim::GetChild>`,
      `p.GetChildren() <UsdPrim::GetChildren>`,
      `p.GetAllChildren() <UsdPrim::GetAllChildren>`, and
      `p.GetFilteredChildren() <UsdPrim::GetFilteredChildren>`.

    * **subtree range iteration** 

      One can also fetch a :usdcpp:`UsdPrimSubtreeRange` directly
      from a prim via `p.GetDescendants() <UsdPrim::GetDescendants>`,
      `p.GetAllDescendants() <UsdPrim::GetAllDescendants>`, and
      `p.GetFilteredDescendants() <UsdPrim::GetFilteredDescendants>`.

      .. note::

         None of the :cpp:`Get*Descendants()` methods are wrapped to Python,
         pending a refactoring project). 

      Additionally, and more commonly used in USD core code, one can traverse
      the subtree rooted at a prim by creating a :usdcpp:`UsdPrimRange`, which
      allows for depth-first iteration with the ability to prune subtrees at any
      point during the iteration. The :usdcpp:`UsdStage::Traverse` method
      creates and returns a UsdPrimRange that will traverse the entire stage
      using the "default predicate."

**Traversal Filtering via PrimFlag Predicates**

All of the methods mentioned above that contain "Filtered" in their name, plus
UsdPrimRange, allow one to specify a predicate consisting of a boolean
combination of "flags" that test against certain (all cached, for fast access)
states of prims. Some examples are whether a prim is defined, or active, or
loaded, etc. For a full list of the possible prim flags and examples of how they
can be logically combined, see `prim predicate flags
<api/prim_flags_8h.html#Usd_PrimFlags>`_. The remaining methods
(e.g. :code:`GetChildren()`) all use a predefined :usdcpp:`Default Predicate
<UsdPrimDefaultPredicate>` that serves a common traversal pattern.

.. _usdglossary-subcomponent:

Subcomponent
************

*Subcomponent* is another `kind <#usdglossary-kind>`_ value like `component
<#usdglossary-component>`_ that can be applied to a prim, but is not a "`model
kind <#usdglossary-model>`_", and serves a different purpose. The model-kinds
collaborate to form a `model hierarchy <#usdglossary-modelhierarchy>`_, whose
purpose is to provide a lightweight prefix/index of a complicated scene. By
contrast, subcomponents have no strict contiguity rules associated with them;
rather, a subcomponent just identifies some prim inside a component that may be
"important". An exporter may choose to identify articulation points in a
complicated model by labeling such prims as subcomponents (for example, the
:usda:`DoorAssembly` :usda:`Xform` inside an architectural model). Thus
subcomponents provide a way of setting up levels of organizational complexity
within component models, providing intermediate interfaces/views of a model's
organization that sit in-between "just show me a single prim for the entire
model" and "expand all hierarchy down to leaf prims".

As another example of how subcomponents may guide interaction behavior, the
pxrUsdReferenceAssembly plugin for Maya uses the presence of subcomponent prims
to guide the behavior of its "expanded" representation.

.. _usdglossary-sublayers:

SubLayers
*********

*SubLayers* is the `composition arc <#usdglossary-compositionarcs>`_ used to
construct `LayerStacks <#usdglossary-layerstack>`_.  As an example, here is
one possible combination of USD layers that could have been the source for the
example in the `LayerStack <#usdglossary-layerstack>`_ entry, and also
demonstrate how SubLayers supports nested LayerStacks:

.. code-block:: usda
   :caption: shot.usd

   #usda 1.0
   (
       subLayers = [
           @shotFX.usd@,
           @shotAnimationBake.usd@,
           @sequence.usd@
       ]
   )

.. code-block:: usda
   :caption: sequence.usd

   #usda 1.0
   (
       subLayers = [
           @sequenceFX.usd@,
           @sequenceLayout.usd@,
           @sequenceDressing.usd@
       ]
   )

Note that SubLayers can specify `Layer Offsets <#usdglossary-layeroffset>`_ to
offset and scale time-varying data contained in the sub-layer(s)

.. _usdglossary-timecode:

TimeCode
********

*TimeCodes* are the unit-less time coordinate in USD. A :usdcpp:`UsdTimeCode` 
can encode the coordinate for a `TimeSample <#usdglossary-timesample>`_ in
double-precision floating point, but can also encode the coordinate that maps to
an attribute's `Default Value <#usdglossary-defaultvalue>`_. For any given
composed scene, defined by its root layer, the TimeCode coordinates of the
TimeSamples contained in the scene are 
:ref:`scaled to seconds <usdglossary-timecodes-scaled>` by the root layer's
:usda:`timeCodesPerSecond` metadata, which can be retrieved with
:usdcpp:`UsdStage::GetTimeCodesPerSecond`.  This allows clients great
flexibility to encode their TimeSamples within the range and scale that makes
the most sense for their application, while retaining a robust mapping to "real
time" for decoding and playback.

TimeCodes can also appear in USD scenes as the :usda:`timeCode` metadata or 
attribute value type, and when they do, queried attribute *values* will receive 
the same time-remapping that TimeSample coordinates do. Such timeCode-valued 
attributes can serve as "timing curves" that maintain their accuracy through 
composed layer offsets.

.. _usdglossary-timecodes-scaled

TimeCodes Scaled to Real Time
*****************************

For a composed scene, :ref:`TimeCode <usdglossary-timecode>` coordinate values 
from :ref:`TimeSamples <usdglossary-timesample>` are scaled to real-time seconds 
by the root layer's (or session layer's) :usda:`timeCodesPerSecond` metadata. 
In the following example layer, the translation TimeSample on Sphere at TimeCode 
240 corresponds to 10 seconds of real time, based on the layer's 
:usda:`timeCodesPerSecond` of 24.

.. code-block:: usda 

  #usda 1.0
  (
      timeCodesPerSecond = 24
      endTimeCode = 240
      startTimeCode = 1
  )

  def Xform "Asset"
  {
      def Sphere "Sphere"
      {
          double3 xformOp:translate.timeSamples = {
              1: (0, 5.0, 0),
              240: (0, -5.0, 0),
          }
          uniform token[] xformOpOrder = ["xformOp:translate"]
      }
  }


If a layer specifies :usda:`timeCodesPerSecond` and is sublayered or referenced 
into another layer, the TimeCode values and TimeSample coordinates in the 
sublayered/referenced layer are automatically scaled to map into the timing 
defined by the root layer's :usda:`timeCodesPerSecond`. If the previous example 
layer was referenced into another layer that specified a 
:usda:`timeCodesPerSecond` value of 48, the TimeSamples on Sphere would be 
scaled accordingly. For example, the TimeSample at TimeCode 240 would be scaled 
to TimeCode 480 to ensure that the translation still occurs at 10 seconds of 
real time.

USD also provides the :usda:`framesPerSecond` layer metadata, however this is 
not used to directly scale TimeCodes, but instead used as an indication of the 
desired play-back rate when the animation is viewed in a playback device 
(DCC tool, usdview, etc). If the previous example layer specified a 
:usda:`framesPerSecond` of 12, this would *not* change the scaling of the 
TimeSample at TimeCode 240, and instead change the playback rate in a playback 
device to march forward by two TimeCodes for each consecutive rendered frame, 
which will be held for 1/12 of a second.

.. code-block:: usda

  #usda 1.0
  (
      timeCodesPerSecond = 24
      framesPerSecond = 12
      endTimeCode = 240
      startTimeCode = 1
  )

Note that :usda:`framesPerSecond` can be used indirectly to scale TimeCodes, 
because it is used as a fallback value for :usda:`timeCodesPerSecond` if 
:usda:`timeCodesPerSecond` is not set. The order of precedence USD uses for 
determining the :usda:`timeCodesPerSecond` to use is: 

* :usda:`timeCodesPerSecond` from session layer
* :usda:`timeCodesPerSecond` from root layer
* :usda:`framesPerSecond` from session layer
* :usda:`framesPerSecond` from root layer
* fallback value of 24 

The general best practice is to use :usda:`timeCodesPerSecond` to specify how 
TimeCodes are scaled to real time, and :usda:`framesPerSecond` if you need to 
encode a specific playback rate on playback devices, regardless of how many 
samples per second are recorded in the USD scene.

.. note::

    We provide the information about :usda:`framesPerSecond` as fallback for 
    :usda:`timeCodesPerSecond` primarily as a debugging aid, should you observe 
    unexpected time-scaling. The fallback behavior derives only from USD's 
    relationship to Pixar's Presto animation system.

.. _usdglossary-timesample:

TimeSample
**********

The term *timeSample* is used in two related contexts in USD:

    * **TimeSamples as source for** `Value Resolution
      <#usdglossary-valueresolution>`_ 
      
      Each `PropertySpec <#usdglossary-propertyspec>`_ for an `Attribute
      <#usdglossary-attribute>`_ can contain a collection called *timeSamples*
      that maps `TimeCode <#usdglossary-timecode>`_ coordinates to values of the
      Attribute's type.

    * **The time-coordinate for an Attribute** 

      USD API sometimes refers to just the coordinate of a time-varying value as 
      a TimeSample; for example, :usdcpp:`UsdAttribute::GetTimeSamples` and
      :usdcpp:`UsdAttribute::GetTimeSamplesInInterval` return
      a simple vector of time coordinates at which samples may be resolved on 
      the attribute.

.. _usdglossary-typedschema:

Typed Schema
************

Synonym for `IsA Schema <#usdglossary-isaschema>`_.

.. _usdglossary-userproperties:

User Properties
***************

:usda:`userProperties:` is a `property namespace <#usdglossary-property>`_
recognized by the majority of DCC plugins as a place to look for "extra custom
properties" that are too informal to warrant creating an `API Schema
<#usdglossary-apischema>`_, but which you might need access to in the DCC's,
and want to ensure they get round-tripped back to USD. We plan to create a
:cpp:`UsdUserPropertiesAPI` schema to aid in authoring and enumerating user
properties, but creating a property in the :usda:`userProperties:` namespace
using :cpp:`UsdPrim::CreateAttribute()` (not all importers may handle custom
relationships properly) is sufficient.

.. _usdglossary-valueclips:

Value Clips
***********

*Value Clips* are a feature that allows one to partition varying attribute
`timeSample <#usdglossary-timesample>`_ overrides into multiple files, and
combine them in a manner similar to how non-linear video editing tools allow
one to combine video clips. Clips are especially useful for solving two
important problems in computer graphics production pipelines:

    #. **Crowd/background animation at scale** 

       Crowd animators will often create animation clips that can apply to many
       background characters, and be sequenced and cycled to generate a large
       variety of animation. USD clips provide the ability to encode the
       sequencing and non-uniform time-mapping of baked animation clips that
       this task requires.

    #. **File-per-frame "big data"** 

       The results of some simulations and other types of sequentially-generated
       special effects generate so much data that it is most practical for the
       simulator to write out each time-step or frame's worth of data into a
       different file. USD Clips make it possible to stitch all of these files
       together into a continuous (even though the data may itself be
       topologically varying over time) animation, without needing to move,
       merge, or perturb the files that the simulator produced. The USD toolset
       includes a utility :ref:`toolset:usdstitchclips` that efficiently
       assembles a sequence of file-per-frame layers into a Value Clips
       representation.

The key advantage of the clips feature is that the resulting resolved animation
on a :cpp:`UsdStage` is indistinguishable from data collected or aggregated into
a single layer. In other words, consuming clients can be completely unaware of
the existence of clips: there is no special schema or API required to access the
data. The disadvantages of using clips are:

    #. Encoding clips on a stage is more complicated than simply recording
       samples on attributes, or adding references (see :usdcpp:`UsdClipsAPI`
       for details on encoding)

        ..

    #. There is some performance overhead associated with the use of clips, both
       in the number of files that must be opened to play back animation (but
       that's what we asked for in using clips!), and also in extra overhead in
       `resolving attribute values <#usdglossary-valueresolution>`_ in the
       presence of clips. Clips are the reason that
       :usdcpp:`UsdProperty::GetPropertyStack` requires a :cpp:`timeCode`
       argument, because the set of layers that contribute to an attribute's
       value can change over time when it is affected by clips.

.. note::

   For performance and scalability reasons, a :cpp:`UsdStage` will ignore any
   composition arcs contained in a "clip" USD file, which means that clips can
   only *usefully* contain direct (local) opinions about the attributes they
   wish to modify.  For more information on value clip behavior and how clips
   are encoded, see `Sequenceable, Re-timeable Animated Value Clips
   <api/_usd__page__value_clips.html>`_ in the USD Manual.

.. _usdglossary-valueresolution:

Value Resolution
****************

*Value Resolution* is the algorithm by which final values for `properties
<#usdglossary-property>`_ or `metadata <#usdglossary-metadata>`_ are "composed"
from all of the various `PropertySpecs <#usdglossary-propertyspec>`_ or
`PrimSpecs <#usdglossary-primspec>`_ that contain data for the property or
metadatum. Even though value resolution is the act of composing potentially
many pieces of data together to produce a single value, we distinguish *value
resolution* from `composition <#usdglossary-composition>`_ because
understanding the differences between the two aids in effective construction
and use of USD:

    * **Composition is cached, value resolution is not** 

      The "indexing" performed by the composition algorithm when a `Stage
      <#usdglossary-stage>`_ is opened, `prims are loaded
      <#usdglossary-load-unload>`_, or new scene description is authored, is
      cached *at the prim-level* for fast access. The USD core does not,
      however, pre-compute or cache any per-composed-property information, which
      is a principal design decision aimed at keeping latency low for
      random-access to composed data, and keeping the minimal memory footprint
      for USD low. Instead, for attribute value resolution, the USD core
      provides opt-in facilities such as :usdcpp:`UsdAttributeQuery` and 
      :usdcpp:`UsdResolveInfo`, objects a client can construct and
      retain themselves that cache information that can make repeated value
      queries faster.

    * **Composition is internally multi-threaded, value resolution is meant to
      be client multi-threaded.** 

      Composition of a large stage can be a big computation, and USD strives to
      effectively, internally multi-thread the computation; therefore clients
      should realize they are unlikely to gain performance from opening multiple
      stages simultaneously in different threads. Value resolution, however, is
      a much more lightweight process (moreso for attributes than
      relationships), and USD's primary guidance for clients wishing to maximize
      USD's performance on multi-core systems is to perform as much simultaneous
      value resolution and data extraction as possible; USD's design was guided
      by this access pattern, and we continue to work on eliminating remaining
      impediments to efficient multi-threaded value resolution.

    * **Composition rules vary by composition arc, value resolution rules vary
      by metadatum.** 

      The composition algorithm is the process of interpreting a collection of
      composition arcs into an "`index <#usdglossary-index>`_" of
      data-containing sites for each composed prim. Value resolution simply
      consumes the ordered (strong-to-weak) list of contributing sites, and is
      otherwise insensitive to the particular set of composition arcs that
      produced that list; but how the data in those sites is combined depends on
      the particular metadatum being resolved.

**Resolving Metadata**

The basic rule for the metadata value resolution provided by
:usdcpp:`UsdObject::GetMetadata` is: **strongest
opinion wins.** Certain metadata such as prim `specifier
<#usdglossary-specifier>`_, attribute typeName, and several others have special
resolution rules; the only one we will discuss here is **dictionary-valued
metadata**, because it is user-facing, as, for example, the :usdcpp:`customData
dictionary <UsdObject::GetCustomData>` authorable on any prim or
property. Dictionaries are resolved element-wise, so that
:cpp:`customData["keyOne"]` authored on a referencing prim will not block, but
rather compose into a single dictionary with :cpp:`customData["keyTwo"]` on the
referenced prim.

**Resolving Relationships** 

Because relationship targets are `list edited <#usdglossary-listediting>`_, we
must, in general, combine *all* of the opinions about the relationship's
targets, not just the strongest. The rules for how the opinions combine (in
weak-to-strong order) are contained inside :usdcpp:`SdfListOp::ApplyOperations`.

**Resolving Attributes** 

Value resolution for attributes, as performed by :usdcpp:`UsdAttribute::Get`, is
unique in three ways:

    #. **Time Offsets** 

       :usdcpp:`UsdAttribute::Get` is a function of time, so all queries except
       those evaluated at :usdcpp:`UsdTimeCode::Default` are affected by
       time-scaling operators such as `Layer Offsets
       <#usdglossary-layeroffset>`_.

    #. **Interpolation** 

       If the requested time coordinate falls between two samples, and the
       :usdcpp:`stage is configured for linear interpolation
       <UsdStage::SetInterpolationType>` (which it
       is by default), then we will `attempt to apply linear interpolation of
       the bracketing timeSamples
       <api/class_usd_attribute.html#Usd_AttributeInterpolation>`_, before
       falling back to holding the earlier of the two timeSamples.

    #. **Three value sources for each site** 
       
       For each site in a prim's Index that may affect a metadatum or
       relationship, there is just a single place to look for a value - if none
       is found, we move on to the next site looking for values. For attributes,
       however, we must examine **three** possible sources for a value for each
       site, before moving on to the next site in strong-to-weak order:

       #. `Value Clips <#usdglossary-valueclips>`_ that are anchored at the site
          or an ancestor site in namespace. If no clips are found, or if clips
          do not provide a value for the attribute, then...

       #. `TimeSamples <#usdglossary-timesample>`_ authored directly at the
          site. If there are no TimeSamples, then...

       #. `Default Value <#usdglossary-defaultvalue>`_ authored directly at the
          site

.. admonition:: Effective use of UsdAttribute::Get()

   The default :cpp:`UsdTimeCode` value for :cpp:`UsdAttribute::Get()` is
   :cpp:`UsdTimeCode::Default()`, which is almost always a poor choice when
   resolving values on a stage that contains animation. When writing code that
   extracts attribute values from a stage, if the codesite is not provided an
   explicit querying time, you should use :cpp:`UsdTimeCode::EarliestTime()`,
   which will ensure that if there is *any* timeSample authored for the
   attribute, it will provide the value, rather than the *default*, which is all
   that is consulted when :cpp:`UsdTimeCode::Default()` is the given time
   coordinate.

.. _usdglossary-variability:

Variability
***********

`Attributes <#usdglossary-attribute>`_ possess a special piece of `metadata
<#usdglossary-metadata>`_ called *variability* that serves as a statement of
intent (typically by a `schema <#usdglossary-schema>`_) of whether the
attribute's value should have `timeSamples <#usdglossary-timesample>`_ that can
vary its value over time, or whether it should be restricted to having only a
`default value <#usdglossary-defaultvalue>`_. Variability can have two values:
:usda:`varying` and :usda:`uniform`; by default, a newly created attribute is
varying (unless you explicitly specify otherwise), and varying attributes appear
"unadorned" in the usda text format. Attributes that are uniform, however, will
appear with the ":usda:`uniform`" modifier in any layer that contains a
properly-authored opinion for the attribute.

Variability is not consulted or consumed by the core during authoring or value
resolution, in order to keep those operations fast. It appears in
:usdcpp:`schema-generated documentation <UsdGeomMesh::GetSubdivisionSchemeAttr>`,
and can be used for validation by higher-level authoring code, and as a hint
to clients that the value is not expected to change over time. See also
:usdcpp:`UsdAttribute::GetVariability`

.. code-block:: usda
   :caption: usda of the uniform attribute "subdivisionScheme" in the Mesh schema

   def Mesh "SimulatableGarment"
   {
       uniform token subdivisionScheme = "loop"
   }

.. _usdglossary-variant:

Variant
*******

A *variant* represents a single, named variation of a `VariantSet
<#usdglossary-variantset>`_; each VariantSet can have zero or a single
**variant selection** expressed in scene description or `as a fallback in a
plugin or specified by an application
<api/class_usd_stage.html#Usd_variantManagement>`_, if
no selection was authored. ":usda:`variants`" is also the keyword in the usda text
syntax for expressing variant selections, as strings. The following valid usda
expresses a selection of ":usda:`High`" for the variantSet named
":usda:`lodVariant`". Variant selections are always strings that refer to
variant names, which can be :usdcpp:`USD identifiers <TfIsValidIdentifier>`
with the addition of allowing the "-" character, and an optional leading ".".

.. code-block:: usda
   :caption: Minimal scene description for expressing a variant selection

   #usda 1.0
   
   over "Model" (
       variants = {
           string lodVariant = "High"
       }
   )
   {
   }

A variant can contain overriding opinions (for properties, metadata, and more),
as well as any arbitrary scene description (entire child prim subtrees, etc).
Variants can also **include additional composition arcs**. This gives us great
flexibility in building up variations out of existing, modular pieces that
can be `referenced <#usdglossary-references>`_, `inherited
<#usdglossary-inherits>`_, etc. In the following example snippet, the 
"referenceVariantSet" VariantSet contains two variants that reference different
USD assets. Changing the variant selection controls which asset is referenced in 
the scene.

.. code-block:: usda
   :caption: VariantSet with references

   over "Model" (
       prepend variantSets = "referenceVariantSet"
       variants = {
          string referenceVariantSet = "asset1"
       }
   )
   {
       variantSet "referenceVariantSet" = {
           "asset1" (
               prepend references = @Asset1.usda@
           ) {          
           }
           "asset2" (
               prepend references = @Asset2.usda@
           ) {          
           }
       }
   }

.. _usdglossary-variantset:

VariantSet
**********

A *VariantSet* is a `composition arc <#usdglossary-compositionarcs>`_ that
allows a content creator to package a discrete set of alternatives, between
which a downstream consumer is able to non-destructively switch, or augment. A
reasonable way to think about VariantSets is as a "switchable reference". Each
`Variant <#usdglossary-variant>`_ of a VariantSet encapsulates a tree of scene
description that will be composed onto the prim on which the VariantSet is
defined, when the Variant is selected. VariantSet names must be 
:usdcpp:`legal USD identifiers <TfIsValidIdentifier>`.
Here is an example of a very simple VariantSet:

.. code-block:: usda
   :caption: simpleVariantSet.usd

   #usda 1.0
   
   def Xform "Implicits" (
       append variantSets = "shapeVariant"
   )
   {
       variantSet "shapeVariant" = {
           "Capsule" {
               def Capsule "Pill"
               {
               }
           }
           "Cone" {
               def Cone "PartyHat"
               {
               }
           }
           "Cube" {
               def Cube "Box"
               {
               }
           }
           "Cylinder" {
               def Cylinder "Tube"
               {
               }
           }
           "Sphere" {
               def Sphere "Ball"
               {
               }
           }
       }
   }

**Things to note about simpleVariantSet.usd**

    * **Variant selections are not required.** 

      If you copy/paste, then open :filename:`simpleVariantSet.usd` in
      :ref:`toolset:usdview`, nothing will be drawn! This is because we have not
      specified a selection for :usda:`shapeVariant` (see `Variant
      <#usdglossary-variant>`_ for what a selection looks like), which is always
      a valid thing to do, and simply means none of the variants contained in
      the VariantSet will be applied. If you select the prim
      :sdfpath:`/Implicits` in :program:`usdview's` browser, and open the
      "Metadata" tab of the inspector in the lower-right-hand corner, you will
      find a drop-down selection box for :usda:`shapeVariant`, from which you
      can select any of the variations and see the viewport update in
      response. Each time you make a selection, usdview is authoring a variant
      selection on :sdfpath:`/Implicits` in the opened stage's `session layer
      <#usdglossary-sessionlayer>`_.

    * **Variants can contain *any* scene description.**

      Variants are not just about overrides! In
      :filename:`simpleVariantSet.usd`, we are creating a
      differently-named-and-typed child prim in each variant. Each variant can
      in fact create an entire subtree of prims (that will be combined on top of
      any referenced scene description), as well as apply overrides.

    * **VariantSets are "inlined" in usda.**

      Although VariantSets, in composition terms, behave very much like
      references and other arcs that target "remote" scene description,
      VariantSets are presented as "inlined scene description" inside the prim
      they modify (or rather, inside the prim, the root of whose subtree the
      VariantSet modifies).

**Characteristics of VariantSets**

    * **A prim can have an unlimited number of VariantSets** declared directly
      on itself; the VariantSets can be ordered (initial order is authored
      order) as part of their `List Editing <#usdglossary-listediting>`_ nature,
      and the final order of the VariantSets provides their relative strength
      with respect to each other, should their opinions overlap.

       ..

    * Because VariantSets are just composition arcs, this implies that
      **VariantSets can be directly nested inside each other**, by allowing a
      Variant of a VariantSet to introduce a new VariantSet. This allows a
      concise encoding for dependent VariantSets, wherein the options/variants
      available for the inner/nested VariantSet depends on the selection of the
      outer VariantSet; `see below for an example
      <#usdglossary-nestedvariants>`_.

       ..

    * Higher-level/downstream contexts can **dynamically add new Variants to
      existing VariantSets.** This ability facilitates the use of VariantSets to
      provide as-you-go asset version control, which can be especially useful,
      for example, in refining simulations or special effects. We might
      reference in a low-res water simulation in a weak "sequence layer" that is
      shared by a number of shots in a sequence. One shot , however, requires a
      closeup, necessitating a higher resolution simulation. If the
      sequence-level simulation was placed inside a :usda:`version` VariantSet,
      in a :usda:`SequenceBase` Variant, then the special shot can, in a layer
      stronger, simply introduce a new :usda:`CloseupShot` Variant to the
      :usda:`version` VariantSet. In that shot, both variants will be available
      and selectable. In other shots, only the :usda:`SequenceBase` Variant will
      be available.

       ..

    * **VariantSets allow optimal scattering of instancing variation**, because
      for any given `instanceable prim <#usdglossary-instanceable>`_, its
      instancing key, which is what we use to decide which `scene-level
      instances <#usdglossary-instancing>`_ will share the same prototype, takes
      the variant selections on the prim into account. So, if you build and
      publish an asset with one-or-more VariantSets on its root prim, an
      environment-building tool can confidently add many instances of the asset
      to a scene, scattering variation by scattering variant-selections, and the
      USD core will ensure only as many unique prototypes are compose as there
      are unique combinations of variant-selections.

.. _usdglossary-nestedvariants:

**Nested VariantSets**

As mentioned above, VariantSets can be nested directly inside each other, on the
same prim. VariantSet nesting in USD can be accomplished straightforwardly by
nesting the use of :usdcpp:`UsdEditContext` objects in code. Following is a
small python program that nests two VariantSets, demonstrating that the contents
of the inner VariantSet can vary from Variant to Variant of the outer
VariantSet.

.. code-block:: python
   :caption: nestedVariants.py

   from pxr import Sdf, Usd
   stage = Usd.Stage.CreateNew("nestedVariants.usd")
   prim = stage.DefinePrim("/Employee")
   title = prim.CreateAttribute("title", Sdf.ValueTypeNames.String)
   variantSets = prim.GetVariantSets()
   
   critters = [ "Bug", "Bear", "Dragon" ]
   jobs = [ "Squasher", "Rider", "Trainer" ]
   
   critterVS = variantSets.AppendVariantSet("critterVariant")
   for critter in critters:
       critterVS.AppendVariant(critter)
       critterVS.SetVariantSelection(critter)
       with critterVS.GetVariantEditContext():
           # All edits now go "inside" the selected critter variant
           jobVS = variantSets.AppendVariantSet("jobVariant")
           for job in jobs:
               if (job != "Squasher" or critter == "Bug") and \
                  (job != "Rider" or critter != "Bug") :
                   jobVS.AppendVariant(job)
                   jobVS.SetVariantSelection(job)
                   with jobVS.GetVariantEditContext():
                       # Now edits *additionally* go inside the selected job variant
                       title.Set(critter + job)

   stage.GetRootLayer().Save()

Try loading the resulting :filename:`nestedVariants.usd` in :program:`usdview`.
Note that as you select different *critterVariants*, the contents of
*jobVariant* will change, as will of course, the resolved value of the
:usda:`title` attribute on the :sdfpath:`/Employee` prim. If you select "Bug"
and "Squasher", respectively, and then change :usda:`critterVariant` to
:usda:`Bear` or :usda:`Dragon"` the :usda:`jobVariant` selection will become
empty, because the selected variant is not a valid selection for the
:usda:`jobVariant` VariantSet defined inside those critter variants. This is not
an error condition, and the result is simply that no :usda:`jobVariant` is
applied.

.. _usdglossary-visibility:

Visibility
**********

*Visibility* is a builtin attribute of the :usdcpp:`UsdGeomImageable` base
schema, and models the simplest form of "pruning" invisibility that is
supported by most DCC apps. Visibility can take on two possible token string
values: 

    * **inherited** (the fallback value if the attribute is not authored)
    * **invisible** 

If the `resolved value <#usdglossary-valueresolution>`_ is **invisible**, then
neither the prim itself nor any prims in the subtree rooted at the prim should
be rendered - this is what we mean by "pruning invisibility", since invisible
subtrees are definitively pruned in their entirety. If the resolve value is
**inherited**, it means that the *computed visibility* (as provided by
:usdcpp:`UsdGeomImageable::ComputeVisibility`) of the prim will be whatever the
computed value of the prim's namespace parent is.

Visibility may be animated, allowing a sub-tree of geometry to be renderable for
some segment of a shot, and absent from others; unlike the action of
`deactivating <#usdglossary-active-inactive>`_ geometry prims, invisible
geometry is still available for inspection, for positioning, for defining
volumes, etc.

