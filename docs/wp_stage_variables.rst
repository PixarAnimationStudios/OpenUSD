==========================
Stage Variable Expressions
==========================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

Copyright |copy| 2022, Pixar Animation Studios,  *version 1.0*

.. contents:: :local:

Introduction
============

We propose adding the ability to specify expressions in scene description that
will be evaluated by USD at runtime. These expressions will be allowed to refer
to "stage variables," which are entries in a dictionary-valued layer metadata
field that (with some limitations) are composed across composition arcs.

Support for these expressions will initially be limited to two specific areas:

- Asset paths (including sublayer, reference, and payload asset paths as well as
  asset path-valued attributes and metadata)
- Variant selections

.. code-block:: usda
   :caption: Mock-up with Stage Variables and Expressions

   #usda 1.0
   (
       stageVariables = {
            string PROD = "s101"
            string SHOT = "01"
            bool IS_SPECIAL_SHOT = "`in(${SHOT}, ['01', '03', '05'])`"
            string CROWDS_SHADING_VARIANT = "baked"
       }
 
      # Examples of expressions in sublayer asset paths.
      subLayers = [
           @`if(${IS_SPECIAL_SHOT}, "special_shot_overrides.usd")`@,
           @`${PROD}_${SHOT}_fx.usd`@
      ]
   )
 
   # Examples of expressions in references and asset-valued attributes.
   def "Sox" (
       references = @`${PROD}/Sox/usd/Sox.usd`@</Sox>
   )
   {
       asset skinTexture = @`Sox_${SHOT}_Texture.png`@
   }

   # Example of conditional variant selection
   def "Buzz" (
       references = @Buzz/usd/Buzz.usd@</Buzz>
       variants = {
           string modelVariant = "`if(eq(${SHOT}, "01"), "01_variant", "regular")`"
       }
   )
   {
      # ...
   }

   # Example of unified variant selection
   def "CrowdCharA" (
       references = @CrowdCharA/usd/CrowdCharA@</CrowdCharA>
       variants = {
           string shadingVariant = "`${CROWDS_SHADING_VARIANT}`"
       }
   )
   {
      # ...
   }
 
   def "CrowdCharB" (
       references = @CrowdCharB/usd/CrowdCharB@</CrowdCharA>
       variants = {
           string shadingVariant = "`${CROWDS_SHADING_VARIANT}`"
       }
   )
   {
      # ...
   }

Expressions
===========

Expressions are written in a custom domain-specific language. The syntax and
initial set of operations is TBD. In particular, expressions should be
functional and strongly-typed. The examples in this document are strawmen for
discussion.

Expressions are represented as strings that are surrounded with backticks
(`````). This allows code to recognize expressions with a simple test, which helps
minimize performance cost when this feature is not being used and allows better
error messages. If we didn't explicitly identify expressions somehow, the only
way we'd know it was an expression would be to try to evaluate it (or at least
parse it).

Since expressions are just decorated strings clients could just set them using
existing API for authoring asset paths and variant selections. For example:

.. code-block:: python

   # Asset Paths:
   assetPathWithExpr = Sdf.AssetPath('`if(eq(${SHOT}, "01"), "shot_01.usd", "shot_other.usd")`')
 
   # Variant Selections:
   primSpec.variantSelections['modelVariant'] = '`if(eq(${SHOT}, "01"), "shot_01_variant", "shot_other_variant")`'

Stage Variables
===============

Stage variables are the only scene description that expressions are allowed to
refer to. These variables may be strings, bools, or ints and are stored in a
dictionary-valued layer metadata field named ``stageVariables``. Stage variables
may themselves be expressions, so that common logic can be factored into a
single location for convenience and brevity.

As the name implies, stage variables are stage metadata, meaning they must be
authored on either the root or session layer of a stage or in the root layer of
a referenced layer stack. See below for more discussion of the composition
behaviors.

.. code-block:: usda

   #usda 1.0
   (
       stageVariables = {
           string SHOT = "01"
           string PROD = "r345"
           bool IS_SHOT_01 = '`eq(${SHOT}, "01")`'
       }
   )

   # ...

Stage Variables DO NOT Compose Across Sublayers
-----------------------------------------------

Like all other stage metadata, stage variables do not compose across sublayers
in a layer stack except for the session and root layers of a stage.

This restriction is primarily needed because sublayers may themselves be
specified using expressions that rely on stage variables. If we allowed stage
variables to compose across sublayers, we'd introduce circular dependencies that
would be, at best, difficult to handle consistently. For example,

.. code-block:: usda

   #usda 1.0
   (
       subLayers = [
           # sub_2.usd specifies stageVariables[MYFLAG] = 2
           @`if(eq(${MYFLAG}, 1), "sub_2.usd", "sub_1.usd")`@,
    
           # Specifies stageVariables[MYFLAG] = 1
           @prod.usd@
       ]
   )

   # ...

Let's assume stage variables did compose across sublayers. This layer stack
could be evaluated two different ways:

- If the expression is evaluated first, it will not find a MYFLAG stage var and
  will load sub_1.usd . Then, prod.usd will be loaded, which introduces MYFLAG=1
  . If a user were to ask the stage for the composed stage vars, it would return
  MYFLAG=1 , but that would be inconsistent with the fact that sub_1.usd had
  been loaded.

- If the weakest layer is evaluated first, the stage var MYFLAG=1 will be set,
  which will cause the expression to evaluate to and load sub_2.usd , which sets
  MYFLAG=2 . If a user were to ask for the composed stage vars, it would return
  MYFLAG=2 (because it was introduced in a stronger sublayer), but again this
  would be inconsistent with the sublayers that had actually been loaded.

This example also shows that allowing composition across sublayers would make it
difficult (impossible?) to enable multithreaded loading of sublayers in layer
stacks, again because of intra-sublayer dependencies.

Stage Variables DO Compose Across References / Payloads
-------------------------------------------------------

If a referenced layer stack contains expressions that refer to stage variables,
the stage variables from the chain of referencing layer stacks will be composed
together and used to evaluate those expressions. However, note that the sublayer
restriction from above still applies, so only stage variables authored on the
root layer of the layer stacks will be considered.

This ensures that assets that internally use expressions referring to stage
variables can be referenced by another asset and still work as expected. This
also gives the referencing asset the ability to override the stage variables
used by the referenced asset. For example:

.. code-block:: usda
   :caption: ModelGroup.usd

   #usda 1.0
   (
       stageVariables = {
           string MODEL_VARIANT = "var_1"
       }
   )
 
   def "ModelGroup"
   {
       def "Model_1" (
           variantSelections = {
               string modelVariant = "`${MODEL_VARIANT}`"
           }
       ) { }
 
        def "Model_2" (
           variantSelections = {
               string modelVariant = "`${MODEL_VARIANT}`"
           }
       ) { }
   }

.. code-block:: usda
   :caption: Stage1.usd

   #usda 1.0
 
   # The expected behavior would be to bring in ModelGroup
   # with variant selection "modelVariant=var_1" for all
   # models in the group.
   def "ModelGroup" (
       references = @./ModelGroup.usd@</ModelGroup>
   )
   {
   }

.. code-block:: usda
   :caption: Stage2.usd

   #usda 1.0
   (
       stageVariables = {
           string MODEL_VARIANT = "var_2"
       }
   )
 
   # Now all models in this model group come in with
   # variantSelection "modelVariant=var_2"
   def "ModelGroup" (
       references = @./ModelGroup.usd@</ModelGroup>
   )
   {
   }

Areas of Consideration
======================

Composition
-----------

During composition, `Pcp <api/pcp_page_front.html>`_ will inspect layer asset
paths in composition arcs (e.g, sublayers, references, payloads) to determine if
they contain an expression and, if so, will evaluate them using the stage
variables composed to that point in the composition graph.

If the expression is successfully evaluated, Pcp will treat the resulting string
like it would any other authored asset path: it will try to find or open the
layer at that path to continue the composition process and issue a composition
error if it fails to do so.

If the expression cannot be evaluated, Pcp will issue a composition error that
will include information about the expression. The expression may fail to
evaluate due to a syntax error, or because it references a stage variable that
has no value. This error is distinct from the "invalid asset path" error
mentioned above that is issued when a layer at a given asset path cannot be
opened. It's important to treat these separately to provide the user with good
diagnostics. However, if an expression evaluates to an empty string, it will be
treated as though there was no value and will *not* result in an "invalid asset
path" error.

The behavior is the same for variant selections containing expressions. If the
expression is successfully evaluated Pcp will treat the resulting string like it
would an authored variant selection. If the expression fails to evaluate (for
the same reasons above), Pcp will issue a composition error including
information about the expression.

Asset Path Attributes and Metadata
----------------------------------

Currently, calling :usdcpp:`UsdAttribute::Get` on asset path-valued attributes
returns an :usdcpp:`SdfAssetPath` containing two fields:

- The "unresolved" asset path, which today is just the strongest authored value
  for the attribute.

- The "resolved" asset path, which is the result of resolving the above via
  :usdcpp:`ArResolver`. If :usdcpp:`UsdAttribute` detects that the unresolved
  asset path contains an expression, it will evaluate them using the stage
  variables composed to the point in the composition graph where the asset path
  was authored.

If the expression is successfully evaluated into an asset path,
:usdcpp:`UsdAttribute` will return an :usdcpp:`SdfAssetPath` with the unresolved
path set to the evaluated asset path and the resolved path set to the result of
resolving that asset path.

If the evaluation fails due to a syntax error, the unresolved asset path will be
set to the authored expression and no attempt to resolve that path will
occur. However, if the evaluation produces an asset path that just has a missing
variable substitution, the unresolved path will be set to that "partial" result
instead and :usdcpp:`UsdAttribute` will attempt to resolve that path. For example:

.. code-block:: usda

   # Usd.Attribute.Get() returns Sdf.AssetPath('`bogus_expression("foo.usd")`', '')
   asset badExpression = @`bogus_expression("foo.usd")`@

   # Usd.Attribute.Get() returns Sdf.AssetPath(`foo_${THIS_VAR_MISSING}.usd`, '<result from ArResolver>')
   asset partialExpression = @`if(${SOME_TRUE_VALUE}, "foo_${THIS_VAR_MISSING}.usd")`@ 

This partial expression behavior allows substitution tokens to pass through Usd
for further evaluation by downstream clients.

The same behavior described above applies for asset path-valued metadata
(including asset paths authored in dictionaries) retrieved via
:usdcpp:`UsdObject::GetMetadata`.

Change Processing
-----------------

Changes to stage variables that are used in expressions in references and other
composition arcs will trigger recomposition as needed, as much of the necessary
dependency tracking for this is already in place in Pcp.

Changes that affect expressions in asset path-valued attributes and metadata
present a bigger problem. Ideally, these would cause :usdcpp:`UsdStage` to send
out a change notice for just the prims and properties that were
affected. However, since :usdcpp:`UsdStage` does not cache or pre-compute any of
these values, it currently has no way of determining exactly what objects on the
stage depend on a given stage variable.

To work around this, when the composed value of a stage variable changes,
:usdcpp:`UsdStage` will send out a notice indicating a full resync of the stage,
even though internally nothing may actually be recomposed. This is a heavy
hammer but ensures that clients will be made aware that there was a possible
change to any asset path-valued attributes. :usdcpp:`UsdStage` already takes
this approach when dealing with "resolver changed" notices from Ar, which
indicate that any asset path that had previously been resolved might now resolve
to a new path (see here).

An alternative solution is to provide a new Usd notice (or an extra bit on the
existing :usdcpp:`UsdNotice::ObjectsChanged` notice) that specifically indicates
that all asset path values may have changed on the associated change, instead of
sending out the full resync notice. This allows clients to choose the level of
invalidation they need to perform in response to this change, but it puts
clients on the hook for processing this new notice.

Yet another alternative is to implement the caching for asset path-valued
attributes and metadata mentioned above. :usdcpp:`UsdStage` would keep track of
the asset paths it has handed out via :usdcpp:`UsdAttribute::Get` and other API
and use that to determine what asset paths were affected by a stage variable
change. This would allow the normal :usdcpp:`UsdNotice::ObjectsChanged` notice
to be sent instead of the coarse invalidation mentioned above. This approach
would fit into existing client code more easily, but would add extra overhead
and complexity to UsdStage when retrieving the asset path values. This overhead
should be limited to just asset path values, although some experimentation would
be needed to confirm this.

Dependency Analysis / Asset Isolation
-------------------------------------

Dependency analysis involves inspecting a given asset for all other assets it
refers to. This information is used by asset isolation to create a separate copy
of an asset in a different location that can be packaged up and distributed to
other locations while maintaining the same behaviors. In USD, this is primarily
handled by code like :usdcpp:`UsdUtilsComputeAllDependencies`.

The analysis process recursively walks layers while looking at scene description
like composition arcs and attributes for references to other assets, which are
recorded as dependencies. This process gets complicated with expressions in the
mix, since the full set of referenced assets may not be enumerable just by
looking at the scene description.

The simplest case is an asset where all of the stage variables referenced by
expressions in that asset are defined. In that case, every expression can be
evaluated and the resulting assets noted as explicit dependencies. For example,
consider something like:

.. code-block:: usda

   #usda 1.0
   (
       stageVariables = {
           string VAR = "x"
       }
   )
 
   def "Asset" (
       references = @`if(eq(${VAR}, "x"), "x.usd" : "y.usd"`@
   )
   {
   }

In this case, dependency analysis can determine that this asset in its current
state depends only on :filename:`x.usd`, so asset isolation would just copy over
that file. That should be sufficient for cases where we only need to consume the
isolated asset as-is.

But, what if we needed to work with the isolated asset fully and change VAR to
"y" sometimes? In that case, we'd need dependency analysis to recognize that
:filename:`y.usd` is a possible dependency and must also be captured. That is
possible in the example above, since the results of the "if" expression are
enumerable just by looking at the expression itself. But just a simple direct
substitution in an asset path makes this impossible:

.. code-block:: usda

   #usda 1.0
   (
       stageVariables = {
           string VAR = "x"
       }
   )
 
   def "Asset" (
       references = @`${VAR}.usd`@
   )
   {
   }

In this case, the best that dependency analysis can do is note that
:filename:`x.usd` is a dependency – it has no idea that :filename:`y.usd` is a
possible value.

Dependency analysis will make a "best effort" attempt at enumerating all
possible dependencies. This implies the ability to evaluate expressions in some
mode where all possible results are returned if possible. As an initial step,
for cases like variable substitutions where the possible results are not
enumerable, dependency analysis will issue a warning.

In the future (or possibly as part of this work, if we deem it necessary), the
dependency analysis functions would allow users to supply a callback or plugin
that would be expected to list all of the possible results of a given
expression. We could provide a default callback implementation that relies on
filesystem operations and globbing.

GUI Support
-----------

:ref:`toolset:usdview` will be updated to display the evaluated expression (or
optionally the authored expression) in its UI for payload/references/sublayer
asset paths.

For variant selections, the combo box that lets users pick a selection will be
disabled, but will display the evaluated expression.

In both cases, we may want to add some kind of affordance to allow users to view
the expression itself.

Example Use Cases
=================

Variant Selections
------------------

Stage variable expressions can provide flexible ways of specifying variations
for logical groups of prims that are controlled via a single stage variable.

For example, consider an asset with a large crowd of models, each with variants
controlling some aspect of their look, like a color palette. The crowds artist
could select models that should have the same color palette and set their
variant selections to corresponding stage variables, like:

.. code-block:: usda
   :caption: Crowd.usd

   #usda 1.0
   (
       stageVariables = {
           string COLOR_GROUP_1 = "regular"
           string COLOR_GROUP_2 = "regular"
       }
   )

   def "Crowd"
   {
       def "Model_1" (
           variantSelection = {
               string palette = "${COLOR_GROUP_1}"
           }
       )
       {
       }

       def "Model_2" (
           variantSelection = {
               string palette = "${COLOR_GROUP_2}"
           }
       )
       {
       }

       # etc., etc. ... 
   }

When an artist references this crowd asset into their own stage, they can
change the color palettes for all of the prims in the groups determined
by the original crowd artist just by overriding the stage variable:

.. code-block:: usda
   :caption: Shot.usd

   #usda 1.0
   (
       stageVariables = {
           string COLOR_GROUP_1 = "rainbow"
           string COLOR_GROUP_2 = "monochrome"
       }
   )

   def "Crowd" (references = @Crowd.usd@</Crowd>)
   {
   }        

The artist can still change the palette variant on individual models if
they desire simply by overriding the variant selection on the specific prim.

Varying Texture Format
----------------------

Disney Animation's Moana Island data set uses :filename:`.png` and
:filename:`.exr` files for environment maps by default. However, RenderMan
does not support these formats, so the data set provides an alternate root
layer that overrides the attributes specifying these maps with
:filename:`.tex` images instead. On one hand, this demonstrates the power
of USD's sparse overrides. On the other hand, it means these overrides
have to be kept in sync with the underlying data set -- if the namespace
location of the skydome prim changed, for example, the overrides would
need to be updated as well.

With stage variable expressions, this difference could be encoded at the
asset path attribute itself:

.. code-block:: usda

   asset inputs:texture:file = @`if(${FOR_PRMAN}, "../textures/islandsunEnv.tex", "../textures/islandsun.exr")@

The data set could still provide the RenderMan-specific root layer as an
entry point, but instead of overriding the texture attributes it would just
set the FOR_PRMAN stage variable to true.

Render Passes
-------------

One intended use case at Pixar is to encode a named "render pass" into a
stage variable and conditionally include sublayers based on the specified
render pass. For example, an artist could specify that they are rendering the
``fx`` render pass for a given shot via a stage variable. The shot's sublayer
list would include a special sublayer that deactivated non-effects-related
geometry to speed up processing. Shots might have many different render pass
layers for the artist to choose from, each of which would prune out geometry
or perform other overrides specific to that pass. This might look like:

.. code-block:: usda

   #usda 1.0
   (
       subLayers = [
           @render_pass_${RENDER_PASS}.usd@,
           ...
       ]
   )

Shot-Level Overrides in Sequences
---------------------------------

At Pixar, animation is organized into sequences (e.g. r720) that are groups of
shots (e.g. r720_1, r720_2, etc.). Sequences may contain tens of shots. The
scene description for each shot typically consists of a root layer for that
shot, which then includes a series of sublayers for various departments that are
specific to that shot, followed by a series of sublayers that are specific to
that sequence and shared among all of the constituent shots. Here's a very
simplified example:

+-------------------+-------------------+
| r720_1 layers     | r720_2 layers     |
+===================+===================+
| r720_1.usd        | r720_2.usd        |
+-------------------+-------------------+
| r720_1_anim.usd   | r720_2_anim.usd   |
+-------------------+-------------------+
| r720_1_layout.usd | r720_2_layout.usd |
+-------------------+-------------------+
| r720.usd                              |
+---------------------------------------+
| r720_anim.usd                         |
+---------------------------------------+
| r720_layout.usd                       |
+---------------------------------------+

During production, there are cases where the same set of overrides must
be authored on a subset of the shots in a sequence. We currently deal with
this by using a tool that iterates over the desired shots and applies the
edits to each one. Scene variable expressions could simplify this greatly
by authoring the edits into a sequence-level sublayer that is only included
for the desired shots. This might look like:

.. code-block:: usda
   :caption: r720_1.usd

   #usda 1.0
   (
       stageVariables = {
           string SHOT = "01"
       }
   )

   #...

.. code-block:: usda
   :caption: r720_2.usd

   #usda 1.0
   (
       stageVariables = {
           string SHOT = "02"
       }
   )

   #...

.. code-block:: usda
   :caption: r720.usd

   #usda 1.0
   (
       subLayers = [
           @`if(in(${SHOT}, ["01", "03"]), "r720_shot_edits.usd")`@,
           @r720_anim.usd@,
           ...
       ]
   )

   #...
