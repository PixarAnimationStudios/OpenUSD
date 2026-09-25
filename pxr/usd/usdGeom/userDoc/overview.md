# Overview

The UsdGeom schema domain contains schemas for working with geometry and 
related concepts. UsdGeom is designed to provide a common way to represent, 
organize, and interchange geometry between different 3D applications and 
pipelines. 

UsdGeom covers everything from simple geometric primitives like {ref}`Sphere` 
and {ref}`Cube`, to complex surfaces such as {ref}`Mesh` and {ref}`NurbsPatch`, 
to scene organization schemas like {ref}`Xform` and {ref}`Scope`, and pipeline 
utilities like {ref}`PointInstancer`. UsdGeom provides transformational 
organization through {ref}`Xformable` as well as basic rendering and 
visualization organization through {ref}`Imageable`.

(usdGeom_schemas_and_concepts)=
## UsdGeom Schemas and Concepts

UsdGeom schemas fall into four broad groups:

**Base schemas** 

These schemas provide base functionality such as whether a prim is imageable or 
transformable. These are abstract schemas inherited by other schemas in UsdGeom 
and not directly instantiable.

- {ref}`Imageable`: Provides visibility, render purpose, and the primvars 
  attribute schema. All UsdGeom non-API schemas derive directly or indirectly
  from Imageable.
- {ref}`Xformable`: Adds transform operators, commonly referred to as 
  **xformOps** (`xformOp:translate`, `xformOp:rotateXYZ`, etc.) to any prim. All 
  geometry prims are directly transformable, no separate "shape node" is needed.
- {ref}`Boundable`: Adds the `extent` attribute (an axis-aligned bounding 
  box in local space) for fast spatial queries and culling.
- {ref}`Gprim`: Base for all geometric primitive schemas. Adds 
  `primvars:displayColor`, `primvars:displayOpacity`, 
  `doubleSided`, and `orientation`.
- {ref}`PointBased`: Base for geometry defined by an array of points 
  (`points`, `normals`, `velocities`, `accelerations`). Used for meshes, 
  curves, point clouds, and more.

**Scene organization**

These schemas are focused on how prims are grouped and/or spatially organized in 
a scene.

- {ref}`Xform`: A transformable prim that groups child prims under a common
  local coordinate frame. A concrete implementation of Xformable.
- {ref}`Scope`: A non-transformable grouping prim, used purely to organize the
  namespace (e.g. grouping all cameras or all lights under a single parent).

**Typed geometry schemas** 

These schemas represent geometric primitives you can instantiate in a scene. 
OpenUSD provides several intrinsic geometric primitives (Sphere, Cube, etc.), 
complex geometric primitives (Mesh, BasisCurve, Points, etc.), and additional 
typed schemas for camera prims and point instancer prims.

*Intrinsic geometric primitives* (all axis-aligned and centered at the origin)

- {ref}`Sphere`: A sphere sized by `radius`.
- {ref}`Cube`: A box sized by `size` (half-extent in each axis).
- {ref}`Cylinder`: A cylinder with closed ends, sized by `radius` and 
  `height`, with a cardinal axis for the cylinder spine.
- {ref}`Cone`: A cone with one closed end, sized by `radius` and 
  `height`, with a cardinal axis for the cone spine.
- {ref}`Capsule`: A cylinder capped by two hemispheres.
- {ref}`Plane`: A plane, centered at the origin, defined by a cardinal axis, 
  width, and length.

*PointBased geometric primitives*

- {ref}`Mesh`: A polygonal or subdivision surface defined by vertex positions 
  and face topology.
- {ref}`TetMesh`: A tetrahedral mesh, often used for volumetric simulation data.
  It encodes both a structured tetrahedral volume and the triangular-face 
  surface covering that volume. 
- {ref}`NurbsPatch`: Parametric NURBS surface patch.
- {ref}`BasisCurves`: Cubic or linear curve segments (Bezier, B-spline, 
  Catmull-Rom).
- {ref}`NurbsCurves`: NURBS curve primitives.
- {ref}`HermiteCurves`: Hermite-interpolated curve segments.
- {ref}`Points`: A point cloud with per-point positions, widths, and optional
  normals for rendering as particles or halos.

*Additional typed primitives*

- {ref}`PointInstancer`: Scatters many instances of prototype subtrees across
  an array of positions, orientations, and scales.
- {ref}`Camera`: A transformable camera with lens, aperture, and shutter 
  settings.
- {ref}`GeomSubset`: A helper schema that encodes a subset of a piece of 
  geometry (faces, points, edges, etc.) as a set of indices, useful for tasks
  like per-region material assignment.

**API schemas** 

These API schemas can be used to provide certain geometric capabilities to
any prim.

- {ref}`PrimvarsAPI`: A non-applied schema that provides a collection of 
  utilities for creating and querying primvars. 
- {ref}`XformCommonAPI`: A non-applied schema that provides convenience APIs for 
  standard SRT (scale-rotate-translate) transforms with pivot support.
- {ref}`MotionAPI`: An applied schema that provides per-prim motion blur and 
  sampling controls, such as `motion:blurScale`.
- {ref}`GeomModelAPI`: An applied schema that extends a model's root prim with 
  an extent hint and constraint targets. Also provides optional "draw modes" for alternate 
  imaging behavior (e.g. texture cards) for subtrees of the model.
- {ref}`VisibilityAPI`: An applied schema that adds 
  {ref}`render purpose <render_purpose>` visibility related attributes (not to 
  be confused with Imageable's `visibility` attribute).
- {ref}`BackPlateAPI`: A multiple-apply schema that adds a tracking backplate to 
  a Camera prim with various compensation and adaptation controls.

(usdGeom_coordinate_systems_and_orientations)=
## Understanding Coordinate Systems and Orientations

UsdGeom uses a right-handed coordinate system where up is +Y, right is +X, and 
forward is -Z. UsdGeom's {ref}`Camera` similarly views the scene using a 
right-handed coordinate system. Note that you can configure the up axis for a 
stage, see {ref}`render_configuring_stage_coordinates`.

UsdGeom also applies the right hand rule to compute the "intrinsic" surface 
normal (also sometimes referred to as the geometric normal) for all non-implicit 
surface and solid types. For example, UsdGeom will use the right handed winding 
rule to determine the "front" or "outward" facing direction when computing a 
polygon's normals.

To work with tools and pipelines that do not use the right hand rule, the Gprim 
schema provides an {ref}`orientation <Gprim_orientation>` attribute that can be 
set to "rightHanded" (the default) or "leftHanded" to specify which winding rule 
to apply to the Gprim.

```{code-block} usda
def Mesh "ImportedMesh"
{
    # ... mesh data omitted for brevity ...
    uniform token orientation = "leftHanded"
}
```

```{admonition} Transforms can flip orientation
:class: note

A Gprim's local-to-world transformation can flip its effective 
orientation when it contains an odd number of negative scales. This condition 
can be reliably detected using the (Jacobian) determinant of the local-to-world 
transform: if the determinant is less than zero, then the Gprim's orientation 
has been flipped, and therefore you must apply the opposite handedness rule when 
computing its surface normals (or just flip the computed normals).
```

(usdGeom_transforming_geometry)=
## Transforming Geometry

All UsdGeom geometry prims inherit from {ref}`Xformable`, which means every
piece of geometry (Mesh, Sphere, BasisCurves, Camera, etc.) carries its
own transform directly, without requiring a separate parent "transform node." 

The Xformable schema provides translation, rotation, scale, and support for 
complex transform sequences. A transform is expressed as an ordered list of 
xformOps: individual attributes (`xformOp:translate`, 
`xformOp:rotateXYZ`, `xformOp:scale`, etc.) whose evaluation order 
is recorded in `xformOpOrder`. The following example specifies various 
xformOps for a {ref}`Sphere` and specifies the evaluation order.

```{code-block} usda
def Sphere "MySphere"
{
    float3 xformOp:rotateXYZ = (30, 60, 90)
    float3 xformOp:scale = (2, 2, 2)
    double3 xformOp:translate = (0, 100, 0)
    uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:rotateXYZ", 
        "xformOp:scale"]
}
```

Transforms can be animated using {ref}`TimeSamples <time_using_timesamples>`
or {ref}`Splines <time_using_splines>` with op attributes. For example the 
following {ref}`Cylinder` has animated translation and rotation using TimeSamples.

```{code-block} usda
#usda 1.0
(
    startTimeCode = 1
    endTimeCode = 48
    timeCodesPerSecond = 24
)

def Cylinder "AnimatedCylinder"
{
    float3 xformOp:translate.timeSamples = {
        1:  (0, 0, 0),
        24: (10, 0, 0),
        48: (0, 0, 0)
    }
    float3 xformOp:rotateXYZ.timeSamples = {
        1:  (0, 0, 0),
        24: (0, 180, 0),
        48: (0, 360, 0)
    }
    uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:rotateXYZ"]
}
```

For common SRT workflows, {ref}`XformCommonAPI` provides a higher-level
interface with built-in pivot support, compatible with most DCC conventions.

Note that while all geometry prims are directly transformable, UsdGeom does 
provide the concrete {ref}`Xform` schema if you need to create a prim that 
represents a transform and can be used to scope child prims (see next section).

(usdGeom_organizing_your_scene)=
## Organizing Your Scene

{ref}`Xform` and {ref}`Scope` provide the two basic building blocks for
structuring a scene namespace.

Xform can be used as a transformable container. It carries its own transform and 
scopes child prims beneath it. In a character assembly, for example, you might 
place the character's geometry, rig controls, and materials under a root Xform. 
Because UsdGeom makes all geometry prims directly transformable 
(via {ref}`Xformable`), you rarely need extra Xform "wrapper" prims around 
individual shapes but you might use them for grouping subtrees and establishing 
coordinate frames.

Scope can be used as a container, but is purely organizational and has no 
transform. Use it for namespacing groups that don't need a local coordinate 
frame, or will never need to be transformed as a unit, such as 
<span class="mono sdfpath">/World/Cameras</span> or 
<span class="mono sdfpath">/World/Lights</span>.

The following example shows a possible shot assembly structure using both Xform 
and Scope.

```{code-block} usda
#usda 1.0

def Xform "World"
{
    def Scope "Cameras" { }

    def Scope "Lights" { }

    def Xform "Characters"
    {
        def Xform "Hero"
        {
            double3 xformOp:translate = (0, 0, 0)
            uniform token[] xformOpOrder = ["xformOp:translate"]

            def Mesh "Body" { # ... mesh data omitted for brevity ... }
        }
    }

    def Xform "Environment" { }
}
```

(usdGeom_primitives_and_meshes)=
## Working with Primitives and Meshes

UsdGeom provides a family of _intrinsic_ geometric primitives -- Sphere, Cube,
Cylinder, Cone, Capsule, and Plane -- as well as complex surface types (Mesh,
BasisCurves, NurbsPatch, etc.). All derive from {ref}`Gprim`, which adds the
common per-primitive graphical properties: `primvars:displayColor`,
`primvars:displayOpacity`, `doubleSided`, and `orientation`.

The intrinsic primitives are compact and have fast inside/outside tests. In 
film production they are most commonly used for:

- **Light blocking and volumes**: Defining spherical or cylindrical volumes for
  lighting effects.
- **Collision shapes**: Interchanging rigid-body colliders with physics
  simulators.
- **Procedural modifiers**: "Kill spheres" that delete particles when they enter
  the volume.
- **Proxy geometry**: Lightweight stand-ins for complex assets.

```{note}
Not all DCC applications can natively render intrinsic primitives. Applications
that don't support them natively may treat them as non-rendered pass-through 
geometry or may tessellate them on demand.
```

The following example sets up a character with a sphere collider and a
proxy sphere for interactive preview, making use of 
{ref}`imageable purpose <render_purpose>` to indicate which geometry is used for 
which purpose.

```{code-block} usda
#usda 1.0

def Xform "Character"
{
    def Mesh "BodyGeo"
    {
        token purpose = "render"
        rel proxyPrim = </Character/BodyProxy>
        # ... mesh data omitted for brevity ...
    }

    def Sphere "BodyProxy"
    {
        token purpose = "proxy"
        double radius = 1.2
        float3[] extent = [(-1.2, -1.2, -1.2), (1.2, 1.2, 1.2)]
    }

    def Sphere "ColliderVolume"
    {
        token purpose = "guide"
        double radius = 1.5
        float3[] extent = [(-1.5, -1.5, -1.5), (1.5, 1.5, 1.5)]
        color3f[] primvars:displayColor = [(0.2, 0.8, 0.2)] (
            interpolation = "constant"
        )
        float[] primvars:displayOpacity = [0.3] (
            interpolation = "constant"
        )
    }
}
```

{ref}`Mesh` is one of the most commonly used complex primitives. It represents a 
polygonal or subdivision surface defined by vertex positions, face vertex 
counts, and face vertex indices.

The following example defines a simple quad plane, a common base mesh for ground 
surfaces or flat panels in a set. Note that `subdivisionScheme` is authored to
"none" to ensure we have a single quad (the fallback value, "catmullClark", 
would subdivide the quad).

```{code-block} usda
#usda 1.0

def Mesh "GroundPlane"
{
    point3f[] points = [(-5, 0, -5), (5, 0, -5), (5, 0, 5), (-5, 0, 5)]
    int[] faceVertexCounts = [4]
    int[] faceVertexIndices = [0, 1, 2, 3]
    float3[] extent = [(-5, 0, -5), (5, 0, 5)]
    uniform token subdivisionScheme = "none"

    # Texture coordinates for the ground
    texCoord2f[] primvars:st = [(0, 0), (1, 0), (1, 1), (0, 1)] (
        interpolation = "varying"
    )
}
```

To enable subdivision, set the `subdivisionScheme` attribute (or leave it 
unauthored, as it is the fallback value as mentioned earlier):

```{code-block} usda
#usda 1.0

def Mesh "SubdivMesh"
{
    point3f[] points = [# ... control cage points ... ]
    int[] faceVertexCounts = [# ... ]
    int[] faceVertexIndices = [# ... ]
    token subdivisionScheme = "catmullClark"
    # ...
}
```

To encode a subset of Mesh elements use {ref}`GeomSubset`. Note that GeomSubset 
is the only UsdGeom primitive type that is allowed beneath Gprim types, and is 
required to be beneath the Gprim it refines.

The following example names and partitions two sets of mesh faces so that the 
subsets can have different material assignment. 

```{code-block} usda
#usda 1.0

def Mesh "CharacterHead"
{
    point3f[] points = [# ... ]
    int[] faceVertexCounts = [# ... ]
    int[] faceVertexIndices = [# ... ]
    float3[] extent = [(-1, -2, -1), (1, 2, 1)]

    def GeomSubset "SkinFaces"
    {
        uniform token elementType = "face"
        uniform token familyName = "materialBind"
        int[] indices = [0, 1, 2, 3, 4, 5]   # face indices for skin region
        rel material:binding = </Materials/Skin>
    }

    def GeomSubset "TeethFaces"
    {
        uniform token elementType = "face"
        uniform token familyName = "materialBind"
        int[] indices = [6, 7, 8]   # face indices for teeth region
        rel material:binding = </Materials/Teeth>
    }
}
```

For more details about binding geometry to materials, see 
{ref}`render_working_with_materials`.

(usdGeom_working_with_primvars)=
## Working with Primvars

A primvar attribute can be used to interpolate the value of the attribute over 
the surface of a geometric prim. Primvars are most commonly used to 
communicate per-primitive overrides to shaders/materials for rendering (e.g. 
texture coordinates for a surface), but can be used wherever a surface or 
volume-varying signal is needed.

The "GroundPlane" example in the previous section used a `primvars:st` 
primvar to specify the UV coordinates for the mesh.

UsdGeom provides the {ref}`PrimvarsAPI` schema to access primvars on a prim.

See the {ref}`Primvars user guide <primvars>` for more details on primvars.

(usdGeom_controlling_visibility_and_purpose)=
## Controlling Visibility and Purpose

{ref}`Imageable` provides two mechanisms for controlling whether 
geometry participates in a given operation: visibility and purpose.

**Visibility** controls if a prim is rendered. Setting a prim's visibility to 
"invisible" hides it and all its descendants from rendering. Unlike deactivating 
a prim, invisible geometry is still present in the scenegraph and can be 
positioned, inspected, etc.

**Imageable purpose** classifies geometry into categories that describe the 
intended purpose for rendering the prim (e.g. for final renders, for fast 
viewport renders in a DCC tool, etc.). Imageable purpose can be used as a type 
of filter for traversals such as rendering, bounding-box computation, and 
interactive display.

See the {ref}`Rendering with USD guide<rendering_with_usd>` for more details on 
visibility and Imageable purpose.

(usdGeom_point_instancer)=
## Instancing at Scale With PointInstancer

{ref}`PointInstancer` efficiently scatters many instances of one or more
prototype subtrees across a scene. Each instance is defined by a position,
orientation (quaternion), and scale, plus an index into the `prototypes`
relationship that selects which prototype to use. Instances can be animated, 
including topology-varying prototypes, and can be masked (hidden) by integer
ID without removing them from the file.

In the following simple example we use a PointInstancer to instance two
basic prototypes in 4 locations.

```{code-block} usda
def PointInstancer "PointInstancerExample"
{
    rel prototypes = [<Prototypes/Prototype1>,
                      <Prototypes/Prototype2>]

    # Prototype index for each instance
    int[] protoIndices = [0, 1, 0, 1]

    # World-space positions for each instance
    point3f[] positions = [
        (0, 3, 1), (3, 0, 3), (6, 2, 0), (9, 1, 1)
    ]

    # Per-instance orientations 
    quath[] orientations = [
        (1, 0, 0, 0), (1, 0, 0, 0), (1, 0, 0, 0), (1, 0, 0, 0)
    ]

    # Per-instance scales
    float3[] scales = [
        (1, 1, 1), (1.1, 1.1, 1.1), (0.9, 0.9, 0.9), (1, 1, 1)
    ]

    over Scope "Prototypes" {
        def "Prototype1" (
            references = @./sphereProto.usda@
        )
        {
        }
        def "Prototype2" (
            references = @./cubeProto.usda@
        )
        {
        }
    }
}
```

Instance positions can be animated using 
{ref}`TimeSamples <time_using_timesamples>`. Additionally, instances can also 
specify `velocities` and `angularVelocities` to describe motion 
beyond what OpenUSD's TimeSample or Spline interpolation can provide. Using 
velocities is useful when you need to encode the motion of primitives whose 
topology is varying over time. See 
[Applying Timesampled Velocities to Geometry](api/usd_geom_page_front.html#UsdGeom_VelocityInterpolation).

(usdGeom_stage_metrics)=
## Stage Metrics

```{note}
There are plans to provide schemas that capture stage metrics information for
greater flexibility. When these schemas are available, the information in this 
section will be updated accordingly.
```

UsdGeom lets you record two stage-level measurements as metadata on the stage's
root layer. These are read by importing applications to apply corrective
transformations when combining assets from different sources:

**Up axis**: specifies whether Y or Z is the vertical axis:

```{code-block} usda
#usda 1.0
(
    upAxis = "Y"   # or "Z" for Maya/Houdini-Z conventions
)
```

**Meters per unit**: defines the real-world scale of one linear unit:

```{code-block} usda
#usda 1.0
(
    metersPerUnit = 0.01   # 1 unit = 1 centimeter (Maya default)
    # metersPerUnit = 1.0  # 1 unit = 1 meter (OpenUSD default)
)
```

```{admonition} Author stage metrics in every asset
:class: note

Omitting `upAxis` and `metersPerUnit` forces downstream tools to make
assumptions that may be wrong. Explicitly authoring both makes your assets
self-describing and reduces errors when combining content from multiple
sources. Additionally, {ref}`OpenUSD validation <usdglossary-validation>` will 
result in validation test errors in the "usdGeomValidators:StageMetadataChecker"
validator if `upAxis` or `metersPerUnit` are missing.
```

You can set these from Python using the {usdcpp}`UsdGeomSetStageUpAxis` and
{usdcpp}`UsdGeomSetStageMetersPerUnit` utilities:

```{code-block} python
from pxr import Usd, UsdGeom

stage = Usd.Stage.CreateNew("myAsset.usda")
UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
UsdGeom.SetStageMetersPerUnit(stage, UsdGeom.LinearUnits.centimeters)
stage.Save()
```
