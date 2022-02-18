==================================
Coordinate Systems in USD Proposal
==================================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `UsdShadeCoordSysAPI page <api/class_usd_shade_coord_sys_a_p_i.html>`_.

Copyright |copy| 2019, Pixar Animation Studios, *version 1.0*

.. contents:: :local:

Purpose
=======

Shading artists sometimes need to work in 3D spaces other than a gprim's object
space or world space, and when they do, the renderer generally must perform its
shading calculations in the same space. A common and motivating example of this
is `projection painting <https://www.youtube.com/watch?v=fODto17By7o>`_ , which
allows texture (or as in the video, complete material) application "projected
through a camera" without requiring UV's. These "custom render spaces" must be
identified in the scenegraph (:cpp:`UsdStage`) by name, so that they can be
discovered and made available to renderers, which allow shaders to access them
by name. A simpler, also common use for render spaces is to define a stable 3D
coordinate system for spatial, procedural textures (e.g. marble), so that an
object does not "swim" through the texture as it animates; in this simpler case
we are not projecting anything, and therefore other camera information beyond a
3D frame of reference are unneeded.

As of USD 19.03, we provide a Renderman-only embodiment of `Scoped Coordinate
Systems
<api/class_usd_ri_statements_a_p_i.html#a631ac5bee48904231b44abf5688362fe>`_
that are very fitted to the Ri specification. The purpose of this document is to
design a more general encoding for coordinate systems that is useful to any
renderer or shading system.

Requirements
============

Coordinate Systems are Identified by Name in Shaders
----------------------------------------------------

This is more of a precondition under which we must operate, rather than a
requirement we impose for this design. Typically in USD, objects that want to
"use" other objects will declare their dependency **in** USD using connections
or relationships to robustly identify the depended-upon object. In addition to
being a robust identification when objects are renamed across references, this
explicit mechanism allows us to identify "load dependencies" such that, for
example, when opening a `masked Stage
<api/class_usd_stage.html#ade1d90d759a085022ba76ff910815320>`_, we can ensure we
include any prims that the requested prims require for correct behavior.

Being part of a model's geometric description, a coordinate system naturally
lives/is-defined-in the model's geometry namespace. :usda:`Material` s and their
constituent shaders are often defined somewhere "off to the side" of the
geometry that is bound to the :usda:`Material` s, with no precedent for shaders
declaring explicit dependencies on geometry: the only "cross hierarchy"
dependency points in the opposite direction: geometries *bind* to a
:usda:`Material` via a relationship that targets the :usda:`Material` prim. The
UsdShade object model reinforces this single-directionality-dependence by
stipulating that **materials must fully encapsulate their shading networks** ,
which means that shaders inside the :usda:`Material` **cannot** possess a
relationship (or connection) that targets an object outside the
:usda:`Material`, in the geometry hierarchy. Therefore we would need to
introduce a level of indirection in which shaders target a relationship on the
containing :usda:`Material` itself, and then the :usda:`Material` targets the
coordinate systems its shaders require. This would be unfamiliar and awkward to
shading artists and workflows. Furthermore, some shading languages (e.g. OSL)
allow the names of coordinate systems and other "external dependencies" to be
computed on the fly, and such dependencies could never be captured by a
relationship or connection.

In consideration of these issues, we therefore stipulate that shaders must be
able to refer to and retrieve coordinate systems based on simple names, rather
than via scenegraph pointers like connections and relationships. This is
consistent, for example, with how `OSL
<https://github.com/imageworks/OpenShadingLanguage/blob/release/src/doc/osl-languagespec.pdf>`_
shaders can use the transform() and matrix() built-in functions to transform
points/vectors/normals between arbitrarily-named coordinate systems, and fetch
transformation matrices by name, respectively.

Coordinate Systems Must be Scoped
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

From the first requirement, we know that shading systems (through their host
renderers) must be able to refer to coordinate systems by some name or
identifier. An artist authoring a shader or shading network that uses the
coordinate system must use that name. To uniquely identify the proper
coordinate system, we have two options:

    #. **Require each coordinate system to have a globally unique name within
       the scene.**

       This allows any shader to access any frame of reference, which provides
       great freedom, but requires that we either use the full scene-path of
       each coordinate system **as its identifier** , which means long,
       hard-to-read entries for shading artists to deal with, **or** we come
       up with some scheme that allows shorter names that are procedurally
       evaluated at render-time leveraging inherited attributes to create a
       "more unique" name, such as ``coordSysName =
       "${ASSEMBLY}_${COMPONENT}_modelSpace"``; such schemes add complexity into
       the pipeline, as well as a runtime performance cost, however.Globally
       uniquely-named coordinate systems can also, generally, be defined
       anywhere in a source scenegraph. This means that to render even a small,
       namespace-coherent part of a scene (subtree), we may potentially need to
       examine the *entire* scene before rendering to find all needed coordinate
       systems. It also makes the use of masked stages error-prone, because we
       have no Usd-level tools to determine we have included enough of the scene
       to ensure we have all the coordinate systems we require.

    #. **Stipulate that coordinate systems are always scoped to their subtrees,
       with normal inheritance override semantics.**

       In this approach, a coordinate system is "visible" only to the prims
       defined (at or) below the prim on which it is defined, in namespace. This
       allows coordinate systems to have short, descriptive names, and be
       usefully packaged in assets that will be referenced/instanced many times
       into scenes, at arbitrary nesting levels, while still providing
       deterministic behavior.  If an ancestor prim and a child prim both define
       a :usda:`modelSpace` coordinate system, it is the **child prim**'s
       :usda:`modelSpace` that will be seen by all the descendants of the
       child. It becomes the responsibility of the renderer (or render-prepper,
       such as Hydra) to keep track of the namespace context while emitting
       shaders, to ensure that the correct mapping happens when a shader refers
       to a coordinate system by name.

The Renderman Specification historically allowed both types of coordinate
systems. For simplicity, we choose to support only one, and opt for the more
artist-friendly and masking/isolate-friendly encoding of **scoped coordinate
systems.**

Multiple Coordinate Systems per Prim
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is often useful to define more than one different coordinate system at the
same prim in namespace - "model root" prims are a natural place to define the
majority of spaces needed by prims within the model.

Proposed API Schema
===================

The schema has two concerns:

    #. Recording the (possibly animated) frame of reference that defines a
       coordinate system

    #. Naming and "binding" the frames of reference to a subtree of namespace.


Recording a Frame of Reference
------------------------------

Because a 3D frame of reference can be fully specified as the transformation
from scene world space to that frame, such as by a USD :usda:`matrix4d` value
type, we could express frames of reference compactly and simply as
:usda:`matrix4d`-valued attributes that we add to the scoping prim. The
disadvantage in doing so is that many interesting frames of reference are
already partially or fully defined by :usda:`Xformable` prims already present in
the scene; therefore, we would be baking down and duplicating data in order to
define coordinate systems, and when the frame of reference is animated, that
duplication could be considerable, and in any case is an extra computation step
we would always need to perform. Further, applications such as Maya already
represent frames of reference as prim-like objects, so we would need to
"deconvolve" on import.

Therefore, we propose to represent frames of reference as full prims. We allow
**any** :usda:`Xformable` prim to serve as the definition of a frame of
reference. We do not require any new schema for defining frames of reference; if
a new prim in the scene is required for no other purpose than to serve as a
coordinate system frame, we can simply create and populate an :usda:`Xform`
prim. This also allows us to give special behavior to :usda:`Camera` prims for
projection, as we will see below.

Binding Frames of Reference
---------------------------

Because we may wish to add coordinate systems to prims that already serve other
primary purposes, such as a :usda:`SkelRoot` prim, it is clear that **we want to
use an** :ref:`glossary:API Schema` **to name/bind coordinate systems** . We
have two patterns for adding "multiple somethings" via an API schema: when the
"something" we want to add is simple, effectively a single property, we use a
`non-applied API schema
<api/_usd__page__generating_schemas.html#Usd_IsAVsAPISchemas>`_ with API that
adds custom, namespaced properties, like `UsdGeomPrimvarsAPI
<api/class_usd_geom_primvars_a_p_i.html#details>`_ ; when the "something" is a
collection of related properties, we use the larger machinery of a
`Multiple-Apply API schema
<api/_usd__page__generating_schemas.html#Usd_IsAVsAPISchemas>`_ , such as for
`UsdCollectionAPI <api/class_usd_collection_a_p_i.html#details>`_ .

Since our frames of reference are defined by independent prims, all that we need
to do on the "scoping prims" that bind coordinate systems for their descendants
is to identify them via **relationship** . If the names of the
frame-of-reference-defining prims were suitable to use as the names for the
coordinate systems, then we could have a single, multi-target relationship for
binding. However, we do not want to rely on that always being true (sometimes
the person needing to add a coordinate system may not be able to set or change
the name of the prim that already defines it), and it also imposes the
limitation that we cannot create two coordinate systems of different names with
the exact same frame of reference, without copying scene description or adding a
composition arc on a new prim.

We opt instead for a "relationship-per-coordinate-system" approach, which makes
the schema look very much like (a greatly simplified) :cpp:`UsdGeomPrimvarsAPI`:
we provide a name for each coordinate system we bind **at the binding site** ,
which becomes the (namespace-prefixed) name of a relationship, and it is that
name by which the coordinate system is known to all descendant prims and their
consumers.

Bindings of coordinate systems inherit **down the geometry hierarchy**. We
find this very useful, for example, in using projection textures to paint on an
entire set/environment full of objects in context. Projection is especially
useful because many of our set models may not have UVs, and inheritance of
coordinate systems means that we can define a single projection "at the set
level" to be shared by all models in the set.

CoordSysAPI
-----------

Concretely, **UsdShadeCoordSysAPI** will provide creation and interrogation API
similarly to `UsdGeomPrimvarsAPI
<api/class_usd_geom_primvars_a_p_i.html#details>`_ , except that instead of
dealing with specific :cpp:`UsdGeomPrimvar` objects, it deals simply in
:cpp:`UsdRelationship` objects, with convenience API for extracting names and
frames-of-reference **by** name. The namespace prefix for these relationships
will be :usda:`coordSys:` (which we choose over coordinateSystem for brevity's
sake, as well as internal Pixar convention. All that remains is to define the
math by which we apply a coordinate system.

USD Sample and Analysis
=======================

Here is an example of three coordinate systems defined on a model, with the
implied composite transformations for the model's gprims.

.. code-block:: usda

   #usda 1.0
   (
      upAxis = "Y"
   )
   
   def Xform "World"
   {
       double3 xformOp:translate = (5, 0, 0)
       token[] xformOpOrder = ["xformOp:translate"]
   
       def Material "SetLevelMaterial"
       {
           # This is NOT a complete Material definition!
           def Shader "PrimvarReader_1"
           {
               uniform token info:id = "PxrPrimvar"
               string inputs:varname = "pref"
               string inputs:coordsys = "instanceSpace"
               string inputs:type = "point"
           }
       }
       def Xform "Model"
       {
           rel coordSys:modelSpace = </World/Model/Geom>
           rel coordSys:instanceSpace = </World/Model>
           rel coordSys:paintSpace = </World/Model/Place3dTexture>
           
           double3 xformOp:translate = (2, 0, 0)
           token[] xformOpOrder = ["xformOp:translate"]
   
           def Camera "Place3dTexture"
           {
               token projection = "orthographic"
               # clippingRange can be specified to ensure projection 
               # only affects front surfaces
               float2 clippingRange = (1, 35)
               double3 xformOp:translate = (0, -10, 0)
               token[] xformOpOrder = ["xformOp:translate"]
           }
   
           def Xform "Geom"
           {
               def Cube "Box"
               {
                   double3 xformOp:translate = (0, 0, 3)
                   token[] xformOpOrder = ["xformOp:translate"]
                   rel material:binding = </World/SetLevelMaterial>
               }
           }
       }
   }

Analysis: Coordinate Systems Evaluated
--------------------------------------

The aggregate transformation for the gprim **Box** in the different possible
Coordinate Systems encoded above is:

    * **localToWorld(Box) is a translation of (7, 0, 3)** units, the
      concatenation of Box's local transformation with that of its ancestors.

    * **modelSpace(Box) is a translation of (0, 0, 3)** units, because
      :usda:`modelSpace` has been bound at :sdfpath:`/World/Model`, so the change
      of coordinate system from :sdfpath:`/World/Model` to
      :sdfpath:`/World/Model/Geom` cancels out all inherited transformation. This
      would be a good choice for defining a marble texture for the model.

    * **paintSpace(Box) is a translation of (0, -10, 3)** units, because
      *paintSpace* has been bound at :sdfpath:`/World/Model`, so the change of
      coordinate system from :sdfpath:`/World/Model` to
      :sdfpath:`/World/Model/Place3dTexture` contributes :usda:`Place3dTexture`
      's Y translation to the result.

    * **instanceSpace(Box) is a translation of (2, 0, 3)** units. This is
      perhaps the most interesting "edge" case, in which a coordSys targets its
      own binding prim, which has the effect of posing geometry into the space
      in which the "model instance" lives, by effectively excluding the
      contributions of the binding prim's ancestors.

Analysis: Coordinate System Binding and Consumption
---------------------------------------------------

The above example also includes a partial definition of a
:cpp:`UsdShadeMaterial` prim, to which the Box geometry is bound. We
specifically placed the :usda:`Material` *outside* the definition and binding of
the named coordinate systems to illustrate that coordinate systems bind and
inherit with **geometry**, not with the :usda:`Material` s whose shaders
ultimately use them.

The example :usda:`Material` contains a single Renderman shading node, the
`PxrPrimvar <https://rmanwiki.pixar.com/display/REN/PxrPrimvar>`_ node, which is
one of several builtin shaders that provide a string input, *coordsys* , that
names a builtin or custom coordinate system into which the named primvar (if of
a spatial *type*) will be transformed. The **value** of that coordinate system
will be different depending on the gprim for which the shader is being
executed. When executing for theBoxgprim, it will be the instanceSpace forBox,
as defined above - but could be different for other geometry bound to the same
:usda:`Material`.

Projections, Cameras, and CoordSysAPI
=====================================

In the above example, the prim targeted by :usda:`coordSys:paintSpace` is a
:cpp:`UsdGeomCamera` prim. Projections for projection paint are most often
established in DCC's by manipulating a camera, as many camera properties are
intuitive and useful for specifying how the projection should be applied, such
as the projection method, aperture (for scaling the texture), and clipping
planes (for determining what geometry should receive paint). But since
ultimately, the work of applying the projection paint happens in a shader, and
most renderers do not give shaders direct access to camera parameters, these
"extra" parameters are typically expressed on the shading prim that performs the
projection (such as `PxrProjector
<https://rmanwiki.pixar.com/display/REN/PxrProjector>`_), and must be *copied*
from a :usda:`Camera` prim to a shader encapsulated inside a :usda:`Material`.

In order to make the USD encoding of projection as natural and robust as
possible for artists, we stipulate as part of the object model that, **when the
target of a** :bi:`coordSys` **is a** :usda:`Camera` **prim**, a Hydra scene
delegate will extract the relevant projection-related properties from the
camera, and make them available to Hydra render delegates as part of the Hd
representation of the coordinate system.** If this seems reasonable in spirit,
it will be a coda of this proposal to define precisely what :usda:`Camera`
properties (and other, API-schema-based properties on a :usda:`Camera` prim,
such as properties defining feathering between projections).

This behavior makes it the responsibility of the render delegate to map those
parameters, just-in-time, to the relevant parameters of its renderer-specific
shading nodes. For example, in this world, when an artist adds a PxrProjector
node to their shading graph, then providing they have established the associated
coordinate system as a :usda:`Camera`, then once they have set the
*coordinateSystem* parameter on the PxrProjector to the name of the
:usda:`coordSys` , they no longer need to provide values for any of the
camera-related parameters: **hdPrman** will propagate them from the
:cpp:`HdCoordSys` to the :cpp:`PxrProjector`.

Although this "parameter propagation" *could* be tied to particular shading
nodes, we believe it could also be facilitated by metadata in
:cpp:`SdrShaderProperty` definitions, similarly to how we already can annotate
string inputs as representing **asset paths** , we can annotate string inputs as
being **coordSys-naming** . Then, a render delegate would attempt parameter
propagation for any node that has one or more coordSys-naming inputs that are
authored.

We have stated this behavior in terms of the Hydra architecture, but it must be
adhered to by any non-Hydra consumer of USD, also, for the data to be useful for
interchange.
