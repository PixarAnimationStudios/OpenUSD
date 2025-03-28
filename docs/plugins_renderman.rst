============================
RenderMan USD Imaging Plugin
============================

.. include:: rolesAndUtils.rst

USD supports a USD Imaging plugin called 'hdPrman' that can be used to render
with the `RenderMan <http://renderman.pixar.com>`_ renderer.

Configuration
#############

Building hdPrman
****************

The RenderMan USD Imaging plugin is not built by default with USD. Building
hdPrman requires that you have RenderMan 25.0 or newer installed. The easiest
way to build hdPrman is to use the :filename:`build_usd.py` script which can be
configured with the following build options:

.. code-block:: none

  --prman               Build Pixar's RenderMan imaging plugin
  --no-prman            Do not build Pixar's RenderMan imaging plugin
                        (default)
  --prman-location PRMAN_LOCATION
                        Directory where Pixar's RenderMan is installed.

To manually configure hdPrman, the following CMake variables are used:

.. list-table::
   :header-rows: 1

   * - CMake Variable
     - Meaning
     - Value
   * - :makevar:`PXR_BUILD_PRMAN_PLUGIN`
     - Build Pixar's RenderMan imaging plugin
     - ON/OFF
   * - :makevar:`RENDERMAN_LOCATION`
     - Directory where Pixar's RenderMan is installed
     - $RMANTREE
   * - :makevar:`PXR_ENABLE_OSL_SUPPORT`
     - Only build rmanOslParser when this is disabled
     - ON/OFF

USD distributes with two OSL parsing plugins: sdrOsl and rmanOslParser. The
primary OSL parsing plugin, sdrOsl, requires a full OSL installation. If you do
not have an OSL installation but still wish to render OSL patterns with
RenderMan, the rmanOslParser plugin can be used. That plugin only depends on a
RenderMan installation and will be built when :makevar:`PXR_ENABLE_OSL_SUPPORT` is
OFF. Users should only build one of these plugins.

For more information see our page on `Advanced Build Configuration
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/BUILDING.md>`_.

Running hdPrman
***************

When running hdPrman, ensure that you have correctly set your environment
variables. We'll refer to the install location of your USD build with
:filename:`<inst>`. This is determined in the build with the CMake flag,
:makevar:`CMAKE_INSTALL_PREFIX`.

.. list-table::
   :header-rows: 1

   * - Environment Variable
     - Meaning
     - Value
   * - :envvar:`PYTHONPATH`
     - This is a path list which Python uses to find modules.
     - :filename:`$PYTHONPATH:<inst>/lib/python`
   * - :envvar:`RMANTREE`
     - Directory where Pixar's RenderMan is installed
     - Required
   * - :envvar:`RMAN_SHADERPATH`
     - Search path for RenderMan OSL shader plugins
     - :filename:`$RMANTREE/lib/shaders:<inst>/plugin/usd/resources/shaders`
   * - :envvar:`RMAN_RIXPLUGINPATH`
     - Search path for RenderMan RIS shading plugins
     - :filename:`$RMANTREE/lib/plugins`
   * - :envvar:`RMAN_TEXTUREPATH`
     - Search path for textures and RenderMan Rtx plugins
     - :filename:`$RMANTREE/lib/textures:$RMANTREE/lib/plugins:<inst>/plugin/usd`
   * - :envvar:`RMAN_DISPLAYPATH`
     - Search path for Renderman display plugins
     - :filename:`$RMANTREE/lib/plugins`
   * - :envvar:`RMAN_PROCEDURALPATH`
     - Search path for Renderman procedural plugins
     - :filename:`$RMANTREE/lib/plugins`

Developer
#########

Supported Render Pass AOVs
**************************

hdPrman supports the following AOVs for view-port compositing and picking:

.. list-table::
   :header-rows: 1

   * - AOV Name
     - Type
     - Meaning
   * - :mono:`color`
     - :mono:`HdFormatUNorm8Vec4`
     - RGBA beauty (Ci,a)
   * - :mono:`depth`
     - :mono:`HdFormatFloat32`
     - Depth pass (z)
   * - :mono:`primId`
     - :mono:`HdFormatInt32`
     - Per-primitive id (id)
   * - :mono:`instanceId`
     - :mono:`HdFormatInt32`
     - Per-instance id (id2)
   * - :mono:`elementId`
     - :mono:`HdFormatInt32`
     - Per-face id (__faceindex)
