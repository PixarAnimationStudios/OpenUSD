# Important Properties of Scene Description {#Usd_Page_PropertiesOfSceneDescription}

## Names, Namespace Ordering, and Property Namespaces {#Usd_Ordering}

## TimeSamples, Defaults, and Value Resolution {#Usd_ValueResolution}

## Defs, Overs, Classes, and Prim Types {#Usd_PrimSpecifiers}

\sa <A HREF="https://openusd.org/release/usdfaq.html#what-s-the-difference-between-an-over-and-a-typeless-def">What's the difference between an "over" and a "typeless def"? </A>

## Model Hierarchy: Meaning and Purpose {#Usd_ModelHierarchy}

Borrowing from \ref kind_coreKinds, we review the primary \em kinds we use
to create the model hierarchy.
<ul>
<li><b>model</b> - Everything in the model hierarchy is a kind of model.</li>
  <ul>
  <li><b>component</b> - A component model is a terminal model in the model
                         hierarchy - it can have no child models.</li>
  <li><b>group</b> - a model that is simply a container for other models.</li>
    <ul>
    <li><b>assembly</b> - An "important" group model - often because it is
                          itself a published asset.</li>
    </ul>
  </ul>
<li><b>subcomponent</b> - Within a component model, subcomponents identify
                          important (generally articulable) sub-trees.
                          Subcomponents are "stopping points" when
                          dynamically unrolling/expanding a component into
                          an application's native scene description.</li>
</ul>

We interrogate and author kinds on UsdPrim using UsdModelAPI::GetKind() and
UsdModelAPI::SetKind(), in order to establish a *Model Hierarchy*, which is a
contiguous prefix of a scene's namespace.  The intention of model hierarchy
is to recognize that we often organize our scenes into hierarchical
collections of "components" each of which is a substantial, meaningful
partition of the scene (generally, but not required to be, referenced
assets), and codify that organization to facilitate navigating and reasoning
about the scene at a high-level.  In other words, *Model Hierarchy* acts as a
scene's table of contents.

Assuming that the prims on a UsdStage are organized into *assembly*, *group*,
and *component* models, we can use the \ref Usd_PrimFlags "Prim Predicate Flags"
`UsdPrimIsModel` and `UsdPrimIsGroup` in constructing predicates for
UsdPrimRange and UsdStage::Traverse() that will visit all models on a stage,
and no "sub-model" prims.  UsdPrim::IsModel() and UsdPrim::IsGroup() answer
the corresponding questions. 

## How "active" Affects Prims on a UsdStage {#Usd_ActiveInactive}

How does one "delete a prim" in USD?  Considering that any given UsdPrim on a
UsdStage may be comprised of SdfPrimSpec's from numerous layers, and that one
or more (typically many) may not be editable by the user who wants to delete
a prim or subtree rooted at a prim, the "obvious" approach of directly
editing all of the participating layers, while *possible*, is not often
practical or desirable.

Therefore USD provides a "non-destructive" and reversible form of prim
deletion, which we call **deactivation**.  One deactivates a prim using
`UsdPrim::SetActive(false)`, which sets the prim metadata *active* to false.
For any prim on a stage whose *active* metadata resolves to false, we
consider the prim to be *deactivated*, which has two important consequences:

\li The prim will be excluded from default stage traversals as determined by
  the \ref UsdPrimDefaultPredicate.  It is important that the deactivated prim
  still be *present* on the stage, or else we would no longer be able to
  re-activate the prim; however, we promote \ref UsdPrimDefaultPredicate as the
  means to visit/process the "important" part of the stage, thus deactivation
  effectively removes the prim.
\li All scene description for prims *beneath* the deactivated prim will not be
  composed - therefore, the deactivated prim will become a leaf node in the
  scenegraph.  This property makes deactivation a useful tool for reducing
  scene complexity and cost.


## Text, Binary, and Plugin Filetypes {#Usd_Filetypes}
     
## Resolving Asset References {#Usd_AssetResolution}
