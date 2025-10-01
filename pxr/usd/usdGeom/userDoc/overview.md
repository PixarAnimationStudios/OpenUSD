# Overview

The UsdGeom schema domain contains schemas for working with 3D geometry and related concepts. UsdGeom is designed to provide a common way to represent, organize, and interchange geometry between different 3D applications, making it easier for artists and developers to work with 3D content across various tools and pipelines.

## Boundable


### Setting Extents

```{code-block} python
from pxr import Usd, UsdGeom

stage = Usd.Stage.CreateInMemory()
sphere = UsdGeom.Sphere.Define(stage, "/MySphere")

# Set the sphere's radius
sphere.CreateRadiusAttr(5)

# Calculate and set the extent (bounding box)
sphere.CreateExtentAttr(UsdGeom.Boundable.ComputeExtentFromPlugins(sphere, Usd.TimeCode.Default()))
```

### Bounds vs Extents

Every 3D object needs to know its size and position in space. UsdGeom uses two related but different concepts:

- **Extents**: Authored bounding box data stored as an attribute on geometry prims. This is a rectilinear (box-shaped) volume in local space that contains the geometry.
- **Bounds**: Computed bounding boxes that can be calculated at runtime, often combining multiple extents or computing them dynamically.

**Why both?** Extents provide fast, pre-computed bounding information for performance, while bounds can be calculated on-demand for more complex scenarios. For animated geometry, extents should be authored using timeSamples.

For more details, see the [USD Boundable documentation](https://openusd.org/release/api/class_usd_geom_boundable.html#UsdGeom_Boundable_Extent).



## UsdGeom Schemas and Concepts

UsdGeom includes several schemas that provide the following features:

- Basic geometric primitives (like cubes, spheres, etc.)
- Support for complex geometry (meshes, curves, etc.)
- Transform capabilities
- Primitive variables (primvars) for shader data
- Motion and animation support
- Purpose-based geometry classification

Each of these is described in the following sections.

### Basic Geometric Primitives

UsdGeom provides several simple geometric primitives that are useful for basic shapes and volumes:

- **Cube**: A rectangular box centered at the origin
- **Sphere**: A sphere centered at the origin
- **Cylinder**: A cylinder with closed ends, centered at the origin
- **Cone**: A cone centered at the origin
- **Plane**: A flat surface centered at the origin
- **Capsule**: A cylinder capped by two half-spheres

**Note**: These "intrinsic" primitives (Cube, Sphere, Cylinder, Cone, Capsule) are not supported by all 3D applications. They're particularly useful for:
- **Lighting effects**: Defining light volumes and areas
- **Physics simulations**: Creating collision shapes for rigid body physics
- **Procedural tools**: Using as "kill spheres" or modifiers in particle systems
- **Proxy geometry**: Creating simplified representations of complex models

While these primitives may not render directly in all applications, they can be converted to meshes or used as pass-through geometry in tools that don't support them natively.

### Complex Geometry Types

For more complex geometry, UsdGeom provides:

- **Mesh**: For representing polygonal surfaces
- **BasisCurves**: For representing curves and hair
- **NurbsCurves**: For representing NURBS curves
- **NurbsPatch**: For representing NURBS surfaces
- **Points**: For representing point clouds
- **PointInstancer**: For efficiently instancing many copies of scene subgraphs.
### Transform Capabilities

All geometry in UsdGeom can be transformed. The Xformable schema provides:

- Translation
- Rotation
- Scale
- Support for complex transform sequences

You can combine multiple transforms in any order. For example:

```usda
def Xform "MyObject"
{
    float3 xformOp:rotateXYZ = (30, 60, 90)
    float3 xformOp:scale = (2, 2, 2)
    double3 xformOp:translate = (0, 100, 0)
    uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:rotateXYZ", "xformOp:scale"]
}
```

### Primitive Variables (Primvars)

Primvars are special attributes that can be attached to geometry to provide additional data for rendering. They are useful for:

- **Texture coordinates (UVs)**: How textures are mapped onto surfaces
- **Vertex colors**: Colors that can be painted directly on geometry
- **Custom data for shaders**: Additional information that materials can use
- **Inheriting data**: Sharing information down the scene hierarchy

Primvars can have different interpolation modes that control how values are distributed across a surface:

- **constant**: One value for the entire primitive (like a single color for a whole object)
- **uniform**: One value per face (like a different color for each face of a cube)
- **varying**: Four values per face, smoothly blended across the face
- **vertex**: Values that blend smoothly between vertices (like smooth color gradients)
- **faceVarying**: Four values per face, allowing for sharp edges and seams (like UV seams in textures)

**Example**: A common primvar is `primvars:st` for texture coordinates:
```usda
def Mesh "MyMesh"
{
    # ... mesh data ...
    texCoord2f[] primvars:st = [(0, 0), (0, 1), (1, 1), (1, 0)] (
        interpolation = "faceVarying"
    )
}
```

### Motion

UsdGeom provides several ways to handle motion beyond functionality that is achievable with timeSamples and animation splines:

1. **Velocity-based Motion**
   - Specify velocities for points to describe how fast they're moving
   - Essential for accurate motion blur rendering
   - Useful for geometry that changes topology (like particles or fluid simulations)
   - Allows renderers to sample motion at multiple times for smooth blur effects

2. **Motion API**
   - Control motion blur amount per object using `motion:blurScale`
   - Adjust perceived motion without changing the actual animation
   - Scale motion blur for different objects (e.g., more blur for fast-moving objects)



### Purpose

UsdGeom provides ways to control how geometry is used.

**Purpose** - Categorizes geometry for different uses:
- **default**: General use
- **render**: For final rendering
- **proxy**: For lightweight preview
- **guide**: For visual guides

### Visibility

**Visibility** - Controls whether geometry is visible
- **inherited** - Follows parent visibility
- **invisible** - Always hidden

### Coordinate System and Orientation

UsdGeom uses a right-handed coordinate system where:
- Up is +Y
- Right is +X
- Forward is -Z


Geometry can be set to use either:
- **rightHanded (default)**: Uses right-hand rule for computing surface normals
- **leftHanded**: Uses left-hand rule for computing surface normals


### Stage Metrics

UsdGeom provides stage-level settings that apply to the entire scene:

- **Up Axis**: Whether the scene uses Y or Z as the up axis (default is Y)
- **Linear Units**: The units used for measurements in the scene (metersPerUnit)

These settings help ensure consistent geometry across different applications and are specified in the stage metadata:

```usda
#usda 1.0
(
    upAxis = "Y"  # or "Z"
    metersPerUnit = 1.0  # 1.0 = meters, 0.01 = centimeters, etc.
)
```

**Why this matters**: Different 3D applications use different coordinate systems and units. This stage-level data helps applications reason about the content when loading it so they can apply corrective transformations as needed.

## Best Practices

When working with UsdGeom, consider these best practices:

1. Use the appropriate primitive type for your needs
2. Keep transformations simple when possible
3. Use primvars for shader data
4. Consider using purpose to organize geometry
5. Be consistent with coordinate system and orientation
6. Use motion blur controls when needed
7. Specify stage metrics for better interoperability
