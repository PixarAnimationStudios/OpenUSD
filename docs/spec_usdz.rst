==============================
Usdz File Format Specification
==============================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

Copyright |copy| 2018, Pixar Animation Studios,  *version 1.3*

.. contents:: :local:

Purpose
=======

USD provides multiple features that could make it a compelling choice for 3D
content delivery, including:

    * Robust schemas for interchange of geometry, shading, and skeletal
      deformation

    * High performance data retrieval and rendering, including powerful
      instancing features

    * The ability to package user-selectable content variations, natively

    * A sound architecture that is flexible enough to adapt to future needs

However, part of USD's appeal is its ability to create a 3D scene by "composing"
many modular data sources (files) together into successively larger and larger
aggregations. While very useful within content creation pipelines, this aspect
can be a very large hindrance to using USD to deliver assets in the form that
they have been built up. In particular, even though USD does provide several
means of "flattening" multiple USD files into a single file, there is, by
design, no mechanism for representing images/textures as the "scene description"
encodable in USD files. Content delivery is simplified and useful on a broader
range of platforms when the content is:

    #. A single object, from marshaling and transmission perspectives

    #. Potentially streamable

    #. Usable without unpacking to a filesystem

We believe we can address these concerns by leveraging USD's `FileFormat plugin
<api/sdf_page_front.html#sdf_fileFormatPlugin>`_
mechanism to design an *archive format* that contains and proxies for files of
other formats embedded within the archive. This document provides the
specification for such a format, for which we will assign the . **usdz**
extension; we refer to the format as "the usdz format", as we similarly refer to
the usda and usdc file formats; only when forming the start of a sentence or
word in a class name do we capitalize it, such as UsdUsdzFileFormat. We refer
to usdz files as :bi:`packages`, and the design rests on a new Ar-level
abstraction of a package, of which the UsdUsdzFileFormat is the first
implementation, but we retain the architecture required to easily add others in
the future, if needed.

Usdz Specification
==================

A usdz package is an uncompressed zip archive that is allowed to contain the
following file types:

 +-------------+----------------------------------------+
 | Kind        | Allowed File Types                     |
 +=============+========================================+
 | USD         | **usda**, **usdc**, **usd**            |
 +-------------+----------------------------------------+
 | Image       | **png**, **jpeg**, **exr**, **avif**   |
 +-------------+----------------------------------------+
 | Audio       | **M4A**, **MP3**, **WAV**              |
 +-------------+----------------------------------------+

The rest of the section goes into more detail about the specification.


Foundation
----------

A usdz package is a `zip archive <https://en.wikipedia.org/wiki/Zip_(file_format)>`_ . 

.. dropdown:: Rationale

    Zip files are ubiquitous and supported by most modern computing
    environments. Although we are only using a subset of zip's featureset, by
    choosing zip over a format of our own devising, we make it easy for usdz 
    files to be useful as a "simple" transport protocol whereby the receiver 
    just unzips the contents and has "normal" USD scene description to work 
    with and inspect.


Zip Constraints
---------------
A usdz package is a **zero compression, unencrypted** zip archive. We reserve
the ability to relax this constraint in the future, but consider it unlikely.  

.. dropdown:: Rationale

    A key consideration for efficient *direct* consumption of usdz files is
    that, given a package already held in heap storage (possibly arriving over
    a network) or as a single file on disk, we be able to use the most direct
    API's available in USD for accessing the data contained within the package,
    **without extracting files to disk, or allocating more heap storage** . If
    we allowed either compression or encryption in usdz packages, we would need
    to violate one or both of the preconditions that allow, for example, the
    usda and usdc formats to access their data via direct memory access
    (typically via mmap).

    We do not believe the lack of zip compression will be a serious concern for
    data size. Most image formats themselves allow internal compression
    schemes, and the usdc format is quite compact, particularly as you collect
    more data into a single file; although we do not yet have an end-user tool
    for doing so, USD now contains all of the core features we require to
    aggregate an arbitrary composition's worth of usd files into a single file,
    without removing any composition features from the scene.

Layout
------

The only absolute layout requirement a usdz package makes of files within the 
package is that the data for each file begin at a multiple of 64 bytes from the beginning of the package.

However, if you wish the package to be presentable on a UsdStage "as is", or be
able to target the package with a simple reference to the package itself, then
**the first file in the package must be a native usd file** , but otherwise,
there are no constraints on the number or layout of other files in the package.
We refer to this "first USD file" as **the Default Layer** , in analogy to the
*defaultPrim* metadata used to allow layer referencers to elide a prim target.

Clients wishing to deliver "streamable content" `may wish to consider other layout constraints <#usdz-fileordering>`_ , as well.


.. dropdown:: Rationale

    **Alignment**

    To achieve access to data within a package without unpacking it, our
    preferred implementation will rely on mmapping into the package, with
    offsets. Some file formats and processing algorithms of the data within
    those packages benefit from aligning array data on boundaries greater than
    one byte. Of particular importance, the "zero copy" feature of usdc (crate)
    files that will appear in the 0.8.5 release of USD returns to clients
    pointers to array data directly into the mmapping of the crate file; to
    avoid unspecified behavior, the starting address of each such array must be
    aligned to the size of the underlying POD datatype. The crate format itself
    internally aligns (and falls back to copying data into heap memory when the
    alignment is off). The performance advantages of zero-copy in both time and
    heap memory are substantial, and we wish to ensure that consumers of usdz
    files will observe the same benefits; therefore, we require a **minimum
    alignment of 8 bytes** . The *largest* such alignment constraint for
    performance boosting of which we are currently aware is the 64 byte
    alignment for `Intel's AVX512
    <https://www.intel.com/content/www/us/en/architecture-and-technology/avx-512-overview.html>`_
    instruction set. While there might in future be an AVX1024 or greater,
    given that 64 bytes is also the intel processor page size, we hopefully
    balance package-bloat with potential optimization and settle on 64 bytes.

    Therefore we require that each file begin on a 64 byte alignment within the
    package. Fortunately, the zip format gives us a variable sized field in
    each file's header section that we can appropriate to this purpose.
    Unfortunately, off-the-shelf zip tools do not allow specification of
    padding or alignment for zipped content, so USD will need to provide its
    own wrapping of the core zip API's, and its own packaging tool that uses
    them.

    **Default Layer and defaultLayer.usd**

    The Default Layer, if it exists, is **always the first file in the
    package** .  The Default Layer is the layer that will be returned by a call
    to SdfLayer::FindOrOpen("package.usdz"), and therefore also the root layer
    when the package is placed on a UsdStage, or when referenced or sublayered
    as a whole.  Given that, due to alignment considerations, we will need to
    provide packaging utilities more sophisiticated than "zip", we can always
    allow you to specify what the Default Layer should be, as a command option.
    However, to make the packaging process as simple as possible - particularly
    focusing on the workflow for "editing" a package, in which we must unpack a
    package, edit files, and then **remake the package** , losing the
    positional information originally present in the package - we provide some
    extra affordances to reduce the scenarios in which one must manually
    specify the Default Layer:

        * If the complete set of files handed to the packager contains only a
          single USD file "at package root scope", then that will be selected
          as the Default Layer, and added first to the package. This does not
          prevent one from including arbitrary other USD files within
          subdirectories within the package. This allows for reliable and easy
          unpacking/repackaging, and multiple packages can be unpacked into the
          same directory as long as their contents are differently named.

        * If there is more than one USD file at the package's root scope, and
          one of them is named defaultLayer.usd , then we select that layer as
          the Default Layer.  This also allows for easy unpacking/repackaging,
          with a bit of up-front work during original package creation, and
          also makes it less likely that one could safely unpack multiple
          packages directly into the same directory, but provides a clear and
          familiar (i.e. to defaultPrim metadata) mechanism for encoding
          arbitrarily complex organizations within a package.

        * Otherwise, the packaging API will fail to run unless the Default
          Layer is manually specified.

.. _spec_usdz_file_types:

File Types
----------
A usdz package can contain only file types whose data can be consumed by the
USD runtime via mmap, pointer to memory, or threadsafe access to a FILE * (i.e.
solely pread-like access). This excludes, for example, Alembic files,
currently. Allowable file types are currently: 

    * **usda, usdc, usd** files (Apple's current usdz implementation allows 
      only a single usdc file, but this restriction will be lifted in future 
      OS updates)  

    * **png**, **jpeg** (any of the multiple common extensions for 
      jpeg), **OpenEXR** and **AV1 Image (AVIF)** files for images/textures. 
      See :ref:`Working With Image File Formats<image_file_formats>` for more 
      details on supported image file formats.

    * **M4A, MP3, WAV** files for embedded audio (given in order of preferred 
      format)

.. dropdown:: Rationale

    We wish to avoid the :bi:`need` to ever unpack a usdz package to consume
    its contents in the USD runtime, both because some target platforms may be
    unable to do so, and because managing caches of extracted package contents
    between processes is fraught.

USD Constraints
---------------
To make usdz packages maximally useful within production pipelines, we impose
no further constraints on the allowable content within a package. Of particular
note: 

    * it is possible to reference individual files within a package
      *from outside the package* 

    * packages can themselves contain other packages.  

When the key intention of a package is to provide perfectly reproducible
results in arbitrary consuming environments, it may be useful to enforce
restrictions on the kinds of *asset paths* encoded in USD files within the
package. We explore this further in `For Reproducible Results, Encapsulate
Using Anchored Asset Paths <#usdz-reproducibleresults>`_ , and imagine that
adherence to such protocols may be something useful to encode in package
metadata (see next item).

.. dropdown:: Rationale

    There is no mandated restriction on types of asset paths encodable in a
    usdz package, so for example if a studio uses URI's whose resolution
    depends on a specific ArResolver implementation, it is allowed to embed
    such paths in a package, with the understanding that they will not be
    resolvable at other sites, and therefore likely not as useful for
    content-delivery to clients. Packages can additionally contain both
    :bi:`anchored` **context dependent paths** , like :usda:`@./model.geom.usd@`, and 
    :bi:`search` **context dependent paths** , like :usda:`@materials/metals.usd@`.

    **Anchored context dependent paths** will always resolve to other files 
    within the package, or be unresolvable.  Because usdz packages are 
    themselves containers, we think it a useful builtin behavior that, for any
    **search context dependent path** contained in the package, the usdz 
    FileFormat itself will attempt to pre-resolve any path to other files 
    within the package using a specialzed *ArPackageResolver* , because a
    generic ArResolver cannot "see inside" a package, and therefore cannot
    resolve to files inside a package.  The behavior of the *UsdUsdzResolver*
    can be described as follows:

        * First attempt to anchor the path to the layer-within-the-package in
          which the path is authored, to attempt to locate another file within
          the package

        * Should that fail to locate a file, attempt to anchor the path to the
          package's Default Layer, which is the same behavior one would get
          from unpacking the package and putting its unpacked Default Layer on
          a stage, when using the ArDefaultResolver.

        * Should that fail to locate a file, perform normal asset resolution on
          the path with whatever ArResolver is installed.

    We can construct references to any file inside a package using a new
    reserved syntax in identifiers, like
    :usda:`@foo.usdz[path/to/file/within/package.usd]@`. :cpp:`ArResolver` 
    understands this syntax and is responsible for decomposing identifiers into
    a prefix that
    gets resolved normally ( *foo.usdz* in the example) and a suffix which is a
    *potentially* nested location of a file contained within the package.
    (Nesting can occur when one usdz file embeds another, e.g.
    :usda:`@set.usdz[areas/shire.usdz[architecture/BilboHouse/Table.usd]]@`). 
    Any asset path that resolves successfully can be turned into an ArAsset, 
    which serves as an abstraction so that clients of USD files, images, or any
    other file type can access the data without knowing whether it resides in
    its own file, inside a package file like a usdz, or is a heap-allocated
    buffer of data that was generated or transferred across the net.

Editability
-----------
A usdz file is read-only - editing its contents requires first unpacking the
package and editing its constituent parts using appropriate tools. Since usdz
is a "core" USD file format, one can use *usdcat* and *usdedit* on packages:

    * If the package contains a Default Layer, usdcat will print its contents,
      otherwise, it prints nothing.  

    * If the package contains a Default Layer file, usdedit will populate an 
      editor with its contents, but with the *--noeffect* option in force, 
      preventing the saving of any changes; otherwise, it will show an empty 
      layer.

Accessibility
-------------
Although not required, if creators of usdz packages use
:usdcpp:`SdfLayer::SetDocumentation` to add a documentation string to **the
Default Layer**, then consumers of usdz packages can retrieve the textual
description using :usdcpp:`SdfLayer::GetDocumentation` on the Default Layer.


Packaging Considerations for Streaming and Encapsulation
========================================================

.. _usdz-fileordering:

File Ordering Within Package for Streaming
------------------------------------------
There are many interpretations and possible implementations/encodings of
"streaming usd context", and we feel it is, at this time, beyond the scope of
USD's concerns to dictate how streaming *must* be achieved. We limit our
concern in this matter to the already-stated "Default Layer" semantics, which
dictates the first file in the package must be a usd file for the package to be
imageable directly on a stage. This means that a consumer app can, using the
zip (or wrapper, provided by the usdz file format) API, determine the size of
the first file and therefore know when it has been fully delivered, and display
the contents of that file in its "default state", while waiting for subsequent
files (described in the layer's metadata) to be delivered. This enables many
kinds of streaming; to suggest just a few:

    * LOD, in which the first file contains low-complexity geometry, with a
      variantSet that can bring in higher quality LOD's in other files, when
      switched

    * LOD, in which the first file binds no or simple materials, with a
      variantSet that binds texture-driven Materials when the textures are
      available

    * Animation, in which the first file contains a static pose, with a payload
      arc to another file containing animation.

It is important to note that the USD runtime has no knowledge or consideration
of streaming: it will be the responsibility of the client application to manage
the scene it is streaming, and ensuring not to ask the USD runtime to consume
any part of the package that has not yet been delivered. In particular, it must
use the provided API's to construct an SdfLayer directly using offsets into an
incompletely downloaded usdz package, rather than trying to point a UsdStage
directly at the incomplete usdz package, as the UsdzFileFormat will assume and
require that the package it is consuming is complete and intact.

.. _usdz-reproducibleresults:

For Reproducible Results, Encapsulate Using Anchored Asset Paths 
----------------------------------------------------------------
To insure uniform consumption of assets shipped to clients via usdz, we feel it
is prudent to curtail the power of USD's asset resolution system, so that the
asset references within a package resolve uniformly on any consuming system,
regardless of how that system's USD ArResolver is configured. Rather than make
restrictions within the FileFormat itself, we propose that such "encapsulation"
of packages be achieved by restricting the *content* to only use **anchored**
paths (paths that begin with "./" or "../", both of which are interpreted in the
virtual filesystem described by the package's internal layout.

This does not prevent one from overriding the textures or other non-USD assets
contained in an "encapsulated" package, by *specifically overriding the
attributes that name the assets, in a layer stronger than the package in a
composition.*

MIME Type
=========

Usdz is registered with IANA, with a media type name of **model** and a subtype
name of **vnd.usdz+zip** . For full details, see `Usdz's IANA registration
page. <https://www.iana.org/assignments/media-types/model/vnd.usdz+zip>`_

Toolset
=======

The UsdzFileFormat includes helper API's for introspecting and extracting data
from a package, as well as tailored wrappings of the zip API functions for
creating and adding files to an package that satisfy the usdz layout
constraints. The toolset also includes:

    * :ref:`usdzip <toolset:usdzip>` - a command-line program that
      accepts either an explicit list of files to package, or when provided the
      :option:`--asset` or :option:`--arkitAsset` options, will "localize" the
      named USD file, discovering all (recursively) referenced files, updating
      the references to point to their new, package-relative locations, and
      create a new usdz package. The reason for a :option:`--arkitAsset` option
      separate from :option:`--asset` is that usdz files intended for
      transmission to iOS devices support a more limited subset of USD
      functionality. **usdzip** can also list the contents of a package file,
      and optionally validate the contents as they are being packaged. Usdz
      files can be unpacked using any zip/unzip program.

    * :ref:`usdcat <toolset:usdcat>`, :ref:`usdedit <toolset:usdedit>` - as 
      described above, these utilities will operate on the defaultLayer of a
      usdz package.

    * :ref:`usdchecker <toolset:usdchecker>` - will validate the
      contents of a package (or any other USD format) ; for usdz files
      specifically, the usdchecker option :option:`--arkit` will make usdchecker
      enforce stricter "web-compliant" rules that disallow certain advanced USD
      features that web-browsers do not yet support.

Changes, by Version
===================

Version 1.3 - Current Head
--------------------------

From version 1.2:

    * :ref:`Adds AV1 Image (AVIF) file support <spec_usdz_file_types>`. 
      AVIF is now a supported file type for images/textures.