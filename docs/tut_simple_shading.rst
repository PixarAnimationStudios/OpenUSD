.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst

=====================
Simple Shading in USD
=====================

This tutorial demonstrates how to create a simple textured material and bind it
to geometry in USD. In it we:

* `Make a Model asset <#simpleshadinginusd-makingamodel>`_

* `Add some simple geometry to the Model
  <#simpleshadinginusd-makingthemesh>`_

* `Make a Material to contain shading data
  <#simpleshadinginusd-makeamaterial>`_

* `Add a surface shader to the Material
  <#simpleshadinginusd-addausdpreviewsurface>`_

* `Add texturing to the surface <#simpleshadinginusd-addtexturing>`_

We create the scene as a simple :ref:`glossary:Model`,
add geometry and a Material with a simple shading network into the model,
then bind the geometry to the Material.

To fully illustrate these concepts, we walk through a Python script that
performs the steps using the USD Python API, as well as showing the resulting
text :filename:`.usda` outputs.

The relevant scripts and data files reside in the USD distribution in
:filename:`USD/extras/usd/tutorials/simpleShading`. Run
:filename:`generate_simpleShading.py` in that directory to generate all of the
snippets for each step shown below.

Making a Model
==============

The first thing we want to do is create a "container" that will hold both the
geometry and the shading prims that we will create. We could have created Mesh
and Material prims at root scope in the scene, but by putting them both under a
common "model prim", we make it possible to reference this asset as a whole into
other scenes. In a python shell, execute the following:

.. code-block:: python

   from pxr import Gf, Kind, Sdf, Usd, UsdGeom, UsdShade
   
   stage = Usd.Stage.CreateNew("simpleShading.usd")
   UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
   
   modelRoot = UsdGeom.Xform.Define(stage, "/TexModel")
   Usd.ModelAPI(modelRoot).SetKind(Kind.Tokens.component)

Adding a Mesh "Billboard"
=========================

In the interests of simplicity and brevity, we stick with a very simple piece of
geometry, a quadrilateral "billboard", with a *st* texture coordinate that maps
the corners of the quadrilateral to the unit square in uv texture space. Note
that we do not create any normals for our Mesh; this is because by default
meshes use catmull-clark subdivision (to change the subdivision scheme, use
:usdcpp:`UsdGeom.Mesh.CreateSubdivisionSchemeAttr() <UsdGeomMesh::CreateSubdivisionSchemeAttr>`)
which provide analytically computed normals to a renderer.

.. code-block:: python

   billboard = UsdGeom.Mesh.Define(stage, "/TexModel/card")
   billboard.CreatePointsAttr([(-430, -145, 0), (430, -145, 0), (430, 145, 0), (-430, 145, 0)])
   billboard.CreateFaceVertexCountsAttr([4])
   billboard.CreateFaceVertexIndicesAttr([0,1,2,3])
   billboard.CreateExtentAttr([(-430, -145, 0), (430, 145, 0)])
   texCoords = UsdGeom.PrimvarsAPI(billboard).CreatePrimvar("st", 
                                       Sdf.ValueTypeNames.TexCoord2fArray, 
                                       UsdGeom.Tokens.varying)
   texCoords.Set([(0, 0), (1, 0), (1,1), (0, 1)])
   
   
   stage.Save()

Let's take a look at what we have so far. The last step above saved the current
state of the stage into simpleShading.usd (USD files can be edited and saved
multiple times). In another command shell, try:

.. code-block:: sh

   > usdview simpleShading.usd

We should see something like:

.. image:: http://openusd.org/images/tut_simple_shading_1.png

Make a Material
===============

In USD, we "bind" geometry to **Material prims** in order to customize how the
geometry should be shaded. A Material is a *container* for networks of shading
prims; a Material can contain one network that defines the *surface* illuminance
response, and another that defines surface *displacement* . It can also contain
different networks for different renderers that don't share a common shading
language. Complex models will define multiple materials, binding different
geometry (`Gprims <glossary.html#usdglossary-gprim>`_) to different Materials.

In our model, we have only one gprim, and therefore need only a single
Material - let us create it!

.. code-block:: python

   material = UsdShade.Material.Define(stage, '/TexModel/boardMat')

Add a UsdPreviewSurface
=======================

The most important shader in our Material will be the "surface" shader. We
create it (and any other shaders in the network) as children of the *material*
prim - this is what it means for the Material to be a **container** for the
shading network, and is an important property to preserve. We set the surface's
*metallic* and *roughness* properties to get familiar with setting shader input
properties. Any input whose value we do not set will be filled in by the
renderer with the fallback value defined in the :doc:`shader specification
<spec_usdpreviewsurface>` . After creating the shader, we connect the material's
*surface* output to the :cpp:`UsdPreviewSurface` 's *surface* output - this is
what identifies the source of the Material's surface shading.

.. code-block:: python

   pbrShader = UsdShade.Shader.Define(stage, '/TexModel/boardMat/PBRShader')
   pbrShader.CreateIdAttr("UsdPreviewSurface")
   pbrShader.CreateInput("roughness", Sdf.ValueTypeNames.Float).Set(0.4)
   pbrShader.CreateInput("metallic", Sdf.ValueTypeNames.Float).Set(0.0)
   
   material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), "surface")

Add Texturing
=============

The last step is to add a texture as the source for the surface's *diffuseColor*
. Texturing requires two nodes in the shading network: aUsdUVTexturenode to read
and map the texture, and a UsdPrimvarReader (more precisely,
aUsdPrimvarReader_float2, the implementation that reads float2Array-valued
:ref:`glossary:Primvar` s) to fetch a texture coordinate
from each piece of geometry bound to the material, to inform the texture node
how to map surface coordinates to texture coordinates.

.. code-block:: python

   stReader = UsdShade.Shader.Define(stage, '/TexModel/boardMat/stReader')
   stReader.CreateIdAttr('UsdPrimvarReader_float2')
   
   diffuseTextureSampler = UsdShade.Shader.Define(stage,'/TexModel/boardMat/diffuseTexture')
   diffuseTextureSampler.CreateIdAttr('UsdUVTexture')
   diffuseTextureSampler.CreateInput('file', Sdf.ValueTypeNames.Asset).Set("USDLogoLrg.png")
   diffuseTextureSampler.CreateInput("st", Sdf.ValueTypeNames.Float2).ConnectToSource(stReader.ConnectableAPI(), 'result')
   diffuseTextureSampler.CreateOutput('rgb', Sdf.ValueTypeNames.Float3)
   pbrShader.CreateInput("diffuseColor", Sdf.ValueTypeNames.Color3f).ConnectToSource(diffuseTextureSampler.ConnectableAPI(), 'rgb')

Note that we have not yet specified what texture coordinate (primvar) the
PrimvarReader should read. We could author the name of the primvar directly on
its *varname* input. However, we instead demonstrate how we can connect shader
inputs to "public interface attributes" on Materials. Any input in a shading
network can connect to an input on its enclosing material, and the renderer will
migrate the Material input's authored value (if any) to any shader input
connected to it **before** rendering and shading begins. In effect, this gives
us a way to "expose" inputs deep in a shading network for easy overriding by
consumers of the Material, since all the inputs on a Material will be exposed
and editable without needing to look inside the Material, which may contain many
shading prims.

.. code-block:: python

   stInput = material.CreateInput('frame:stPrimvarName', Sdf.ValueTypeNames.Token)
   stInput.Set('st')
   
   stReader.CreateInput('varname',Sdf.ValueTypeNames.Token).ConnectToSource(stInput)

And lastly, apply MaterialBindingAPI on the billboard prim, bind the Mesh to our Material and save the results!

.. code-block:: python

   billboard.GetPrim().ApplyAPI(UsdShade.MaterialBindingAPI)
   UsdShade.MaterialBindingAPI(billboard).Bind(material)
   
   stage.Save()

In usdview, we should now see something like:

.. image:: http://openusd.org/images/tut_simple_shading_2.png

