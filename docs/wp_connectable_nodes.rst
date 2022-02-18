==============================================
Generalizing Connectable Nodes Beyond UsdShade
==============================================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `UsdShade overview page <api/usd_shade_page_front.html>`_.

Copyright |copy| 2020, Pixar Animation Studios,  *version 1.1*

.. contents:: :local:

Background and Goals
====================

In computer graphics, the term "shader" originally referred to a unit of
computation used to define material appearance of geometry. The underlying need
was for a user-configurable description of a high-throughput parallel
computation, as needed in rendering. The success of this model led to "shading"
being used in other rendering domains with similar needs. Today a "shading"
node might refer to a computation unit used for geometry deformation,
displacement, or tessellation; light emission; light path integration; camera
lens projection; or image pixel filtering.

UsdShade was designed to foster interchange of material shading networks - that
original domain of shading - between DCC's and renderers. The intent is not to
provide an evaluation runtime, but rather to facilitate transport of shading
networks between contexts where a suitable runtime (i.e. renderer) would be
available. To achieve this it provides concepts of nodes, connected node
graphs, and a node definition registry populated with runtime-discovered node
types. It also provides some basic but useful high-level guidance for valid
structure of these networks, to aid their creation, transport and composition as
coherent assets.

We propose small adjustments to make these core concepts available to those
additional, non-material domains within rendering. The initial motivation is to
`enable light and light-filter networks in UsdLux
<wp_usdlux_for_renderers.html>`_ . We anticipate subsequent use for other
node-plugin-based renderer capabilities.

These changes will not require any changes to existing USD files. Existing
clients of UsdShade API will be almost entirely unaffected, as well, with the
exception of two removed convenience helper methods proposed in the
Connectability section below.

Proposal
========

Node Definition as API Schema
#############################

UsdShadeShader is a concrete prim type with attributes that select a node
definition / implementation entry in the Sdr Node Definition Registry. To make
this "node definition" aspect available to other prim types, we propose to split
this aspect out to an API schema, **UsdShadeNodeDefAPI** . This API would
provide access to the shader definition selector attributes (implementation, id,
source asset / sub-identifier / code), as well as Sdr metadata helpers for
SdrShaderNode lookup. UsdShadeShader would continue to exist but would
pre-apply this new API schema rather than defining these attributes and API
itself, with the goal that existing code clients of UsdShadeShader and USD data
files would require no changes.

The reason to split out UsdShadeNodeDefAPI rather than use UsdShadeShader as a
base-type for other shading-like domains is because those domains may already
have a different concrete base type, such as UsdGeomXformable. Rather than
pursue multiple-inheritance of concrete base types in USD - with its attendant,
well-known complexity - we see API schemas as a better fit for this purpose,
representing the node-ness component, aspect, or facet of a prim.

Plugin-defined ConnectableAPI Behavior
######################################

UsdShade does not and cannot by design know full details of the evaluation
semantics of its networks, but it does aim to support a basic but useful level
of structural guidance for network construction. Specifically, the
**UsdShadeConnectableAPI** is compatible with only certain prim types, and it
constrains how inputs and outputs may be connected. For example, a material's
output may only be connected to an internal node's output, as a way to forward
that result to the material's interface, whereas a shader's output is not
allowed to connect to anything at all. To support a similar level of
construction guidance for other domains, we propose to allow schema plugin
libraries to configure connectability of their UsdShadeInput and UsdShadeOutput
attributes using runtime callbacks. We take inspiration from UsdGeom's design
for spatial extent computations.

UsdShadeConnectableAPI will determine compatibility with a prim type by checking
plugin metadata for that prim type. If the key "implementsConnectableAPI" is
set to true, the schema will be considered compatible, and the plugin will be
loaded before continuing. At load time, a schema plugin library can use the
TF_REGISTRY_FUNCTION system to register connectability callbacks for a prim
type. Only a single set of callbacks may be defined per prim type, similar to
C++'s One Definition Rule; attempting to re-define them will trigger a
TF_RUNTIME_ERROR. The callbacks will be invoked to determine the results for the
CanConnect() methods.

This approach separates node-ness (UsdShadeNodeDefAPI) from connectability
(UsdShadeConenctableAPI). This allows a schema library to define prim types
that contain nodes and are wired up to them, but do not themselves have a node
def. UsdNodeGraph and UsdShadeMaterial are examples of this idea. UsdShade will
register its own callbacks using this mechanism to implement its current
connectability rules for UsdShadeShader, UsdShadeNodeGraph, and UsdShadeMaterial
prims - which we will clarify in the next section.

**We propose to remove** two convenience API methods on UsdShadeConnectableAPI:
IsShader() and IsNodeGraph(). This is the only part of this proposal that
could require updates to existing code clients. The motivation is to remove
the ConnectableAPI's mention of any specific prim types. A call
to :cpp:`connectableAPI.IsShader()` would
become :cpp:`connectableAPI.GetPrim().IsA<UsdShadeShader>()`;
similarly :cpp:`IsNodeGraph()` would become a call to 
:cpp:`IsA<UsdShadeNodeGraph>()`. Arguably these API's provided little 
convenience, and simply obscured the underlying mechanics of how to use prim 
types and schemas.

Connectability Rules for UsdShade Types
#######################################

As part of this refactoring we intend to begin strengthening and clarifying the
connectability behavior of the various schema types in UsdShade, which will be
surfaced through UsdShadeConnectableAPI::CanConnect(). The first part will be
tightening and documenting the "structural" connectivity rules, which are the
following (in addition to the restrictions imposed by `connectability metadata
<api/class_usd_shade_input.html#a9d956346a78692c5e9e1aa1aad3e3b93>`_):

    * **Shader** 

      Inputs can be connected to any Output of any other shader or NodeGraph
      encapsulated by the **same** nearest-in-namespace encapsulating Material
      or NodeGraph, **or** to an Input of the nearest-in-namespace encapsulating
      Material or NodeGraph. Outputs cannot be connected.

    * **NodeGraph**

      Inputs follow the same rule as Shaders. Outputs can be connected to any
      Output on a prim (Shader or NodeGraph) encapsulated by the NodeGraph
      itself, **or** to an Input of the same NodeGraph itself, creating a "pass
      through" connection.

    * **Material**

      Inputs **and Outputs** follow the same rule, which is that they can be
      connected to any Output on a prim  (Shader or NodeGraph) encapsulated by
      the Material itself.

Eventually we hope to make CanConnect() also consult a prim's SdrShaderNode
definition to query type-connectability information (when the prim has an Sdr
definition), once such connectability rules are pluggable by shading-systems
(SdrShaderProperty::CanConnectTo() is still hardcoded to Pixar's use of OSL, as
of release 20.11).

Intended use in UsdLux and UsdRi (RenderMan USD schema)
#######################################################

This current proposal is strictly about UsdShade, but it is motivated by
enabling future work on UsdLux and UsdRi, which we describe here. We intend for
UsdLuxPluginLightFilter to be a schema type with pre-applied UsdShadeNodeDefAPI,
representing a light filter wholly defined by a renderer plugin. Networks of
light filters will use the connectability API. Similarly we anticipate
usdLuxPluginLight to represent a plugin light, with its own node def. UsdRi, to
support the full range of plugin-customizable RenderMan behavior, will also use
UsdShadeNodeDefAPI as needed for its own plugin points, such as Integrators,
Projection plugins, Sample Filters, Display Filters, etc.

Discussion
==========

Sdr & Ndr
#########

The node definition registry libraries Ndr and Sdr have been designed to
separate the shading-ness layer (Sdr) from the basic concept of a node (Ndr). 
This raises the question of whether node-ness in Usd should be factored out as a
separate library that would correspond to Ndr in the way UsdShade corresponds to
Sdr.

In practice, it appears that most if not all applications of node networks in
renderer behaviors share concepts and terminology with shading as represented by
Sdr. Therefore, we propose to use UsdShade and Sdr as the basis for these
rendering-related domains. This appears to resonate with common mental models
of rendering capabilities, and has the significant benefit of avoiding any
deeper refactoring, or impact on existing clients or USD data files. In a
future project, we propose to collapse Ndr into Sdr to significantly simplify
the architecture, given our experience and belief that it is unlikely any
registries *other* than Sdr would be specialized from Ndr.

Flexibility of Connectability Callbacks
#######################################

Adding callbacks to advise connectability raises the question of how flexible
the protocol should be. At one extreme, a runtime with full semantic knowledge
of the node definitions and a fully-contextual API could have fine-grained
control. To consider a concocted example: it could prohibit wiring a
vec3-valued displacement through a color-valued HueSaturationValueAdjust node. 
To provide this one could imagine a multiple-dispatch or pattern-matching
approach that could vary behavior on both the upstream and downstream prim
types.

We believe the current callback API that exists in UsdShadeConnectableAPI is
both sufficient to cover the important cases, and the right trade-off point to
avoid undue complexity, so we propose to maintain that but simply allow it to
switch off the consuming prim type, i.e. the type of the prim into which data
would flow. This would be the "destination prim" in the parlance of
UsdShadeConnectableAPI, the API which provides the ConnectToSource() method for
creating connections.

Non-shading Networks
####################

There have been efforts to investigate embedding rigging, constraint, or other
network types into USD. First, it is worth noting that USD's schema
extensibility makes it possible to do this locally to a facility or pipeline. 
The question is whether any deeper integration would be suitable with the USD
runtime. In the context of "shading networks", all of the behaviors provided by
the nodes do not affect value resolution behavior of the USD scene - they are
implementation details that guide how an image should be produced in a rendering
environment. Domains like rigging or constraints would require deeper
integration, since they could arguably affect the desired results of calling
UsdGeom API for spatial extents, among other things.

For this reason, we do not intend UsdShade-based networks to configure or
specify any deeper behavior changes that would affect the semantics of the USD
runtime. We acknowledge that several organizations are layering more
sophisticated procedural and execution behaviors on top of USD, and we believe
that UsdShade-based networks *could* be appropriate for describing and
interchanging the computation graphs that might feed such systems.

