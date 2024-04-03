==========================
Maximizing USD Performance
==========================

.. include:: rolesAndUtils.rst

Internally, Pixar relies on USD for low-latency access, preview, and
introspection of production-scale 3D datasets throughout our film-making
pipeline. We would like to help ensure that USD works as well and with as good
performance for you as it does for us. Here are a few tips that can make a huge
difference.

Use an allocator optimized for multithreading
##############################################

*Most of the tips here are about how to construct and structure your USD data
files, but this one tip is about a software-build issue, and it can make a 2x
difference in USD's performance.*

It requires a significant amount of computation and building of data structures
to load a composition of hundreds to thousands of files, and efficiently extract
the data needed to render a scene. Like many modern systems, USD tries to
leverage multithreaded computations and graph-building as extensively as
possible, and attempting to ensure that the USD code scales as closely to
linearly as possible as we make more compute cores available to it.

Unfortunately, the "malloc" memory allocators that still ship with some popular
compilers do not scale well in multithreaded algorithms that must allocate
memory in many threads concurrently. Our primary experience is on Linux with
gcc's glibc which is based on `ptmalloc
<https://github.com/emeryberger/Malloc-Implementations/tree/master/allocators/ptmalloc/ptmalloc3>`_, 
neither of which hold up very well under high thread-count. At Pixar,
we have been successfully and stably using `jemalloc <http://jemalloc.net/>`_,
which has the following virtues:

    * Outperforms glibc consistently for USD stage loading and imaging through
      Hydra by a factor of **2x** on a 16 core Intel workstation

    * Does an outstanding job of returning freed memory to the kernel

In the `Advanced Build Configuration
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/BUILDING.md>`_
document, you can find instructions for how to build USD linked against a
third-party malloc package such as jemalloc. If you are using USD in a third
party application as an embedded plugin, you can force the application to use a
different allocator on Linux by using the :envvar:`LD_PRELOAD` environment
variable. For example:

.. code-block:: console

   > env LD_PRELOAD=/path/to/jemalloc.so thirdPartyApplication

Use binary ".usd" files for geometry and shading caches
#######################################################

:filename:`.usda` text files can be a good choice for small files
that primarily reference/subLayer other files together, such as the top-level
"asset interface" file that defines a :ref:`Model <glossary:Model>`, provides
its :ref:`AssetInfo <glossary:AssetInfo>`, declares its :ref:`VariantSets
<glossary:VariantSet>`, and contains a :ref:`payload arc <glossary:Payload>`
to the asset's contents. But for files that contain more than a few small
definitions or overrides, the binary :ref:`"usdc" format <glossary:Crate File
Format>` will open faster and consume substantially much less memory while
held open (a :ref:`UsdStage <glossary:Stage>` keeps open all the layers that
participate in a composition). You should not need to exert any extra effort
to get this behavior since creating a new layer or stage with a filename
:filename:`someFile.usd` will, by default, create a usdc file.

As much as your pipeline allows, you should also prefer binary USD files over
Alembic caches, for performance. Alembic is a ground-breaking and outstanding
interchange format for geometry caches, and can perform very well in an
Alembic-based pipeline. However, while USD is committed to supporting Alembic
as an input to composed USD scenes, it cannot perform as well as native USD
files for two reasons:

    #. In order to provide lockless data access in a highly multithreaded
       client, one must configure an Alembic Ogawa archive to open the file
       redundantly as many times as you have threads. Each opened file retains
       a file descriptor, and file descriptors, on Unix systems, are a finite
       resource, especially in applications that still use the *select()* system
       call. We commonly construct USD scenes that reference thousands to tens
       of thousands of files, which would result in hundreds of thousands of
       required file descriptors to make Ogawa competitive with usdc for
       multithreaded data access, where usdc, by default, need retain no file
       descriptors for the files it keeps open. Our `Alembic File Format plugin
       </display/usddoc/alembic+usd+plugin>`_ does not explicitly configure the
       archives it opens, which results in a single file descriptor being
       consumed, but all threads contending to read data from it.

         ..

    #. Although the Alembic and USD schemas and datatypes are mostly equivalent,
       they require translation at the C++ data structure level. So there will
       always be an extra level of copying/translation required when serving
       Alembic-backed data to USD.

You should not need to replace all your Alembic exporters with USD exporters to
achieve this, because the USD File Format plugin system makes it very easy to
convert between supported file formats. To convert a file from Alembic to USD,
one need only use :ref:`usdcat <toolset:usdcat>`:

.. code-block:: console

   > usdcat -o snowman.usd snowman.abc

Package assets with payloads
############################

When dealing with very large scenes, many important pipeline tasks can be
accomplished without knowing about or processing all of the geometry and shading
on many (or all!) of the assets in the scene. These tasks can therefore be
accomplished much more quickly if we can get a view of the scene that does not
populate those aspects of the referenced assets. USD provides a composition arc
called a :ref:`Payload <glossary:Payload>` that is essentially a
"deferred reference". It allows us to structure scenes so that we can open a
:ref:`Stage <glossary:Stage>` "unloaded", meaning that we USD will populate the stage
only "up to" the payload arcs. One effective way to make use of this is to
publish each of your "model assets" such that the file that gets referenced into
assemblies and shots is a very lightweight description of the model's
"interface" (e.g. its :ref:`AssetInfo <glossary:AssetInfo>`,
:ref:`VariantSets <glossary:VariantSet>`, rest bounding box), and
a payload arc to a separate file that pulls in the complete geometric and
shading description of the asset.

When a scene constructed from references to assets built this way is opened
unloaded, you get a summary view of the scene that will contain its :ref:`Model
Hierarchy <glossary:Model Hierarchy>`, which is sufficient for
some entire tasks, and if not, provides all the information necessary to load
*just the model instances required for the task*. Large scenes can take
seconds or minutes to open, but typically the Model Hierarchy view can be opened
in under a second, or a small number of seconds.

The USD distribution includes an `example python script
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/extras/usd/examples/usdMakeFileVariantModelAsset/usdMakeFileVariantModelAsset.py>`_
that demonstrates one simple kind of asset packaging using payloads.

What makes a USD scene heavy/expensive?
#######################################

It is common knowledge that the more geometry you have in a scene, and the more
complex shading you have, the more expensive it will be to render - and that is
true regardless of how you feed the data to the renderer. While we do not have
a mathematical formula for how to structure data in USD for minimal latency and
lowest memory footprint in getting data to a renderer, we do have some
guidelines that we try to keep in mind:

* **Prefer crate files.** 

  As described above, putting big data in :filename:`.usda` files increases
  latency to opening a :ref:`Stage <glossary:Stage>` and memory footprint.

* **Monitor Layer count.**

  The cost and weight of opening a scene scales with the number of files
  that must be opened. Of course, much of the workflow power of USD comes
  from its ability to maintain references to assets, and this caution is not
  meant to inspire anyone to flatten out their sets or environments into a
  single file. Rather, it is to encourage careful consideration of common
  published asset structure. In our own experience, it is all too easy to
  solve an asset authoring / organization problem by throwing new layers at
  it, and the number of published layers **per asset** can quickly rise from
  three or four to ten or more, and those increases multiply by every
  (uniquely) referenced asset in a scene. By the time an asset is ready to
  be published, typically the workflow issues that benefited from many
  layers are no longer relevant, so when possible, try to collapse layers as
  part of a publishing step. Currently, the only provided flattening tools
  are :ref:`usdcat --flatten <toolset:usdcat>` and :ref:`usdstitch
  <toolset:usdstitch>`, each of which are limited in their
  handling of composition arcs. We hope to provide more flattening options
  in the future. You can count and examine the number of layers that are
  required to produce *the current view* of a stage using
  :usdcpp:`UsdStage::GetUsedLayers`.
  .

* **Minimize Prim count.**

  The cost and weight of opening a scene also scales with the ultimate number of
  :ref:`prims <glossary:Prim>` populated on the resultant :ref:`UsdStage
  <glossary:Stage>`, but much, much less so with the number of :ref:`properties
  <glossary:Property>` contained in the scene. This is due to the fact that
  prims can introduce new :ref:`composition arcs <glossary:Composition Arcs>`
  into the scene, and therefore each prim must be uniquely :ref:`indexed
  <glossary:Index>` , and the results cached for later property
  evaluation. This leads to the following guidelines:

    * **Leverage transformable gprims.** 

      Take advantage of the fact that geometric primitives in USD are
      transformable. Do not create an parent :usdcpp:`Xform <UsdGeomXform>` for
      a :usdcpp:`Mesh <UsdGeomMesh>` for the sole purpose of transforming the
      Mesh, since you can transform the Mesh directly using its
      :usdcpp:`Xformable <UsdGeomXformable>`
      properties. The :doc:`Alembic file format plugin <plugins_alembic>`
      collapses "xform + shape" combinations into a single "transformable gprim"
      in UsdGeom. Leveraging this feature of the UsdGeom schemas typically cuts
      the prim count of scenes by 40% to 50%, depending on the scene's branching
      structure.

    * **Use Instancing at higher granularities** . 

      Even if your renderer is only able to instance individual gprims, there is
      a compelling advantage to expressing instancing at a higher granularity
      and simply letting your renderer process the USD prototype redundantly for
      each instance to get at the instanceable gprims. While gprim-level
      instancing is expressible in USD, it provides zero prim-count reduction,
      and actually adds somewhat to the cost of opening a stage since you have
      introduced at least one instancing composition arc onto each "leaf" prim -
      the overhead should not be substantial enough to discourage you from
      instancing at the fine-grain, but rather encourage you to add another
      level of instancing on top of it, such as at the asset level.

    * **Prefer Property namespaces for organization.**

      Properties in USD can be organized into "namespaces", similarly to
      "compound properties" in Alembic. Property namespaces are simply a
      reserved separator character, ":" with API for creating and enumerating
      properties by (multiple) levels of namespacing. User properties, primvars,
      and other "property schemas" leverage property namespaces effectively.

