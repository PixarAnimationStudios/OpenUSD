# UsdSemantics : Semantic Labeling of Scenes {#usd_semantics_overview}
\if ( PIXAR_MFB_BUILD )
\mainpage UsdSemantics : Semantic Labeling of Scenes
\endif

While prims have a unique name and hierarchical identifier, it is sometimes
useful to reason about the scene graph using a set of labels.

```
def Xform "OfficeBookshelf" (apiSchemas = ["SemanticsLabelsAPI:category"])
{
    token[] semantics:labels:category = ["furniture", "bookcase", "bookshelf"]
}
```

## Inheritance and Comparison to Primvars
Semantic labels have hierarchical inheritance semantics similar
to primvars. However, instead of descendants strictly overriding
parents, labels may be accumulated.

Consider the following snippet of scene description.
```
def Xform "OfficeBookshelf" (apiSchemas = ["SemanticsLabelsAPI:category"])
{
    token[] semantics:labels:category = ["furniture", "bookcase"]
    def Xform "TopShelf" (apiSchemas = ["SemanticsLabelsAPI:category"])
    {
        token[] semantics:labels:category = ["shelf"]
        def Xform "Screw" (apiSchemas = ["SemanticsLabelsAPI:category"])
        {
            token[] semantics:labels:category = ["screw", "hardware"]
        }
    }
}
```
The labels create two tiers of semantics: direct and inherited labels.
* Direct labels
    * `/OfficeBookshelf` has direct labels `bookcase` and `furniture`
    * `/OfficeBookshelf/TopShelf` has a single direct label `shelf`
    * `/OfficeBookshelf/TopShelf/Screw` has a direct labels `screw` and `hardware`
* Inherited labels
    * `/OfficeBookshelf/TopShelf` inherits `furniture` and
      `bookcase`, and adds its direct label `shelf`.
    * `/OfficeBookshelf/TopShelf/Screw` inherits `furniture`,
      `bookcase`, and `shelf` labels

## Taxonomy and Comparison to Model Hierarchy
Model hierarchy is a strict hierarchical taxonomy that
tries to aid discovery of "important" prims in the scene graph. 
Only a single `kind` value may be specified at each prim.

In contrast, multiple instances of semantic labels may be applied, 
as well as multiple values per instance.

```
def Xform "OfficeBookshelf" (
    # Only one kind may be specified
    kind = "component"
    # Multiple taxonomies may be specified
    apiSchemas = ["SemanticsLabelsAPI:category",
                  "SemanticsLabelsAPI:style"]
)
{
    # Multiple labels may be specified per instance
    token[] semantics:labels:category = ["furniture", "bookcase"]
    token[] semantics:labels:style = ["chic", "modern"]
}
```

There is also no registry of taxonomies akin to `kind` in model
hierarchy. Hierarchy required for downstream consumers may be
embedded in the token list.

For example, a prim may be labeled for a particular instance with
`["sedan", "car", "vehicle", "machine"]` capturing the different 
levels of hierarchy in the array values. All labels are weighted 
equally. There is no implied hierarchy in the order of the labels.

## Time Varying Considerations
Labels may be used to describe actions or states and may vary over time.
```
def Xform "Dog" (
    apiSchemas = ["SemanticsLabelsAPI:state"]
)
{
    token[] semantics:labels:state.timeSamples = {
        0 : ["walking"],
        # Transition from walking to running at time code 100
        100 : ["running"]
        200 : ["jumping"]
    }
}
```
Labels may be queried at explicit time samples or over an interval 
(like a shutter window). Queries over a range `[Start, Stop]` will merge
any time samples explicitly authored in the range with the resolved value
for `Start`. In the above example, over the interval `[50, 150]`, the `/Dog`'s
computed set of states are `["walking", "running"]`.

### Intervals and State Transitions
If a state transition occurs during an interval, the compute state of an object 
during that interval may have conflicting labels (say a switch being both `on` 
and `off`). Downstream consumers should be aware of this and be able to either
negotiate multiple states over an interval or only query at explicit time
samples.

## Filtering and Selection by Label
For workflows involving selection or filtering by label, queries
can be combined. For example, "select all wheel prims that are part
of a car prim" could be modeled by filtering the scene graph by prims
with direct label `wheel` and whose parent has an inherited label `car`.

## Relationship to Other Domains
### UsdGeom
The most common application of semantic labels will be to `Gprim`s and their
ancestral `Scope`s and `Xform`s.

#### Subsets
Consumers of labels should consider `GeomSubset`s as valid targets of semantic
labels as well. For example `/Human/Face` `Mesh` prim may have a
`/Human/Face/LeftEar` and `/Human/Face/RightEar` `GeomSubset`s with `["ear"]`
labels applied.

### UsdShade
`Material`s may be semantically labeled as well. For example, a `RustyMetal`
Material could be semantically labeled with both `metal` and `corroded` labels.
As `usdSemantics` is a domain separate from `UsdGeom` and `UsdShade`, there are
no queries which tries to resolve semantics across relationship boundaries. A
`Gprim` may have a computed Material binding to a `Material` labeled as `metal`
or `corroded`, but the `Gprim` is not `metal` or `corroded` in the context
of the scene graph.

Once an element of the render product output has been computed (i.e. sample,
pixel, etc.), this distinction no longer needs to be maintained and the
render product may contain merged sets of labels from materials, geometry,
and other sources.

This particular consideration is important to specify as a primitive may have
multiple material bindings with different purposes bound at different scopes.
Additionally, scene index filters may remove bindings or edit materials.
Materials should not be used for general labeling of geometry.

#### Nested Materials
`Material`s are often nested underneath asset interface prims.

```
def Xform "Car" (
    apiSchemas = ["SemanticsLabelsAPI:category"]
) {
    token[] semantics:labels:category = ["car", "vehicle"]
    def Scope "Materials" {
        def Material "Metal" (
            apiSchemas = ["SemanticsLabelsAPI:material"]
        )
        {
            token[] semantics:labels:material = ["metal", "shiny"]
        }
    }
}
```
In the above example, it's worth noting that the `/Car/Materials/Metal` prim
has inherited labels `car` and `vehicle`, as hierarchical labels apply to the
entire hierarchy.

#### Shaders and Node Graphs
While shaders and node graphs may be semantically labeled for general tagging
purposes, there's no specification for if or how labels should feed into render 
products, as shader nodes and node graphs (unlike `UsdGeomSubset`) generally
don't yield discrete segmentations.

### UsdRender (To be proposed and implemented)
A common application of semantics is for downstream labeling and segmentation
of renderer output. Semantics may make their way into outputs as
either additional metadata or as matte channels.

## Examples
See `extras/usd/examples/usdSemanticsExamples/bookshelf.usda` for an example 
layer that uses semantic labels.