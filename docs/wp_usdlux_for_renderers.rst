=========================================
Adapting UsdLux to the Needs of Renderers
=========================================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `UsdLux overview page <api/usd_lux_page_front.html>`_.

Copyright |copy| 2020, Pixar Animation Studios,  *version 1.4*

.. contents:: :local:


Background and Goals
====================

UsdLux was designed to foster interchange of lights between DCC's *and* between
renderers. Its declarative schemas provide an independent (from any particular
renderer's implementation), documentable specification that is conducive to
interchange. The tradeoff in representing lights declaratively, however, is
that it hides the reality that many high quality renderers (including RenderMan,
an ongoing top priority for Pixar) actually represent lights as shading
networks, or something very like shading networks. Our initial assessment was
that the combination of extensible Lights and LightFilters might be rich enough
to provide shader-network capabilities to lights while maintaining the
possibility of interchangeability, since LightFilters essentially encode a chain
of shaders modifying a light's output, but we have received feedback that it is
not. In particular, some renderers (including, soon, RenderMan) support driving
light **inputs** with pattern networks. As we consider expanding the role
shaders play in UsdLux, we also reconsider how we will organize LightFilters,
and how we encode light "portals".

Additionally, as we have gained experience integrating UsdLux through Hydra to
RenderMan we have encountered limitations of the purely schema-based approach
that we would like to address; the principal problem is that every renderer has
its own "signature" for each of the core light types that will not perfectly
match the UsdLux schemas, thus necessitating a remapping and conversion process
that ideally should be data-driven, but which currently must be largely
hard-coded in Hydra render delegates.

We propose to address all of these issues by imbuing UsdLux with qualities of
our UsdShade schemas such that we can process lights, light filters, and their
inputs more like shading networks (through usdImaging and Hydra, to render
delegates) which *are* data driven, without compromising the interchangeability
of the core schemas. In the following sections we will review some relevant
core USD features, finally making a proposal that, while potentially
non-data-backwards-compatible, should be largely API compatible with the UsdLux
of today. We will refer to RenderMan throughout, but we believe the analysis
extends to other top-tier renderers as well.

Foundational Technologies in USD
================================

Sdr
###

The :usdcpp:`SdrRegistry` is USD's mechanism for dynamically discovering shaders
defined (via plugin) by any consuming system, and mapping their interfaces to
types, concepts, and GUI elements familiar to USD. To RenderMan (and most other
high-quality renderers), lights, light filters, integrators, and display filters
are all plugins that map very well to the concept of "shader" (some plugin types
interact with the renderer in richer ways than, say, pattern shaders, but from
an authoring and data transmission perspective, treating them as shaders
generally suffices).

Although Sdr does rely on plugins to locate shader definitions and to parse
those definitions, the definitions themselves are either the shader assets (in
the case of OSL) or metadata files, such as the *args* files RenderMan already
maintains for its clients. Given that every renderer defines its lights in its
own way, and that any renderer that desires to render via a Hydra render
delegate is already building and shipping USD-based plugins, we believe that Sdr
provides a versatile way for renderers to adapt their own light definitions to a
form known to the USD ecosystem.

USD Schemas 
###########

The **limitation** of fully representing lights as Sdr-based shaders is that
Sdr's dynamic and fluid nature cannot provide any mechanism for standardization
of classes or types of shaders, and therefore it cannot help us define a
standard for cross-renderer interchange - this analysis extends to the pattern
(which has been suggested on the OpenUSD forum) of binding lights to
UsdShadeMaterials to achieve complete expressiveness of light-shader networks.
Cross-renderer interchange of lights is the entire point of UsdLux, which, in
addition to providing certain core encodings and behaviors such as light
linking, provides a handful of common, concrete light schemas that should be
interpretable by most DCC's and renderers. The USD approach of canonizing
graphics concepts into schemas has been very successful for geometry, material,
and skeletal skinning data, and is underway for volumes and spatial audio, and
some VFX studios had already begun using UsdLux lights for interchange in their
own pipelines, even before Hydra supported UsdLux at all.

Schemas in USD have two components:

    * **A dynamic, runtime component** that is completely data-driven, which
      provides information to core USD value resolution and to clients and GUIs
      about the properties that a schema defines (names, types, default values,
      etc). The hub for this functionality is the :cpp:`UsdSchemaRegistry`,
      which consumes USD files that describe the schemas at runtime. For a
      schema to participate in this mechanism, all that is required is a
      suitable USD file (which is typically generated by
      :program:`usdGenSchema`) and a modification to a :filename:`plugInfo.json`
      file.

    ..

    * **A compiled API class** that contains named accessors for the various
      schema properties (to aid in robust coding to/from the schema) and
      provides a place to hang behaviors specific to the schema, such as
      computations based on the schema's raw data. While schemas have proven
      valuable for both maintaining robust import/export in all the DCC's we use
      *and* in pipeline use, the techniques we use for versioning/evolving
      schemas are too heavyweight to handle frequent churn.

Having light interfaces represented as schemas allows lights to integrate
seamlessly into USD-based tools just like other schema-based prims do, without
needing to know about or consult a different definition system like
Sdr. Recently completed work in USD's core allows applied (single and multi)
schemas to contribute alongside the prim's primary schema type to the prim's
"definition". This means that if a renderer follows the guidance UsdLux
provides in extending the core light types via applied API schema, that the
*fully configured, renderer-specific definition* of the light will be available
to clients through core methods like UsdPrim::GetProperties(), and the GUI in
usdview.

It is less clear that the code-generated C++ and python interfaces to
renderer-specific light extensions will be useful, but we believe we can
leverage the separation between the two halves of schemas enumerated above to
allow renderer-providers to generate only the dynamic, runtime components of
lighting related schemas, should they desire. Given that plugins do not
(cannot) generally provide linkable API, we think this may be more useful than
full-fledged schema generation, and also allows a renderer to provide useful
information **for** USD consumption, without itself *linking* against USD.

UsdImaging and Hydra
####################

UsdImaging provides a concrete adapter class for each type of :cpp:`UsdLuxLight`
that it supports (not all types are yet supported), though they currently do
little more than populate an Sprim with a light-type token defined at the Hd
level that is similar to the UsdLux prim-type names. An imaging adapter would
definitely be needed for more complicated light types like the
:cpp:`UsdLuxGeometryLight`, though it seems like for most light types, a common
:cpp:`UsdLuxLight` based adapter would suffice, if (as has been independently
requested) usdImaging were able to leverage base-class adapters for concrete
prim types.

But the key problem with the current mechanism is defining how render delegates
get values and information about specific light parameters through Hydra.
Currently, :cpp:`HdSceneDelegate` provides a :cpp:`GetLightParamValue()` method
that allows a render delegate to query a light parameter by name; this requires
knowing the right name to use, which Hd attempts to provide by defining core
light tokens in :cpp:`HdLightTokens`. These tokens wind up baking in UsdLux
naming conventions, and the file that defines them, along with render delegates
that consume them, must be kept in sync with any changes to UsdLux. It is this
organization that has generated the most friction to date in supporting lights
in hdPrman, since hdPrman must map its own light plugin parameter names to the
HdLightTokens names in order to query the data it needs, and it must do so
**regardless** of whether any data was authored for the corresponding parameter.

By contrast, Hydra already uses the :cpp:`SdrRegistry` as its way to transmit
material shading network identity from scene delegates to render delegates,
which hdPrman uses to process material networks in a completely data-driven way.
If we were to represent lights (etc.) as SdrRegistry-based shaders in Hydra, we
expect it would substantially simplify and make more maintainable hdPrman's (and
other render delegates') existing handling of lights and light filters.

Proposals
=========

Currently lights and light filters are completely schema-based, with an
assumption that either Hd will reflect the USD schema for a new light type, or
render delegates must just know the proper names to query from a scene delegate.
Further, nothing about :cpp:`UsdLuxLight` is connectable: the *filters*
relationship allows specification of a linear chain of :cpp:`UsdLuxLightFilter`
that act as modifier-shaders, but that is the extent of procedural
customization. We propose to imbue lights and light filters in USD with the
following qualities of Sdr and UsdShade, resulting in core changes to UsdLux,
UsdImaging, and Hd, which we will explore in further detail in following
sections.

    * **UsdLuxLights and UsdLuxLightFilters will become Connectable, like a
      UsdShadeNodeGraph**. This will allow any light input parameter to be
      driven by a shading network (encapsulated by the Light), and, if useful,
      allow renderers to publish multiple computations from lights as outputs. 
      Renderers that are incapable of handling more general shading networks for
      lights can ignore any networks connected to light inputs, consuming
      instead the value authored on the input, thus *basic* light interchange is
      preserved.

    ..

    * **Every Light and LightFilter will have (per-renderer) Sdr definitions**
      that will allow Hydra to marshal Light networks similarly to how it
      marshals shading networks, and will allow renderers to translate from the
      UsdLux specifications in a data-driven fashion. The Sdr identifiers for
      the core UsdLux lights will be the schema typeName, and we will introduce
      PluginLight and PluginLightFilter schemas for use in applications that are
      using USD simply as the most convenient way to get data to a renderer
      through Hydra, which would allow all the same ways of identifying an
      :cpp:`SdrShaderNode` as a :cpp:`UsdShadeShader` has, and without providing
      a specific schema for each such light.

    ..

    * **UsdLuxLightPortal will become UsdLuxPortalLight**, to reflect the need
      (for RenderMan, and possibly other renderers) that portals:

        * Benefit from possessing LightFilters

        * Want to serve as a source of light parameter overrides to the
          environment (or potentially other) light that they modify

Changes to UsdLux
#################

Making Lights Connectable
*************************

In UsdShade, NodeGraphs, Materials, and Shaders are all "connectable", by virtue
of conforming to the **non-applied** API schema :usdcpp:`UsdShadeConnectableAPI`,
which requires that all "input parameters" be placed in the "inputs:" property
namespace, and all outputs be placed in the "outputs:" namespace, to facilitate
clear point-to-point connections. **We propose to create a new schema domain,
UsdNode**, that abstracts the concepts of connectability, NodeGraph, and
"bearing an Ndr node definition", because these concepts are now deemed
applicable to UsdLux, and potentially other domains. We will explore the
specifics of refactoring UsdShade and UsdNode elsewhere.

The point of making Connectability a lower-level concept is so that we can make
every :cpp:`UsdLuxLight` and :cpp:`UsdLuxLightFilter` be connectable. We
believe we can do so without greatly perturbing the API's in UsdLux; however, it
will necessitate a change to the core encodings themselves, as **every existing
light attribute's name will change to be prefixed by the "inputs:"
namespace**. We will investigate the cost of making this change be backwards
compatible for existing Light assets (at least for the existing usdImaging
adaptors for UsdLux). Backwards compatibility is not a high priority for Pixar
as we have not yet begun using UsdLux in earnest in production, so **if
backwards compatibility is important to you, please let us know**.

We also propose that, like Materials, **Lights be made to encapsulate their
shading networks**. This makes it easier to reference lights robustly, and is
an aid to dependency tracking for interactive rendering.  Note that while
Lights and NodeGraphs will both encapsulate all of their connected shading
networks, NodeGraph inputs cannot be connected to any shader contained inside
the NodeGraph, but Light inputs **will** be connectable to (and only to) outputs
of shaders contained inside the Light (or LightFilter).

Associating Lights with Sdr Definitions
***************************************

Primarily as a matter of documentation of expected downstream behavior, UsdLux
will decree that **every typed (IsA) Light typeName constitutes an Sdr
identifier** that renderers can rely on to retrieve a renderer-specific
definition for the light type. We would expect, for example, that hdPrman would
register RenderMan's definition (provided by an *args* file that ships with each
RenderMan release) for `PxrRectLight
<https://rmanwiki.pixar.com/display/REN23/PxrRectLight>`_ under the Sdr
identifier "RectLight", with a *source type* of "ri", using
:usdcpp:`SdrShaderProperty`'s :usda:`implementationName` metadata to map the
:cpp:`PxrRectLight` names to the :usdcpp:`UsdLuxRectLight` names.

For every concrete light type defined in UsdLux, we will add a companion API
schema as a base class for renderer-specific extensions. For example, we would
add a :usda:`UsdLuxRectLightAPI` applied schema. The purpose of having such
(empty) base-classes is to allow renderer plugins to include **runtime schema
definitions** for their implementations of the core UsdLux lights that derive
from the appropriate base class: when DCC's such as Presto add a RectLight to a
scene, they can automatically apply the extensions of all registered Hd
renderers for a given pipeline by simply finding the TfType-derived subclasses
of :usda:`UsdLuxRectLightAPI`. When a renderer's definition for a light is
"applied" to the base UsdLux-typed prim, it allows USD (as of 20.05) to
configure the prim's definition so that all tools can enumerate and query the
available "builtin" properties. Applied API schemas can therefore augment or 
modify a typed schema's definition; in general, modifications should be 
undertaken with great caution (lest we violate the promise that 
:cpp:`prim.IsA<SomeType>()` makes), but can be leveraged here usefully: if one 
of the core :cpp:`UsdLuxRectLight` parameters really does not make sense for a 
particular renderer (cannot be translated to something useful), then the 
renderer's subclass of RectLightAPI can make that parameter ":usdcpp:`hidden
<UsdObject::SetHidden>`" to keep it from showing up in GUI's (usdview does not
yet honor hidden metadata).

It may also be useful for UsdLux to register "fallback" or "universal"
:usdcpp:`SdrShaderNode` definitions for the UsdLux light types so that even if
there are no render delegates registered, one can always find an Sdr definition
for any UsdLux light type, and a render delegate *could* use the fallback
definitions if it chose to model its lights directly on the schema spec. Doing
so would require adding an :usdcpp:`NdrDiscoveryPlugin` to UsdLux, and either
sharing the USD Sdr parser currently provided by UsdShade, or crafting another
solution for registering the definitions.

UsdLuxPluginLight, UsdLuxPluginLightFilter
******************************************

We expect "published, pipeline citizen" render delegates to provide typed and
applied dynamic schemas that derive from the UsdLux base types (even if only
LuxLight) for all of the renderer's core light types. But some render delegates
may be developed as part of a proprietary application package that is only using
USD as a mechanism to communicate to that render delegate... in other words, the
application doesn't *really* want to participate in the open USD ecosystem, but
needs to use it for rendering scenes imported from USD and augmented using
application/renderer-specific lighting features, or finds it useful to use USD
as an archiving format to send jobs to a render-farm so that the full
application need not be run there.

In order to lower the barrier somewhat for such applications, we will provide
:cpp:`UsdLuxPluginLight` and :cpp:`UsdLuxPluginLightFilter` , concrete
Light/Filter schema types that bear the same "node identification" encoding that
UsdShadeShader does today so that the application need only plug its extensions
into Sdr (by providing node definitions that will be consumed by render
delegates), and not be required to take the extra step of generating dynamic USD
schema definitions.   Because they provide less easily accessible information to
users, we do not advocate using these new types in pipeline-persistent
"user-level" USD files, and at Pixar we expect to enforce this.

UsdLuxPortalLight
*****************

At Pixar we conceive of portals as not just a sampling-strategy modifier, but
also as filters/modifiers to the environment lights with which they are
associated. We find it useful to both attach LightFilters to portals, which
will apply to all light "passing through" the portal, and to change the quality
of light by either adjusting the light linking, or by modifying core Light
parameters, by authoring light parameters on the portal prim, which serve as
overrides (or in some cases, modifiers, e.g. *intensity* serves as a multiplier
to the environment light's intensity). We understand that many renderers may
support none of these "advanced" Portal behaviors (and some renderers do not
support Portals at all), and believe it is fine for a render delegate to ignore
unsupported features, consuming only the basic geometric information associated
with the Portal.

Making both of these abilities possible requires only re-deriving the portal
schema from :cpp:`UsdGeomXformable` to :cpp:`UsdLuxLight`. But since we have a
convention of ending all light names with "Light" we would also rename the
schema PortalLight; this is a change we can make backwards compatible with
existing scene description for a period of time, **if we hear that backwards
compatibility is important**. By making Portals into lights, we enable them to
be linkable, and additionally define their own LightFilters:

Portal Filter and Linking Behavior

    * **Filters:** 

      We stipulate that a Portal inherits all of the filters of its associated
      environment light, and appends its own filters to the end of that list.

    * **Linking:** 

      All linking include/exclude rules specified on a Portal must be
      *intersected* against those inherited from the environment light; i.e. a
      portal cannot "see" anything the environment light itself cannot see.

Changes to UsdImaging
#####################

With the exceptions of DomeLight (because of its possession of PortalLights) and
GeometryLight (because of its complex relationship to geometry), the
:cpp:`UsdImagingAdapter` behavior for all UsdLux light types would now be
uniform: we populate a similar data-structure as we extract from
:cpp:`UsdShadeMaterial` networks (currently an :cpp:`HdMaterialNetwork`) with
the authored data for the light and its associated light filters, and set the
identifier for the light's and filters' :cpp:`HdMaterialNodes` to the
light/filter's schema typeName.  The primary extra information beyond what is
carries by :cpp:`HdMaterialNetwork` for lights is the inclusion of per-light and
per-filter linking information. Neither PortalLights nor Geometry lights have
been incorporated into UsdImaging or Hd yet, so we will defer speculating about
how to handle those. Other scene delegates, such as those in Katana, Maya,
Houdini, and Presto will need to be similarly updated.

The pattern by which DomeLights and their associated PortalLights get populated
and represented in Hydra still needs to be worked out; as neither UsdImaging nor
Hydra yet support existing LightPortals, we will not address the problem in this
document.

Changes to Hd
#############

Instead of the :cpp:`HdIdentifier` they currently contain, the Render Index
representation for lights would contain the data structure created by UsdImaging
or other scene delegates, which describes the light's shading network. The key
idea here is that a scene delegate packages up the **authored light properties**
, as we do for materials, and associates "nodes" (where now a node is either a
light shader, a shader associated with a light filter, or a pattern network)
with Sdr identifiers. Thus the need for :cpp:`GetLightParamValue()` and the
pattern of render delegates querying for every possible parameter goes away.

    * Hd no longer needs to define any tokens for specific light property names,
      and possibly no longer even needs to define tokens for the "core"
      UsdLux-defined, interchangeable lights, because it is a scene delegate's
      responsibility to supply the UsdLux typeName as identifier, and from then
      on all processing is data-driven.

    ..

    * Sdr already supports the idea that different renderers can each provide
      different definitions for the same *identifier*, by registering a
      definition for its own, unique "source type". Although Sdr requires that
      all definitions for the same implementation must have the same value types
      for properties that are defined as having the same name in both, it allows
      different definitions to each define unique properties, and specify
      different metadata even for properties shared by multiple definitions. Of
      note, Sdr Properties support name aliases (
      :usdcpp:`SdrShaderProperty::GetImplementationName`), which gives us a
      completely data-driven way of conforming RenderMan Pxr lights to UsdLux
      lights. It is possible that in some cases name-changes alone are
      insufficient; :usdcpp:`SdrShaderProperty` supports arbitrary metadata
      authored in the definition, so it may be possible to handle such cases
      without hardcoding for specific light types in hdPrman and other render
      delegates.

Changes to HdPrman and Other Render Delegates 
#############################################

The proposed changes to hdPrman *code* are mostly implied by the changes to Hd:
we can rip out all of the manual light translation code and parameter lists, and
handle lights similarly to how we handle materials, possibly sharing some of
the same code. It will also be the new responsibility of every render delegate
to **minimally** publish Sdr definitions for each of the core UsdLux lights and
any other renderer-specific typed light schemas it provides. The provisioning of
these definitions should adhere to the following rules:

    * The schema typeName should be the identifier for each definition

    * The *source type* each renderer queries is the same string it already uses
      as a "`material network selector
      <https://github.com/PixarAnimationStudios/USD/blob/release/pxr/imaging/hd/renderDelegate.h>`_",
      for simplicity (for RenderMan and hdPrman, this is "ri")

    * The :usdcpp:`context <NdrNode::GetContext>` provided for each definition
      should be SdrNodeContext->Light

    * The definition uses the ":usdcpp:`implementation name
      <SdrShaderProperty::GetImplementationName>`" Sdr concept to map Lux
      definition names to the renderer's own implementation input names.

This new responsibility should be more than compensated by the ability to
maintain much simpler, data-driven code in the render delegate's implementation.
Additionally, we **strongly encourage** render delegates to package dynamic USD
API schema definitions for their extensions to core UsdLux lights, to aid in
debugging in :program:`usdview`, and to provide DCC's with a uniform mechanism
for interacting with lights (e.g. when populating GUI's). We plan to provide a
utility that can generate a :filename:`generatedSchema.usda` from a selection of
(properly instrumented) :cpp:`SdrShaderNode` definitions, so that the added
development cost to providing dynamic schemas should be, we hope, minimal.

