==================
Alembic USD Plugin
==================

.. include:: rolesAndUtils.rst

.. admonition:: Configure Environment

  The Alembic USD plugin is not built by default. To enable it, 
  the :makevar:`PXR_BUILD_ALEMBIC_PLUGIN` option must be specified when running CMake.
  Check the `3rd Party Library and Application Versions <https://github.com/PixarAnimationStudios/OpenUSD/blob/release/VERSIONS.md>`_
  document for the Alembic library versions that have been used for testing.

  For more information see our page on `Advanced Build Configuration <https://github.com/PixarAnimationStudios/OpenUSD/blob/release/BUILDING.md>`_.

As shipped, USD supports its own text (usda) and binary (usdc) file
formats. However, it is possible for USD to support additional formats by
providing it with a custom file format plugin. One of these plugins, usdAbc,
which provides the ability to interact with Alembic files, is included in the
:filename:`pxr/usd/plugin` directory of the USD distribution. In this example,
we'll demonstrate the extended USD functionality using the existing USD tools.

Let's start by creating a simple USD file (save it as :filename:`hello.usda`):

.. code-block:: usda

  #usda 1.0

  def Xform "hello" 
  {
      def Sphere "world"
      {
      }
  }

The previous snippet defined a simple sphere in the :filename:`.usda` file
format. Now, let's use :ref:`toolset:usdcat` to convert it to an Alembic
file!

.. code-block:: sh

  $ usdcat hello.usda --out hello.abc

Now we can use :ref:`toolset:usdview` to view our new alembic file.

.. code-block:: sh

  $ usdview hello.abc

.. image:: http://openusd.org/images/plugins_alembic_usdview.jpg

We can also convert this Alembic file back to USD using a similar call to usdcat.

.. code-block:: sh

  $ usdcat hello.abc --out hello_converted.usda

Known Limitations
#################

In general, Alembic files can be used in place of USD files. There are some
caveats to our current Alembic support in usdAbc. For example, we translate
`AbcGeom Schemas <http://docs.alembic.io/reference/abcg.html>`_ except for
FaceSet, NuPatch and Light.

Also, we currently don't translate Alembic's notion of Component Ops, instead we
currently flatten them into a 4x4 Matrix, this could be added in the future.

Finally, while we hope to achieve lossless translation from abc to usd, 
:ref:`Alembic files will never perform as well as native USD binary files in composed scenes
<maxperf:use binary ".usd" files for geometry and shading caches>`

