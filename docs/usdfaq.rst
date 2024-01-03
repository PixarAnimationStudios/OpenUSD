==============================
USD Frequently Asked Questions
==============================

.. include:: rolesAndUtils.rst

.. contents:: :local:
   :depth: 2

General Questions
=================

What is USD and why should I use it?
####################################

USD stands for "Universal Scene Description." It is a system for encoding
scalable, hierarchically organized, static and time-sampled data, for the
primary purpose of interchanging and augmenting the data between cooperating
digital content creation applications. USD also provides a rich set of
composition operators, including asset and file references and variants, that
let consumers aggregate multiple assets into a single scenegraph while still
allowing for sparse overrides.

See the `Overview and Purpose
<api/_usd__overview_and_purpose.html>`_
section of the USD documentation for a more detailed discussion of what USD is
(and isn't).

What programming languages are supported?
#########################################

USD consists of a set of C++ libraries with Python bindings for scripting.

Isn't USD just another file format?
###################################

No.  While USD does provide interchange services (more below), its
primary function is to compose together scene description spread across many
files (or other data sources) in many different formats (extensible via user
plugins) to present a single hierarchical scenegraph view of the data.  This
:ref:`high-performance scenegraph <glossary:Stage>` helps USD stand on
its own as **a platform for live, collaborative scene construction.**

Well-known file formats for interchange in the VFX industry like `OBJ
<https://www.loc.gov/preservation/digital/formats/fdd/fdd000507.shtml>`_,
`FBX <https://en.wikipedia.org/wiki/FBX>`_, and `Alembic
<https://www.alembic.io/>`_ primarily deal with interchanging flat/cached
geometry, with no facility for assembling assets or editing the data
directly.  USD *does* serve the role of an "interchange file format," more
comprehensively than previous public efforts, because it already interchanges
not only geometry, but also shading/materials, lights, rendering,
linear-blend skinning and blend-shape animation, rigid body physics, and is
extensible along numerous axes.

**But USD possesses key features that distinguish it from interchange file
formats: its composition engine and high-performance scenegraph.** USD's
native file formats encode :ref:`glossary:Composition Arcs` that weave
small (or large) scene components into assemblies, environments, and complete
scenes, wherein each "elemental" asset can be efficiently shared between
instances and scenes, and retain its integrity thanks to USD's many
non-destructive editing features.  USD's many plugin points, including
`customizable asset-resolution behavior <api/ar_page_front.html>`_, the
ability to add new data schemas and associated imaging behaviors, and the
ability to teach USD how to read other file formats, provide great power in
adapting USD into existing pipelines and infrastructure.

Pixar continues to focus on the scalability and performance of USD's
:ref:`live scenegraph <glossary:Stage>`, which presents the "composed view"
of all the references, layerings, variations, and other *arcs* encoded in the
scene's assets.  But in addition to providing efficient, on-demand access to
all data contained in the scene, the scenegraph is also *interactive*,
allowing both destructive and non-destructive scene editing, **with features
designed to foster multi-user collaboration.** These features, in combination
with the modern :ref:`glossary:Hydra` rendering architecture that sits on top
of USD's scenegraph and is included in the USD distribution, have attracted a
growing number of software vendors to use USD in its "live" capacity as an
integral part of the user-experience, rather than simply as an interchange.


So what file formats does USD support?
######################################

Out of the box, USD the following three file formats:

  +--------------------------------+--------------------------------+
  | Extension                      | Content                        |
  +================================+================================+
  | :filename:`.usda`              | Human-readable UTF-8 text      |
  +--------------------------------+--------------------------------+
  | :filename:`.usdc`              | Random-access "Crate" binary   |
  +--------------------------------+--------------------------------+
  | :filename:`.usd`               | Either of the above            |
  +--------------------------------+--------------------------------+

It also includes plugins that provide support for
reading and writing :doc:`Alembic <plugins_alembic>` files, and for reading
`MaterialX files <http://www.materialx.org/>`_.  Developers can extend
USD to read and write from other file formats by writing a plugin that will
translate the data in that format to the USD scenegraph.

What file format is my :filename:`.usd` file?
#############################################

As shown in the table above, USD files created with the plain
:filename:`.usd` extension, can be either binary-backed
:ref:`Crate <glossary:Crate File Format>` files, or text files. 
This can be advantageous in certain scenarios; for example, if one has USD
files which contain references to plain :filename:`.usd` assets, those assets
can be converted between binary and text without changing the sources which
refer to them.

What character encoding does :filename:`.usda` support?
#######################################################

The :filename:`.usda` file format encodes text as UTF-8.

How can I convert USD files between binary and text?
####################################################

:ref:`usdcat <toolset:usdcat>` can be used to convert files between file
formats. With plain, :filename:`.usd` files, you can change them using the
:option:`--usdFormat` option. 

.. note::

  Although you can specify :option:`--usdFormat usdc`, it is not necessary, since
  the :option:`--out file.usd` argument will, by default choose the usdc format.

.. code-block:: sh

   $ usdcat file.usd --out file.usd --usdFormat usda

Alternatively, one can convert files using the explicit file extensions, such as
:filename:`usda` and :filename:`usdc`, directly.

.. code-block:: sh

   $ usdcat file.usd --out file.usdc

For more information on USD file formats, see
:doc:`tut_converting_between_layer_formats`.

What data types are supported?
##############################

See the `Basic Datatypes for Scene Description
<api/_usd__page__datatypes.html>`_ section of
the USD documentation for a complete list of supported data types. USD does
**not** support the addition of new basic datatypes via plugin.

What does a USD file look like?
###############################

Here's an example using references, inheritance, and variants. This is the
representation generated by the native text file format, whose extension is 
:filename:`.usda`.

.. code-block:: usda
   :caption: solarSystem.usda

   #usda 1.0
   
   class "_class_Planet"
   {
       bool has_life = False
   }
   
   def Xform "SolarSystem"
   {
       def "Earth" (
           references = @./planet.usda@</Planet>
       )
       {
           bool has_life = True
           string color = "blue"
       }
   
       def "Mars" (
           references = @./planet.usda@</Planet>
       )
       {
           string color = "red"
       }
   
       def "Saturn" (
           references = @./planet.usda@</Planet>
           variants = {
               string rings = "with_rings"
           }
       )
       {
           string color = "beige"
       }
   }

.. code-block:: usda
   :caption: planet.usda

   #usda 1.0
   
   class "_class_Planet"
   {
   }
   
   def Sphere "Planet" (
       inherits = </_class_Planet>
       kind = "model"
       variantSets = "rings"
       variants = {
           string rings = "none"
       }
   )
   {
       variantSet "rings" = {
           "none" {
               bool has_rings = False
           }
           "with_rings" {
               bool has_rings = True
           } 
       }
        
   }

This example is purely illustrative. It does not make use of all of the
composition operators that USD provides and should not necessarily be taken as a
model for how assets should be structured. Different users have different needs;
USD simply provides the data encoding and composition operators and lets users
decide how best to construct their assets.

The :doc:`tut_usd_tutorials` page contains more examples as well as
demonstrations of how to construct those examples using the provided APIs.

Subtler Aspects of Scene Description and Composition
====================================================

I have some layers I want to combine: Should I use SubLayers or References?
###########################################################################

If you have two or more layers that you want to combine so that they override in
a particular order, you have two simple options, :ref:`glossary:SubLayers` and
:ref:`glossary:References`. Which one you should choose depends on both the
contents of the layers and what you will need to do with the resulting
composition.

**If it satisfies your requirements, prefer sublayering over referencing.**

Sublayering has the following advantages:

    * Sublayering is the simplest :ref:`Composition Arc <glossary:Composition Arcs>` 
      and results in the most compact runtime data structures and efficient
      :ref:`glossary:Value Resolution`.

    * Layers combined via sublayering can easily participate in 
      :usdcpp:`UsdStage Layer Muting <UsdStage::MuteLayer>`.

    * Choosing layers to edit via :ref:`Edit Targets <glossary:EditTarget>`
      is most straightforward for layers combined via sublayering.

**What requirements might make SubLayering inappropriate?**

    * If the layers you need to combine to do not share the same namespace,
      sublayering will simply "overlay" the different namespaces without any
      possibility of renaming parts of the namespace. By contrast, references
      allow you to remap specific prims into a different namespace. For example,
      the prim :sdfpath:`/Bob` in the file :filename:`Bob.usd` can be
      *renamed* to :sdfpath:`/Bob_1` as it is referenced onto the prim
      :sdfpath:`/Characters/Bob_1` in the file :filename:`lineup.usd`.

       ..

    * Related to the first point, if one or more of your layers contains root
      prims that you do not want "exposed" in the resulting composition,
      references provide a means of doing so: because each time you add a
      reference you target a specific prim within a layer, you may pick out just
      the prims you want/need in the composed result. You can add as many
      references (even on the same prim) as you require, targeting different
      prims in the same layer - even targeting prims in the same layer in which
      you are adding the reference.

       ..

    * If you want to add :ref:`VariantSets <glossary:VariantSet>`
      that override the result of your composition, you will need to reference
      your results together rather than sublayer them - or add another layer
      into the mix so that you can reference in the root layer of your sublayer
      stack. This is because the data in the layers that you SubLayer form the
      **L** in :ref:`LIVRPS Strength Ordering <glossary:LIVRPS Strength Ordering>`, 
      whereas opinions from VariantSets form the **V**, so anything defined in
      VariantSets will be *weaker* than the data in your sublayers. By
      extension, if you want to compose several layers into the existing file
      :filename:`root.usd`, if you sublayer them into :filename:`root.usd`,
      the direct opinions they contain will be stronger than any Inherits or
      VariantSet opinions in :filename:`root.usd`, whereas if you reference
      them in, their opinions will be weaker than any Inherits or VariantSets.

What happens to "overs" when their underlying prim is moved to a different location in the scenegraph?
######################################################################################################

USD allows sparse overrides on assets that are brought in to the scenegraph via
one of the various composition operators. These overrides are referred to as
"overs." For instance, consider the following example involving sublayers:

.. code-block::usda
   :caption: shot.usda

   #usda 1.0
   (
       subLayers = [
            @sequence.usda@            
       ]
   )
   
   over "World"
   {
       over "Sets"
       {
           over "Bookshelf"
           {
               over "Book_1"
               {
                   string name = "Wall-E"
               }
           }
       } 
   }

.. code-block:: usda
   :caption: sequence.usda

   #usda 1.0
   
   def Xform "World"
   {
       def Xform "Sets"
       {
           def Xform "Bookshelf"
           {
               def Xform "Book_1"
               {
                   string name = "Toy Story"
               }
           }
   
           def Xform "Desk"
           {
           }
       }
   }

In this example, the book defined in :filename:`sequence.usda` has the title
"Toy Story". However, this layer is brought in to shot.usda via a sublayer
statement, and the book's title is overridden to "Wall-E." The question is: what
happens if a user independently working on :filename:`sequence.usda` moves
:mono:`Book_1` to :mono:`Desk`, or if :mono:`Book_1` is renamed to
:mono:`Video_1`?  In such a case, the "over" in :filename:`shot.usda` would
be "orphaned" and be ignored when composing and evaluating
:sdfpath:`/World/Sets/Desk/Book_1` in :filename:`shot.usda`. It is the
responsibility of the user working on :filename:`sequence.usda` to ensure that
:filename:`shot.usda` is updated to avoid this problem.

When can you delete a reference (or other deletable thing)?
###########################################################

USD allows several kinds of data (including :ref:`composition arcs<glossary:Composition Arcs>`) 
to be :ref:`list-edited <glossary:List Editing>`, i.e. prepending, appending, and
deleting elements in a strong layer from a list that has been built up in weaker
layers. In addition to all composition arcs except :ref:`SubLayers <glossary:SubLayers>`,
you can also list-edit :ref:`Relationships <glossary:Relationship>`, 
:ref:`Attribute Connections <glossary:Attribute Connection>`, and custom
string, token, and int metadata declared in :filename:`plugInfo.json` files. For
any datatype that *can* be list-edited, it is syntactically legal and
semantically meaningful to place a **delete** operator at any site (any path in
any layer) on which the data can be hosted. What **effect** the delete will have
depends on the type of the data, and is different for each of three categories
of data.

List-edited string, token, and int metadata
*******************************************

Custom, list-edited metadata is the simplest case. Items can be removed from
the list (actually *ordered set*, since list-editing in USD de-duplicates items
added more than once) anywhere in the composition graph.

List-edited relationships and connections
*****************************************

Similarly to custom, list-edited metadata, :ref:`relationship 
<glossary:Relationship>` and :ref:`connection <glossary:Connection>` targets can
be removed (and added!)  anywhere in a composition graph. However, because most
authoring takes place in the root :ref:`layerStack <glossary:LayerStack>` of 
a composed :ref:`UsdStage <glossary:Stage>`, we stipulate that when you delete a
relationship or connection target, you specify **not** the originally authored
path, but the translation of the path into the root layerStack. This is because
one of the key behaviors USD provides as part of "composing a scene" is making
sure that all paths are presented to you in the final namespace of the Stage
that you open; See :ref:`Path Translation <glossary:Path Translation>` for an
example that includes deleting a relationship target that was defined "across"
two levels of referencing.

List-edited composition arcs
****************************

Meaningfully deleting composition arcs imposes some restrictions, because of the
central aspect of *encapsulation* in USD composition. Encapsulation means that
whenever you use a references, payload, inherits, variantSet, or specializes arc
on a prim, **the result that gets woven into the composition is immutable by
stronger layers** - values can be overridden, and objects can be added, but the
"internal referencing structure" of the arc's target cannot be changed. This is
important for several reasons:

    * It makes the :ref:`LIVRPS composition algorithm <glossary:LIVRPS Strength
      Ordering>` explainable and (more easily) understandable, because it is
      properly recursive.

    * It enables more efficient composition engine implementations, because it
      allows great sharing of composition (sub)results

    * It preserves strength ordering of the internal referencing structure - we
      will explain this more below.

The rule, therefore, for meaningfully deleting composition arcs, is that you can
only remove an arc if it was introduced in the same layerStack, :ref:`as
discussed with an example here <glossary:LayerStack>`. This means you cannot
delete a reference that was introduced from across another reference.

There is a single exception to composition encapsulation in which we do allow a
stronger layer to control how composition across an arc behaves, which is
:ref:`variant selections <glossary:VariantSet>`, and we *can*, with some
preparatory work, use them to *effectively* delete arcs from across other arcs,
though doing so may produce undesirable results! The whole point of variantSets
in USD is to be able to make a *selection,* non-destructively, in an overriding
layer, that will "pierce composition encapsulation" to guide which variant of a
variantSet should be chosen and composed, regardless of how distant (in terms of
composition arcs traversed) the actual variant definition(s) may be from the
selection.

Therefore, in a context in which we would ordinarily just add a reference, if we
instead create a variantSet with a single variant, and add the reference on the
variant instead of on the prim itself, then in a stronger referencing context
across an arbitrary number of arcs, we can simply add and select another variant
to the variantSet, and add a different reference there. Because no such variant
exists *inside* the referenced asset, the original reference will not be
processed.

While this "works", it creates a "strength inversion" that will likely be
undesirable in most scenarios, and is a key reason why we believe encapsulation
to be so valuable. The reference that you are adding in the outer referencing
context will now be stronger than the context (and opinions) from which the
original reference was introduced, so any overrides that context applied will no
longer be overrides - they will instead be "underrides" because they are
weaker. If you think of this in terms of building an environment out of
referenced component models, you will see the problem with employing this
technique at a shot-level in which the environment is referenced in: the
environment places each object with transform overrides. But those overrides
will now be weaker than the default transformation of the "swapped in" component
at the shot level, and so its positioning will need to be redone or recovered
manually from the environment layer. This will be true for all overrides the
environment makes for each component-to-be-replaced. Following is a (very
contrived, for brevity) example of how this technique works and doesn't
work. Given:

.. code-block:: usda
   :caption: assets.usd

   #usda 1.0
      
   def Xform "CubeModel"
   {
       custom int levelOfNesting = 0
      
       def Cube "Gprim"
       {
       }
   }
   
   def Xform "SphereModel"
   {
       custom int levelOfNesting = 0
   
       def Sphere "Gprim"
       {
       }
   }

then we can construct an assembly that dresses in an instance of one of the
models, inside a variantSet:

.. code-block:: usda
   :caption: assembly.usd

   #usda 1.0
   (
       defaultPrim = "FlexibleSet"
   )
   
   def Xform "FlexibleSet"
   {
       def "Model_1" (
           prepend variantSets = "dressingVariant"
           variants = {
               string dressingVariant = "default"
           }
       )
       {
           variantSet "dressingVariant" = {
               "default" (
                   references = @assets.usd@</CubeModel>
               )
               {
               }
           }
   
           # Anything added at the set level has nesting level 1
           custom int levelOfNesting = 1
       }
   }

Finally, in a shot, we can replace the assembly's reference at
:mono:`Model_1` by adding a new variant for *dressingVariant* and adding a
new reference inside the variant:

.. code-block:: usda
   :caption: shot.usd

   #usda 1.0
   (
       defaultPrim = "World"
   )
   
   def Xform "World"
   {
       def "ShapeFactory" (
           references = @assembly.usd@
       )
       {
           over "Model_1" (
               prepend variantSets = "dressingVariant"
               variants = {
                   string dressingVariant = "shotOverride_1"
               }
           )
           {
               variantSet "dressingVariant" = {
                   "shotOverride_1" (
                       references = @assets.usd@</SphereModel>
                   )
                   {
                   }
               }
           }
       }
   }

If one were to inspect :filename:`shot.usd` in :program:`usdview's` Composition
inspector, one would find no trace of :sdfpath:`/CubeModel` from
:filename:`assets.usd` in the :ref:`Index <glossary:Index>` for
:sdfpath:`/World/ShapeFactory/Model_1`, so we *have* succesfully replaced it
from across a reference. However, on the composed stage, the value of
:sdfpath:`/World/ShapeFactory/Model_1.levelOfNesting` has been reset to zero,
which is unintuitive and almost certainly undesirable; The value of one is still
present in the property's :ref:`PropertyStack <glossary:PropertyStack>` , but it
is now weaker than the value coming from the referenced leaf asset. By toggling
the selection for *dressingVariant* in the Metadata inspector, you can see the
value of *levelOfNesting* bounce between zero and one.

We neither endorse nor discourage use of this construct, but simply wish to
emphasize that it should be approached with care, when its use is warranted.

What's the difference between an "over" and a "typeless def" ?
##############################################################

In a lot of usd files, I see something like:

.. code-block:: usda

   def "MyModel" (
       payload = @./MyModel_payload.usd@
   )
   {
   }

Since we're not saying what type of prim "MyModel" is at this level, why isn't
it just

.. code-block:: usda

   over "MyModel"

**Answer** : When you *def* a prim, you're providing more information to the
system than when you *over* a prim.

    * **def** means "Even though I may not be declaring a typed prim in this
      layer, by the time the :usdcpp:`UsdStage` has finished composing, I
      **expect** a type to have been provided by referenced/layered scene
      description" (in the example, presumably the type would be provided by the
      layer targeted by the *payload* arc.

    * **over** means "If some referenced layer happens to define this prim, then
      layer the information I contain onto it; but if not, just consider this
      information ancillary"

This distinction is actually used in the Usd core to define, for e.g., default
stage traversal behavior, as :usdcpp:`UsdPrim::GetChildren`
only iterates over a prim's *defined* children (regardless of whether they
possess a type in the current view of the stage), skipping prims that are just
*over*.

Why Can't I Instance a Leaf Mesh Prim Directly?
###############################################

:ref:`USD Native Instancing <glossary:Instancing>` makes no restrictions on what
"source scene description" can and cannot be instanced. A UsdStage is perfectly
happy to have instances that target a single, leaf, source prim, although the
"implicit prototype" that gets created as a result will be empty, and for that
reason the instancing, in such a case, is meaningless, from a composition-cost
and data sharing perspective. Perhaps most importantly, it is also meaningless
from the perspective of being able to assume that *anything* is actually the
same or similar between two instances of the same implicit prototype, because you
can override **any** property or metadata of any instance - only prims **beneath
the implicit prototype's root** are protected from being overridden, 
per-prototype, and in this case there are none.

Our imaging system, :ref:`Hydra <glossary:Hydra>`, cares very much about that
last property, or lack thereof. Every renderer of which we're aware that
supports instancing requires that the basic geometric properties of a Mesh (or
other gprim) **must be unvarying** across instances since it is the tessellation
and storage/indexing costs of said geometry that we are trying to share for
scalability. The UsdImaging Hydra scene delegate, therefore, imposes a further
restriction on "meaningful" instancing and considers any "leaf instanced gprims"
to be an error condition for which we issue a warning and do not image
anything. To do otherwise would force the scene delegate to compare/hash all the
"relevant" properties up-front to create buckets of "really instanceable
instances", since Hydra passes instances to render delegates in an explicit form
that would otherwise assume the characteristics of the first instance it sees,
dropping any variation in the other instances, thus potentially losing artistic
intent, and producing non-deterministic results.  Therefore we felt it better to
treat this as an error condition, since we do **not** want to impose full
instance deduplication on the scene delegate, and it is part of the "USD
philosophy" that consumers of scene description should not need to care about
where in the composition graph particular opinions come from.

Some DCC's that support instancing do so with an "explicit", relationship-based
(or equivalent) scheme, and in such schemes (as with :cpp:`UsdGeomPointInstancer`),
single gprim instancing is not problematic, because the "overrides" (which are
generally selective/restrictive) are on separate prims from the prototype prims,
and therefore the overrides are easy to identify. We realize that in this
specific case, the USD model of instancing may be less user-friendly at first
glance, but we wanted our instancing mechanism to be a natural extension of
USD's powerful composition operators, which provides pipeline benefits (such as
the ability to de-instance and re-instance with a trivial, non-destructive edit,
without losing any overrides) and scalability benefits (as mentioned in the
glossary entry, instances from different assets can "unify" and share prototypes
when assembled into an aggregate or scene), and the reality for Pixar is that we
rarely instance single gprims, rather instancing entire models or parts of
models.

Build and Runtime Issues
========================

Why Isn't Python Finding USD Modules?
#####################################

If you get an error message when using USD's Python bindings like this:

.. code-block:: pycon

   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   ImportError: No module named pxr

Make sure that the :envvar:`PYTHONPATH` environment variable includes
the directory containing the USD Python modules:

.. code-block:: sh

   $ export PYTHONPATH=$PYTHONPATH:<inst>/lib/python

Where :file:`<inst>` represents the :makevar:`CMAKE_INSTALL_PREFIX` set in your
build configuration. See `Advanced Build Configuration
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/BUILDING.md>`_ for
more information.

Why Isn't This Plugin Being Built?
##################################

The plugins included in the USD distribution are disabled by default.
See `Advanced Build Configuration
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/BUILDING.md>`_ for
instructions on how to enable these plugins.

Why Isn't My App Finding USD DLLs and Plugins on Windows?
#########################################################

If you get errors similar to "This program can't start because USD_AR.DLL is 
missing from your computer" or "Failed to load plugin 'usd': The specified 
module could not be found" when running your app, this means the USD DLLs or 
USD plugins can't be located at runtime. 

One way to fix this is to install the USD DLLs and USD plugins in the same 
directory as your executable. For example, if you built ``MyCustomUSDTool.exe``
and installed it into ``C:\MyPrograms\MyCustomUSDTool\``, your directory layout 
might look something like:

.. code-block:: text

  C:\MyPrograms\MyCustomUSDTool\MyCustomTool.exe
  C:\MyPrograms\MyCustomUSDTool\tbb.dll
  C:\MyPrograms\MyCustomUSDTool\usd_ar.dll
  C:\MyPrograms\MyCustomUSDTool\(...remaining USD DLLs...)
  C:\MyPrograms\MyCustomUSDTool\usd\plugInfo.json
  C:\MyPrograms\MyCustomUSDTool\usd\ar\resources\plugInfo.json
  C:\MyPrograms\MyCustomUSDTool\usd\(...remaining core USD plugin directories and files...)
  C:\MyPrograms\MyCustomUSDTool\usd\(...other non-core plugin directories as needed...)

Another approach is to install your DLLs and plugins to a directory that is 
added to your Windows :envvar:`PATH` environment variable. See `Dynamic-link 
library search order <https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order>`_
for more details on how Windows searches for DLLs at runtime.

If you prefer not to have to install a large number of DLLs, you can build a 
single monolithic USD DLL using the :code:`PXR_BUILD_MONOLITHIC` cmake flag. See 
`Advanced Build Configuration 
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/BUILDING.md>`_ for
more information. Note that you will still need to install the USD plugins 
directory along with the monolithic USD DLL.
