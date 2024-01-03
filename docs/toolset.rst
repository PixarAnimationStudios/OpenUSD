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

.. code-block:: none

   > usdedit -h
   
   usage: usdedit [-h] [-n] [-f] usdFileName
   
   Convert a usd-readable file to the .usda text format in a temporary
   location and invoke an editor on it. After saving and quitting the editor,
   the edited file will be converted back to the original format and
   OVERWRITE the original file, unless you supply the "-n" (--noeffect) flag,
   in which case no changes will be saved back to the original file. The
   editor to use will be queried from the EDITOR environment variable.
   
   positional arguments:
     usdFileName           The usd file to edit.
   
   optional arguments:
     -h, --help           Show this help message and exit
     -n, --noeffect       Do not edit the file.
     -f, --forcewrite     Override file permissions to allow writing.

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

.. code-block:: none

   > usdcat -h
   usage: usdcat [-h] [-o file] [--usdFormat usda|usdc] [-l] [-f]
                 [--flattenLayerStack] [--skipSourceFileComment]
                 [--mask PRIMPATH[,PRIMPATH...]] [--layerMetadata]
                 inputFiles [inputFiles ...]
   
   Write usd file(s) either as text to stdout or to a specified output file.
   
   positional arguments:
     inputFiles
   
   optional arguments:
     -h, --help            show this help message and exit
     -o file, --out file   Write a single input file to this output file instead
                           of stdout.
     --usdFormat usda|usdc
                           Use this underlying file format for output files with
                           the extension 'usd'. For example, passing '-o
                           output.usd --usdFormat usda' will create output.usd as
                           a text file. The USD_DEFAULT_FILE_FORMAT environment
                           variable is another way to achieve this.
     -l, --loadOnly        Attempt to load the specified input files and report
                           'OK' or 'ERR' for each one. After all files are
                           processed, this script will exit with a non-zero exit
                           code if any files failed to load.
     -f, --flatten         Compose stages with the input files as root layers and
                           write their flattened content.
     --flattenLayerStack   Flatten the layer stack with the given root layer, and
                           write out the result. Unlike --flatten, this does not
                           flatten composition arcs (such as references).
     --skipSourceFileComment
                           If --flatten is specified, skip adding a comment
                           regarding the source of the flattened layer in the
                           documentation field of the output layer.
     --mask PRIMPATH[,PRIMPATH...]
                           Limit stage population to these prims, their
                           descendants and ancestors. To specify multiple paths,
                           either use commas with no spaces or quote the argument
                           and separate paths by commas and/or spaces. Requires
                           --flatten.
     --layerMetadata       Load only layer metadata in the USD file. This option
                           cannot be combined with either --flatten or
                           --flattenStack

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

.. code-block:: none

   > usddiff -h
   usage: usddiff [-h] [-n] [-f] files [files ...]
   
   Compares two usd-readable files using a selected diff program. This is chosen
   by looking at the $USD_DIFF environment variable. If this is unset, it will
   consult the $DIFF environment variable. Lastly, if neither of these is set, it
   will try to use the canonical unix program, diff. This will relay the exit
   code of the selected diff program.
   
   positional arguments:
     files           The files to compare. These must be of the form DIR DIR,
                     FILE... DIR, DIR FILE... or FILE FILE.
   
   optional arguments:
     -h, --help      show this help message and exit
     -n, --noeffect  Do not edit either file.
     -f, --flatten   Fully compose both layers as Usd Stages and flatten into
                     single layers.
     -q, --brief     Do not return full results of diffs. Passes --brief to the
                     diff command.

**Notes:**

    * :program:`usddiff` does not do any fuzzy numerical comparison. The
      slightest precision difference will cause a diff.

*******
usdview
*******

:program:`usdview` is the most fully-featured USD tool, combining interactive
gl preview, scenegraph navigation and introspection, a (growing) set of
diagnostic and debugging facilities, and an interactive python interpreter.

.. code-block:: none

   > usdview -h
   usage: usdview
          [-h] [--renderer {GL,Embree,Prman}]
          [--select PRIMPATH] [--camera CAMERA] [--mask PRIMPATH[,PRIMPATH...]]
          [--clearsettings] [--defaultsettings] [--norender] [--noplugins]
          [--unloaded] [--timing] [--memstats {none,stage,stageAndImaging}]
          [--numThreads NUMTHREADS] [--ff FIRSTFRAME] [--lf LASTFRAME]
          [--cf CURRENTFRAME] [--complexity {low,medium,high,veryhigh}]
          [--quitAfterStartup] [--sessionLayer SESSIONLAYER]
          [--mute MUTELAYERSRE]
          usdFile
   
   View a usd file
   
   positional arguments:
     usdFile               The file to view
   
   optional arguments:
     -h, --help            show this help message and exit
     --renderer {GL,Embree,Prman}
                           Which render backend to use (named as it appears in
                           the menu).
     --select PRIMPATH     A prim path to initially select and frame
     --camera CAMERA, -cam CAMERA
                           Which camera to set the view to on open - may be given
                           as either just the camera's prim name (ie, just the
                           last element in the prim path), or as a full prim
                           path. Note that if only the prim name is used, and
                           more than one camera exists with the name, which is
                           used will be effectively random (default=main_cam)
     --mask PRIMPATH[,PRIMPATH...]
                           Limit stage population to these prims, their
                           descendants and ancestors. To specify multiple paths,
                           either use commas with no spaces or quote the argument
                           and separate paths by commas and/or spaces.
     --clearsettings       Restores usdview settings to default
     --defaultsettings     Launch usdview with default settings
     --norender            Display only hierarchy browser
     --noplugins           Do not load plugins
     --unloaded            Do not load payloads
     --timing              echo timing stats to console. NOTE: timings will be
                           unreliable when the --mallocTagStats option is also in
                           use
     --memstats {none,stage,stageAndImaging}
                           Use the Pxr MallocTags memory accounting system to
                           profile USD, saving results to a tmp file, with a
                           summary to the console. Will have no effect if
                           MallocTags are not supported in the USD installation.
     --numThreads NUMTHREADS
                           Number of threads used for processing(0 is max,
                           negative numbers imply max - N)
     --ff FIRSTFRAME       Set the first frame of the viewer
     --lf LASTFRAME        Set the last frame of the viewer
     --cf CURRENTFRAME     Set the current frame of the viewer
     --complexity {low,medium,high,veryhigh}, -c {low,medium,high,veryhigh}
                           Set the initial mesh refinement complexity (low).
     --quitAfterStartup    quit immediately after start up
     --sessionLayer SESSIONLAYER
                           If specified, the stage will be opened with the
                           'sessionLayer' in place of the default anonymous
                           layer. As this changes the session layer from
                           anonymous to persistent, be aware that layers saved
                           from Export Overrides will include the opinions in the
                           persistent session layer.
     --mute MUTELAYERSRE   Layer identifiers searched against this regular
                           expression will be muted on the stage prior to, and
                           after loading. Multiple expressions can be supplied
                           using the | regex separator operator. Alternatively
                           the argument may be used multiple times.
     
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

.. code-block:: none

   > usdrecord -h
   usage: usdrecord [-h] [--mask PRIMPATH[,PRIMPATH...]] [--camera CAMERA]
                    [--defaultTime | --frames FRAMESPEC[,FRAMESPEC...]]
                    [--complexity {low,medium,high,veryhigh}]
                    [--colorCorrectionMode {disabled,sRGB,openColorIO}]
                    [--renderer {GL,Embree,Prman}]
                    [--imageWidth IMAGEWIDTH]
                    usdFilePath outputImagePath
   
   Generates images from a USD file
   
   positional arguments:
     usdFilePath           USD file to record
     outputImagePath       Output image path. For frame ranges, the path must
                           contain exactly one frame number placeholder of the
                           form "###" or "###.###". Note that the number of hash
                           marks is variable in each group.
   
   optional arguments:
     -h, --help            show this help message and exit
     --mask PRIMPATH[,PRIMPATH...]
                           Limit stage population to these prims, their
                           descendants and ancestors. To specify multiple paths,
                           either use commas with no spaces or quote the argument
                           and separate paths by commas and/or spaces.
     --camera CAMERA, -cam CAMERA
                           Which camera to use - may be given as either just the
                           camera's prim name (i.e. just the last element in the
                           prim path), or as a full prim path. Note that if only
                           the prim name is used and more than one camera exists
                           with that name, which one is used will effectively be
                           random (default=main_cam)
     --defaultTime, -d     explicitly operate at the Default time code (the
                           default behavior is to operate at the Earliest time
                           code)
     --frames FRAMESPEC[,FRAMESPEC...], -f FRAMESPEC[,FRAMESPEC...]
                           specify FrameSpec(s) of the time codes to operate on -
                           A FrameSpec consists of up to three floating point
                           values for the start time code, end time code, and
                           stride of a time code range. A single time code can be
                           specified, or a start and end time code can be
                           specified separated by a colon (:). When a start and
                           end time code are specified, the stride may optionally
                           be specified as well, separating it from the start and
                           end time codes with (x). Multiple FrameSpecs can be
                           combined as a comma-separated list. The following are
                           examples of valid FrameSpecs: 123 - 101:105 - 105:101
                           - 101:109x2 - 101:110x2 - 101:104x0.5
     --complexity {low,medium,high,veryhigh}, -c {low,medium,high,veryhigh}
                           level of refinement to use (default=high)
     --colorCorrectionMode {disabled,sRGB,openColorIO}, -color {disabled,sRGB,openColorIO}
                           the color correction mode to use (default=sRGB)
     --renderer {GL,Embree,Prman}, -r {GL,Embree,Prman}
                           Hydra renderer plugin to use when generating images
     --imageWidth IMAGEWIDTH, -w IMAGEWIDTH
                           Width of the output image. The height will be computed
                           from this value and the camera's aspect ratio
                           (default=960)

**Further Notes on Command Line Options**

    * :option:`--camera` : When a camera name or prim path is given, images
      of the scene will be generated as viewed from that camera. Otherwise, a
      default camera framing all of the geometry on the stage (similar to the
      free camera in `usdview`_ ) will be used.

******************
usdresolve
******************

Command-line ArResolver resolution of asset paths.

.. code-block:: none

   > usdresolve -h
   
   usage: usdresolve [-h] [--configureAssetPath CONFIGUREASSETPATH]
                     [--anchorPath ANCHORPATH]
                     inputPath
   
   Resolves an asset path using a fully configured USD Asset Resolver.
   
   positional arguments:
     inputPath             An asset path to be resolved by the USD Asset
                           Resolver.
   
   optional arguments:
     -h, --help            show this help message and exit
     --configureAssetPath CONFIGUREASSETPATH
                           Run ConfigureResolverForAsset on the given asset path.
     --anchorPath ANCHORPATH
                           Run AnchorRelativePath on the given asset path.

******************
usdtree
******************

Prints to terminal a unixtree-like summary of a USD layer or composition.

.. code-block:: none

   > usdtree -h
   usage: usdtree [-h] [--unloaded] [--attributes] [--metadata] [--simple]
                  [--flatten] [--flattenLayerStack]
                  [--mask PRIMPATH[,PRIMPATH...]]
                  inputPath
   
   Writes the tree structure of a USD file. The default is to inspect a single
   USD file. Use the --flatten argument to see the flattened (or composed) Stage
   tree. Special metadata "kind" and "active" are always shown if authored unless
   --simple is provided.
   
   positional arguments:
     inputPath
   
   optional arguments:
     -h, --help            show this help message and exit
     --unloaded            Do not load payloads
     --attributes, -a      Display authored attributes
     --metadata, -m        Display authored metadata (active and kind are part of
                           the label and not shown as individual items)
     --simple, -s          Only display prim names: no specifier, kind or active
                           state.
     --flatten, -f         Compose the stage with the input file as root layer
                           and write the flattened content.
     --flattenLayerStack   Flatten the layer stack with the given root layer.
                           Unlike --flatten, this does not flatten composition
                           arcs (such as references).
     --mask PRIMPATH[,PRIMPATH...]
                           Limit stage population to these prims, their
                           descendants and ancestors. To specify multiple paths,
                           either use commas with no spaces or quote the argument
                           and separate paths by commas and/or spaces. Requires
                           --flatten.

******************
usdzip
******************

Utility for creating :doc:`USDZ packages <spec_usdz>` from USD compositions and
the assets (images and others in future) they reference.

.. code-block:: none

   > usdzip -h
   usage: usdzip [-h] [-r] [-a ASSET] [--arkitAsset ARKITASSET] [-c]
                 [-l [LISTTARGET]] [-d [DUMPTARGET]] [-v]
                 [usdzFile] [inputFiles [inputFiles ...]]
   
   Utility for creating a .usdz file containging USD assets and for inspecting
   existing .usdz files.
   
   positional arguments:
     usdzFile              Name of the .usdz file to create or to inspect the
                           contents of.
     inputFiles            Files to include in the .usdz file.
   
   optional arguments:
     -h, --help            show this help message and exit
     -r, --recurse         If specified, files in sub-directories are recursively
                           added to the package.
     -a ASSET, --asset ASSET
                           Resolvable asset path pointing to the root layer of
                           the asset to be isolated and copied into the package.
     --arkitAsset ARKITASSET
                           Similar to the --asset option, the --arkitAsset option
                           packages all of the dependencies of the named scene
                           file. Assets targeted at the initial usdz
                           implementation in ARKit operate under greater
                           constraints than usdz files for more general 'in
                           house' uses, and this option attempts to ensure that
                           these constraints are honored; this may involve more
                           transformations to the data, which may cause loss of
                           features such as VariantSets.
     -c, --checkCompliance
                           Perform compliance checking of the input files. If the
                           input asset or "root" layer fails any of the
                           compliance checks, the package is not created and the
                           program fails.
     -l [LISTTARGET], --list [LISTTARGET]
                           List contents of the specified usdz file. If a file-
                           path argument is provided, the list is output to a
                           file at the given path. If no argument is provided or
                           if '-' is specified as the argument, the list is
                           output to stdout.
     -d [DUMPTARGET], --dump [DUMPTARGET]
                           Dump contents of the specified usdz file. If a file-
                           path argument is provided, the contents are output to
                           a file at the given path. If no argument is provided
                           or if '-' is specified as the argument, the contents
                           are output to stdout.
     -v, --verbose         Enable verbose mode, which causes messages regarding
                           files being added to the package to be output to
                           stdout.

******************
usdchecker
******************

:program:`usdchecker` attempts to validate a USD or usdz file using a series
of rules and metrics that will evolve over time.  This tool currently
provides the best assurance that an asset will be properly interchangeable
and renderable by Hydra.

.. code-block:: none

   > usdchecker -h
   usage: usdchecker [-h] [-s] [-p] [-o [OUTFILE]] [--arkit] [-d] [-v] inputFile
   
   Utility for checking the compliance of a given USD stage or a USDZ package.
   
   positional arguments:
     inputFile             Name of the input file to inspect.
   
   optional arguments:
     -h, --help            show this help message and exit
     -s, --skipVariants    If specified, only the prims that are present in the
                           default (i.e. selected) variants are checked. When
                           this option is not specified, prims in all possible
                           combinations of variant selections are checked.
     -p, --rootPackageOnly
                           Check only the specifiedpackage. Nested packages,
                           dependencies and their contents are not validated.
     -o [OUTFILE], --out [OUTFILE]
                           The file to which all the failed checks are output. If
                           unspecified, the failed checks are output to stdout.
     --noAssetChecks       If specified, do NOT perform extra checks to help
                           ensure the stage or package can be easily and safely
                           referenced into aggregate stages.
     --arkit               Check if the given USD stage is compatible with
                           ARKit's initial implementation of usdz. These assets
                           operate under greater constraints that usdz files for
                           more general in-house uses, and this option attempts
                           to ensure that these constraints are met.
     -d, --dumpRules       Dump the enumerated set of rules being checked.
     -v, --verbose         Enable verbose output mode.

******************
usdfixbrokenpixarschemas
******************

:program:`usdfixbrokenpixarschemas` attempts to fix usd(a|c|z) layers for any
updates introduced by newer Pixar schema revisions. Note that this does not 
provide a fixing mechanism for all validation tests listed in `usdchecker`_.

.. code-block:: none

    > usdfixbrokenpixarschemas -h
    usage: usdfixbrokenpixarschemas [-h] [--backup BACKUPFILE] [-v] [inputFile]

    Fixes usd / usdz layer by applying appropriate fixes defined in the
    UsdUtils.FixBrokenPixarSchemas. If the given usd file has any fixes to be
    saved, a backup is created for that file. If a usdz package is provided, it 
    is extracted recursively at a temp location, and fixes are applied on each 
    layer individually, which are then packaged into a new usdz package, while 
    creating a backup of the original.

    positional arguments:
      inputFile            Name of the input file to inspect and fix.

    optional arguments:
      -h, --help           show this help message and exit
      --backup BACKUPFILE  optional backup file path, if none provided creates a
                           <inputFile>.bck.<usda|usdc|usdz> at the inputFile
                           location
      -v, --verbose        Enable verbose mode.

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

.. code-block:: none

   > usdstitch -h
   usage: [-h] [-o OUT]
          usdFiles [usdFiles ...]
   
   Stitch multiple usd file(s) together into one. Opinion strength is determined
   by the order in which the file is given, with the first file getting the
   highest strength. The exception to this general behavior is in time samples,
   which are merged together as a union across the input layers. It is consistent
   in that, if two time sample keys conflict, the strong layer takes precedence.
   
   
   positional arguments:
     usdFiles
   
   
   optional arguments:
     -h, --help         show this help message and exit
     -o OUT, --out OUT  specify a file to write out to

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

.. code-block:: none

   > usdstitchclips -h
   usage: usdstitchclips [-h] [-o OUT] [-c CLIPPATH] [-s STARTTIMECODE]
                         [-r STRIDE] [-e ENDTIMECODE] [-t] [-p TEMPLATEPATH]
                         [--clipSet CLIPSET] [--activeOffset ACTIVEOFFSET] [-n]
                         usdFiles [usdFiles ...]
   
   Stitch multiple usd file(s) together into one using value clips.
   An example command is:
   
   usdstitchclips --out result.usd --clipPath /World/fx/Particles_Splash clip1.usd clip2.usd 
   
   This will produce two files, a result.topology.usd and a result.usd.
   
   positional arguments:
     usdFiles
   
   optional arguments:
     -h, --help            show this help message and exit
     -o OUT, --out OUT     specify the filename for the top-level result file,
                           which also serves as base-name for the topology file.
     -c CLIPPATH, --clipPath CLIPPATH
                           specify a prim path at which to begin stitching clip
                           data.
     -s STARTTIMECODE, --startTimeCode STARTTIMECODE
                           specify the time at which the clips will become active
     -r STRIDE, --stride STRIDE
                           specify a stride for the numeric component of
                           filenames for template metadata
     -e ENDTIMECODE, --endTimeCode ENDTIMECODE
                           specify the time at which the clips will cease being
                           active
     -t, --templateMetadata
                           author template clip metadata in the root layer.
     -p TEMPLATEPATH, --templatePath TEMPLATEPATH
                           specify a template asset path to author
     --clipSet CLIPSET     specify a named clipSet in which to author clip
                           metadata, so that multiple sets of clips can be
                           applied on the same prim.
     --activeOffset ACTIVEOFFSET
                           specify an offset for template-based clips, offsetting
                           the frame number of each clip file.
     -n, --noComment       do not write a comment specifying how the usd file was
                           generated

******************
usddumpcrate
******************

:program:`usddumpcrate` provides information on usd files encoded using
:ref:`USD's Crate File Format <glossary:Crate File Format>`.  This can be
useful for very low-level debugging.

.. code-block:: none

   > usddumpcrate -h
   usage: usddumpcrate [-h] [-s] inputFiles [inputFiles ...]
   
   Write information about a usd crate (usdc) file to stdout
   
   positional arguments:
     inputFiles
   
   optional arguments:
     -h, --help     show this help message and exit
     -s, --summary  report only a short summary

******************
sdfdump
******************

Provides information on :ref:`Sdf Layers <glossary:Layer>`, which are the
containers for USD data.

.. code-block:: none

   > sdfdump -h
   Usage: sdfdump [options] <input file>
   Options:
     -h [ --help ]                   Show help message.
     -s [ --summary ]                Report a high-level summary.
     --validate                      Check validity by trying to read all data 
                                     values.
     -p [ --path ] regex             Report only paths matching this regex.
     -f [ --field ] regex            Report only fields matching this regex.
     -t [ --time ] n or ff..lf       Report only these times or time ranges for 
                                     'timeSamples' fields.
     --timeTolerance tol (=0.000125) Report times that are close to those 
                                     requested within this relative tolerance.
     --sortBy path|field (=path)     Group output by either path or field.
     --noValues                      Do not report field values.
     --fullArrays                    Report full array contents rather than number
                                     of elements.

******************
sdffilter
******************

Provides information in a variety of formats (including usda-like) about
:ref:`Sdf Layers <glossary:Layer>` or specified (filtered) parts of a
layer. Uses range from finding full information about specific properties, to
creating "suitable for web/index display" versions of USD files that elide
large array data.

.. code-block:: none

   > sdffilter -h
   Usage: sdffilter [options] <input files>
   Options:
     -h [ --help ]                         Show help message.
     -p [ --path ] regex                   Report only paths matching this regex. 
                                           For 'layer' and 'pseudoLayer' output 
                                           types, include all descendants of 
                                           matching paths.
     -f [ --field ] regex                  Report only fields matching this regex.
     -t [ --time ] n or ff..lf             Report only these times or time ranges 
                                           for 'timeSamples' fields.
     --timeTolerance tol (=0.000125)       Report times that are close to those 
                                           requested within this relative 
                                           tolerance.
     --arraySizeLimit N                    Truncate arrays with more than this 
                                           many elements.  If -1, do not truncate 
                                           arrays.  Default: 0 for 'outline' 
                                           output, 8 for 'pseudoLayer' output, and
                                           -1 for 'layer' output.
     --timeSamplesSizeLimit N              Truncate timeSamples with more than 
                                           this many values.  If -1, do not 
                                           truncate timeSamples.  Default: 0 for 
                                           'outline' output, 8 for 'pseudoLayer' 
                                           output, and -1 for 'layer' output.  
                                           Truncation performed after initial 
                                           filtering by --time arguments.
     -o [ --out ] outputFile               Direct output to this file.  Use the 
                                           'outputFormat' for finer control over 
                                           the underlying format for output 
                                           formats that are not uniquely 
                                           determined by file extension.
     --outputType validity|summary|outline|pseudoLayer|layer (=outline)
                                           Specify output format; 'summary' 
                                           reports overall statistics, 'outline' 
                                           is a flat text report of paths and 
                                           fields, 'pseudoLayer' is similar to the
                                           sdf file format but with truncated 
                                           array values and timeSamples for human 
                                           readability, and 'layer' is true layer 
                                           output, with the format controlled by 
                                           the 'outputFile' and 'outputFormat' 
                                           arguments.
     --outputFormat format                 Supply this as the 'format' entry of 
                                           SdfFileFormatArguments for 'layer' 
                                           output to a file.  Requires both 
                                           'layer' output and a specified 
                                           'outputFile'.
     --sortBy path|field (=path)           Group 'outline' output by either path 
                                           or field.  Ignored for other output 
                                           types.
     --noValues                            Do not report field values for 
                                           'outline' output.  Ignored for other 
                                           output types.
