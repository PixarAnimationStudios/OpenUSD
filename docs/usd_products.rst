==================
Products Using USD
==================

USD has support in many 3D Content Creation Applications and Ecosystems.

This list is maintained by the community, and is not meant to be an exhaustive 
or complete list of resources, nor is it an endorsement of any of the linked 
material. We haven't checked the linked materials for accuracy, nor to see if 
they represent best practices. Please enjoy this material in the spirit 
intended, of celebrating community and industry achievements around the use and 
advancement of USD.

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. contents::
   :local:
   :class: threecolumn

--------

3Delight
========

`3Delight <https://www.3delight.com>`_ is a path-traced renderer that supports USD and has
a `Hydra Delegate <https://gitlab.com/3Delight/HydraNSI>`_.

--------

Adobe
=====

Adobe is a developer of several content generation applications. Their Substance suite of applications are geared
towards 3D content creation

Substance 3D Painter
--------------------

`Substance 3D Painter <https://www.adobe.com/products/substance3d-painter.html>`_ is a software for painting textures
for 3D content.

Substance 3D Modeler
--------------------

`Substance 3D Modeler <https://www.adobe.com/products/substance3d-modeler.html>`_ is a 3D modelling application.

Substance 3D Stager
-------------------

`Substance 3D Stager <https://www.adobe.com/products/substance3d-stager.html>`_ is a 3D package to build and assemble
3D scenes.

--------

AMD ProRender
=============

`AMD ProRender <https://www.amd.com/en/technologies/radeon-prorender>`_ is a GPU and CPU path-traced renderer.
There is an `open source Hydra delegate <https://github.com/GPUOpen-LibrariesAndSDKs/RadeonProRenderUSD>`_.

--------

Apple
======

Apple uses USD as the primary format for their 3D renderers. USDZ is their format of choice for Augmented Reality content.


`Creating USD files for Apple devices <https://developer.apple.com/documentation/realitykit/creating-usd-files-for-apple-devices?changes=_1>`_
outlines creating content that works on their platforms. macOS, iOS and iPadOS ship with USD integrations.

Preview and QuickLook
---------------------

iOS and iPadOS ship with `AR QuickLook <https://developer.apple.com/augmented-reality/quick-look/>`_ , an augmented reality
USD viewer.

macOS provides Preview and QuickLook in Finder, allowing you to directly preview files.

AR Creation Tools
-----------------

Apple has several `AR Creation Tools <https://developer.apple.com/augmented-reality/tools/>`_ , including:

* Reality Composer : An application for creating augmented reality applications
* Reality Converter : An application for converting popular 3D formats to USDZ
* USDZTools : Command line tools for converting other 3D formats to USD

Rendering Engines
-----------------

Apple has multiple render engines available to developers:

* `RealityKit <https://developer.apple.com/documentation/realitykit/>`_ is a 3D rendering engine focused on augmented reality applications.
* `SceneKit <https://developer.apple.com/documentation/scenekit>`_ is a rendering engine for 3D games and content.

`Validating feature support for USD files <https://developer.apple.com/documentation/realitykit/validating-usd-files?changes=_3>`_
documents feature support for USD across the different renderers available.

ModelIO
-------

`ModelIO <https://developer.apple.com/documentation/modelio>`_ is a framework for importing, exporting and manipulating
common 3D formats.

Motion
------

`Motion <https://www.apple.com/final-cut-pro/motion/>`_ is a motion graphics application.

--------

AnimVR
======

`AnimVR <https://nvrmind.io/features>`_ is a virtual reality animation tool.

--------

ArcGIS CityEngine
=================

`ArcGIS CityEngine <https://www.esri.com/en-us/arcgis/products/arcgis-cityengine/overview>`_ is a city design application.

--------

Autodesk
========

Autodesk is a developer of several 3D Content Creation, Rendering and CAD software packages.

`Autodesk's USD Website <https://makeanything.autodesk.com/usd>`_ highlights their USD work.

3ds Max
-------

`3ds Max <https://www.autodesk.com/products/3ds-max/overview>`_ is general purpose 3D package.

`3ds Max USD Documentation <https://knowledge.autodesk.com/support/3ds-max/learn-explore/caas/CloudHelp/cloudhelp/2022/ENU/3dsMax-USD/files/GUID-04F1DF51-0079-4DF8-8457-5AD12B6C0673-html.html>`_

Arnold
------

`Arnold <https://www.autodesk.com/products/arnold/overview>`_ is a path-traced renderer.

`Arnold USD Documentation <https://docs.arnoldrenderer.com/display/A5ARP/Introduction+to+Arnold+USD>`_

Fusion 360
----------

`Fusion 360 <https://www.autodesk.com/products/fusion-360/overview>`_ is a CAD, CAM, CAE and PCB software.


Maya
----

`Maya <https://www.autodesk.com/products/maya/overview>`_ is a general purpose 3D package


Maya comes bundled with an `open source plugin <https://github.com/Autodesk/maya-usd>`_ .

`Maya USD Documentation <https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2022/ENU/Maya-USD/files/GUID-9E9D45F2-4DA9-497B-8D69-1573ED6B2BA8-html.html>`_

There are also plug-ins from other Developers. See the :ref:`J Cube<usdproducts-jcube>` Multiverse section as an alternative.

Revit
-----

`Revit <https://www.autodesk.com/products/revit/overview>`_ is a BIM software for designers and builders
with a `third party USD extension <https://apps.autodesk.com/RVT/en/Detail/Index?id=127804203175527993&appLang=en&os=Win64>`_.

--------

Blender Foundation
==================

The `Blender Foundation <https://www.blender.org/about/foundation/>`_ are a public benefit
organization that develop 3D software.

Blender
-------

`Blender <https://www.blender.org>`_ is a free and open source 3D creation suite.

`Blender USD Documentation <https://docs.blender.org/manual/en/latest/files/import_export/usd.html>`_

Blender also has a `Hydra addon <https://github.com/GPUOpen-LibrariesAndSDKs/BlenderUSDHydraAddon>`_ courtesy of AMD.

Cycles
------

The `Cycles Renderer <https://www.cycles-renderer.org>`_ is an open source, path-traced renderer.

It has a `Hydra Delegate <https://wiki.blender.org/wiki/Building_Blender/CyclesHydra>`_ , originally developed by Tangent Animation.

--------

Chaos V-Ray
===========

`V-Ray <https://www.chaos.com>`_ is a 3D path-traced renderer
that supports USD and has a Hydra delegate.

`V-Ray Documentation <https://www.chaos.com/blog/getting-started-with-usd-and-hydra-in-v-ray>`_

--------

E-on
====

E-on are a software developer of world building 3D software.

`E-on Documentation for USD Export <https://info.e-onsoftware.com/learning-center/usd_export_in_vue_plantfactory>`_

Plant Factory
-------------

`Plant Factory <https://info.e-onsoftware.com/plantfactory/overview>`_ is a vegetation generation software.

Vue
---

`Vue <https://info.e-onsoftware.com/vue/overview>`_ is world building application.


--------

Gaffer
======

`Gaffer <https://www.gafferhq.org>`_ is an `open source <https://github.com/GafferHQ/gaffer>`_ , node based application for lighting and compositing.

--------

Golaem
======

`Golaem <https://golaem.com>`_ is a crowd simulation program with an `open source, USD plugin <https://github.com/Golaem/GolaemForUSD>`_
that allows viewing Golaem crowd caches within USD.

--------

Guerilla Render
===============

`Guerilla Render <http://guerillarender.com>`_ is a 3D renderer.

--------

Foundry
=======

The Foundry are a developer of 3D Content Creation, Lighting and Compositing software.

Katana
------

`Katana <https://www.foundry.com/products/katana>`_ is  look development and lighting package.

Katana has `open source USD plugins <https://learn.foundry.com/katana/dev-guide/Plugins/KatanaUSDPlugins/index.html>`_ as
well as a support for `Hydra render delegates <https://learn.foundry.com/katana/dev-guide/Plugins/HydraRenderDelegates/index.html>`_.

Mari
----

`Mari <https://www.foundry.com/products/mari>`_ is a 3D texture painting application.
Mari has `open source USD plugins <https://github.com/TheFoundryVisionmongers/MariUsdPlugins>`_.

Modo
----

`Modo <https://www.foundry.com/products/modo>`_ is a general purpose 3D package.

Nuke
----

`Nuke <https://www.foundry.com/products/nuke-family/nuke>`_ is a compositing package with support for both USD and Hydra.

`Nuke documentation <https://learn.foundry.com/nuke/content/comp_environment/3d_compositing/usd.html>`_

--------

Intel OSPRay
============

`Intel's OSPRay renderer <https://www.ospray.org/>`_ is a path-traced renderer with an `open source Hydra delegate <https://github.com/ospray/hdospray>`_.


--------

Isotropix Clarisse
==================

`Clarisse <https://www.isotropix.com/products/clarisse/ifx>`_ is a 3D application for look development, lighting and rendering.

--------

Dreamworks Moonray
==================

`Moonray <https://openmoonray.org/>`_ is an open-source renderer that comes with a USD Hydra render delegate.


--------

.. _usdproducts-jcube:

J Cube
======

J Cube are a developer of 3D software and cloud services.

Multiverse for Maya
-------------------

`Multiverse <https://j-cube.jp/solutions/multiverse/>`_ is a USD plugin for Maya with a range of pipeline features.

Muse
----

`Muse <https://j-cube.jp/solutions/muse/>`_ is a standalone USD Editor

--------

Left Angle Autograph
====================

`Autograph <https://www.left-angle.com/#page=95>`_ is a compositing and motion design package that comes with a 3d system based on USD.

--------

Maxon
=====

Maxon are a developer of 3D content creation, rendering and motion graphics software.

Cinema 4D
---------

`Cinema 4D <https://www.maxon.net/en/cinema-4d>`_ is a general purpose 3D package.

Redshift
--------

`Redshift <https://www.maxon.net/en/redshift>`_ is a GPU-accelerated path-traced renderer.

Redshift supports USD and is also available as a Hydra delegate for rendering integration.

ZBrush
------

`ZBrush <https://www.maxon.net/en/zbrush>`_ is a 3D sculpting and painting application.

`ZBrush USD documenation <http://docs.pixologic.com/user-guide/zbrush-other-programs/usd-format/>`_


--------

NVIDIA Omniverse
=================

`NVIDIA Omniverse <https://www.nvidia.com/en-us/omniverse/>`_ is a platform for creating and operating metaverse applications.
It is based on USD.

`NVIDIA USD Documentation <https://www.nvidia.com/en-us/omniverse/usd/>`_

Omniverse also adds USD connectors to many application, `listed here <https://docs.omniverse.nvidia.com/con_connect/con_connect/connecting-to-omniverse.html>`_.
Some of the applications are:

* Archicad
* Character Creator
* Creo
* iClone
* ParaView
* Revit
* Rhino
* SketchUp

Please see the `list <https://docs.omniverse.nvidia.com/con_connect/con_connect/connecting-to-omniverse.html>`_  for a
fuller range of connectors, as there are more than listed here.

--------

Procreate
=========

`Procreate <https://procreate.art>`_ is a 2D and 3D painting application for the iPad that
supports `import of USDZ models <https://procreate.art/handbook/procreate/3d-painting/import/>`_ .

--------

Shapr3D
=======

`Shapr3D <https://www.shapr3d.com>`_ is a CAD application that supports USD export on iPads, Mac and Windows.


--------

SideFX Houdini
==============

`Houdini <https://www.sidefx.com/products/houdini/>`_ is a 3D package with a focus on procedural content and effects.

Houdini includes `Solaris <https://www.sidefx.com/products/houdini/solaris/>`_ to create composed USD content.

`SideFX USD Documentation <https://www.sidefx.com/docs/houdini/solaris/usd.html>`_

--------

SynthEyes
=========

`SynthEyes <https://www.ssontech.com>`_ is a match move application.

--------

Tilt Brush
==========

`Tilt Brush <https://www.tiltbrush.com>`_ is a VR painting application.

--------

Unity
=====

`Unity <https://unity.com>`_ is a real time 3D engine and editor.
It includes an `open source USD Package <https://github.com/Unity-Technologies/usd-unity-sdk>`_.

`Unity USD Documentation <https://docs.unity3d.com/Packages/com.unity.formats.usd@3.0/manual/index.html>`_

--------

Unreal Engine
=============

`Unreal Engine <https://www.unrealengine.com/en-US>`_ is a real time 3D engine and editor.

`Unreal USD Documentation <https://docs.unrealengine.com/latest/en-US/universal-scene-description-usd-in-unreal-engine/>`_

--------

Usdtweak
=============

`usdtweak <https://github.com/cpichard/usdtweak>`_ is a free and open source editor for USD.


--------

Vicon Shogun
============

`Vicon Shogun <https://www.vicon.com/software/shogun/>`_ is a motion capture application.

--------

Wizart
======

`Wizart DCC Platform <https://wizartsoft.com/>`_ is a USD based general purpose 3D application.
