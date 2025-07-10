.. include:: ../rolesAndUtils.rst

.. _color_users_guide:

##################
Color User's Guide
##################

This guide describes how to work with color and color spaces in OpenUSD. OpenUSD 
provides ways to specify color space information, and supports a canonical set 
of interoperable color spaces, along with custom color spaces.

This guide also aims to provide users with a solid foundation in color science 
concepts and practical implementation details on the treatment of color within 
OpenUSD both in scene data and via OpenUSD's interfaces. It will navigate the 
complexities of color and is of interest to users working with color in OpenUSD 
content or color configuration in tools and renderers that interface with 
OpenUSD.

Developers can also use this guide, along with the 
`Color Programmer's Guide <../api/_page__color__programmers__guide.html>`__, to 
understand how to integrate OpenUSD color features into their renderer or 
tool.

.. contents:: Table of Contents
    :local:
    :depth: 2

.. _color_working_with_color_in_usd:

*****************************
Working With Color in OpenUSD
*****************************

When working with renderable scene data in OpenUSD, in order to ensure 
consistent and accurate colors, OpenUSD allows using and defining 
:ref:`color spaces <color_what_is_a_color_space>`. There are two areas where 
you may need to specify color space information:

1. For scene-referred colors. 

   By "scene-referred" we mean specifying the *source color space* for any 
   authored color attribute or asset path identifying a texture or other 
   external color-containing asset.

   USD provides API schemas for authoring source color space information on 
   prims, as well as methods for specifying per-attribute color space 
   information. More details on this are provided in 
   :ref:`color_working_with_color_space_schemas` below.

2. For rendering/lighting calculations involving colors. 

   USD provides a way to configure which color space a renderer should use for 
   rendering/lighting calculations, via the 
   :usda:`RenderSettings.renderingColorSpace` attribute. See 
   :ref:`RenderSettings_renderingColorSpace` and 
   :ref:`the UsdRender schemas <usdRender_overview>` for more details on using 
   RenderSettings.

Color Spaces Supported by OpenUSD
=================================

OpenUSD, `MaterialX <https://materialx.org/>`__, and 
`OCIO <https://opencolorio.org/>`__ have jointly agreed on a canonical set of 
interoperable color spaces, and OpenUSD provides a means of supplying additional 
custom color spaces. The canonical color spaces and their OpenUSD tokens are:

.. _color_space_table:

.. list-table:: 
   :header-rows: 1

   * - | Color Space
       |
     - | OpenUSD token 
       | (format is: transfer_colorPrimaries_imageState)
     - | Specification or other references
       |
   * - ACEScg
     - lin_ap1_scene
     - | A wide-gamut color space used in the Academy Color Encoding System. 
       | See: https://docs.acescentral.com/specifications/acescg/
   * - ACES2065-1
     - lin_ap0_scene
     - https://doi.org/10.5594/SMPTE.ST2065-1.2021
   * - Linear Rec.709 (sRGB)
     - lin_rec709_scene
     - See primaries described in: https://www.itu.int/rec/R-REC-BT.709
   * - Linear P3-D65
     - lin_p3d65_scene
     - | See primaries standardized as the minimum gamut of the Digital Camera 
       | Initiatives reference projector standard `SMPTE RP 431-2 <https://pub.smpte.org/pub/rp431-2/rp0431-2-2011.pdf>`__
   * - Linear Rec.2020
     - lin_rec2020_scene
     - See primaries described in: https://www.itu.int/rec/R-REC-BT.2020
   * - Linear Adobe RGB
     - lin_adobergb_scene
     - N/A     
   * - CIE XYZ-D65
     - lin_ciexyzd65_scene
     - | See `CIE 015:2018 - Colorimetry, 4th edition <https://store.accuristech.com/cie/standards/cie-015-2018?product\_id=2025773>`__ 
       | and `<https://doi.org/10.5594/SMPTE.ST2065-1.2021>`__  
   * - sRGB Encoded Rec.709 (sRGB)
     - srgb_rec709_scene
     - https://www.colour-science.org/posts/srgb-eotf-pure-gamma-22-or-piece-wise-function/
   * - Gamma 2.2 Encoded Rec.709
     - g22_rec709_scene
     - See primaries described in: https://www.itu.int/rec/R-REC-BT.709
   * - Gamma Encoded 1.8 Rec.709
     - g18_rec709_scene
     - See primaries described in: https://www.itu.int/rec/R-REC-BT.709
   * - sRGB Encoded AP1
     - srgb_ap1_scene
     - See primaries described in: https://docs.acescentral.com/specifications/acescg/
   * - Gamma Encoded 2.2 AP1
     - g22_ap1_scene
     - See primaries described in: https://docs.acescentral.com/specifications/acescg/
   * - sRGB Encoded P3-D65
     - srgb_p3d65_scene
     - | This is a scene-referred version of Apple's Display P3 color space, and
       | is the same as Linear P3-D65, but sRGB encoded
   * - Gamma 2.2 Encoded AdobeRGB
     - g22_adobergb_scene
     - See ICC profile here: https://www.adobe.com/digitalimag/adobergb.html          
   * - Data
     - data
     - | This designation indicates that the asset it describes is actually not
       | color data (e.g. an image file format being used to represent a normal
       | or volume map, etc.), and that no color conversion of the data should be 
       | performed.          
   * - Unknown
     - unknown
     - | This designation indicates that the color space is not known (e.g. an
       | image file was created in a tool that did not save the color space
       | information, or provided incorrect color space information).

For backwards compatibility with older assets, OpenUSD also provides "Raw" (raw)
and "Identity" (identity) color space designations that are equivalent to 
:ref:`"Data" <color_space_table>` and 
:ref:`"Unknown" <color_space_table>` designations.

See `Color Space Encodings for Texture Assets and CG Rendering <https://github.com/AcademySoftwareFoundation/ColorInterop/blob/main/Recommendations/01_TextureAssetColorSpaces/TextureAssetColorSpaces.md#summary-table--overview-of-the-recommendations>`__
for additional details on these color spaces.

.. _color_working_with_color_space_schemas:

Working With Color Space Schemas
================================

OpenUSD provides the **ColorSpaceAPI** schema for specifying source color spaces 
for color attributes or external color-containing assets.

To specify the source color space, apply the ColorSpaceAPI schema
and set the :usda:`colorSpace:name` attribute to the appropriate color space 
token. The :usda:`colorSpace:name` attribute is a uniform value applied to a 
prim.

The following example uses ColorSpaceAPI to set the source color space 
for a texture asset in a Shader prim.

.. code-block:: usda

    def Shader "usduvtexture1"
    (
        prepend apiSchemas = ["ColorSpaceAPI"]
    )
    {
        uniform token info:id = "UsdUVTexture"
        asset inputs:file = @./assetTexture.png@

        # Specify the source color space for the texture
        uniform token colorSpace:name = "srgb_p3d65_scene"

        # ... other shader attributes omitted ...
    }

This will set the source color space for all color attributes and 
color-containing assets in the prim. If you need to provide finer-grain details
with source color spaces for specific attributes, set the :usda:`colorSpace` 
metadata for the attribute.

.. code-block:: usda

    def Material "NewMaterial"
    (
        prepend apiSchemas = ["ColorSpaceAPI"]
    )
    {
        uniform token colorSpace:name = "srgb_p3d65_scene"

        # diffuseColor needs to specify a different color space
        color3f inputs:diffuseColor = (0.2, 0.5, 0.8) (
            colorSpace = "srgb_rec709_scene"
        )
        # ...
    }   

You can also use ColorSpaceAPI to query and set color spaces programmatically.
For example, to query the color space for the :usda:`inputs:diffuseColor` 
attribute in the "NewMaterial" example above, you might use Python code similar 
to the following:

.. code-block:: python

    prim = stage.GetPrimAtPath("/path/to/NewMaterial")
    attr = prim.GetAttribute("inputs:diffuseColor")
    # Compute color space for attribute, no cache
    colorSpaceName = Usd.ColorSpaceAPI.ComputeColorSpaceName(attr, None)
    # colorSpaceName will be srgb_rec709_scene

OpenUSD also provides the **ColorSpaceDefinitionAPI** multiple-apply schema for 
defining custom color spaces. See the 
`Color Programmer's Guide <../api/_page__color__programmers__guide.html>`__
for programmatic examples of using this schema to define custom color spaces,
and additional programmatic examples of using ColorSpaceAPI.

.. _color_space_inheritance:

Color Space Inheritance and Resolution
--------------------------------------

In OpenUSD, color spaces can be specified at any level in the scene hierarchy 
and are inherited by child prims. This hierarchical approach allows for:

- Setting a global color space at the root.
- Providing a color space for a subgraph of the scene.
- Specifying a color space for attributes on individual prims.
- Authoring the color on individual attributes.

OpenUSD uses this hierarchical approach when determining the color space for a
value. The specific resolution order is:

1. Check if the attribute has an explicitly assigned color space.
2. Check if the attribute's prim has the ColorSpaceAPI schema applied.
3. Search up the hierarchy until a prim with ColorSpaceAPI is found.
4. If no color space is found, an empty token is returned. In this case, the 
   color space must be assumed to be the 
   :ref:`default color space <color_default_color_space>`.

The following example illustrates how color space information is propagated and
resolved. The "Root" prim has the ColorSpaceAPI schema applied and specifies a 
color space of lin_rec709_scene. All child prims of "Root" will inherit 
lin_rec709_scene as their color space unless explicitly overridden. Three
child prims are defined, with color space resolution as follows:

- The "Material1" child prim overrides the color space from its parent. All
  attributes of "Material1" will have srgb_p3d65_scene as their color space.
- The "Material2" child prim does not provide an opinion for 
  :usda:`colorSpace:name` and therefore will inherit the lin_rec709_scene color 
  space from "Root". Note that because "Material2" also does not have the 
  ColorSpaceAPI schema applied, if it did provide an opinion for 
  :usda:`colorSpace:name`, this would *not* be used in color space resolution.
- The "Material3" child prim also does not provide an opinion for 
  :usda:`colorSpace:name` and therefore will inherit the lin_rec709_scene color 
  space from "Root". However, the :usda:`inputs:diffuseColor` attribute *does*
  explicitly define a color space of srgb_rec709_scene, which will be used as
  the source color space for that attribute only.

.. code-block:: usda

    def Xform "Root" (
        prepend apiSchemas = ["ColorSpaceAPI"]
    )
    {
        uniform token colorSpace:name = "lin_rec709_scene"

        def Material "Material1"
        (
            prepend apiSchemas = ["ColorSpaceAPI"]
        )
        {
            # Material1 overrides Root's color space with srgb_p3d65_scene
            uniform token colorSpace:name = "srgb_p3d65_scene"

            color3f inputs:diffuseColor = (0.2, 0.5, 0.8) 
        }

        def Material "Material2"
        {
            # Material2 inherits Root's lin_rec709_scene color space
            color3f inputs:diffuseColor = (0.2, 0.5, 0.8) 
        }
        
        def Material "Material3"
        {
            # Even though Material3 inherits lin_rec709_scene from Root,
            # the diffuseColor attribute specifically uses srgb_rec709_scene
            color3f inputs:diffuseColor = (0.2, 0.5, 0.8) (
                colorSpace = "srgb_rec709_scene"
            )
        }
    }

.. _color_default_color_space:

Default Color Space
-------------------

OpenUSD uses a default color space of Linear Rec.709. This color space is used
for the source color space for prims and attributes when color space resolution
takes place (as described in :ref:`color_space_inheritance`) and no authored 
color space is found. 

.. _color_what_is_a_color_space:

**********************
What is a Color Space?
**********************

A color space is a specific organization of colors that provides a common 
reference for interpreting color values. Each color space defines:

1. **Primaries**: The fundamental RGB colors that, when combined, can reproduce 
   a specific gamut (range) of colors. In general primaries represent a weighted 
   combination of wavelengths. So-called spectral primaries are special in that 
   they give full weighting to single wavelengths.
2. **White Point**: A reference point for an assumed color source. Daylight 
   illumination is often referred to as D65, and incandescent as D50. It's 
   possible to match colors between images under different illuminants if we 
   recognize the whitepoint appropriate to an image.
3. **Transfer Function**: The relationship between numerical values in an 
   image and light intensity. This term is often referred to as gamma, or an 
   sRGB transfer function. The terms EOTF and OETF are often invoked; one refers 
   to electro-optical transfer (turning numeric values into light), and the 
   other refers to opto-electrical (turning light into numeric values).

A color space establishes the context needed to properly interpret RGB values. 
Without knowing the color space, an RGB value like ``(1.0, 0.0, 0.0)`` could 
represent any number of different actual colors.

.. _color_gamut_limitations:

Gamut Limitations and Considerations
====================================

No color space can represent all possible colors:

1. **Gamut clipping**: Colors outside the target gamut are often clipped to the 
   nearest reproducible color
2. **Gamut mapping**: More sophisticated approaches to handling out-of-gamut 
   colors
3. **Wide gamut spaces**: Modern color spaces like ACES can represent a much 
   wider range of colors
4. **Imaginary colors**: Sometimes intermediate results in calculations are 
   outside the range of perceptible values.

.. _color_common_white_points:

Common White Points
===================

These reference illuminants determine the overall color cast of an image:

- **D65**: Daylight at 6500K, the standard for most modern color spaces.
- **D60**: The somewhere in the middle illuminant used by ACES.
- **D55**: Mid-morning/afternoon daylight at 5500K.
- **D50**: Used in print industries.
- **E**: Equal energy white point, used in theoretical settings, but not in 
  practice.

Others might be encountered, but they are typically used as calibration 
references. See `Standard Illuminant <http://en.wikipedia.org/wiki/Standard_illuminant>`__
for additional details on reference illuminants.

.. _color_linear_vs_nonlinear:

Linear vs. Non-Linear Spaces
============================

A fundamental concept in color management is the distinction between linear and 
non-linear color spaces:

- **Linear spaces** have a direct 1:1 relationship between values and light 
  intensity.
- **Non-linear spaces** involve a transfer function and are typically used to 
  bias precision to certain parts of a gamut.

Most computations involving light (like lighting calculations, compositing, or 
blending) should be performed in linear space because computations in the 
non-linear basis for the most part lack a physical basis, and the resulting
values can be nonsensical.

.. _color_considerations_in_content_creation:

**********************************
Considerations in Content Creation
**********************************

While specific UI elements vary, most USD-compatible applications provide ways 
to:

1. **Set a rendering color space** for your project
2. **View renders in different color spaces**
3. **Specify source color spaces for textures**
4. **Export USD with proper color space metadata**

When working with textures:

1. **Know your source color space**: Is your texture in sRGB, linear, or 
   something else?
2. **Tag textures appropriately**: Ensure your textures have the correct color 
   space metadata.
3. **Use linear textures for data**: Normal maps, roughness, and metallicity 
   should typically be in linear space. These textures should also specify 
   :ref:`"Data" <color_space_table>` as their source color space in the
   USD scene.
4. **Use appropriate color spaces for display textures**: Color and albedo maps 
   are often prepared by artists in a display referenced space, i.e., they 
   are painted to match what their monitors showed. These textures are typically 
   encoded as sRGB. Physically based workflows on the other hand typically 
   involve maps created in linear spaces.

When creating materials:

**Be aware of value ranges** -- some color spaces support values outside the 
0-1 range.

Most modern DCCs provide color management controls for their viewports:

1. **Rendering color space**: The color space used for internal calculations.
   In some tools and contexts, this might be referred to as "working color 
   space".
2. **View transform**: The transformation applied for display.
3. **Look modifications**: Creative adjustments on top of the technical color 
   transforms.

When rendering from your USD scenes:

1. Use the facilities your DCC or renderer provide to **choose the appropriate 
   output color space** based on your delivery requirements.
2. **Apply the correct display transform** for your target viewing conditions.
3. **Consider OCIO integration** when available for advanced color management.

Working across multiple applications adds complexity, but USD helps maintain 
color consistency:

1. **Establish a pipeline-wide color management policy**.
2. **Use OCIO configs** where possible to ensure consistent transforms.
3. **Test color transforms** between applications to verify consistency.
4. **Document your color spaces** so all artists understand the workflow.

When working with multiple artists:

1. **Standardize monitor calibration** across your team.
2. **Document color space expectations** for all deliverables.
3. **Include color space metadata** in all USD files. One recommended approach
   is to add a prim that applies the facility/show/production default color 
   space in a facility/show/production-wide layer, and reference that prim
   onto all asset root prims during asset setup.
4. **Create clear color management guidelines** for your project.

.. _color_glossary:

***********************
Glossary of Color Terms
***********************

**Chromaticity**: The quality of color regardless of luminance.

**Color gamut**: The range of colors that can be represented or displayed.

**Color space**: A specific organization of colors that provides a common 
reference.

**Display-referred**: Color values adapted for direct display on a particular 
device.

**Gamma**: A non-linear operation applied to encode or decode luminance values.

**Linear color space**: A color space where values are directly proportional to 
light energy.

**OCIO (OpenColorIO)**: An open-source color management solution used in visual 
effects and animation.

**Primaries**: The fundamental red, green, and blue components that define a 
color space.

**Scene-referred**: Color values that represent actual light in a scene before 
display adaptation.

**Transfer function**: The mathematical relationship between numeric color 
values and displayed brightness.

**White point**: The reference neutral point in a color space, typically defined 
as a specific color temperature.