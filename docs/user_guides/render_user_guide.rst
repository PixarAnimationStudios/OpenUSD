.. include:: ../rolesAndUtils.rst

##################
Rendering with USD
##################

This document describes the conventions and rules USD provides for producing 
correct and reproducible renders. Use this information to help build a robust 
USD pipeline and configure your assets for rendering. As USD can be used for 
both interactive renders and "final frame" renders, some of the conventions in 
this document will be more applicable to one use case than the other, whereas 
some conventions will apply to any rendering use case.

.. contents:: Table of Contents
    :local:
    :depth: 2

*****************************
Configuring Imageable Content
*****************************

Renderable content for USD is anything that is considered *imageable*. Imageable 
content is often geometry-based, such as meshes and curves, but can also 
be content such as lights, volumes, and physics joints. Imageable content also
isn't necessarily content that has a specific position in 3D space. For example, 
a Scope prim is considered imageable but has no independent position (but can 
contain a group of other prims that might have positions and bounds). 

For imageable content that does have an independent position in 3D space, USD 
defines a sub-class of imageable objects called *Xformable*. Xformable prims can 
specify an ordered set of transform operations (translate, scale, rotate, etc.). 
Some examples of Xformable content are cameras, volumes and volume fields, point 
instancers, and Xform prims. 

A *geometric prim*, or "Gprim", is an Xformable prim that has a defined extent in 
space, and directly defines some renderable geometry. Some examples of Gprims 
are meshes, NURBS surfaces, curves, points, and "instrinsic" Gprims such as 
spheres, cubes, and cylinders. 

See `UsdGeom <https://openusd.org/release/api/usd_geom_page_front.html>`__ for more 
details on Xformable prims and geometric prims.

Configuring the Stage Coordinate System
=======================================

For Xformable objects in your stage, USD uses a right-handed coordinate system  
but allows a configurable up axis, which can be either +Y (the fallback) or +Z. 
The configurable up axis is specified at the stage level, so if the up axis is 
Y, the stage's Z axis will be pointing out of the screen, and when viewing a 
stage with a Z up axis, the stage's Y axis will be pointing into the screen.

The configurable up axis lets you use objects with geometry that was modeled 
with either up axis. Keep in mind that since the up axis is declared in your 
stage metadata at the stage's root layer, the up axis applies to all Xformable 
objects in the stage, including any `referenced <#usdglossary-references>`_ 
geometry and assets. If all your objects in the stage were modeled with the same 
up axis, you just need to ensure that the stage's up axis is set appropriately. 
If your objects were modeled with a mix of Y or Z up axes, you may need to apply 
a corrective transform for some of those objects to be consistent with your 
preferred stage up axis.

The following example sets the up axis to Z in the stage metadata.

.. code-block:: usda

    #usda 1.0
    (
        upAxis = "Z"
    )

See `Encoding Stage UpAxis <https://openusd.org/release/api/group___usd_geom_up_axis__group.html>`__ 
for more details on up axis.

Note that for Camera prims used by renderers, the *Camera* always views the 
scene in a right-handed coordinate system where up is +Y, right is +X, and the 
forward viewing direction is -Z. 

Understanding Render Visibility
===============================

USD provides several options when configuring whether imageable prims are 
visible in the render output and, if so, if they are rendered with alternate 
representations, such as a lower-complexity representation of a Gprim used by 
a DCC tool. These options include:

- Visibility attribute: This controls whether the imageable prim is rendered at 
  all, and takes precedence over other render visibility settings.
- Imageable purpose: This describes the intended purpose for rendering the prim 
  (e.g., for final renders, for fast viewport renders in a DCC tool, etc.) and can 
  be used by renderers as a type of visibility filter. For example, a DCC tool 
  may opt to only render prims with the "proxy" purpose, and ignore "render" 
  prims intended for final renders.

Note that there is a higher-level method for controlling render visibility at 
the model (not prim) level, using `draw modes <https://openusd.org/release/api/class_usd_geom_model_a_p_i.html#details>`__. 
As this method depends on a specific production model workflow, it isn't 
discussed in this document. 

Additionally, you can specify a collection of prims that are visible to a 
renderer for a specific render pass. This is discussed in the 
:ref:`Render Settings <configuring_render_settings>` section.

.. _visibility_attribute:

Using the Visibility Attribute
------------------------------

For imageable Gprims, use the :usda:`visibility` attribute to control the visibility 
of a prim and its descendant prims in the rendered results. This can 
act as a basic toggle for displaying or not displaying the prim in a DCC tool. 
Unlike using the :usda:`active` attribute to deactivate a prim, invisible Gprims 
are still available for inspection, positioning, defining volumes, etc. Unlike 
the :ref:`purpose attribute <render_purpose>`, setting visibility 
does not communicate any sort of higher-level visibility purpose to the renderer.

Visibility can be set to :usda:`invisible` or :usda:`inherited` (the fallback 
value). Child prims with no visibility set, or with visibility set to 
:usda:`inherited`, inherit the parent visibility value, if any. In the following 
example, all child prims will inherit the parent visibility value except 
"cube23", which explicitly sets visibility to :usda:`invisible`.

.. code-block:: usda

    def Xform "xform" 
    {
        def Cube "cube21"
        {
            # Inherits visibility from parent
            token visibility = "inherited"
        }
    }

    def Xform "xform2"
    {
        token visibility = "inherited"

        def Cube "cube22"
        {
            # No visibility set, so inherits visibility from parent by default
        }
    }

    def Xform "xform3"
    {
        def Cube "cube23"
        {
            # Overrides parent visibility
            token visibility = "invisible"
        }
    }

Because the :usda:`invisible` visibility value is strongly inherited 
down the namespace, child prims of a prim set as invisible cannot be 
switched to visible. This may differ from systems you might be used to that 
don't propagate visibility settings through the node hierarchy, and also from 
systems in which visibility can be turned on and off multiple times in a prim 
hierarchy. In Pixar's experience, this latter, "non-pruning" visibility behavior 
does not interact well with USD's ability to allow multi-layered overriding of 
opinions.

Keep in mind that the :usda:`invisible` visibility value effectively overrules 
other higher-level render visibility-related settings, such as :ref:`imageable 
purpose <render_purpose>`. For example, if a prim is configured to use a proxy 
representation in a DCC tool via the :usda:`purpose` attribute and the DCC tool 
is set to display only "proxy" prims but the prim is also set to be invisible, 
the prim will not be rendered in the DCC tool.

Visibility is animatable, allowing a subtree of geometry to be present for some 
segments of a shot, and absent from others. Pixar strongly recommends using animated
visibility over approaches like changing the object scale to 0, as these
approaches can trigger degenerate cases in renderers and also impact render
results for things like motion blur. When rendering with motion blur, an 
object's visibility at shutter open time is held over the entire shutter interval, 
regardless of whether it may change in value over that interval in the scene.

.. _render_purpose:

Using Imageable Purpose
-----------------------

USD provides a builtin attribute for imageable prims called *purpose*, which 
describes the underlying purpose for why a prim may be rendered, and can 
thereby be used to control render visibility. A prim can only have one purpose, 
and the purpose can be one of the following:

- :usda:`default`: The prim has no special imageable purpose and should be rendered in 
  all situations. This is the fallback value if a purpose has not been authored.
- :usda:`render`: The prim represents scene data that should be used in "final 
  quality" renders.
- :usda:`proxy`: The prim should be used when rendering lightweight renders or for 
  interactive tools. For example, the prim might be a lower-complexity 
  representation of a mesh rendered to Hydra Storm for display in the viewport of a 
  DCC tool.
- :usda:`guide`: The prim should be used for viewable guides in interactive 
  applications. An example might be a spline used as a visual aid when rigging 
  a character, which should not be visible when actually animating the character 
  or doing a final render for that character.

Renderers will use the purpose of imageable prims to determine how the prim 
should be rendered, and also to potentially filter prims while gathering data 
to render. For example, a DCC tool might want to filter out :usda:`render` 
purpose and possibly :usda:`guide` purpose prims, to provide the right user
experience in a viewport. Or, an offline renderer might traverse the scene and 
filter out :usda:`proxy` and :usda:`guide` prims for final render pass output.

If you just need to unconditionally control whether a specific imageable prim 
and its child prims are visible in a render or not, 
the :ref:`visibility attribute <visibility_attribute>` should be used instead of 
imageable purpose.

Purpose Inheritance
^^^^^^^^^^^^^^^^^^^

If a prim does not have an authored purpose but has an ancestor that may have 
an authored purpose, the rules about how the ancestor purpose is inherited at 
render-time are as follows. If a prim is not imageable or does not have an 
authored opinion about its own purpose, then it will inherit the purpose of the 
closest imageable ancestor with an authored purpose opinion. If there are no 
imageable ancestors with an authored purpose opinion then the prim uses its 
fallback purpose.

In the following example, purpose is defined on the prims as follows:

- :usda:`</Root>` is not imageable so its purpose attribute is ignored and its 
  effective purpose is :usda:`default`.
- :usda:`</Root/RenderXform>` is imageable and has an authored purpose of 
  :usda:`render` so its effective purpose is :usda:`render`.
- :usda:`</Root/RenderXform/Prim>` is not imageable so its purpose attribute is 
  ignored.
- :usda:`</Root/RenderXform/Prim/InheritXform>` is imageable but with no authored 
  purpose. Its effective purpose is :usda:`render`, inherited from the authored 
  purpose of :usda:`</Root/RenderXform>`.
- :usda:`</Root/RenderXform/Prim/GuideXform>` is imageable and has an authored 
  purpose of :usda:`guide` so its effective purpose is :usda:`guide`.
- :usda:`</Root/Xform>` is imageable but with no authored purpose. It also has no 
  imageable ancestor with an authored purpose so its effective purpose is the 
  fallback value of :usda:`default`.

.. code-block:: usda

    def "Root" {
        token purpose = "proxy"
        def Xform "RenderXform" {
            token purpose = "render"
            def "Prim" {
                token purpose = "default"
                def Xform "InheritXform" {
                }
                def Xform "GuideXform" {
                    token purpose = "guide"
                }
            }
        }
        def Xform "Xform" {
        }
    }

Using Purpose for Stand-in Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Purposes :usda:`render` and :usda:`proxy` can be used together to partition a 
complicated model into a lightweight proxy representation for interactive use, 
and a fully realized representation for rendering. If prims are authored with a 
pair of :usda:`render` and :usda:`proxy` purpose prims, the :usda:`proxyPrim` 
relationship can be authored on the render prim to encode that correspondence 
for DCCs and pipeline tools, although it is entirely optional and does not 
affect renders in any way.

The following simplified example shows a Mesh Gprim with purpose :usda:`render` 
that will be used for final renders, and a Sphere Gprim that is set as the Mesh 
prim's proxy used in an interactive render or DCC tool.

.. code-block:: usda

    def Mesh "renderMesh"
    {
        token purpose = "render"
        rel proxyPrim = </proxySphere>
        ...
    }

    def Sphere "proxySphere"
    {
        token purpose = "proxy"
        ...
    }

Avoid using variant sets instead of :usda:`purpose` and :usda:`proxyPrim` for 
defining proxy stand-ins in your scene. While variants do provide a mechanism 
to switch prims for different rendering needs, they have the following drawbacks 
when used this way.

- Variant selection changes the composed scene data, whereas switching a 
  renderer to filter on a particular purpose does not. Ideally the client or 
  tool (and underlying renderer) that needs to control render visibility should 
  be responsible for implementing visibility control (using purpose) without 
  having to change the scene data.
- If using variant sets, parts of the stage need to be recomposed in order to 
  change to a different runtime level of detail, and that does not interact 
  well with the needs of multithreaded rendering since a 
  :ref:`UsdStage <usdglossary-stage>` cannot be queried from any thread while 
  it is recomposing.

While variant sets shouldn't be used for creating render proxy 
stand-ins, variant sets are appropriate for creating "workflow stand-ins" such 
as lighter-weight rigging and geometry versions of a model for layout/blocking 
and animation workflows. In these scenarios, switching a variant is appropriate, 
because it is a persistent decision by the user that affects behavior beyond 
just rendering.

Understanding Intrinsic and Explicit Normals
============================================

For every point on every surface prim (e.g., Meshes) in USD (and potentially 
Curves and Points prims also) a renderer must be able to compute a surface 
normal in order to perform its shading and lighting calculations. Surfaces 
always possess intrinsically computable normals, defined by the topology of 
the surface and its orientation, but normals can also be explicitly specified. 
In most scenarios you won't need to specify normals for your meshes, as by 
default a Mesh prim in USD is an `OpenSubdiv <https://opensubdiv.org/docs/intro.html>`__
compliant subdivision mesh that automatically calculates surface normals. 

USD by default uses the right-handed winding rule to compute normals, so the 
right-handed rule will determine if a normal is a "face" or 
"outward-facing" normal. You can optionally set the orientation attribute on a 
Mesh to "leftHanded" to use a left-handed winding rule instead.

.. code-block:: usda

    def Mesh "BasicMesh"
    {
        uniform token orientation = "leftHanded"
        ...face and vertex data...
    }

If you need to set normals explicitly, set the Mesh to 
:mono:`subdivisionScheme = "none"` and provide normals as needed. You can specify 
any primvar interpolation mode for normals, with :usda:`vertex` being the default 
and most common choice.

USD provides two ways to specify normals: using the 
:usda:`primvars:normals` primvar from the PrimvarsAPI schema and using the 
:usda:`normals` attribute from the GeomPointBased schema. Both approaches will be 
used by renderers the same way, but the recommended approach is to use 
:usda:`primvars:normals` because the primvar form supports encoding your normals 
as indexed data, as described in :ref:`Working with Primvars <working_with_primvars>`.  
Note that if a Gprim specifies normals using both approaches, the normals set 
via :usda:`primvar:normals` will be used.

The following example sets the normals using the :usda:`primvars:normals` primvar.

.. code-block:: usda

    def Mesh "Mesh" 
    {
        float3[] extent = [(-430, -145, 0), (430, 300, -30)]
        int[] faceVertexCounts = [4]
        int[] faceVertexIndices = [0, 1, 2, 3]
        point3f[] points = [(-430, -145, 0), (430, -145, 0), (430, 145, 0), (-430, 145, 0)]

        # Make polygonal and not subdiv mesh
        uniform token subdivisionScheme = "none"

        normal3f[] primvars:normals = [(0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 0, 1)] (
            interpolation = "vertex"
        )
    }

Your Gprim might have different transforms applied via parent prim transforms, 
Xformable transform operations, or composition. The renderer is responsible 
for ensuring that computed or manually created normals are properly transformed 
as well. For example, any given Gprim's local-to-world transformation can flip 
its effective orientation when it contains an odd number of negative scales. 
This condition can be reliably detected using the (Jacobian) determinant of the 
local-to-world transform: if the determinant is less than zero, then the Gprim's 
orientation has been flipped, and therefore the renderer will apply the opposite 
handedness rule when computing its surface normals (or just flip the computed 
normals) for the purposes of hidden surface detection and lighting calculations.

*******************
Working with Lights
*******************

USD provides the UsdLux LightsAPI schema for adding light behavior to prims, 
along with built-in common light types such as DistantLight, SphereLight, etc. 
UsdLux also provides additional schemas, such as ShadowAPI (for configuring 
shadows) and ShapingAPI (for configuring light emission shape, e.g., light cones 
and falloff), for adding more complex light-associated behaviors. 

By convention, most lights with a primary axis (except CylinderLight) emit along 
the -Z axis. Area lights are centered in the XY plane and are 1 unit in diameter.

The built-in light types are Xformable, so they can have transform operators 
applied as usual. It is recommended that only translation and rotations be used 
with these lights, to avoid introducing difficulties for light sampling and 
integration. Lights have explicit attributes to adjust their size.

The LightAPI schema can be applied to any prim to add light behavior. One 
use case for adding light behavior to prims is to create arbitrarily shaped
"mesh light" light sources. However, not all renderers support  
LightAPI on every Gprim type (e.g., a Capsule), so you should consider using the 
MeshLightAPI schema for mesh lights for better renderer support. 

To use MeshLightAPI or LightAPI, add the desired schema to 
your prim and author the schema inputs attributes as needed. The following 
example adds the MeshLightAPI schema to a Mesh prim.

.. code-block:: usda

    def Mesh "meshLight" (
        prepend apiSchemas = ["MeshLightAPI"]
    )
    {
        # Geometry data
        float3[] extent = [(-430, -145, 0), (430, 300, -30)]
        int[] faceVertexCounts = [4]
        int[] faceVertexIndices = [0, 1, 2, 3]
        point3f[] points = [(-430, -145, 0), (430, -145, 0), (430, 145, 0), (-430, 145, 0)]
        ....

        # LightAPI 
        color3f inputs:color = (1, 0.5, 0.5)
        float inputs:intensity = 300
        ....
    }

See the `UsdLux documentation <https://openusd.org/release/api/usd_lux_page_front.html>`__ 
for more details on LightAPI, MeshLightAPI, and the common light types.

Using Light-linking to Filter Objects Affected by Lights
========================================================

You might need to control which geometry 
is illuminated by a particular light. To support this, USD provides 
*light-linking* which uses the 
:usda:`collection:lightLink` :ref:`object collection<collections_and_patterns>`
to specify which objects are illuminated by a light. 
By default the collection has :usda:`includeRoot` set to true, indicating that 
the light will potentially illuminate all objects. To illuminate only a specific 
set of objects, there are two options. One option is to modify the collection 
paths to explicitly exclude everything else. The other option is to set 
includeRoot to false and explicitly include just the desired objects. These are 
complementary approaches that may each be preferable depending on the scenario 
and how to best express the intent of the light setup. The following example 
specifies that only the :usda:`</World/characters>` and :usda:`</World/trees>` 
objects are affected by CylinderLight.

.. code-block:: usda

    def CylinderLight "CylinderLight" 
    {
        uniform bool collection:lightLink:includeRoot = 0
        prepend rel collection:lightLink:includes = [
            </World/characters>,
            </World/trees>,
        ]
        ....
    }

A similar collection, :usda:`collection:shadowLink`, exists for specifying which 
objects will cast shadows from a particular light that uses the ShadowAPI schema. 

**********************
Working with Materials
**********************

In USD, a Material prim describes the surface and volumetric properties of 
geometry in a scene. A Material prim is a container of Shader nodes connected 
into networks. The networks describe shading computations and dataflow, and 
their outputs are connected to and described by their containing Material prim.

Geometry can be bound to Materials to subscribe to the "look" described by the 
Material. To add a material binding, the MaterialBindingAPI schema must be 
applied to a prim. In the following example a Mesh prim has the 
MaterialBindingAPI schema applied and is bound to the 
:usda:`</materials/MyMaterial>` Material prim (not shown in this example) via
the :usda:`material:binding` relationship.

.. code-block:: usda

    def Mesh "Mesh"
    (
        prepend apiSchemas = ["MaterialBindingAPI"]
    )
    {
        # ... geometric info omitted ...

        # Material binding
        rel material:binding = </materials/MyMaterial>
    }

Material bindings inherit down the prim namespace and come in two forms:

- Direct bindings, in which a prim directly names (via relationship) the 
  Material it wants to bind, as shown in the previous example.

- Collection-based bindings, in which a 
  :ref:`USD Collection<collections_and_patterns>` identifies a set of prims, 
  and the binding (again, a relationship) names both the Collection and the 
  Material to which the collected prims should be bound. See 
  :ref:`material_collection_binding` for more information on collection-based 
  bindings.

For any given Gprim, the direct or collection binding closest to it in the 
namespace is the one that wins. If a prim has both a direct binding and is 
directly targeted by a collection binding, by default the direct binding wins. 
USD also provides the :usda:`bindMaterialAs` metadata to control the strength
of collection-based bindings, as described in :ref:`material_collection_binding_strength`.

.. _usd_preview_material:

Using the USD Preview Material
==============================

USD provides a general-purpose preview material with a basic set of Shader 
nodes that is designed to work across most renderers and is supported by 
production renderers that ship with USD and Hydra. You can use the USD preview 
material if you want to enable reliable interchange of material settings between 
DCCs and real-time rendering clients in your workflow.

The preview material includes the following shaders:

- UsdPreviewSurface: Models a physically-based surface and can be configured to
  follow either a "specular" or a "metalness" workflow.
- UsdUvTexture: Reads a source UV texture.
- UsdPrimvarReader: Reads and evaluates :ref:`primvars <working_with_primvars>`.
- UsdTransform2D: Takes a 2D input and applies an affine transformation to it. 
  Useful for transforming 2D texture coordinates.

See :ref:`working_with_primvars` for a simple example that renders a texture. 
For more examples and full details of the preview material and shader inputs and 
outputs, see the :doc:`../spec_usdpreviewsurface`.

Using GLSLFX Shaders 
--------------------

For interactive rendering use cases, you can use GLSLFX shaders as 
nodes in your material network, with connections to UsdPrimvarReader or 
UsdUvTexture preview material shaders, and Hydra + Storm will render with 
those shaders. Use the :usda:`info:glslfx:sourceAsset` Shader attribute to set 
the GLSLFX shader asset. For example, if you have a custom GLSLFX shader named 
"MyShader.glslfx" that uses a texture coordinate parameter, a simple Material 
network that uses this shader along with the UsdPrimvarReader shader to access 
the "st" primvar might look something like:

.. code-block:: usda

    def Material "MyMaterial"
    {
        token outputs:glslfx:surface.connect = </materials/MyMaterial/glslSurface.outputs:surface>

        def Shader "usdprimvarreader1"
        {
            uniform token info:id = "UsdPrimvarReader_float2"
            string inputs:varname = "st"
            float2 outputs:result
        }

        def Shader "glslSurface"
        {
            # Specify the GLSLFX shader
            uniform asset info:glslfx:sourceAsset = @MyShader.glslfx@
            uniform token info:implementationSource = "sourceAsset"

            # Connect to PrimvarReader shader for st coords
            float2 inputs:st.connect = </materials/MyMaterial/usdprimvarreader1.outputs:result>
            token outputs:surface
        }
    }

.. _working_with_primvars:

Working with Primvars
=====================

A primvar is a special type of attribute that a renderer associates with an 
imageable prim, and can vary (interpolate) the value of the attribute over the 
surface/volume of the prim. Primvars allow for:

- Binding data to Gprims made available to the USD material/shader pipeline 
  during rendering.
- Associating data with prim vertices or faces and interpolating that data 
  across the prim's surface using a variety of interpolation modes.
- Inheriting constant interpolation primvars down the namespace to allow sparse 
  authoring of sharable data that is compatible with native scenegraph 
  instancing. Unlike regular USD attributes, constant interpolation primvars 
  implicitly also apply to any child prims, unless those child prims have their 
  own opinions about those primvars.

A commonly used primvar when rendering is the "st" primvar that specifies 
texture coordinates (sometimes referred to as "UVs"). The following simple 
example specifies four texture coordinates for a quad mesh using the 
:usda:`primvars:st` primvar.

.. code-block:: usda

    def Mesh "Mesh"
    {
        int[] faceVertexCounts = [4]
        int[] faceVertexIndices = [0, 1, 3, 2]
        point3f[] points = [(-5, 0, 5), (5, 0, 5), (-5, 0, -5), (5, 0, -5)]

        texCoord2f[] primvars:st = [(0, 0), (0, 1), (1, 1), (1, 0)] ( 
            interpolation = "faceVarying"
        )            
    }

Primvar Interpolation
---------------------

Primvars support several *interpolation modes*, based on interpolation modes of 
the primvar equivalent `Primitive Variable <https://renderman.pixar.com/resources/RenderMan_20/appnote.22.html#classSpecifiers>`__
from RenderMan. These are:

- :usda:`constant`: One value remains constant over the entire surface prim. 
  Note that only constant interpolation primvars will inherit down the namespace.
- :usda:`uniform`: One value remains constant for each UV patch segment of the 
  surface prim (which is a face for meshes).
- :usda:`varying`: Four values are interpolated over each UV patch segment of the 
  surface. Bilinear interpolation is used for interpolation between the four 
  values.
- :usda:`vertex`: Values are interpolated between each vertex in the surface 
  prim. The basis function of the surface is used for interpolation between 
  vertices.
- :usda:`faceVarying`: For polygons and subdivision surfaces, four values are 
  interpolated over each face of the mesh. Bilinear interpolation is used for 
  interpolation between the four values.

For visual examples of these modes, see :ref:`primvars`.

As :usda:`faceVarying` allows for per-vertex-per-face values, you can use this 
interpolation to create discontinuous vertex UVs or normals. For example, with 
discontinuous vertex UVs, you could create a "seam" in your texture-coordinate
mapping to emulate wrapping a label around a cylinder.

Primvar interpolation for curves, such as :usda:`BasisCurves` interprets the 
interpolation values differently, and is described in detail `here <https://openusd.org/release/api/class_usd_geom_basis_curves.html#UsdGeomBasisCurves_PrimvarInterpolation>`__.

Indexed Primvars
----------------

Primvars support *indexed data*, so data can be referred to by index rather 
than repeating the full values, thereby reducing memory usage. For example, 
if multiple vertices on a surface all use the same normal, you can specify the 
primvar index for those normals rather than repeat the same normal multiple 
times.

.. code-block:: usda

    normal3f[] primvars:normals = [(0, 0, 1),(0,0.5,0.5)] (
        interpolation = "faceVarying"
    )
    int[] primvars:normals:indices = [0,0,0,0,0,1,0,0]

Consuming Primvars in Materials
-------------------------------

USD Materials and Shaders bound to Gprims will evaluate Gprim primvars as needed 
at render-time. Your Gprim must apply the MaterialBindingAPI API schema and 
bind the material via the :usda:`material:binding` relationship.

.. code-block:: usda

    def Mesh "Mesh"
    (
        prepend apiSchemas = ["MaterialBindingAPI"]
    )
    {
        # ... geometric info and primvars omitted ...

        # Material binding, which will access various primvars on this prim
        rel material:binding = </materials/MyMaterial>
    }

Then, in your shaders, you can reference the Gprim primvars as needed using 
shader inputs connected to a UsdPrimvarReader shader. The following simple  
:ref:`USD preview Material <usd_preview_material>` applies a texture using
the UV coordinates of the Gprim bound to the Material. The example uses:

- A UsdPrimvarReader_float2 Shader that has a "varname" input to access
  the :usda:`st` primvar from the Gprim the Material has been bound to, and a 
  "result" output for the UV results.
- A UsdUVTexture Shader that has a "st" input connected to the 
  UsdPrimvarReader_float2 Shader, a "file" input to access a texture image file, 
  and an "rgb" output for the texture results.
- A UsdPreviewSurface Shader that has a "diffuseColor" input connected to the
  UsdUVTexture Shader, and "displacement" and "surface" outputs (that are
  connected to the containing Material).

.. code-block:: usda

    def Scope "materials"
    {
        def Material "MyMaterial"
        {
            token outputs:displacement.connect = </materials/MyMaterial/usdpreviewsurface1.outputs:displacement>
            token outputs:surface.connect = </materials/MyMaterial/usdpreviewsurface1.outputs:surface>

            def Shader "usdprimvarreader1"
            {
                uniform token info:id = "UsdPrimvarReader_float2"
                string inputs:varname = "st"
                float2 outputs:result
            }

            def Shader "usduvtexture1"
            {
                uniform token info:id = "UsdUVTexture"
                asset inputs:file = @./usdLogo.png@
                float2 inputs:st = (0, 1) # default provided here in case connection to usdprimvarreader1 ever broken
                float2 inputs:st.connect = </materials/MyMaterial/usdprimvarreader1.outputs:result>
                vector3f outputs:rgb
            }

            def Shader "usdpreviewsurface1"
            {
                uniform token info:id = "UsdPreviewSurface"
                color3f inputs:diffuseColor.connect = </materials/MyMaterial/usduvtexture1.outputs:rgb>
                token outputs:displacement
                token outputs:surface
            }
        }
    }

Material Primvar Fallbacks
--------------------------

Renderers also support a special "fallback" evaluation of primvars for Shaders. 
If a shader needs to evaluate a primvar that is not defined on the bound Gprim 
(or any of the Gprim's ancestors), the renderer will fallback to getting the 
primvar on the *Material* that contains the shader, if it exists.

For example, suppose you have the following Gprims defined, with both Gprims 
bound to the same MatteMaterial but with the "roughness" primvar defined on 
"cube" and not on "sphere".

.. code-block:: usda

    def Cube "cube" (
        prepend apiSchemas = ["MaterialBindingAPI"]
    )
    {
        double3 xformOp:translate = (0, 0, 0)
        token[] xformOpOrder = ["xformOp:translate"]

        rel material:binding = </MatteMaterial>
        float primvars:roughness = 1
    }

    def Sphere "sphere" (
        prepend apiSchemas = ["MaterialBindingAPI"]
    )
    {
        double3 xformOp:translate = (0, 2, 0)
        token[] xformOpOrder = ["xformOp:translate"]

        rel material:binding = </MatteMaterial>
    }

You then define MatteMaterial and the contained shaders as follows.

.. code-block:: usda

    def Material "MatteMaterial"
    {
        float primvars:roughness = 0

        token outputs:surface.connect = </MatteMaterial/Surface.outputs:surface>

        def Shader "Surface"
        {
            uniform token info:id = "UsdPreviewSurface"
            float inputs:roughness.connect = </MatteMaterial/PrimvarRoughness.outputs:result>
            token outputs:surface
        }

        def Shader "PrimvarRoughness"
        {
            uniform token info:id = "UsdPrimvarReader_float"
            string inputs:varname = "roughness"
            float outputs:result
        }
    }

At render-time, when the PrimvarRoughness shader needs the roughness input, for 
"cube" the Gprim :usda:`roughness` primvar will be used, whereas for "sphere", the 
value will be obtained through the MatteMaterial :usda:`roughness` primvar. The 
renderer is ultimately responsible for determining that the primvar does not 
exist on the bound Gprim, and substituting the Material primvar if it exists. 

.. _material_binding_purpose:

Using Material Binding Purpose
==============================

USD provides the ability to bind a material to a Gprim and also indicate the 
purpose of that binding using a *material binding purpose*. This is conceptually 
similar to the :ref:`imageable purpose <render_purpose>` attribute discussed 
earlier, but is defined at the material binding level and applies to different 
use cases. A material binding purpose lets the user describe the intent of the 
material binding, and informs the renderer which material(s) to use for 
different renders.

USD provides two material binding purposes that renderers are expected to 
support if possible:

- :usda:`material:binding:full`: used when the purpose of the render is 
  entirely to visualize the truest representation of a scene, considering all 
  lighting and material information, at the highest fidelity.
- :usda:`material:binding:preview`: used when the render is in service of a 
  goal other than a high fidelity "full" render (such as scene manipulation, 
  modeling, or realtime playback). Latency and speed are generally of greater 
  concern for preview renders, therefore preview materials should generally be 
  designed to be "lightweight" compared to full materials.

A binding can also have no specific purpose at all, in which case it is 
considered the fallback or "all purpose" binding, applying to any 
use case. All-purpose bindings have no additional suffix and are what all of 
the previous material examples have used. When no specific "full" or "preview" 
binding is found, renderers should attempt to fall back to an all-purpose 
binding, if present.

To use material binding purposes, your prim must apply the MaterialBindingAPI 
API schema, and then use the :usda:`material:binding:full` and 
:usda:`material:binding:preview` relationships to specify which materials to use. 
For example, you can specify that a Gprim is bound to a "MaterialPreview" 
Material for preview renders, and a "MaterialFinal" Material for final renders.

.. code-block:: usda

    def Mesh "MyMesh" (
        prepend apiSchemas = ["MaterialBindingAPI"]
    )
    {
        ...vertex and face data...

        # Material bindings

        # Default binding
        rel material:binding = </materials/MaterialFinal>

        # Preview/Viewport render binding
        rel material:binding:preview = </materials/MaterialPreview>

        # Final render binding
        rel material:binding:full = </materials/MaterialFinal>
    }

    ...

    # Materials definitions
    def Scope "materials"
    {
        def Material "MaterialPreview"
        {
            ...
        }   

        def Material "MaterialFinal"
        {
            ...
        }  
    }

Specifying Material Binding Purpose in Render Settings
------------------------------------------------------

USD provides prims for configuring global render settings, as described in 
:ref:`Configuring Render Settings <configuring_render_settings>`. You can 
specify a per-render material binding purpose setting via the RenderSettings 
prim. For example, you might have separate RenderSettings for final render, 
using the "full" purpose, and for your DCC tool, using the "preview" purpose.

.. code-block:: usda

    def RenderSettings "FinalRenderSettings"
    {
        uniform token[] materialBindingPurposes = ["full", ""]
        ...
    }
    def RenderSettings "DefaultRenderSettings"
    {
        uniform token[] materialBindingPurposes = ["preview"]
        ...
    }

Note that in the material binding purpose list for "FinalRenderSettings" we
specify both "full" and "" (indicating no specific purpose, or "all purpose")
bindings should be used. This ensures the full render renders prims with 
no material binding purpose specified, in case the scene is inconsistent with
applying the "full" purpose. For "DefaultRenderSettings" you only want to 
render prims with a "preview" binding purpose set, so only "preview" is 
specified in the render settings purpose list.

.. _material_collection_binding:

Binding Materials to Collections
================================

For more flexible and expressive binding of materials, you can bind materials 
to :ref:`USD object collections <usdglossary-collection>`. You might have a 
model with a complex hierarchy of Gprims, where prims in different parts of the 
hierarchy all correspond to a logical "part" of the model, and therefore are all 
grouped together in a collection. Rather than setting the same material binding 
for each individual "part", you can specify a material binding that applies to 
a collection using a :usda:`material:binding:collection:<collection name>` binding 
syntax. 

Note that the bindings are applied in namespace order, with the earliest ordered 
binding relationship the strongest. If you have prims that are included in 
multiple collections, and collection-based bindings defined for each collection, 
the binding defined first (for one of the collections that contains the prims) 
is the binding that gets applied.

The following example uses collections for the "windows" and "doors" for a model 
representing a building, and applies a material binding to those collections.

.. code-block:: usda

    def Scope "Model" (
        prepend apiSchemas = ["CollectionAPI:windows", "CollectionAPI:doors", "MaterialBindingAPI"]
    )
    {
        # Collections definitions

        rel collection:windows:includes = [
            ...
        ]
        rel collection:doors:includes = [
            ...
        ]

        # Material collection binding definitions

        rel material:binding = </materials/PreviewMaterial>  # fallback
        rel material:binding:collection:windows = [</materials/WindowMaterial>, </Model.collection:windows>]
        rel material:binding:collection:doors = [</materials/DoorMaterial>, </Model.collection:doors>] 

        # Gprims

        ...
    }

    # Materials definitions
    def Scope "materials"
    {
        def Material "WindowMaterial"
        {
            ...
        }   

        def Material "DoorMaterial"
        {
            ...
        }  

        def Material "PreviewMaterial"
        {
            ...
        }
    }

Note that a fallback binding is provided in the above example, which is used if 
no collection-based binding applies to Gprims in the model hierarchy. 

For more details on collections, see :ref:`collections_and_patterns`.

.. _material_collection_binding_strength:

Setting Collection Binding Strength
-----------------------------------

When working with layers that already come with some direct material bindings
applied, you can control the strength order of how collection-based material 
bindings are applied using the :usda:`bindMaterialAs` material binding metadata. 
The two valid token values are:

- :usda:`weakerThanDescendants`: Material bindings on descendant prims 
  in the collection override the bindings specified in the collection-based 
  bindings. This is the default behavior.
- :usda:`strongerThanDescendants`: Collection-based bindings override  
  descendant prim bindings. 

Using the previous example, if some or all of the "door" prims already have 
direct material bindings applied and you want to override those bindings, you 
could specify that, for the current stage, the collection-based binding should 
be used by setting :mono:`bindMaterialAs = "strongerThanDescendants"`.

.. code-block:: usda

    rel material:binding:collection:doors = [</materials/DoorMaterial>, </Model.collection:doors>] (
        bindMaterialAs = "strongerThanDescendants"
    )

Combining Collection Binding with Material Binding Purpose
----------------------------------------------------------

You can combine collection material binding with material binding purpose for 
use cases where you have collections that need specific material bindings and 
also need separate bindings for rendering versus preview. The binding syntax for 
this combination is 
:usda:`material:binding:collection:<purpose type>:<collection name>`. 

For example, if you wanted to specify separate materials for full renders versus 
preview renders for prims in the "windows" collection, you might have bindings 
that look as follows.

.. code-block:: usda

    # Material collection binding definitions

    # default/fallback bindings for preview and full
    rel material:binding:preview = </materials/PreviewMaterial>
    rel material:binding:full = </materials/FullRenderMaterial>

    # More specific bindings for windows collection
    rel material:binding:collection:preview:windows = [</materials/WindowPreviewMaterial>, </Model.collection:windows>]
    rel material:binding:collection:full:windows = [</materials/WindowFullRenderMaterial>, </Model.collection:windows>]

Using Material Render Contexts
==============================

Using *render contexts*, a Material can specify which output 
(or Shader output) is used for a specific renderer. For example, you might have 
a Material that specifies one Shader is used when being rendered with a display 
renderer that supports GLSLFX (e.g., Storm), and a different Shader is used when 
being rendered with RenderMan.

.. code-block:: usda

    def Material "rendererSpecificMaterial"
    {
        # Use material render context to have a single material use different 
        # outputs depending on which render context is being used

        # Fallback output used for any renderers that we didn't specify
        token outputs:surface.connect = </materials/rendererSpecificMaterial/DefaultProgramShader.outputs:surface>

        # Output used for renderers that support GLSLFX (Storm)
        token outputs:glslfx:surface.connect = </materials/rendererSpecificMaterial/GlslfxProgramShader.outputs:surface>

        # Output used for RenderMan renderers
        token outputs:ri:surface.connect = </materials/rendererSpecificMaterial/RendermanProgramShader.outputs:surface>

        ...Shader implementations omitted...
    }

You can use render contexts in combination with 
:ref:`material binding purpose <material_binding_purpose>` to provide 
finer-grain control over what shaders are used for which renderer in a "full" or 
"preview" render.

Note that, unlike material binding purpose, there's no way to specify at a global 
level in RenderSettings which material render context to use. The context is 
chosen by the renderer at render-time.

.. _image_file_formats:

*******************************
Working With Image File Formats
*******************************

USD and renderers such as Hydra support various image formats used for textures
and other purposes. This section provides guidelines for using specific image
formats with USD and Hydra.

.. _image_file_formats_all:

Guidelines for All Supported Image Formats
==========================================

If the RGB output of a UsdUVTexture shader using a supported image format is
connected to the "diffuseColor" input of a UsdPreviewSurface shader, and the
UsdUVTexture's alpha output is also connected to the "opacity" input of the
same UsdPreviewSurface, Hydra will premultiply the RGB values with the alpha 
channel. This is the only circumstance under which Hydra performs automatic 
premultiplication. This convention exists for historic reasons, and unless your 
use-case requires this historic behavior we recommend avoiding this Shadegraph 
configuration.

JPEG
====

Single channel JPEG files will be treated as raw data, three channel files will 
be treated as sRGB.

PNG
===

PNG files may be encoded as linear, or with an sRGB gamma curve. The data in an 
alpha channel, if it exists, will not be premultiplied except in the case 
mentioned in :ref:`image_file_formats_all`.

OpenEXR
=======

UsdImaging and Hydra have built-in support for OpenEXR files, with no need to
compile additional plugins. USDZ, the zip-packaged USD archive and distribution 
format, also includes OpenEXR support. This is intended to facilitate 
consistent handling of HDR imagery in platforms and applications that rely on 
USDZ.

Tiled and scanline OpenEXR images are supported, however, some
of the advanced features of OpenEXR are not. Deep pixels, multi-part images, 
layered images, depth images, and cube maps are currently not supported 
(see below for what happens if USD/Hydra is given multi-part or layered images).
Note that tiled images are read in their entirety, partial 
loading of tiled images is unsupported at this time.

Currently OpenEXR files must use linear Rec709 chromaticities and whitepoint. 
This is equivalent to sRGB without its associated electro-optical transfer 
function (i.e. the same primaries and white-point as rec709). Pixels can be 
stored in float16 or float32 format, and all compression types are supported.

Alpha values encoded in referenced OpenEXR files are not considered to represent 
geometric object coverage (i.e. alpha is unassociated with geometric 
properties).

If an OpenEXR file contains multiple layers, the layers will be inspected in order
and the first layer discovered named r or red, or ending in .r or .red, will
be chosen as the red channel, and any others ignored. Similar discovery will  
occur for green, blue, and alpha.

If an OpenEXR file contains multiple parts, only the first part will be read.

OpenEXR files are read assuming that the data window and display windows agree
in size. (0,0) corresponds to the left-bottom-most pixel.

When reading an OpenEXR file, all metadata is read and stored unaltered. When
writing, all stored metadata is written to the output file. When
writing, any standard OpenEXR attributes that are not present in the stored 
metadata are initialized to the default OpenEXR values.

OpenEXR files are scene-referred, meaning that a value of 1.0 is considered to 
be scene white, and is interpreted by a renderer accordingly. The value of the 
white luminance standard attribute found in the OpenEXR file is presently ignored.

Currently only single layer, single part, float16 and float32 RGB or RGBA images 
are written by Hio and tools like :program:`usdrecord`. A moderate lossless 
compression is applied. It is expected that more complex treatment of OpenEXR 
files including the construction of multilayer files will be completed by 
pipeline tools.

AV1 Image File Format (AVIF)
============================

The AV1 Image Format is a royalty-free open-source picture format, with modern 
compression, flexible color specification, high dynamic range values, depth 
images and alpha channels, and support for layered and sequential images. 

The supported feature set in Hydra's builtin texture manager is currently 
restricted to single frame images, which are decoded to linear Rec709 RGB or 
RGBA if an alpha channel is present.

Reading is implemented through the use of libaom, and libavif. 
YUV to RGB decoding is accomplished by libyuv. 

libaom is the reference codec library created by the Alliance for Open Media. 
libavif is a portable C implementation of the AV1 Image File Format.

See also:

- `OpenEXR reference <https://openexr.com/en/latest/index.html#openexr>`__
- `Rec709 standard <https://www.itu.int/rec/R-REC-BT.709-6-201506-I/en>`__
- `Wikipedia entry on Rec709 standard <https://en.wikipedia.org/wiki/Rec._709>`__
- `AV1 Image File Format specification <https://aomediacodec.github.io/av1-avif/>`__

.. _render_camera:

**************************
Defining the Render Camera
**************************

Renderers image the scene through a particular Camera prim. 

Camera prims used by renderers always view the scene in a right-handed 
coordinate system where up is +Y, right is +X, and the forward viewing direction 
is -Z. 

The camera has additional attributes to bound the frustum, the volume of space 
viewed in the image. These attributes include the near and far clipping range, 
as well as optional additional clipping planes. The camera also specifies an 
aperture, which bounds the X and Y axes of screen space. The projection of the 
aperture bounds to screen coordinates is known as the screen window.

The aperture is specified in view coordinates using the same units as focal 
length. Aperture and focal length units are treated as *1/10* of the current scene 
world unit, so if your world unit is centimeters, the aperture/focal unit will 
be millimeters. This automatic use of 1/10 of a scene world unit is done to help 
match real-world physical camera configurations, however if your scene world 
unit is not in centimeters, the aperture/focal length units will be 1/10 of 
whatever your world unit is, and you may have to adjust accordingly. 

For a perspective projection, the aperture describes an axis-aligned rectangle 
in the plane sitting at the focal length in front of the camera origin.

.. image:: https://openusd.org/release/api/aperture.svg
    :align: center

The following example sets the camera focal length and aperture to typical 
defaults.

.. code-block:: usda

    def Camera "Camera"
    {
        token projection = "perspective"
        float focalLength = 50
        float horizontalAperture = 20.955
        float verticalAperture = 15.29
        float2 clippingRange = (1, 1000000)
    }

For an orthographic projection, no reference plane is needed, and the aperture 
simply bounds the X/Y axes of view space. The aperture is still expressed in 
the same units as focal length in this case, although the focal length does not 
itself pertain to orthographic projection.

The camera :usda:`clippingRange` attribute sets the near and far ranges of the 
camera frustrum. Anything falling outside the clipping range is omitted. Note 
that unlike aperture and focal length, the clipping range is set in scene units, 
not 1/10 scene units. If you need more fine-grained control beyond near and far 
clipping planes, you can also provide arbitrarily oriented clipping 
planes by setting plane vectors in the :usda:`clippingPlanes` attribute.

*Depth of field* describes the finite range of focus that a camera can produce. 
Depth of field for a renderer is normally configured using the Camera :usda:`fstop`, 
:usda:`focalLength`, and :usda:`focusDistance` attributes to create the illusion of 
depth of field or realistic cinematographic effects. The focus distance is the 
distance from the camera through the center of projection to the plane of focus. 
The ratio of the focal length to the fstop determines the size of the lens 
aperture (and therefore the amount of blur), while the relation between the 
focus distance and the focal length determines the depth of field. 

Configuring Motion Blur
=======================

Motion blur in USD is controlled by setting the Camera shutter open and close 
times, along with refining how some objects participate in motion blurring using 
the MotionAPI schema. Additionally, there are :ref:`Render Settings <configuring_render_settings>` 
to control the overall application of motion blur.

For renderers that support motion blur, set the Camera :usda:`shutter:open` and 
:usda:`shutter:close` attributes to control the sampling frames used for motion 
blur. If :usda:`shutter:close` is equal to :usda:`shutter:open`, no motion blur 
will be rendered.

.. code-block:: usda

    #usda 1.0
    (
        framesPerSecond = 30
        timeCodesPerSecond = 30
        startTimeCode = 0
        ...
    )

    def Camera "Camera"
    {
        # configure sampling initial 2 frames for motion blur
        double shutter:close = 2
        double shutter:open = 0
        ...
    }

Use the MotionAPI schema and the :usda:`motion:blurScale` attribute to specify the 
scale of motion blur applied to the object and its child prims. A blurScale of 
0 turns motion blur off for the given object and its children. 
A blurScale value greater than 1.0 will exaggerate the blurring by sampling 
motion progressively further outside the sampling range indicated by the 
camera's shutter interval; for some renderers, this may have a performance 
impact but provides an easy way to achieve some artistic looks.

.. code-block:: usda

    def Xform "Xform"
    (
        prepend apiSchemas = ["MotionAPI"]
    )
    {
        # Double the amount of motion blur (x2) for this Xform and all imageable child prims
        float motion:blurScale = 2
        ... 
    }

See `Modulating Motion and Motion Blur <https://openusd.org/release/api/usd_geom_page_front.html#UsdGeom_MotionAPI>`__ 
for more details on using the MotionAPI schema for applying motion blur.

You can disable all motion blur for a particular render pass by using the 
:usda:`disableMotionBlur` attribute of the RenderSettings prim. 

.. code-block:: usda

    def RenderSettings "NoBlurRenderSettings"
    {
        uniform bool disableMotionBlur = 1
        ...
    }

.. _configuring_render_settings:

***************************
Configuring Render Settings
***************************

USD provides a set of *render settings* prims to configure how your scene will 
be rendered. When you define these prims in your USD scene, the renderer is 
responsible for applying these settings (as best as possible) when rendering 
the scene. Note that renderers that support USD should have reasonable defaults 
that will be applied if render configuration isn't available in the USD data.

As a best practice, group all your render settings prims (RenderSettings, 
RenderProduct, RenderVar, RenderPass) in a scene under a common root prim named 
"Render". This encapsulates render-related specifications from the rest of your 
scene data so that, even in large scenes, render specifications can be accessed 
efficiently using features like UsdStage masking. The following example has two 
RenderSettings and a RenderProduct grouped under a Scope "Render" prim.

.. code-block:: usda

    def Scope "Render"
    {
        def RenderSettings "PrimarySettings" {
            rel products = </Render/PrimaryProduct>
            int2 resolution = (512, 512)
        }
        def RenderSettings "PrimarySettingsRaw" {
            rel products = </Render/PrimaryProduct>
            int2 resolution = (1024, 1024)
            uniform token renderingColorSpace = "raw"
        }
        def RenderProduct "PrimaryProduct" {
            rel camera = </World/main_cam>
            token productName = "/scratch/tmp/render000009.exr"
        }
    }

A USD stage may contain one or more *RenderSettings* prims. The stage metadata can 
specify the default RenderSettings to be used via the :usda:`renderSettingsPrimPath`
layer metadata field.

.. code-block:: usda

    #usda 1.0
    (
        renderSettingsPrimPath = "/Render/PrimarySettings"
    )

Each RenderSettings prim encapsulates all the settings and components that tell 
a renderer what render settings to use, and what render output to produce, for 
a single invocation of rendering the scene. A RenderSettings prim will contain 
global renderer settings, such as working colorspace, and the prim that 
represents the camera for the render. RenderSettings also may contain one or 
more RenderProducts, although to get a default RGB color image render, you can 
omit the products specification. 

A *RenderProduct* represents a single render output artifact, such as a rendered 
image file or an output depth buffer. RenderProducts can override some of the 
configuration in a RenderSetting (such as the camera), but also have 
product-specific settings, such as the output "product name" (for a rendered 
image, the image filename).

RenderSettings and RenderProduct can designate the :ref:`render camera <render_camera>` 
used for rendering via setting the :usda:`camera` relationship to a Camera prim. The 
Camera prim determines the visual composition of the scene as an 
image, and represents creative choices distinct from the technical render 
settings used to configure image generation. This is why some attributes 
originate from the camera and others (such as pixel resolution) are expressed 
separately as render settings, and may vary per RenderProduct.

A RenderProduct can specify one or more *RenderVars*. Each RenderVar represents 
a quantity or "channel" of computed output data that can vary across an output 
artifact. A product may contain multiple channels of data representing related 
values or variables sampled by a render process. The RenderVar prim specifies 
what values the renderer should output and how the renderer should produce them. 
Examples of render variables include geometric measurements such as 
camera-space depth, quantities emitted by material shaders, light path 
expressions (LPE's), and quantities intrinsic to the renderer such as 
computation time per pixel. Note that USD does not yet enforce a set of 
universal RenderVar names and formats, so renderer-specific RenderVars are 
expected. In the following example, the "PrimaryProduct" RenderProduct 
specifies four RenderVars representing channels for color, alpha, directDiffuse, 
and a general ID value.

.. code-block:: usda

    def RenderProduct "PrimaryProduct" {
        rel camera = </World/main_cam>
        token productName = "/scratch/tmp/render000009.exr"
        rel orderedVars = [
            </Render/Vars/color>,
            </Render/Vars/alpha>,
            </Render/Vars/directDiffuse>,
            </Render/Vars/id>
        ]
    }
    def Scope "Vars"
    {
        def RenderVar "color" {
            string sourceName = "Ci"
        }
        def RenderVar "alpha" {
            token dataType = "float"
            string sourceName = "a"
        }
        def RenderVar "directDiffuse" {
            string sourceName = "C<RD>[<L.>O]"
            token sourceType = "lpe"
        }
        def RenderVar "id" {
            token dataType = "int"
            string sourceName = "id"
        }
    }

*RenderPass* encapsulates multi-pass rendering workflows, letting you specify a 
different RenderSetting for each render pass. For example, you might have a 
workflow that uses separate render passes and settings to render the foreground 
and background portions of a scene, and a third pass that composites the 
foreground and background render output into the final frame. RenderPass can 
point to a RenderSettings for render configuration for the pass, or point to 
product-specific configuration for external renderers that may not describe a 
render in terms of RenderSettings (e.g., compositing applications). In addition 
to organizing the different renders and processes (such as denoising and 
compositing) that collectively produce a "final frame", RenderPass codifies the 
dependencies between passes. A single pass generally represents not just a 
single set of products, but a sequence of temporally varying frames of outputs 
that depend on temporally varying inputs.

RenderPass also lets you specify 
:ref:`collections of prims<collections_and_patterns>` that are visible to the 
renderer for that pass, using the :usda:`collection:renderVisibility` collection. 
Use this collection if you have separate passes for different sets of objects 
in the stage (e.g., separate foreground and background passes), or passes that 
only apply to specific types of objects.

The following example shows three RenderPasses. A "foreground" pass and a 
"background" pass are specified that use RenderMan and the "PrimarySettings" 
RenderSettings configuration, but specify different parts of the stage to render 
using the RenderPass :usda:`renderVisibility` collection. A final "composite" pass 
is also specified that uses Nuke and takes the results from the other two passes 
as :usda:`inputPasses`. Note that the :usda:`nuke:writeNode` attribute and Nuke 
:usda:`renderSource` are hypothetical examples that would be associated with a 
Nuke-supplied API schema applied to the "composite" RenderPass prim -- USD does 
not provide any default Nuke render configuration support. 

.. code-block:: usda

    def Scope "Render"
    {
        ...settings and products...

        def Scope "Passes"
        {
            def RenderPass "foreground" 
            {
                token passType = "prman"
                rel renderSource = <Render/PrimarySettings>
                string[] command = ["prman"]
                uniform bool collection:renderVisibility:includeRoot = false
                prepend rel collection:renderVisibility:includes = [
                    </World/characters>,
                    </World/sets/Kitchen/Table_grp>,
                ]
            }
            def RenderPass "background" 
            {
                token passType = "prman"
                rel renderSource = <Render/PrimarySettings>
                string[] command = ["prman"]
                uniform bool collection:renderVisibility:includeRoot = true
                prepend rel collection:renderVisibility:excludes = [
                    </World/characters>,
                    </World/sets/Kitchen/Table_grp>,
                ]
            }
            def RenderPass "composite"
            {
                token passType = "nuke"
                asset fileName = @composite.nk@
                # this nuke-namespaced property might come from a hypothetical Nuke-supplied API schema
                string nuke:writeNode = "WriteFinalComposite"
                rel renderSource = </Render/Passes/composite.nuke:writeNode>
                string[] command = ["nuke", "-x", "-c", "32G"]
                rel inputPasses = [
                    </Render/Passes/foreground>,
                    </Render/Passes/background>
                ]
            }
        }
    }

For more details on the standard USD attributes for the render settings prims, 
see:

- `RenderSettingsBase <https://openusd.org/release/api/class_usd_render_settings_base.html>`__
- `RenderSettings <https://openusd.org/release/api/class_usd_render_settings.html>`__
- `RenderProduct <https://openusd.org/release/api/class_usd_render_product.html>`__
- `RenderVar <https://openusd.org/release/api/class_usd_render_var.html>`__
- `RenderPass <https://openusd.org/release/api/class_usd_render_pass.html>`__

Note that renderers are expected to add renderer-specific properties to the 
USD render schemas via auto applied API schemas, and document those settings in 
the renderer documentation. 