============================
UsdShade Material Assignment
============================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `UsdShade overview page <api/usd_shade_page_front.html>`_.

Copyright |copy| 2017, Pixar Animation Studios,  *version 1.0*

.. contents:: :local:


Background
==========

In UsdShade of summer 2017, material assignment is expressed as a single-target
relationship that identifies a Material that should be bound to the anchoring
prim and all of its descendants, unless some descendant itself possesses a
direct material assignment, in which case its binding will be considered
stronger than the inherited one. "Material resolution" itself is somewhat more
nuanced, because what actually matters to a particular renderer is not truly the
bound Material, but the Material *output* (referred to sometimes as *terminals*
or *ports* ) that the renderer understands: each output identifies a shading
network, and different renderers may consume different shading networks. In the
example below, the Material :sdfpath:`/PreviewMaterial` bound to a root prim
:sdfpath:`/Bob` may provide only a :usda:`glslfx:surface` output, while a
descendant Mesh prim :sdfpath:`/Bob/Geom/Body` may bind a Material
:sdfpath:`/Skin` that only defines a renderman-specific, :usda:`ri:bxdf`
output. Even though the ancestral binding is weaker (for the mesh) than the
descendant-binding, when a GL renderer asks :sdfpath:`/Bob/Geom/Body` what
shading network it should use, the answer should be the :usda:`glslfx:surface`
defined on the ancestor prim. This nuance points out that addressing the problem
of binding materials must be done in consideration of the larger problem of
identifying useful shading networks; we will address this problem in the second
half of this proposal.

.. code-block:: usda
   :caption: Resolving Hierarchically-bound Materials

   def Material "PreviewMaterial"
   {
   	outputs:glslfx:surface.connect = </PreviewMaterial/PreviewSurface.outputs:surface>
   
   	def Shader "PreviewSurface"
   	{ ... }
   }
   
   def Material "Skin"
   {
   	outputs:ri:surface.connect = </Skin/pxrSurface1.outputs:bxdf>
   
   	def Shader "pxrSurface1"
   	{ ... }
   }
   
   def Xform "Bob"
   {
       rel material:binding = </PreviewMaterial>
   
   	def Xform "Geom"
   	{
   		def Mesh "Body"
   		{
   			rel material:binding = </Skin>
   		}
   	}
   }

In the several years we have been using this binding mechanism, we have bumped
up against two key limitations:

    #. **Direct bindings cannot be established for gprims inside aggregate
       instances without breaking the instances.** 

       This can be problematic in systems like Maya that natively allow unique
       shader bindings to instances, as recently reported in `Issue 332
       <https://github.com/PixarAnimationStudios/USD/issues/332>`_ . This has
       been problematic for our Katana-based set-shading workflows, as well, in
       which a set of unshaded, instanced components are referenced into a set,
       and we wish to bind set-level Materials to the primitives inside the
       components.

    #. **For high-level distinctions like "preview" vs "full" rendering, wherein
       we frequently wish to bind materials at different granularity, it can be
       onerous to manage and interrogate multiple materials bound at different
       points in the hierarchy.**

       This is especially true in Katana, where the cost of examining a material
       in order to discover whether it is the one you want is high.


Other systems, such as Katana, Houdini Material Style Sheets, MaterialX, and
Maya allow materials to be assigned to a *collection* that identifies the gprims
(or ancestors of gprims) that should bind to the given Material. In USD,
collections are represented by *include* and *exclude* relationships in the
:usdcpp:`UsdCollectionAPI` schema, and relationships authored "outside"
instances are completely free to target prims **inside** instances, in which
case USD core will readily resolve the targets to " `instance proxy prims
<api/_usd__page__scenegraph_instancing.html#Usd_ScenegraphInstancing_InstanceProxies>`_".
This means that we can create collections above/outside instances that refer to
constituents of *individual instances,* and bind the collections to Materials
defined either inside or outside of instances.

In this document we present a model for binding materials via collections, with
an eye towards efficient evaluation and paying only for the mental/pipeline
complexity you need, together with a new concept we call material purpose whose
goal is to make it easier for users and renderers to manage and discover the
materials relevant to a particular rendering task.

Basic Proposal for Collection-Based Assignment
==============================================

Currently, the :usda:`material:binding` relationship is restricted to targeting
a single Material. We propose adding an additional set of binding properties
that can appear on any prim, :usda:`material:binding:collection:XXX` that
identifies a (Material, Collection) pair to be bound:

    #. :usda:`material:binding:collection` defines a namespace of binding
       relationships to be applied in namespace order, with the earliest ordered
       binding relationship the strongest.  Generally, we expect binding
       collections to be non-intersecting, and therefore order to be
       unimportant, but when ordering is necessary, it relies on the hosting
       prim's :usdcpp:`native property ordering <UsdPrim::GetProperties>`.

    #. For a :usda:`material:binding:collection:NAME` relationship to be a valid
       binding, it must target a single Material and a single Collection, and
       represents the binding of the targeted Material to all of the prims
       identified by the Collection that live beneath the binding relationshp's
       prim in namespace. The name chosen as the last element of the binding
       property's name is irrelevant, save that:

        #. it establishes an identity for the binding that is unique on the
           prim. Attempting to establish two collection bindings of the same
           name on the same prim will result in the first binding simply being
           overridden. In the future, we may allow a single colllection binding
           to target multiple collections, if we can establish a reasonable
           round-tripping pattern for applications like Maya.

        #. NAME must be a single token, **not** a namespaced token sequence.
           This restriction facilitates advanced features (purpose) described
           later in this document.

    #. If a prim has both authored :usda:`material:binding` and authored
       :usda:`material:binding:collection:XXX` properties, the "direct"
       :usda:`material:binding` is weaker than the collection-based assignments,
       reflecting our belief that the combination would appear primarily to
       define a "fallback" material to be used by any child prims that are not
       targeted by a more specific assignment.

    #. The "specificity" with which a prim is included in a collection **is
       irrelevant** to the binding strength of the collection. For example, if a
       prim contains the **ordered** collection bindings
       :usda:`material:binding:collection:metalBits` and
       :usda:`material:binding:collection:plasticBits`, each of which targets a
       collection of the same name, then if *metalBits* includes
       :sdfpath:`/Chair/Back`, while *plasticBits* includes
       :sdfpath:`/Chair/Back/Brace/Rivet`, the binding for
       :sdfpath:`/Chair/Back/Brace/Rivet` will be **metalBits** , because the
       *metalBits* collection is bound more strongly than the *plasticBits* ,
       and includes an ancestor of :sdfpath:`/Chair/Back/Brace/Rivet`.

This formulation addresses both of the concerns raised in the last section:

    #. Because we still require a :usda:`material:binding:collection:NAME`
       relationship on a prim or one of its ancestors, it is irrelevant whether
       any Collection associated with the binding identifies objects in
       arbitrary corners of the scenegraph, because we know we will only need to
       care about objects at or below the bound prim.

    #. The rule presented in item (3) above provides resolution behavior for a
       single prim. For collections bound on different prims, the simple rule
       already in use for direct bindings applies: bindings lower in namespace
       (closer to leaf gprims) are stronger than bindings on ancestors.

Example Collection-Based Assignment
###################################

Let us suppose we have a set constructed of unshaded, instanced assets. The set
contains a group model called :usda:`Desk_Assembly` at some point in its
namespace, and the :usda:`Desk_Assembly` contains a number of groups and
component models, among them, a number of referenced, instanced, "Pencil"
models. The shading artist locates (in a shader library) or creates Materials
for :usda:`PinkPearl` and :usda:`YellowPaint` and adds them to the set - the
Materials themselves can live anywhere in the scene. The shading artist then
creates collections that identify all of the erasers in all of the pencils, and
all of the wooden shafts of the pencils, authoring them on :usda:`Desk_Assembly`
or some other set-level prim (need not be a direct ancestor of the Pencil
instances); although the declarative collections may wind up being large if
there are many pencils, the procedural representation (such as authored in
Katana) from which we baked the collections would generally be quite compact. To
each collection, the artist binds the appropriate Material. Finally, the artist
would add two material assignments (again on an ancestor of the instanced
pencils) that identifies the two collections. The result might look something
like:

.. code-block:: usda
   :caption: Set-Shaded Office_set

   #usda 1.0
   
   over "Office_set"
   {
       def Scope "Materials"
       {
           def Material "Default" { ... }
           def Material "PinkPearl" { ... }
           def Material "YellowPaint" { ... }
   		...
       }
   
       ...
   
       over "Desk_Assembly"
       {
   		rel material:binding = </Office_set/Materials/Default>
   		rel material:binding:collection:Erasers = [</Office_set/Materials/PinkPearl>,
                                                      </Office_set/Desk_Assembly/Cup_grp.collection:Erasers>]
   		rel material:binding:collection:Shafts =  [</Office_set/Materials/YellowPaint>,
                                                      </Office_set/Desk_Assembly/Cup_grp.collection:Shafts> ]
   		 
           over "Cup_grp" 
           {
               rel collection:Erasers:expansionRule = "expandPrims"
               rel collection:Erasers:includes = 
                   [</Office_set/Desk_Assembly/Cup_grp/Pencil_1/Geom/EraserHead>,
                    </Office_set/Desk_Assembly/Cup_grp/Pencil_2/Geom/EraserHead>,
                    </Office_set/Desk_Assembly/Cup_grp/Pencil_3/Geom/EraserHead>,
                    </Office_set/Desk_Assembly/Cup_grp/Pencil_4/Geom/EraserHead>]
               rel collection:Shafts:expansionRule = "expandPrims"
               rel collection:Shafts:includes = 
                   [</Office_set/Desk_Assembly/Cup_grp/Pencil_1/Geom/Shaft>,
                    </Office_set/Desk_Assembly/Cup_grp/Pencil_2/Geom/Shaft>,
                    </Office_set/Desk_Assembly/Cup_grp/Pencil_3/Geom/Shaft>,
                    </Office_set/Desk_Assembly/Cup_grp/Pencil_4/Geom/Shaft>]
   
               # The Pencil_N instances are children of "Cup_grp", but for our purposes, do not
               # even need to be addressed/overridden directly at all in the shading layer, because
               # we can express all the required data on ancestor prims!
   
               ...
           }
       ...
       }
   ...
   }

.. note::

    * The :usda:`material:binding` of the "Default" Material serves the role of
      a "fallback material", which will provide a material definition for any
      prim in the Desk_Assembly that possesses no other material bindings. It
      will also be present in the list of bound materials to be resolved for
      **all prims** in the assembly, but at a weaker strength

Refinement 1: Specifying Binding Strength
#########################################

The :usda:`Office_set` above is a simple set-shading example, because none of the
component models were shaded as component assets. For *simple* assets and uses
of UsdShade, what we have presented thus far should be sufficient. But let us
consider a more realistic example in a film pipeline, in which some of the
assets referenced into a set are unshaded, but some **have already been shaded**
at the asset-level, so that as the assets are referenced into the set, they
bring their materials and material assignments along with them. Because
bindings established on components will be stronger than bindings established on
grouping models introduced at the containing set-level, it may now be difficult
or impossible to assign materials "at the set level" using established and
favored tools. For referenced, uninstanced models, it will be *possible* to
override the component-level bindings via many direct overrides on the bound
prims. But for **instanced components** , we may not even have that option if
the bindings are established internally to the component models, without
uninstancing the components, which we wish to avoid.

Thinking ahead also to how we will represent layered materials and facilitate
such authoring actions as "here at the set-level, I want to add a layer of dirt
onto all (or some) of the prim materials in some sub-tree", we may desire the
ability to specify a modifier to the binding that can determine:

    * whether the binding should be weaker (the default) or **stronger** than
      bindings that appear lower in namespace

    * Alternatively, whether the bound material should be consumed as a named
      material layer by layered materials beneath the bound prim in
      namespace... this is speculative since we do not yet know exactly how we
      will specify layered materials, but it suggests that a binding modifier
      might better be cast as a string/token rather than bool.

Possessing such an ability can also help with a key problem for component assets
that, beneath their root, reference in other complete assets, namely, that it
leads to Materials showing up in unusual/unexpected places, which is a
particular problem in Katana, which expects Materials to be in particular
places. With the ability to add bindings on the referencing (enclosing) model
that targets and overrides bindings on its referenced sub-models, we can provide
a model structure in which all used-Materials for a component model can be
discovered without needing to explore the entire model.

We can add a ":usda:`token bindMaterialAs`" metadatum to the binding schema that
allows the bindings on a prim to individually specify their target
strength/layer. We stipulate the metadata appear on each binding relationship
rather than on the bound prim, because it keeps related data together, for
diagnostic ease, and because copying a binding property from one prim to another
will work without needing to remember to copy some of the source prim's metadata
along with the relationship. Let us revisit the Desk_set example, and presume
that some of the referenced assets may already possess material bindings that we
wish to stomp, at the set-level. This would require two applications of the
metadatum: one for each of the collection-based bindings, but **not** for the
"direct" binding, because it is just intended as a fallback:

.. code-block:: usda
   :caption: Set-Shaded Office_set

   #usda 1.0
   
   over "Office_set"
   {
       def Scope "Materials"
       {
           def Material "Default" { ... }
           def Material "PinkPearl" { ... }
           def Material "YellowPaint" { ... }
   
   		...
       }
   
       ...
   
       over "Desk_Assembly"
       {
   		# We do not want to make the default material binding any stronger
   		rel material:binding = </Office_set/Materials/Default>
   		rel material:binding:collection:Erasers = [</Office_set/Materials/PinkPearl>,
                                                      </Office_set/Desk_Assembly/Cup_grp.collection:Erasers>] (
   			bindMaterialAs = "strongerThanDescendants" 
   		)
   		rel material:binding:collection:Shafts =  [</Office_set/Materials/YellowPaint>,
                                                      </Office_set/Desk_Assembly/Cup_grp.collection:Shafts> ] (
   			bindMaterialAs = "strongerThanDescendants" 
   		)
   
          # No other changes required to collections or references...
       }
   ...
   }

The :usda:`bindMaterialAs` metadatum could be extended, in the future, to accept
tokens such as "layer:dirt" to facilitate layer injection, but initially would
just possess the two values ":usda:`weakerThanDescendants`" and
":usda:`strongerThanDescendants`".

Refinement 2: Material Purpose
##############################

The basic material binding scheme presented here, without the
:usda:`bindMaterialAs` refinement presented in the last section, should be
sufficient for many uses of USD, but even TurboSquid's `StemCell
<https://www.turbosquid.com/stemcell-3d-models>`_ technology packages one set of
textures feeding specular/gloss signals "For use in DCCs and offline renderers"
and another set of textures feeding Metallic PBR "for real-time game engines".
This distinction between :bi:`full` **render** **materials** and :bi:`preview`
**render materials** is common in the VFX industry. From Pixar's own experience,
we find it desirable to potentially have both full and preview materials - *and
the binding of geometry to these materials* - available at the same time in a
scene, with the ability for an application to switch between them without
needing to recompose the scene.

In Pixar's pipeline, the "full" shading is authored for consumption by
Renderman, and the "preview" shading authored to be consumed by Hydra's OpenGL
"stream" backend - but the association of full or preview with particular
renderers need not be made; for example, we should be able to produce a
Renderman render that consumes preview materials. The concept we are
discriminating is not which particular renderer is important, but what the goal
or **purpose** of a given render is, and to choose the "one material used to
shade a particular gprim" based on that criterion. We therefore propose to
introduce the *optional* concept of **material purpose** to our binding schema
and material resolution algorithm, with possible values:

    * **full**
      
      When the purpose of the render is entirely about visualizing the truest
      representation of a scene, considering all lighting and material
      information, at highest fidelity.

    * **preview**
      
      When the render is in service of some other goal (such as scene
      manipulation, modeling, or realtime playback). Latency and speed are
      generally of greater concern for preview renders, therefore preview
      materials might generally be designed to be "lighterweight" than full
      materials. However, there might be more fundamental representational
      differences between full and preview, should the domain benefit from it.
      For example, preview materials might be employed to emphasize or
      categorize objects along some dimension(s) not important for a final
      render, but useful for selection and manipulation, such as rendering all
      fruits one way and all vegetables another so as not to pick an unintended
      edible.

We see no reason to limit the possible purposes for material use in a pipeline
or DCC, but to promote reliable interchange, we propose to make **full** and
**preview** purposes canonical.

Having settled on the concept, we need to decide how material purpose is
encoded. While it would be possible to use naming schemes in Material output
terminals to encode purpose *within materials* , we find it more useful to
encode purpose *as part of the material binding itself* . Recording purpose as
a material binding property rather than as an internal property of Materials has
two key advantages:

    * It is possible for a prim to bind multiple materials for different
      purposes, each material having distinct public interfaces. This is a need
      that comes up regularly in Pixar's character pipeline, and is a
      problem-spot for our current "single binding" schema.

    * Material resolution, using purpose as the primary selector, need only
      examine the bindings, not the materials themselves, in order to locate
      "the one material" for a gprim. For some DCC's this provides a
      substantial performance benefit.

To understand how purpose is encoded and could be used, let us consider an
extension of the first example in this document that demonstrated hierarchical
material resolution:

.. code-block:: usda
   :caption: Resolving Hierarchically-bound Materials

   def Material "PreviewMaterial"
   {
   	outputs:glslfx:surface.connect = </PreviewMaterial/PreviewSurface.outputs:surface>
   
   	def Shader "PreviewSurface"
   	{ ... }
   }
   
   def Material "Skin"
   {
   	outputs:ri:surface.connect = </Skin/pxrSurface1.outputs:bxdf>
   
   	def Shader "pxrSurface1"
   	{ ... }
   }
   
   def Material "Leather"
   {
   	outputs:surface.connect = </Leather/uberSurface.outputs:out>
   
   	def Shader "uberSurface"
   	{ ... }
   }
   
   
   def Xform "Bob"
   {
       rel material:binding:preview = </PreviewMaterial>
   
   	def Xform "Geom"
   	{
   		def Mesh "Body"
   		{
   			rel material:binding:full = </Skin>
   		}
   
           def Mesh "Belt"
           {
               rel material:binding = </Leather>
           }
   	}
   }

In this example we see three bindings, to three different materials. Of note:

    * **Purpose is encoded by relationship name**

      Rather than as metadata on the binding relationship. This makes the
      purpose more salient in most DCC's, and also makes the encoding of
      multiple bindings on a single prim easier to maintain and understand
      (since, as metadata, we would need to arbitrarily adjust binding property
      names for uniqueness if we wanted to bind different materials to the same
      collection for different purposes).

    * **We preserve the ability to add bindings with no specific purpose** 
      
      The binding on :sdfpath:`/Bob/Geom/Belt` is the current, simple
      :usda:`material:binding` relationship. The **semantic** associated with a
      no-purpose binding is to serve as a "fallback" or "all purpose" binding,
      when a specific purpose is requested, but no more-specific binding is
      available. This addresses several concerns:

        * **Backwards compatibility**

          Existing material bindings continue to work in the new scheme, in
          perpetuity

        * **Simple needs result in simple scene description**

          Assets whose shading does not require special handling can simply bind
          Materials without any specific purpose, which means they will be
          available for all purpose requests.

        * **Aids deployment of "library materials" -** 
          
          If we have taken the time to craft reusable materials that contain
          both preview-usable and final-render-usable shaders, it would be
          unfortunate to require that each gprim we bind actually create *two*
          binding relationships targetting the same prim. We can instead just
          use the all-purpose binding.

    * **Similarly for collection-based bindings...** the binding property for a
      *preview* material looks like

.. code-block:: usda

   rel material:binding:collection:preview:Erasers = </Materials/PinkPearl>

Thus, simply from the number of tokens in a valid binding relationship's name,
we immediately know its role:

    * **Two tokens** : the fallback, "all purpose", direct binding,
      :usda:`material:binding`

    * **Three tokens** : a purpose-restricted, direct, fallback binding,
      e.g. :usda:`material:binding:preview`

    * **Four tokens** : an all-purpose, collection-based binding,
      e.g. :usda:`material:binding:collection:metalBits`

    * **Five tokens** : a purpose-restricted, collection-based binding,
      e.g. :usda:`material:binding:collection:full:metalBits`

So, using the example, if we asked for the :bi:`preview` purpose Materials for
either of the gprims, we would get :sdfpath:`/PreviewMaterial`, because the
:usda:`material:binding:preview` binding on :sdfpath:`/Bob` provides a
preview-purpose material for all descendant prims, and there are no other
preview-specific bindings beneath it in namespace. If we ask for the :bi:`full`
binding, we will get :sdfpath:`/Skin` for :sdfpath:`/Bob/Geom/Body` thanks
to its :usda:`material:binding:full` binding, **and** get :sdfpath:`/Leather`
for :sdfpath:`/Bob/Geom/Belt`, because its :usda:`material:binding`
relationship will substitute for lack of a specific :bi:`full` purpose binding.
Finally, were we to remove or block the :usda:`material:binding:preview` binding
on :sdfpath:`/Bob` and then resolve the *preview* bindings, we would get **no
binding** for :sdfpath:`/Bob/Geom/Body`, but :sdfpath:`/Leather` for
:sdfpath:`/Bob/Geom/Belt`, again because the all-purpose binding fills in for
preview just as it does for full.

Material Resolve: Determining the Bound Material for any Geometry Prim
######################################################################

We define *Material Resolve* as the process by which a renderer selects, for
each renderable primitive, **the single** :cpp:`UsdShadeMaterial` from which it
should extract the data needed to build the shading/displacement
shaders/operators it will use in illumination and displacement calculations. In
either the existing, direct-binding-only, or in the proposed, dual
direct/collection binding systems, there will often be an ordered list of
*possible* Materials from which to choose - i.e. Materials bound to the given
prim or its ancestor prims.

The material resolve algorithm in place prior to this new OM required clients to
potentially search through all of the materials in the list, starting with the
strongest, stopping only when they find the particular output terminal they were
seeking - or if they ran out of Material candidates. In addition to the onus of
needing to examine all of the materials, this meant that there might not be "one
bound material" since we were, in theory, free to author surface and
displacement terminals/networks on different Materials, bound at different
points in the hierarchy.

With the introduction of **Material Purpose** , we seek to simplify the resolve
step by parameterizing it with material purpose, and requiring that each
Material contain all the shading required for the purpose, so that there *can*
be a single result Material from the resolve process. At the client API level,
we accept just a single purpose ( *preview* or *full* ), to which is
automatically added the all-purpose binding as a second, weaker binding source. 
If needed, we can optionally provide API that returns the actual purpose
(specified or all-purpose fallback) that we resolved, as well as the prim on
which the winning binding was found, etc.

Following is very un-optimized pseudocode for :cpp:`GetBoundMaterial()`, for a
given prim :bi:`prim` :

.. code-block:: cpp
   :caption: GetBoundMaterial()

   UsdShadeMaterial GetBoundMaterial(UsdPrim const& prim, TfToken const  &materialPurpose)
   {
       vector<TfToken> materialPurposes = { materialPurpose, UsdShadeMaterial::AllPurpose };
       for (auto const& purpose : materialPurposes){ 
           UsdShadeMaterial> boundMaterial;
           for (p = prim; not p.IsPseudoRoot(); p = p.GetParent())
           {
   	        if ( DirectBindingStrongerThanDescendants(p, purpose) or not boundMaterial)
   	        {
   		        if (UsdShadeMaterial directBind = GetDirectlyBoundMaterialForPurpose(p, purpose))
                       boundMaterial = directBind;
   	        }
       
               for (auto const& collBinding : GetCollectionMaterialBindingsForPurpose(p, purpose))
               {
   	            if (collBinding.GetCollection().Contains(prim) and
                       (collBinding.IsStrongerThanDescendants() or not boundMaterial)){
                       boundMaterial = collBinding.GetMaterial();
                       // The first one we match will always be the only one we care about
                       break;
                   }
   	        }
           }
   
           if (boundMaterial)
               return boundMaterial;
       }
   
       return UsdShadeMaterial();
   }

UsdShade API
############

Currently, material binding is exposed as a handful of static methods on
:cpp:`UsdShadeMaterial`. Given the substantial added functionality and API we
are adding to binding, we propose to follow a similar path to that taken with
UsdSkel, and add a :cpp:`UsdShadeMaterialBindingAPI`, to be applied to geometry
prims, which contains all binding-related mutators and queries.

Analysis of Collection-Based Binding
====================================

**Pros**

    #. This encoding maps well to how material assignment is often authored in
       Katana, especially in set-shading workflows. It also closely matches
       MaterialX's encoding of material assignments. The lack of ability to
       assign materials to sub-instanced geometry was also one of the early
       (awaiting resolution) stumbling blocks for the Autodesk engineering team
       investigating USD-encoded shading.

    #. This encoding captures grouping information that we have found useful in
       the past, but have needed to compute more expensively, just-in-time. For
       example, optimizations like shader coalescing in the REYES architecture
       derived benefits from finding all the gprims bound to each shader, so
       that the shader could be emitted and bound just once, followed by all
       affected gprims. While that particular optimization may no longer be
       relevant in the RIS architecture, the fact that we can now easily reason
       about the collection of objects bound to each shader may be beneficial in
       other computations.

    #. This encoding allows a more formal handoff from modeling to shading,
       especially for complex models. Rather than relying on naming conventions
       to identify all the window glass in a building, or all the
       same-construction bolts in a car, the modeler can craft and ship the
       collections that identify these generally uniformly-shadable groups of
       prims (and facesets).

**Cons**

    #. Although both the direct-binding encoding and collection-based encoding
       require a traversal of ancestors to discover any single gprim's
       assignment, the amount of work required to answer the question for a
       single gprim is tremendously more expensive for the collection-based
       encoding, because we may need to resolve all collections bound to all of
       the gprim's ancestors to perform membership tests in all of them. It is
       a practical requirement that we provide an efficient, memoized
       computation for material assignment for use by any client that wants to
       look up assignments one gprim at a time, which currently is all known
       clients.

    #. If material-assignment collections and bindings are authored on component
       model roots, as one might reasonably decide to do, and the models are
       instanced (as is common in Pixar's pipeline), then the "Prototypes" for
       these instances on the :cpp:`UsdStage` will have no binding information,
       because we do not compose any properties on the root prims of prototypes,
       whose properties would all be overridable by the instances themselves.
       The collections and bindings will be expressed on the **instance roots**
       themselves, which, by the nature of instancing, can freely vary,
       per-instance. Given that we do not expect to need or want to vary the
       bindings on every instance, we would prefer to be able to reap the
       benefits of instancing that we get from knowing that data is encapsulated
       inside a prototype, and therefore need be computed only once and shared
       by all instances. Therefore we propose to promote **authoring collections
       and bindings inside component models** . In Pixar's pipeline, this would
       mean authoring collections and material bindings on
       :sdfpath:`/MODEL/Geom`. This need not be a schema requirement, but
       computations will be much more efficient if the pattern is followed.

Integration
===========

Katana Import
#############

We have considerable flexibility in how we translate collection-based bindings
in pxrUsdIn. With our eye on round-tripping, we might want to preserve the
UsdShade encoding as closely as possible; the biggest impact (apart from a new
data model/presentation for downstream-pipeline users) is the need to create a
new material resolver op that gets injected into the proper place in the recipe.
Steve LaVietes believes that it should be able to leverage Katana's existing
infrastructure for pattern-based evaluation, and should be at no substantial
performance disadvantage over direct bindings. **However** , although we will
likely find it convenient to encode the majority of our collection-based
bindings such that the collections are authored on ancestors of the prims
targeted by the collection, there is no performance reason in USD for requiring
that collections target only objects beneath their owning prim, and we do not
want to place such a restriction on clients ability to organize collections.
The Katana implementation must be prepared for such a situation, though we will
advertise "best practice" for material collection authoring to get the best
Katana consumption performance.

Maya I/O
########

Collection-based material binding maps quite well to Maya's native encoding.
Maya does not support, however:

    * Hierarchical resolution of materials (as described in the opening example
      of this document). Therefore, this is a problem we already face (and
      currently ignore) in importing USD into Maya.

    * Any notion of "binding point" in the geometry hierarchy. In Maya, all
      collections (sets) and materials are global in scope (except for the
      unique, per-instance binding arrays, which are logically associated with
      the instance in the dag), and material-binding sets simply apply to all
      targets of the set. We would need to figure out a way to preserve binding
      information on Maya import if round-tripping is a concern.

We may hope for a closer match to Maya with its MaterialX-inspired
pattern/collection bindings.

Houdini
#######

We have not yet explored Houdini I/O implications deeply, but hope to soon.

Remaining Questions
===================

Performance
###########

There is no question that the "material resolve" algorithm for any given, single
prim, can become substantially more expensive with collection-based material
assignment. Previously, we needed only to resolve a single relationship for each
ancestor of any prim. Now, in addition to resolving that same relationship, we
may need to evaluate the prim's path against any number of collections targeted
by the :usda:`material:bind:collections` relationships. The only reasonable way
to resolve materials involves either a top-down (pre or as part of main)
traversal of the scene to discover, compute, and cache results efficiently, or
with memoization. The former approach is more threading-friendly, and the option
we will likely pursue for UsdImaging, however the "computer" we would provide
would also submit to requests that would memoize only the sub-computations
needed to answer queries for single prims, allowing subsequent queries to reuse
the results, when applicable.

A potential concern about memory consumption has been raised over such a caching
computer; we will need to monitor memory consumption in both batch and
interactive rendering applications.

Implication on Renderer Instancing
##################################

A primary motivator for this change to material binding is to support
(re)shading of instanced assets without needing to break the instancing. This
does not deteriorate the two primary advantages we derive from instancing -
reduced prim count on a :cpp:`UsdStage`, and sharing of diced geometry in
renderers - but it does mean renderers that directly support aggregate
instancing (Renderman and HdStream for sure, not Arnold, unsure about other
renderers) will need to gracefully fall back to gprim-level instancing when two
aggregate instances bind different sets of materials. Is this something that Hd
itself addresses? Or does it want to pass along (aggregate instance, resolved
materials closure) pairs to backends to apply/transform as they deem necessary?

It is interesting to note that Houdini already allows a similar, but very much
more powerful and procedural mechanism that allows material and
material-parameter overrides that can penetrate into "packed prims", Houdini's
equivalent of aggregate instances. This mechanism is called `Material
Stylesheets <http://www.sidefx.com/docs/houdini/shade/stylesheets>`_ ; it is
similar in spirit to Pixar's refinement of Katana's "Material Override" operator
(MOP) when the MOP is used in the mode that ties its application to specific
parts of the geometry hierarchy - however MOPs cannot vary instance-specific
geometry inside of Katana native instances. On the other hand, Houdini's
Material Stylesheets can only be consumed (we believe) by the Mantra renderer.

Material Layering
#################

Although there is little possibility we can answer this question prior to
**having** a material layering scheme, the question remains of what interaction
collection-based assignment would have with material layering.

