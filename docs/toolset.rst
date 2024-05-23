###########
USD Toolset
###########

.. include:: rolesAndUtils.rst

*******
usdedit
*******

:program:`usdedit` is a simple script that converts any single USD-readable
file into its (temp) :filename:`.usda` text equivalent and brings the
result up in your editor of choice, which is taken from the :envvar:`EDITOR`
environment variable. Upon quitting the editor, any changes to the temp file
will be converted back to the original file's format (assuming the
:code:`FileFormatPlugin` for the format allows writing), and the original
file's contents will be replaced with the edited contents.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdedit start ====
   :end-before: ==== usdedit end ====

**Notes:**

    * :code:`usdedit foo.abc` works as an alembic editor (in USD schema) for
      all elements of the alembic file that our translator currently
      handles. As we do not yet cover all aspects of the alembic schema,
      however, please be careful and **do not** *usdedit* an alembic file
      being used as a source file, since the roundtripping is lossy!

    * Running :program:`usdedit` on a very large file with lots of dense,
      numeric data may take a long time, create a really large text file in
      your temp area (wherever python's :python:`tempfile` package decides to
      put it), and may push the boundaries of your editor's scalability.

******
usdcat
******

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdcat start ====
   :end-before: ==== usdcat end ====

**Notes:**

    * The multi-file input to :program:`usdcat` does not perform any kind of
      merge of the content in the separate files (and there is no such
      utility to do so, yet... parameterizing that problem is an interesting
      challenge!); it simply dumps the contents of each file, sequentially.

    * The :option:`--flatten` option uses `UsdStage::Export()
      <http://openusd.org/docs/api/class_usd_stage.html#a0185cf581fbcceba34d567c6bc73d351>`_
      , which, as one might expect, "bakes in" the effects of all composition
      operators, simultaneously removing the operators themselves, in the
      result; this applies both to namespace operators like references,
      sublayers, and variants, and also to value-resolution operators like
      layer and reference time offsets. Flattening a stage *does preserve*
      `USD native instancing
      <http://openusd.org/docs/api/_usd__page__scenegraph_instancing.html>`_
      by flattening each prototype into the generated layer and adding
      references on each instance to its corresponding prototype. Thus, the
      exported data may appear structurally different than in the
      participating source files, but should evaluate/compute identically to
      that of the source files.

*******
usddiff
*******

:program:`usddiff` runs a diff program on the result of :program:`usdcat`
'ing two named USD-readable files. It is currently quite primitive, with
limitations noted below, but even so can be quite useful.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usddiff start ====
   :end-before: ==== usddiff end ====

**Notes:**

    * :program:`usddiff` does not do any fuzzy numerical comparison. The
      slightest precision difference will cause a diff.

*******
usdview
*******

:program:`usdview` is the most fully-featured USD tool, combining interactive
gl preview, scenegraph navigation and introspection, a (growing) set of
diagnostic and debugging facilities, and an interactive python interpreter.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdview start ====
   :end-before: ==== usdview end ====

**Further Notes on Command Line Options**

    * :option:`--renderer` : Can be used to select any of the render
      delegates whose plugins have been installed.  The default Hydra
      renderer is Storm.

    * :option:`--select primPath` : loads and images the entire stage,
      but selects *primPath* in the prim browser, and positions the free
      camera as if one had hit the :kbd:`f` ("frame selection") hotkey.

    * :option:`--mask primPath [primPath ...]` : opens the stage in
      masked mode, restricted to these paths for viewing only a subset of the
      prims on a stage.

    * :option:`--norender` : if one does not require a rendered visualization
      of the stage, needing only to access navigation and inspection of the
      data, this option can substantially reduce the startup time and file
      I/O. Expect some errors in the terminal if you should happen to select
      some of the menu/shortcut options, as we have yet to do a cleanup pass
      for this mode. Errors should be harmless, however.

    * :option:`--unloaded` : Populates the stage without including any
      payloads. For a scene constructed of references to models built with
      payloads, this can create a summary :ref:`glossary:Model Hierarchy`
      view of the scene in a small fraction of the time it would take to
      compose and present all the prims in the scene. You can then *load* or
      *unload* subtrees of the prim hierarchy using either the "Edit > Load"
      menu, or the RMB context menu in the browser.

    * :option:`--numThreads` : this will determine how many threads the Hydra
      renderer, boundingBox computer, and other multi-threaded computations
      will be able to use.

    * :option:`--complexity` : Hydra uses OpenSubdiv to refine Mesh gprims
      whose :usda:`subdivisionScheme` is anything other than polygonal. A
      complexity of :code:`low` indicates no subdivision, for maximum
      performance and lowest fidelity.

******************
usdrecord
******************

:program:`usdrecord` is a command-line utility for generating images (or
sequences of images) of a USD stage. Images output by this tool are generated
by Hydra and are equivalent to those displayed in the viewer in `usdview`_ .

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdrecord start ====
   :end-before: ==== usdrecord end ====

**Further Notes on Command Line Options**

    * :option:`--camera` : When a camera name or prim path is given, images
      of the scene will be generated as viewed from that camera. Otherwise, a
      default camera framing all of the geometry on the stage (similar to the
      free camera in `usdview`_ ) will be used.

******************
usdresolve
******************

Command-line ArResolver resolution of asset paths.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdresolve start ====
   :end-before: ==== usdresolve end ====

******************
usdtree
******************

Prints to terminal a unixtree-like summary of a USD layer or composition.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdtree start ====
   :end-before: ==== usdtree end ====

******************
usdzip
******************

Utility for creating :doc:`USDZ packages <spec_usdz>` from USD compositions and
the assets (images and others in future) they reference.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdzip start ====
   :end-before: ==== usdzip end ====

******************
usdchecker
******************

:program:`usdchecker` attempts to validate a USD or usdz file using a series
of rules and metrics that will evolve over time.  This tool currently
provides the best assurance that an asset will be properly interchangeable
and renderable by Hydra.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdchecker start ====
   :end-before: ==== usdchecker end ====

************************
usdfixbrokenpixarschemas
************************

:program:`usdfixbrokenpixarschemas` attempts to fix usd(a|c|z) layers for any
updates introduced by newer Pixar schema revisions. Note that this does not 
provide a fixing mechanism for all validation tests listed in `usdchecker`_.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdfixbrokenpixarschemas start ====
   :end-before: ==== usdfixbrokenpixarschemas end ====

******************
usdstitch
******************

:program:`usdstitch` aggregates any number of usd files into a single
file. This process is different from :ref:`flattening a stage
<glossary:Flatten>` . The key differences are:

    * The result of :program:`usdstitch` will contain the ordered union of
      :ref:`glossary:Composition Arcs` present on prims in the input sequence
      of files, whereas :program:`usdcat --flatten` removes (by evaluating or
      :ref:`flattening <glossary:Flatten>` the
      composed opinions) composition arcs.

    * :program:`usdstitch` merges the :ref:`timeSamples
      <glossary:TimeSample>` from the input sequence of files, whereas
      flattening preserves the TimeSamples of only the strongest layer that
      contains TimeSamples, which is consistent with how :ref:`glossary:Value
      Resolution` interprets layered TimeSamples on a stage.

The goal and purpose of :program:`usdstitch` is to create a time-wise *merge*
of all of its input files, as if each file represents a timeslice of a
complete scene, and we wish to merge all the scenes together. This is neither
equivalent to stage flattening, as just described, nor will it produce an
equivalent result to flattening a :ref:`glossary:LayerStack`, which is the
process of combining all the (nested) :ref:`glossary:SubLayers` of a root
layer into a single layer, preserving *all* composition arcs (other than
subLayers) and guaranteeing the same evaluation behavior, thus (purely)
optimizing N layers into one. One can flatten a LayerStack using `usdcat`_
with its :option:`--flattenLayerStack` option.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdstitch start ====
   :end-before: ==== usdstitch end ====

******************
usdstitchclips
******************

:program:`usdstitchclips` is a tool that produces an aggregate representation
of a common prim (and all its descendant prims) shared across a set of USD
files using the :ref:`glossary:Value Clips` feature. This utility will
provide a Value Clip representation of the input files, which are presumed to
form a sequential animation, determined by the `startTimeCode
<http://openusd.org/docs/api/class_usd_stage.html#a3657dfbec23dce4bc59e890eadbdbe6b>`_
and :usda:`endTimeCode` of each layer in the sequence. It produces two files,
as described in the usage output below:

    * :filename:`result.topology.usd` - this file will contain the unioned
      prim and property topology and metadata of all the input files - that
      is, all prims and properties that appear in *any* of the input USD
      files.

    * :filename:`result.usd` - this file will minimally contain the prim
      hierarchy described by the :option:`--clipPath` command argument, to
      host the Value Clip metadata at *CLIPPATH* . This means that in the
      resulting composition, all prims at or below *CLIPPATH* in namespace
      will receive values from the input clip layers. :filename:`result.usd`
      will also contain a reference, on its root prim (the root path
      component of *CLIPPATH* ) to the same-named prim in
      :filename:`result.topology.usd`

:filename:`result.usd` is suitable for :ref:`sublayering <glossary:SubLayers>`
into a :ref:`Layer Stack <glossary:LayerStack>` whose animation (at
*CLIPPATH* ) is intended to be supplied by the input files.

Note that the animation content of the stitched result is **completely
dependent** on the layout of timeSamples in the input USD files. So, in
particular:

    * If the input files contain only :ref:`default values <glossary:Default
      Value>` , there will be no animation, as Value Clips only look for
      timeSample values.

    * When the :option:`--templateMetadata` argument has **not** been
      specified, then the *startTimeCode* and *endTimeCode* metadata of each
      clip layer will be used to determine the time-range over which that
      clip will be active, and all timeSamples within that range within the
      clip layer will be consumed.

    * When the :option:`--templateMetadata` argument **has** been specified,
      then the range over which each clip will be active is determined by the
      required :option:`--templatePath` argument, and the two optional
      arguments :option:`--stride` and :option:`--activeOffset`:

      * :option:`--templatePath`: A regex-esque template string representing
        the form of our asset paths' names. This can be of two forms:
        :filename:`'path/basename.###.usd'` and
        :filename:`'path/basename.###.###.usd'` . These represent integer
        stage times and sub-integer stage times respectively. In both cases
        the number of hashes in each section is variable, and indicates to
        USD how much padding to apply when looking for asset paths. Note that
        USD is strict about the format of this string: there must be exactly
        one or two groups of hashes, and if there are two, they must be
        adjacent, separated by a dot.

      * :option:`--stride`: An optional (double precision float) number
        indicating the stride at which USD will increment when looking for
        files to resolve. For example, given a start time of 12, an end time
        of 25, a template string of :filename:`'path/basename.#.usd'`, and a
        stride of 6, USD will look to resolve the following paths:
        :filename:`'path/basename.12.usd'`,
        :filename:`'path/basename.18.usd'` and
        :filename:`'path/basename.24.usd'`. **If no stride is specified, USD
        will use the value 1.0.**

      * :option:`--activeOffset`: An optional (double precision float) number
        indicating the offset USD will use when calculating the
        :usda:`clipActive` value that determines the time-range over which
        each clip is active.

        Given a start time of 101, an endTime of 103, a stride of 1, and an
        offset of 0.5, USD will generate the following:

        - clipTimes = [(100.5,100.5), (101,101), (102,102), (103,103), (103.5,103.5)]
        - clipActive = [(101.5, 0), (102.5, 1), (103.5, 2)]

        In other words, an offset allows us to slide the window into each
        clip in which USD will look for timeSamples to map into the stage's
        time, rather than simply using the window implied by each clip's
        filename.  **Note** that the *ACTIVEOFFSET* cannot exceed the
        absolute value of *STRIDE*.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usdstitchclips start ====
   :end-before: ==== usdstitchclips end ====

******************
usddumpcrate
******************

:program:`usddumpcrate` provides information on usd files encoded using
:ref:`USD's Crate File Format <glossary:Crate File Format>`.  This can be
useful for very low-level debugging.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== usddumpcrate start ====
   :end-before: ==== usddumpcrate end ====

******************
sdfdump
******************

Provides information on :ref:`Sdf Layers <glossary:Layer>`, which are the
containers for USD data.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== sdfdump start ====
   :end-before: ==== sdfdump end ====

******************
sdffilter
******************

Provides information in a variety of formats (including usda-like) about
:ref:`Sdf Layers <glossary:Layer>` or specified (filtered) parts of a
layer. Uses range from finding full information about specific properties, to
creating "suitable for web/index display" versions of USD files that elide
large array data.

.. literalinclude:: toolset.help
   :language: none
   :start-after: ==== sdffilter start ====
   :end-before: ==== sdffilter end ====
