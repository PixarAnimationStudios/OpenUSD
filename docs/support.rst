===============
Support for USD
===============

USD has support in many 3D Content Creation Applications and Ecosystems.

This page lists several of them, but is not fully comprehensive.

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

Copyright |copy| 2018, Pixar Animation Studios,  *version 1.2*

.. contents:: :local:

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

Apple
======

Apple use USD as the primary format for their 3D renderers. USDZ is their format of choice for Augmented Reality content.


`Creating USD files for Apple devices <https://developer.apple.com/documentation/realitykit/creating-usd-files-for-apple-devices?changes=_1>`_
outlines creating content that works on their platforms. macOS, iOS and iPadOS ship with USD integrations.

AR QuickLook
------------

`AR QuickLook <https://developer.apple.com/augmented-reality/quick-look/>`_ is a component of iOS and iPadOS that allows you to view USDZ content in augmented reality.

AR Creation Tools
-----------------

Apple has several `AR Creation Tools <https://developer.apple.com/augmented-reality/tools/>`_ , including
Reality Converter and the USDZ Tools that helps convert other file formats to USDZ.

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


Maya
----

`Maya <https://www.autodesk.com/products/maya/overview>`_ is a general purpose 3D package

Maya USD
^^^^^^^^

Maya comes bundled with an `open source plugin <https://github.com/Autodesk/maya-usd>`_ .

`Maya USD Documentation <https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2022/ENU/Maya-USD/files/GUID-9E9D45F2-4DA9-497B-8D69-1573ED6B2BA8-html.html>`_

Multiverse for Maya
^^^^^^^^^^^^^^^^^^^

`Multiverse <https://j-cube.jp/solutions/multiverse/>`_ is another USD plugin for Maya that adds other pipeline features
to Maya as well.

Blender
=======

`Blender <https://www.blender.org>`_ is a free and open source 3D creation suite.

`Blender USD Documentation <https://docs.blender.org/manual/en/latest/files/import_export/usd.html>`_


Foundry
=======

The Foundry are a developer of 3D Content Creation, Lighting and Compositing software.

Katana
------

`Katana <https://www.foundry.com/products/katana>`_ is  look development and lighting package.

Katana has `open source USD plugins <https://learn.foundry.com/katana/dev-guide/Plugins/KatanaUSDPlugins/index.html>`_ as
well as a support for `Hydra render delegates <https://learn.foundry.com/katana/dev-guide/Plugins/HydraRenderDelegates/index.html>`_.

Modo
----

`Modo <https://www.foundry.com/products/modo>`_ is a general purpose 3D package.

Nuke
----

`Nuke <https://www.foundry.com/products/nuke-family/nuke>`_ is a compositing package with support for both USD and Hydra.

`Nuke documentation <https://learn.foundry.com/nuke/content/comp_environment/3d_compositing/usd.html>`_

ZBrush
------

`ZBrush <https://www.maxon.net/en/zbrush>`_ is a 3D sculpting and painting application.

`ZBrush USD documenation <http://docs.pixologic.com/user-guide/zbrush-other-programs/usd-format/>`_

Houdini
=======

`Houdini <https://www.sidefx.com/products/houdini/>`_ is a 3D package with a focus on procedural content and effects.

Houdini includes `Solar <https://www.sidefx.com/products/houdini/solaris/>`_ to create composed USD content.

`SideFX USD Documentation <https://www.sidefx.com/docs/houdini/solaris/usd.html>`_


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


Omniverse
=========

`NVIDIA Omniverse <https://www.nvidia.com/en-us/omniverse/>`_ is a platform for creating and operating metaverse applications.
It is based on USD.

`NVIDIA USD Documentation <https://www.nvidia.com/en-us/omniverse/usd/>`_

Unity
=====

`Unity <https://unity.com>`_ is a real time 3D engine and editor.
It includes an `open source USD Package <https://github.com/Unity-Technologies/usd-unity-sdk>`_.

`Unity USD Documentation <https://docs.unity3d.com/2020.1/Documentation/Manual/com.unity.formats.usd.html>`_

Unreal Engine
=============

`Unreal Engine <https://www.unrealengine.com/en-US>`_ is a real time 3D engine and editor.

`Unreal USD Documentation <https://docs.unrealengine.com/5.0/en-US/universal-scene-description-usd-in-unreal-engine/>`_


