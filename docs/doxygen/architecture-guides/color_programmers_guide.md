# Programmer's Guide to Color in OpenUSD {#Page_Color_Programmers_Guide}

# Overview

This guide aims to provide developers with a solid foundation in color science 
concepts and practical implementation details on the treatment of color within 
OpenUSD both in scene data and via OpenUSD's interfaces. This guide will 
navigate the complexities of color and is of interest to developers integrating 
OpenUSD into an existing renderer, building tools for artists, or creating 
content procedurally.

Readers should familiarize themselves with fundamental color science concepts
and terminology by reading the 
[Color User's Guide](https://openusd.org/release/user_guides/color_user_guide.html) 
in addition to this document.

## OpenUSD Color Management Architecture {#Color_Management_Architecture}

OpenUSD provides a rigorous and well defined system for managing scene-referred 
color spaces through its API schemas. By "scene-referred" we mean specifying the 
*source color space* for any authored color attribute or asset path identifying 
a texture or other external color-containing asset. The primary applied schemas 
are:

1. **UsdColorSpaceAPI**: Allows specifying color space information on prims.
2. **UsdColorSpaceDefinitionAPI**: A multiple-apply schema that enables 
definition of custom color spaces.

### Key Concepts in USD Color Management {#Color_Key_Concepts_in_Color_Management}

#### Color Space Inheritance {#Color_Space_Inheritance}

In OpenUSD, color spaces can be specified at any level in the scene hierarchy 
and are inherited by child prims. This hierarchical approach allows for:

- Setting a global color space at the root.
- Providing a color space for a subgraph of the scene.
- Specifying a color space for attributes on individual prims.
- Authoring the color on individual attributes.

#### Color Space Resolution {#Color_Space_Resolution}

When determining the color space for a value, OpenUSD follows a specific 
resolution order:

1. Check if the attribute has an explicitly assigned color space.
2. Check if the attribute's prim has UsdColorSpaceAPI applied.
3. Search up the hierarchy until a prim with UsdColorSpaceAPI is found.
4. If no color space is found, an empty token is returned. If no color space is found, an empty token is returned. In this case, the color space must be assumed 
to be Linear Rec.709.

## Programming With UsdColorSpaceAPI {#Color_Programming_With_UsdColorSpaceAPI}

### Applying Color Space Information {#Color_Applying_Color_Space_Information}

The colorSpaceName attribute is a uniform value applied to a prim. To apply 
color space information to a prim:

```cpp
// Get a prim
UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Root/Geom"));

// Apply the color space API
UsdColorSpaceAPI colorSpaceAPI = UsdColorSpaceAPI::Apply(prim);

// Set the color space
colorSpaceAPI.CreateColorSpaceNameAttr(
                               VtValue(GfColorSpaceNames->LinearRec709));
```

### Querying Color Space Information {#Color_Querying_Color_Space_Information}

To determine the color space for an attribute:

```cpp
// Get an attribute
UsdAttribute attr = prim.GetAttribute(TfToken("diffuseColor"));

// Compute its color space
TfToken colorSpaceName = UsdColorSpaceAPI::ComputeColorSpaceName(attr);

// Convert to a GfColorSpace for transformation operations
GfColorSpace colorSpace = UsdColorSpaceAPI::ComputeColorSpace(attr);
```

### Performance Optimization With Caching {#Color_Performance_Optimization_With_Caching}

For performance-critical applications, OpenUSD provides a caching mechanism to 
accelerate large numbers of look ups:

```cpp
// Create a cache
UsdColorSpaceAPI::ColorSpaceHashCache cache;

// Use the cache when computing color spaces
TfToken colorSpaceName = UsdColorSpaceAPI::ComputeColorSpaceName(attr, &cache);
```

## Defining Custom Color Spaces {#Color_Defining_Custom_Color_Spaces}

OpenUSD allows defining custom color spaces using the 
UsdColorSpaceDefinitionAPI:

```cpp
// Get a prim to which we'll attach the color space definition
UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Root"));

// Apply a color space definition named "customSpace"
TfToken definitionName("customSpace");
UsdColorSpaceDefinitionAPI definition = 
    UsdColorSpaceDefinitionAPI::Apply(prim, definitionName);

// Set the name attribute
definition.CreateNameAttr(VtValue(definitionName));

// Define the color space parameters
GfVec2f redChroma(0.64, 0.33);
GfVec2f greenChroma(0.30, 0.60);
GfVec2f blueChroma(0.15, 0.06);
GfVec2f whitePoint(0.3127, 0.3290);  // D65
float gamma = 1.0;  // Linear
float linearBias = 0.0;

definition.CreateColorSpaceAttrsWithChroma(
    redChroma, greenChroma, blueChroma, whitePoint, gamma, linearBias);
```

Alternatively, you can define a color space from an RGB to XYZ transformation 
matrix:

```cpp
GfMatrix3f rgbToXYZ = /* RGB to XYZ transformation matrix */;
float gamma = 1.0;
float linearBias = 0.0;

definition.CreateColorSpaceAttrsWithMatrix(rgbToXYZ, gamma, linearBias);
```

## Color Space Transformations {#Color_Space_Transformations}

To convert colors between spaces, use the GfColorSpace class:

```cpp
// Get color spaces
GfColorSpace srcSpace = UsdColorSpaceAPI::ComputeColorSpace(srcAttr);
GfColorSpace targetSpace = GfColorSpace(GfColorSpaceNames->LinearAP1);

// Convert a single color
GfVec3f srcColor(1.0f, 0.5f, 0.0f);
GfVec3f targetColor = srcSpace.Convert(targetSpace, srcColor);

// Convert an array of colors
std::vector<GfVec3f> colorArray = /* Array of colors */;
srcSpace.ConvertRGBSpan(targetSpace, colorArray.data(), colorArray.size());
```

## Best Practices for USD Color Management {#Color_Best_Practices}

### Establish a Clear Color Management Policy {#Color_Establish_a_Clear_Color_Management_Policy}

Define which color spaces your pipeline uses for:
- Texture inputs
- Material parameters
- Render outputs

### Be Explicit About Color Spaces {#Color_Be_Explicit_About_Color_Spaces}

Specify color spaces explicitly rather than relying on defaults. 

A general best practice is to first ensure that the stage's root prim has the 
UsdColorSpaceAPI schema applied, and set the prim's color space to the 
studio/pipeline default (e.g. LinearRec2020). 

Then, for textures or other color assets created outside the studio/pipeline
using a different source color space, make sure to explicitly set the 
appropriate color space.

```cpp
// Explicitly set the color space for attribute
UsdAttribute attr = prim.GetAttribute(TfToken("inputs:file"));
attr.SetColorSpace(GfColorSpaceNames->SRGBRec709);
```

### Perform Computations in Linear Space {#Color_Perform_Computations_in_Linear_Space}

Ensure all lighting calculations, blending, and compositing happen in linear 
color space. In most scenarios, the color space to be used for these
computations would be specified in \ref 
UsdRenderSettings::GetRenderingColorSpaceAttr() "RenderSettings.renderingColorSpace".

```cpp
// Convert to linear if needed before computation
GfColorSpace srcSpace = UsdColorSpaceAPI::ComputeColorSpace(attr);
GfColorSpace linearSpace = GfColorSpace(GfColorSpaceNames->LinearRec2020);
GfVec3f linearColor = srcSpace.Convert(linearSpace, originalColor);

// Perform computation...
GfVec3f result = /* computation */;

// Convert back if needed
GfVec3f displayColor = linearSpace.Convert(displaySpace, result);
```

### Cache Color Space Lookups {#Color_Cache_Color_Space_Lookups}

For render loops or other performance-critical sections:

```cpp
UsdColorSpaceAPI::ColorSpaceHashCache cache;

// Later in your render loop:
for (const auto& attr : attributes) {
    GfColorSpace colorSpace = UsdColorSpaceAPI::ComputeColorSpace(attr, &cache);
    // Use colorSpace...
}
```

### Clear Cache When Scene Changes {#Color_Clear_Cache_When_Scene_Changes}

If your application caches color space information, ensure the cache is 
invalidated when:

- Stage contents change
- Layer stacks are modified