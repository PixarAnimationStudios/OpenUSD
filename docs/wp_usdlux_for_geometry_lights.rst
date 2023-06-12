==============================================
Adapting UsdLux to Accommodate Geometry Lights
==============================================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `UsdLux overview page <api/usd_lux_page_front.html>`_.

Copyright |copy| 2021, Pixar Animation Studios,  *version 1.2*

.. contents:: :local:


This document contains the most relevant sections of a much longer,
Pixar-internal document that also deeply discussed alternative formulations, as
well as engineering challenges that have either already been resolved, or relate
to UsdLux's integration into Presto.

Introduction and Background
===========================

As of USD 21.05, :cpp:`UsdLuxGeometryLight` is skeletally defined, lacking
critical specification (and possibly controls) for reproducible-across-renderers
behavior. A GeometryLight is simply a Light that possesses a (single target? -
not enforced) relationship to :cpp:`UsdGeomGprim` source geometry. Before we can
add GeometryLight support to Hydra and HdPrman, we need to address two concerns:

    * Ensure that we have sufficient and robust controls exposed, such that we
      can accommodate the workflow needs our artists developed for Mesh Lights
      in Katana.

    ..

    * Sufficiently lock down and describe the behavior of both the GeometryLight
      prim and the source Gprim so that renderers have a good chance of
      reproducing the artist's intent when the lighting was developed in a
      different renderer.

In support of the first concern, we have had extensive internal discussions
about workflow. This document and the proposal it contains are derived from
those discussions, with inspiration also from several key discussions on the
OpenUSD forum.

In support of the second concern, we have done a documentation search on
`RenderMan <https://rmanwiki.pixar.com/display/REN23/PxrMeshLight>`_, `Arnold
<https://docs.arnoldrenderer.com/display/a5AFMUG/Ai+Mesh+Light>`_, `Octane
<https://docs.otoy.com/StandaloneH_STA/StandaloneManual.htm#StandaloneSTA/MeshEmitters.htm>`_
, and `3Delight NSI
<https://documentation.3delightcloud.com/display/3DFM9/Area+and+Mesh+Light>`_
(see also `this usd-interest discussion
<https://groups.google.com/g/usd-interest/c/wsBarHHGXH8/m/A6XAo3S_AQAJ>`_).

Mesh Light Support in the Industry
##################################

At the renderer level, there are two different philosophies about what
constitutes a "mesh light". Renderers like RenderMan and Arnold consider a mesh
light to be a non-analytic light whose shape (and potentially emission pattern)
are defined by a mesh, which is effectively subsumed into the light; so
basically, this conception views meshes as a way to define arbitrarily-shaped
light-sources. Renderers like Octane and 3DelightNSI, on the other hand, simply
consult the material's emissive or glow properties on **normal gprims** to
decide whether the gprim should be recognized as contributing illumination; this
makes it easy and natural to create meshes that have a mixture of diffuse,
specular, and emissive responses, such as control panels, but makes it more
awkward to provide sufficient lighting-specific controls, and to identify the
objects with which lighters must interact.

Either formulation *can* accomplish the goal of the other, though users'
experience in this area will largely be determined by the DCC they use to
establish the lighting - most popular DCC's require mesh lights to be set up
using a renderer-specific toolset/specification, and the featureset and
specifiable behavior generally differs from renderer to renderer.

Recently, the RenderMan team has been thinking about generalizing support for
arbitrary light sources, in a *similar* vein to Octane and 3Delight in that no
special "light primitive" would be required, but *without* sacrificing the
features or artistic control that we currently have with "traditional" lights. 
This would be a transformative change that necessarily (and desirably) ripples
up and out through the UsdLux encoding of lights. We believe that making such
changes to UsdLux will become increasingly expensive within Pixar and especially
in the larger community the longer we wait; therefore we are willing to consider
more extensive changes to UsdLux if they address not only "the geometry light
problem," but also looks forward to the "any prim can be a light" world.

Goals
#####

At Pixar we regularly need both of the above formulations of mesh lights.
Taking into account :cpp:`UsdStage`'s envisionment as a user-level scenegraph,
our chief goal is to provide and describe a schema and setup for mesh lights
that reduces the amount of manual work artists need to do to achieve either
formulation, drawing from the pain-points and customizations we've experienced
and built in our pre-UsdLux pipelines. Additionally, we want to address the
following:

    * Make it easier for DCC's to provide a uniform specification and user
      experience for Mesh Lights, independent of targeted renderer. 
      Standardizing user scenegraph representation means we are necessarily
      pushing more work to the "render prep" software layer to translate, but
      USD's Hydra architecture provides appropriate entry-points for that
      translation to happen, with the benefit that renderers need only implement
      that translation once, rather than per-DCC.

    ..

    * Ensure that whatever formulation we provide for "mesh light" can be
      extended to accommodate other gprim light sources, as well. For example,
      some, if not all, renderers additionally allow at least *volumes* to be
      used as light sources, though there is no standardized means to accomplish
      this as a user.

Workflow Considerations
#######################

Our design must accommodate some workflow needs particular to geometry lights.

Emission and Glow: In Sync Except When Not
******************************************

When the emissive geometry is a modeled, shaded surface built into the
environment and intended to be visible to camera, we often find that achieving
the desired look requires separating the directly viewed "visible glow" from the
indirect light scattered into the environment. For example, we might want the
intensity of light emitted into the environment to be somewhat greater than the
glowing lines and buttons embedded in a glass control panel, as determined by
the panel's Material's emissive lobe.

We will explore this issue and its implications on our design in the
:ref:`Behaviors<wp_usdlux_for_geometry_lights:proposal and behaviors>` section
below.

Lighting Post-Modeling
**********************

Although the existence of UsdLux allows practical lights to be added to assets
as part of the modeling process, we anticipate that will not always be the
case. Further, assets created in environments whose renderers address geometry
lighting purely as a material property may not contain geometry lights. 
Therefore, our design must allow for adding "geometry light-ness" into/onto
existing assets, non-destructively.

Design Space and Issues
=======================

The primary design question is whether to represent geometry lights as a single
prim or as a pair of related prims.

Dual-Prim Geometry Light Problems
#################################

The 21.05 :cpp:`UsdLuxGeometryLight`, inspired by Katana and other DCC's,
represents the geometry light with two prims, one for the source geometry, and
the other for the light emission (and identification *as* a Light). This
separation of concerns into different prims seems flexible, and also maps nicely
to the (non-physical) distinction between emission and glow that we discussed in
the :ref:`Workflow Considerations<wp_usdlux_for_geometry_lights:workflow
considerations>` section.

However, splitting the representation into two prims introduces a different set
of problems, primarily centered on inherited scenegraph state. Examining our
experience building a custom, two-prim geometry light solution in Katana
revealed that it required significant code complexity to address problems of
position/transformation, visibility, and inherited primvar/attribute uncertainty
arising from the fact that the "geometry light" can typically provide two
answers to each of these scenegraph-state questions, and we require exactly one.

We attempted to mitigate the confusion that can arise from the
:cpp:`UsdRelationship` based two-prim formulation by instead posing geometry
light as a "parent and child" prim relationship, where either the Light is a
child of the Mesh, or the Mesh is a child of the Light. While this formulation
*reduces* the potential for confusion over the inherited state for the geometry
light, it does not eliminate it, and also leads to the questions: should the
Light be the parent, or the Mesh? Or, given that each has different impacts on
workflow and asset-construction considerations, would we need to support *both*
formulations?

Single-Prim Geometry Light
##########################

We can avoid all of the synchronicity and inheritance complications of the "two
prim" encodings by stipulating that a geometry light is represented as a single
prim.

Because USD does not allow multiple inheritance in the schema type hierarchy, we
cannot derive a MeshLight from Mesh and Light, nor a VolumeLight from Volume and
Light. USD's mechanism for behavior mixing is applied API schemas. So, the
only way we could formulate a single-prim geometry light would be to provide a
"LightAPI" schema that would endow Light-ness onto a Mesh or Volume prim. Thus,
**there need be no GeometryLight schema at all** , though as we will see when
discussing behaviors below, there may be a few new properties we would wish to
add to LightAPI itself.

We *could* accomplish this by gutting the concrete :cpp:`UsdLuxLight` schema,
moving its entire definition into :cpp:`UsdLuxLightAPI`, and then "simplify" the
definition of Light to simply add LightAPI as a builtin applied schema. This
would complicate the schema domain somewhat, especially the API. Because the
core light properties and methods would no longer be defined on the Light
base-class, things like python tab-completion on derived light-types would only
find the properties and methods unique to the derived type, not the shared base
properties, though coders would hopefully soon become accustomed to interacting
with lights primarily through the LightAPI schema, directly.

It would also become somewhat more awkward and expensive to identify lights in
the scene, since instead of asking simply :cpp:`prim.IsA<UsdLuxLight>()`, we
would need to additionally ask :cpp:`prim.HasA<UsdLuxLightAPI>()`, **and**
provide warning that the former, easy-to-reach-for query will miss geometry
lights. In discussions last year, this was given as the predominant reason for
rejecting a single-prim encoding for geometry lights.

However, if we instead **remove the UsdLuxLight base-class altogether** , with
"Is a light" status bestowable **only** by :cpp:`prim.HasA<LightAPI>`, then
confusion and identification expense are greatly reduced. While this is about as
fundamental a change as we could make to the UsdLux domain, and the engineering
expense of making it will be non-trivial, it is worth examining closely,
because:

    * We *can* roll it out in a way that is non-breaking to existing assets
      (beyond deprecating :cpp:`UsdLuxGeometryLight`), in deference to vendors
      already producing UsdLux-containing documents.

    ..

    * This reformulation of UsdLux provides us with almost exactly the object
      model that forward-looking RenderMan requires: any geometry can be
      emissive, based on its surface shader, but when we require more artistic
      control over the intensity, shape, or selectivity of the emission or its
      shadows, we can simply augment the prim with a LightAPI (and other API's
      already factored separately, such as ShapingAPI and ShadowAPI), which
      provides precisely the needed controls in a way that can be recognized by
      renderers.

In the following sections we will first lay out what this refactoring might look
like (and why), and then confirm our proposal with a concrete behavior
specification for cross-renderer "geometry light" support.

UsdLux LightAPI OM
==================

Light -> LightAPI
#################

From a UsdLux perspective, this change is fairly straightforward, though we draw
attention to a couple of issues:

    * Applying :cpp:`UsdLuxLightAPI` to a prim will bestow UsdShade
      Connectability to a prim, making it a "container", since lights can
      contain pattern networks and LightFilters. When applied to a Mesh, for
      example, *none of the Mesh's schema properties will become connectable* .
      Recall that connectable properties must be in the "inputs" or "outputs"
      namespaces, so there is no collision or confusion of behaviors: only the
      LightAPI properties will be connectable.

    ..

    * We may want to provide a utility function for identifying prims that are
      lights, which simply applies the :cpp:`HasA<LightAPI>()` test.

Introduce Convenience Base Classes
##################################

We had already noted in our own early use that it is problematic for all of the
Lux lights to inherit from a single base class, because some lights *are*
`boundable
<api/class_usd_geom_boundable.html#details>`_
, while others, like DomeLight and DistantLight, are not. The introduction of
LightAPI liberates us to address this situation. To make things easy and obvious
for concrete light types, we propose to introduce *two* base classes:
:cpp:`UsdLuxBoundableLightBase` , and :cpp:`UsdLuxNonboundableLightBase` , which
inherit, respectively, from :cpp:`UsdGeomBoundable` and :cpp:`UsdGeomXformable`.
Each of these schemas will have LightAPI applied as a builtin, in its
definition.

Adding these classes for the concrete lights to derive from also gives us an
opportunity to keep existing client code of UsdLux from breaking, by adding a
feature to :program:`usdGenSchema` that will *reflect the builtin API* of
single-apply API schemas onto the schemas that apply them as builtins. This
presents the entire set of Light mutators and accessors to all of the typed
schemas that derive from the two new base classes. Without taking this extra
step, one would still be able to get a RectLight's *width* attribute as
:cpp:`rectLight.GetWidthAttr()`, but getting its *intensity* attribute would
require :python:`UsdLux.LightAPI(rectLight).GetIntensityAttr()`.

Preserve Existing UsdLux Concrete Schemas
#########################################

Interestingly, in this new formulation, :cpp:`UsdLuxCylinderLight`,
:cpp:`UsdLuxSphereLight`, and possibly :cpp:`UsdLuxRectLight` and
:cpp:`UsdLuxDiskLight` *could* be deprecated in favor of the UsdGeom Sphere,
Cylinder, and Cube intrinsic primitives, with LightAPI applied, while
DistantLight, PortalLight, and DomeLight could not. However, we argue that we
should preserve all of the existing concrete classes, for the following reasons:

    * These light types are still the bread and butter of most DCC's, so
      providing salient, concrete classes for them fosters robust interchange.

    ..

    * Some renderers recognize these light types and sample them much more
      efficiently than geometry lights since they can be evaluated analytically,
      so making them easy to identify again adds value.

    ..

    * Keeping these concrete classes preserves existing scenes that contain
      them.

    ..

    * Having concrete classes for the "core" light types makes it easier for
      renderers to auto-apply their extension schemas (e.g.
      :usda:`PxrRectLightAPI` can auto-apply to RectLight.

Since Light will no longer be the parent class for these lights, we must
reparent them.  We propose the following re-basing to the new convenience base
classes:

    .. list-table::
       :header-rows: 1

       * - UsdLux Light
         - UsdGeom Base Class
       * - SphereLight
         - BoundableLightBase
       * - CylinderLight
         - BoundableLightBase
       * - RectLight
         - BoundableLightBase
       * - DiskLight
         - BoundableLightBase
       * - PortalLight
         - BoundableLightBase
       * - DomeLight
         - NonboundableLightBase
       * - DistantLight
         - NonboundableLightBase
       * - PluginLight
         - Xformable

PluginLight inherits from :cpp:`Xformable` as a reinforcement that it is
effectively a black box.

Deprecate GeometryLight
#######################

We have no need for GeometryLight as a typed schema anymore, so the one
**breaking** aspect of this proposal is its deprecation and eventual
elimination. We hope that since it has never been supported by Hydra that its
adoption has been limited. This move from GeometryLight to LightAPI has a nice
fallback behavior in that, as long as each geometry *does* bind a suitable
emissive Material, even a renderer that does not support that geometry as a
light will still get a chance at rendering the object as emissive, even if
proper intensity, linking, etc. "fall off". We **do** still have a problem to
solve that motivates us to add some new API schemas in its place. The problem
is twofold:

    #. Different renderers will have their own special controls for geometry
       lights, and possibly *different* controls for different types of
       geometry. We would therefore like "geometry lights" to be automatically
       enhanced with these controls, just as a RectLight comes "fully loaded"
       with all PxrRectLight controls in a RenderMan-using environment, via the
       auto-applied API schema mechanism

    ..

    #. But the generated :usda:`PxrMeshLightAPI` schema cannot stipulate that it
       should apply to all Mesh prims, or even to all LightAPI-prims. For speed 
       and complexity reasons, we do not wish to enhance the auto-apply 
       mechanism to get more sophisticated than the simple list of schema types 
       described by :usda:`apiSchemaAutoApplyTo`.

Our solution is to introduce :cpp:`UsdLuxMeshLightAPI` and
:cpp:`UsdLuxVolumeLightAPI` to handle the two most common types of geometry
lights we see, though others could be added. Each of these schemas will itself
apply LightAPI as a builtin schema, thus imbuing "lightness" to any Mesh or
Volume prims, and using the new :usda:`apiSchemaCanOnlyApplyTo` schema
generation tag, will indicate that each can only be applied to its respective
gprim type. :usda:`PxrMeshLightAPI` will then target MeshLightAPI as its
:usda:`apiSchemaAutoApplyTo`.

How do we Identify the Appropriate SdrShaderNode for a Light?
#############################################################

When the UsdLux module is loaded, we register a :cpp:`SdrShaderNode` with a
"USD" source type for each of the canonical lights, thus providing a "fallback"
definition (with no actual shader implementation) for renderers that want to
consume lights in the Sdr-based, "material-like" way that Hydra now represents
them, but which do not represent lights as shaders. In these cases, usdImaging
can rely on the fact that the Sdr identifier to assign for the light network is
the same as the prim typeName. But this does not work for Gprim lights that have
LightAPI (or MeshLightAPI or VolumeLightAPI) applied to them. Therefore we allow
for a more explicit statement of the shader id, by adding

.. code-block:: usda

    token light:shaderId = ""

to the LightAPI definition. MeshLightAPI will override the fallback value to
"MeshLight" and VolumeLightAPI to "VolumeLight". Great!

But we have been overlooking a related issue. The shader that RenderMan
registers for RectLight is called "PxrRectLight", not "RectLight", and the
Arnold shader is probably called something different, also. So we require that
when the :usda:`PxrRectLightAPI` has been auto-applied to a RectLight prim and
someone is rendering with RenderMan, that usdImaging selects "PxrRectLight" as
the shader identifier, but that if an :usda:`AiRectLightAPI` is also
auto-applied and one is rendering with Arnold, usdImaging will select the Arnold
identifier.

Continuing the analogy of Lights to Material networks, we address this
identification problem by additionally looking for :usda:`light:shaderId`
attributes on the light prim whose name is prefixed with one of the renderer's
registered `MaterialRenderContexts
<https://github.com/PixarAnimationStudios/USD/blob/7a5f8c4311fed3ef2271d5e4b51025fb0f513730/pxr/imaging/hd/renderDelegate.h#L393-L399>`_
. So, using RenderMan and its "ri" context as an example, usdImaging would use
the following logic to determine the shader to assign to a particular light.

    #. If :usda:`ri:light:shaderId` exists, is token-valued, and has a non-empty
       value, use it.

    #. Otherwise, if :usda:`light:shaderId` exists and has a non-empty value, 
       use it.

    #. Otherwise, use the prim's typeName.

Supplied with the necessary metadata in the shaders,
:program:`usdgenschemafromsdr` will add the properly-namespaced
:usda:`light:shaderId` attribute as a builtin for each codeless schema it
creates, so these attributes should never need to be authored by users.

Lights with Materials?
######################

Another subtle issue is that we would now have scenarios in which lights would
have bound Materials, which establishes a precedent that we previously argued is
:ref:`antagonistic to robust interchange of lights
<wp_usdlux_for_renderers:USD Schemas>`. However, that concern was primarily
questioning the *meaning* of having a Material bound to a Light. In this
proposal, it is clear that:

    * Lights are still themselves Connectable containers, with well-understood
      and documented use-cases for connecting pattern networks to their inputs.

    ..

    * The Material(s) that may be bound to a given "prim that is a light" are
      for the same purposes and meanings they would have if the prim were not a
      light, i.e. providing surface, volume, displacement shading behaviors, and
      potentially physical properties for simulation.

Therefore we do not believe this concern is still weighty.

Proposal and Behaviors
======================

Given that the single-prim, LightAPI solution provides the more consistent and
less-surprising user model, and that it scales and fits better in the
forward-looking scene formulation where any geometry can be a light, **we
propose to adopt LightAPI as the means for specifying** :bi:`all` **lights** ,
even though it requires a core change to UsdLux, and therefore carries a
somewhat higher engineering cost.

Independently of the prim encoding, there are also several important behaviors
around the interaction of surface and light shading that we wish to codify,
which will become impositions on renderers wishing to adhere to the OM.

Shading and Emission
####################

Even though we are unifying geometry and light into a single prim, we have an
important tension to reconcile for the "gprim that happens to emit light". In
Pixar's pipeline, it is the responsibility of the shading department to craft
and receive director approval for the final look of assets in-camera. Further
down the pipeline, operating on already-approved, shaded models, it is the
lighting department's job to propagate light in scenes to artistically
illuminate the whole, while being sensitive to the look and balance of the
individual shaded models - they must ensure that achieving the desired lighting
does not destroy the "look of things".

For example, consider a scene in which an astronaut is interacting with a
glowing control panel in a dark cockpit. The control panel has glowing parts of
several different hues, and the emission controls were set to produce a desired
look in control renders. However, in the scene with the astronaut peering
intently at the panel, the lighting artist observes that the emission from the
panel causes a technicolor play of light across the astronaut's face, which is
too distracting for the scene. However, the control panel itself is also
visible, in-camera. The behavior we have built into our "geometry lights" in
RenderMan and Katana (using custom extensions to RFK) allows us to control the
*illumination* from a geometry light **independently** from the
visible-to-camera *surface shading* of the geometry light, when we need to. In
our example, this allows the lighter to tone-down the emission from the control
panel, and add a (not visible to camera) uniform blue-white RectLight on top of
the panel to provide primary illumination for the astronaut's face.

In other circumstances we use this ability to increase the emission from a
geometry light, again without altering the look - increasing light output will
often cause the direct-look of the emitter to blow out, an effect enhanced by
our limited gamut display devices, so it is again important to detach the
illumination response from the primary and secondary responses.

In fact, we rarely if ever find the physically correct response (in which
LightAPI controls affect all rays in a pathtracer equally) useful, which is
reflected in the specification of light behavior below. In thinking about the
behavior we are prescribing, it may be useful to understand *how* we achieve
this behavior (though certainly there may be other ways to achieve it). For
true geometry lights with bound Materials that are entirely or partly glowing,
we instantiate the geometry **twice** in the renderer. The first copy's
response is governed by the bound Material, and is **not** sampled as a light. 
The second copy is only visible to light rays, and "is" the light, participating
in light sampling, with an energy distribution derived from both LightAPI inputs
and potentially spliced patterns from the geometry's bound Material. There are
other advantages to this practice, such as:

    * being able to independently control the refinement (or granularity, in the
      case of volumes) for the geometry used for light sampling, where we can
      often get away with reduced fidelity in service of sampling speed

    ..

    * it facilitates using light linking to specify that the emissive gprim
      should not self-illuminate or cast shadows (we would expose a specific
      control for this, which under the hood translates into excluding the
      geometry mesh/volume from the light's *lightLink* or *shadowLink*
      collections)

    ..

    * in past versions of RenderMan this separation is what allowed us to keep
      the light invisible to camera rays while keeping the glowing gprim visible

With this in mind, we stipulate the behavior of geometry lights when Materials
are bound to the prim, and when they are not.

No-Material Behavior
********************

If a gprim has an applied LightAPI but no bound Material, we are modeling the
"light whose shape is defined by a geometry" philosophy described at the
beginning of this document. The geometry in question will have no specular,
diffuse, or transmissive response. Some renderers require special materials to
be bound in situations like this, but we choose to not require any such
specification in the USD scenegraph - render delegates can take care of this for
us, if needed. It should be possible for users to drive the
:usda:`LightAPI.inputs:color` input with a texture or other pattern network that
refers to texture coordinates provided by the geometry (see note about
:ref:`wp_usdlux_for_geometry_lights:primvars in light networks`).

Synchronizing Light with Material
*********************************

For geometry that actually has a bound, entirely or partly emissive Material, we
need to be able to specify the relationship of the Material response to the
lighting response. We therefore introduce a new token-valued input to LightAPI,
:usda:`materialSyncMode`, which has one of three values (if there is strong
industry call for the "physically correct" behavior, we would consider adding a
fourth, but that option adds a greater burden on the design described above, so
we will not add it simply for the sake of completeness):

    * :usda:`materialGlowTintsLight`

      This is the fallback value. All primary and secondary rays see the
      emissive/glow response as dictated by the bound Material, while the base
      color seen by light rays (which is then modulated by all of the other
      LightAPI controls) is the **multiplication** of the color feeding the
      emission/glow input of the Material (i.e. its surface or volume shader)
      with the scalar or pattern input to :usda:`LightAPI.inputs:color`. This
      allows us to use the Light's color to tint the geometry's glow color,
      while preserving our access to intensity and other light controls to
      further modulate the illumination.

    ..

    * :usda:`independent`

      All primary and secondary rays see the emissive/glow response as dictated
      by the bound Material, while the base color seen by light rays is
      determined **solely** by :usda:`LightAPI.inputs:color`. Note that for
      partially emissive geometry (in which some parts are reflective rather
      than emissive), a suitable pattern must be connected to the light's color
      input, or else the light will radiate uniformly from the geometry. (again,
      see :ref:`wp_usdlux_for_geometry_lights:primvars in light networks`).

    ..

    * :usda:`noMaterialResponse`

      The behavior in this mode is to exactly model the "No-Material Behavior"
      described above. We feel it is useful to present this as a
      :usda:`materialSyncMode` primarily so that the "canonical" lights in
      UsdLux, which in Renderers like RenderMan provide analytic sampling for
      greatly enhanced sampling speed, can indicate to the user that bound
      Materials **are ignored** , by overriding the LightAPI fallback to this
      value instead.

Volumes 
#######

We have found that exercising RenderMan's volume lighting capabilities
efficiently often encourages us to use a lower-resolution manifestation of a
Volume than we would use for visibility rays. An extra benefit of creating
:cpp:`UsdLuxVolumeLightAPI` is that we now have a place to host such extra,
non-light-parameter controls. You can expect both :cpp:`UsdLuxVolumeLightAPI`
**and** :cpp:`UsdLuxMeshLightAPI` to evolve as we gain experience in this new
formulation, and we also welcome thoughts and ideas from the community.

Primvars in Light Networks
##########################

Some rendering architectures, such as Hydra, attempt to optimize renderer memory
usage by pruning the primvars emitted for any given gprim to just those that we
can verify are being used. That calculation currently only examines the gprim's
bound Material networks to look for primvar references. Given that geometry
lights can, in some modes, directly consume primvars to evaluate the light's
base color, we must ensure that "used primvar" calculations now additionally
check to see if a gprim has a LightAPI, and if so, examine its connected inputs.

