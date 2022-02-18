===============================
Render Settings in USD Proposal
===============================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `UsdRender overview page <api/usd_render_page_front.html>`_.

Copyright |copy| 2019, Pixar Animation Studios,  *version 1.2*

.. contents:: :local:

Purpose and Scope
=================

As of release 19.05, USD schemas provide for encoding geometry, lights, cameras,
and shading suitable for interchange between DCC's, and direct consumption by
high-quality renderers. However, it takes more than a description of the scene
elements to produce a rendered image useful in a VFX pipeline - we also require
a set of instructions/configuration to the renderer to specify the camera(s)
through which it should render, and potentially numerous other settings to
configure the products the renderer must produce. While a user will often
select such options in a DCC, there is value in being able to record these
settings in USD, so that we can maintain a separation of concerns between DCC's
and renderers; further, because USD provides composition mechanisms for
constructing and varying prim configurations, there is intrinsic value in
pipeline complexity management to encoding "Render Settings" in USD, and
allowing them to travel with scenes.

The purpose of this document is to propose encoding and organization of Render
Settings in USD, such that a properly-encoded USD file contains all the
information necessary to invoke a render of the scene in a single "scene
configuration" resulting in one or more render products. Some of this will be
schemas; some will be namespace conventions (facilitated by schemas) aimed at
making it robust to organize Render Settings for fast and easy discovery and
composability.

We acknowledge that Render Settings are insufficient specification for producing
a VFX "final frame", which often requires multiple *r* *ender* *passes* that
each produce "elements" of the frame that are then *composited* together... and
performing all of this work efficiently often requires explicit *job*
*structuring* . These concerns are, however, beyond the scope of this document,
though we are likely to visit them in subsequent phases of an umbrella project
of "Rendering from USD". As a step in that direction, we expect that a given
render pass would produce a specific render product (and possibly more than
one).

Overall Design and Concerns
===========================

We concern ourselves with all of the inputs necessary to configure a renderer to
produce a specified *set* of outputs, since a single render can in general
produce multiple outputs. We organize these inputs into three different prim
schemas in USD, as part of a "UsdRender" module:

    * **RenderSettings** provides the "global renderer configuration" options,
      also identifying the camera(s) through which to render, and identifying
      custom outputs (RenderProducts).

    * **RenderVar** specifies a custom variable computed by the renderer and/or
      shaders, providing data that can be organized and formatted into an
      artifact called a RenderProduct.

    * **RenderProduct** creates one or more artifacts (typically files on a
      filesystem, but more generally specified by *output drivers* ) populated
      by a set of RenderVars. RenderProducts are generally (multi-channel)
      images, but can also be 3D encodings such as point-clouds or volumes. 
      Additionally, each RenderProduct can override most settings established on
      its owning RenderSettings.

We will examine the three schemas in more detail, and then explore surrounding
issues such as the organization of these prims in scenes, and workflow
considerations.

Concrete Schemas
================

The usdRender domain defines a set of schemas. They are closely informed by
precedent set by RenderMan and Katana.

.. code-block:: usda
   :caption: UsdRender Schemas

   class "RenderSettingsBase" (
       inherits = </Typed>
       doc = """Abstract base class that defines render settings that
       can be specified on either a RenderSettings prim or a RenderProduct 
       prim."""
       customData = {
            string className = "SettingsBase"
       }
   )
   {
       rel camera (
           doc = """The _camera_ relationship specifies the primary
           camera to use in a render.  It must target a UsdGeomCamera."""
       )
       uniform int2 resolution = (2048, 1080) (
           doc = """The image pixel resolution, corresponding to the
           camera's screen window."""
       )
       uniform float pixelAspectRatio = 1.0 (
           doc = """The aspect ratio (width/height) of image pixels..
           The default ratio 1.0 indicates square pixels."""
       )
       uniform token aspectRatioConformPolicy = "expandAperture" (
           doc = """Indicates the policy to use to resolve an aspect
           ratio mismatch between the camera aperture and image settings.
   
           This policy allows a standard render setting to do something
           reasonable given varying camera inputs.
   
           The camera aperture aspect ratio is determined by the
           aperture atributes on the UsdGeomCamera.
   
           The image aspect ratio is determined by the resolution and
           pixelAspectRatio attributes in the render settings.
   
           - "expandAperture": if necessary, expand the aperture to
             fit the image, exposing additional scene content
           - "cropAperture": if necessary, crop the aperture to fit
             the image, cropping scene content
           - "adjustApertureWidth": if necessary, adjust aperture width
             to make its aspect ratio match the image
           - "adjustApertureHeight": if necessary, adjust aperture height
             to make its aspect ratio match the image
           - "adjustPixelAspectRatio": compute pixelAspectRatio to
             make the image exactly cover the aperture; disregards
             existing attribute value of pixelAspectRatio
           """
           allowedTokens = ["expandAperture", "cropAperture", "adjustApertureWidth", "adjustApertureHeight", "adjustPixelAspectRatio"]
       )
       uniform float4 dataWindowNDC = (0.0, 0.0, 1.0, 1.0) (
           doc = """dataWindowNDC specifies the axis-aligned rectangular
           region in the adjusted aperture window within which the renderer
           should produce data.
   
           It is specified as (xmin, ymin, xmax, ymax) in normalized
           device coordinates, where the range 0 to 1 corresponds to the
           aperture.  (0,0) corresponds to the bottom-left
           corner and (1,1) corresponds to the upper-right corner.
   
           Specifying a window outside the unit square will produce
           overscan data. Specifying a window that does not cover the unit
           square will produce a cropped render.
   
           A pixel is included in the rendered result if the pixel
           center is contained by the data window.  This is consistent
           with standard rules used by polygon rasterization engines.
           \\ref UsdRenderRasterization
   
           The data window is expressed in NDC so that cropping and
           overscan may be resolution independent.  In interactive
           workflows, incremental cropping and resolution adjustment
           may be intermixed to isolate and examine parts of the scene.
           In compositing workflows, overscan may be used to support
           image post-processing kernels, and reduced-resolution proxy
           renders may be used for faster iteration.
   
           The dataWindow:ndc coordinate system references the
           aperture after any adjustments required by
           aspectRatioConformPolicy.
           """
       )
       uniform bool instantaneousShutter = false (
           doc = """Override the targeted _camera_'s _shutterClose_ to be
           equal to the value of its _shutterOpen_, to produce a zero-width
           shutter interval.  This gives us a convenient way to disable
           motion blur."""
       )
   }
    
   class RenderSettings "RenderSettings" (
       inherits = </RenderSettingsBase>
       doc = """A UsdRenderSettings prim specifies global settings for
       a render process, including an enumeration of the RenderProducts
       that should result, and the UsdGeomImageable purposes that should
       be rendered.  \\ref UsdRenderHowSettingsAffectRendering"""
       customData = {
            string className = "Settings"
            string extraIncludes = """
   #include "pxr/base/gf/frustum.h"
   """
       }
   )
   {
       rel products (
           doc = """The set of RenderProducts the render should produce.
           This relationship should target UsdRenderProduct prims.
           If no _products_ are specified, an application should produce
           an rgb image according to the RenderSettings configuration,
           to a default display or image name."""
       )
       uniform token[] includedPurposes = ["default", "render"] (
           doc = """The list of UsdGeomImageable _purpose_ values that
           should be included in the render.  Note this cannot be
           specified per-RenderProduct because it is a statement of
           which geometry is present."""
       )
       uniform token[] materialBindingPurposes = ["full", ""] (
           allowedTokens = ["full", "preview", ""]
           doc = """Ordered list of material purposes to consider when
           resolving material bindings in the scene.  The empty string
           indicates the "allPurpose" binding."""
       )
   }
   
   class "RenderSettingsAPI" (
       inherits = </APISchemaBase>
       customData = {
           token apiSchemaType = "singleApply"
           string className = "SettingsAPI"
       }
       doc = """UsdRenderSettingsAPI is a base class for API schemas
       to encode renderer-specific settings."""
   ) {
   }

   class RenderVar "RenderVar" (
       inherits = </Typed>
       doc = """A UsdRenderVar describes a custom data variable for
       a render to produce.  The prim describes the source of the data, which
       can be a shader output or an LPE (Light Path Expression), and also
       allows encoding of (generally renderer-specific) parameters that
       configure the renderer for computing the variable.
    
       \note In the future, UsdRender may standardize RenderVar representation
       for well-known variables under the sourceType `intrinsic`,
       such as _r_, _g_, _b_, _a_, _z_, or _id_.
       """
       customData = {
            string className = "Var"
       }
   ) {
       uniform token dataType = "color3f" (
           doc = """The type of this channel, as a USD attribute type."""
       )
       uniform string sourceName = "" (
           doc = """The renderer should look for an output of this name
           as the computed value for the RenderVar."""
       )
       uniform token sourceType = "raw" (
           doc = """Indicates the type of the source.
   
           - "raw": The name should be passed directly to the
             renderer.  This is the default behavior.
           - "primvar":  This source represents the name of a primvar.
             Some renderers may use this to ensure that the primvar
             is provided; other renderers may require that a suitable
             material network be provided, in which case this is simply
             an advisory setting.
           - "lpe":  Specifies a Light Path Expression in the
             [OSL Light Path Expressions language](https://github.com/imageworks/OpenShadingLanguage/wiki/OSL-Light-Path-Expressions) as the source for
             this RenderVar.  Some renderers may use extensions to
             that syntax, which will necessarily be non-portable.
           - "intrinsic":  This setting is currently unimplemented,
             but represents a future namespace for UsdRender to provide
             portable baseline RenderVars, such as camera depth, that
             may have varying implementations for each renderer.
           """
           allowedTokens = ["raw", "primvar", "lpe", "intrinsic"]
       )
   
       # XXX We propose that filtering and remapping parameters be added as 
       #     custom/dynamic properties in a "parameters:" namespace.  
       #     Renderer-specific parameters for RenderMan would go in the 
       #     namespace "parameters:ri:"
   }
    
   class RenderProduct "RenderProduct" (
       inherits = </RenderSettingsBase>
       doc = """A UsdRenderProduct describes an image or other
       file-like artifact produced by a render. A RenderProduct
       combines one or more RenderVars into a file or interactive
       buffer.  It also provides all the controls established in
       UsdRenderSettingsBase as optional overrides to whatever the
       owning UsdRenderSettings prim dictates.
   
       Specific renderers may support additional settings, such
       as a way to configure compression settings, filetype metadata,
       and so forth.  Such settings can be encoded using
       renderer-specific API schemas applied to the product prim.
       """
       customData = {
            string className = "Product"
            dictionary schemaTokens = {
               dictionary raster = {
                   string doc = """RenderProduct productType value that
                   indicates a 2D raster image of pixels."""
               }
            }
       }
   ) {
       uniform token productType = "raster" (
           doc = """The type of output to produce.
           The default, "raster", indicates a 2D image.
   
           \note In the future, UsdRender may define additional product
           types."""
       )
       uniform token productName = "" (
           doc = """Specifies the name that the output/display driver
           should give the product.  This is provided as-authored to the
           driver, whose responsibility it is to situate the product on a
           filesystem or other storage, in the desired location."""
       )
       rel orderedVars (
           doc = """Specifies the RenderVars that should be consumed and
           combined into the final product.  If ordering is relevant to the
           output driver, then the ordering of targets in this relationship
           provides the order to use."""
       )
   
   
       # XXX configuration parameters for the output driver should be created
       #     in the "driver:parameters:" namespace.
   }


Renderer-Specific Schemas
#########################

While we hope to capture the majority of useful render configuration in these
core schemas to enhance USD compliance between renderers, we understand that
each renderer will likely have some configuration unique to it. As with our
UsdLux lighting schemas, our expectation is that plugins that provide render
delegates will also include any required **additional** schemas, as applied API
schemas that can be added to RenderSettings and RenderProduct prims.

Prim and Scene Organization
===========================

Discovering Render Settings
###########################

We have found that some pipeline processes want to operate purely on
RenderSettings data, and it therefore is quite valuable to be able to load and
discover Settings rapidly, without paying costs proportional to the size of a
scene. We therefore propose to enforce a naming convention whereby **all
UsdRenderSettings prims must be located under the path </Render>** .By requiring
all render-related specification to be located under a unique root-level prim,
we gain several advantages:

    * We can leverage `UsdStage::OpenMasked()
      <api/class_usd_stage.html#ade1d90d759a085022ba76ff910815320>`_
      to compose *just* the rendering-related prims under </Render>, making the
      extraction of this data very fast, regardless of scene complexity.

    * This constrained organization facilitates sharing of rendering data within
      a production pipeline, as every "shot" can sublayer in sequence,
      production, and facility-level layers that each contain rendering data,
      and we are ensured consistent composition (and ease of discovery) of all
      the data.

This does, in one sense, divorce a scene from its rendering specification,
necessitating further management if you need to combine scenes. For example,
should you want to reference one scene into another, you will need to reference
in the root of the scene's geometry (</World> in Pixar's pipeline), but then the
scene's render specification gets left behind. The settings can be recaptured
by separately referencing the scene's </Render> prim onto the referencing
scene's </Render> prim, although some path corrections may be required. We
think these situations arise infrequently enough that they do not outweigh the
benefits of Render data always being in a known location. In practice, while we
do organize and compose together scene geometry, and we also organize and
compose together render settings, these are usually independent concerns that
are not often tightly coupled.

.. _rendersettings-selecting:

Selecting and Combining Render Settings
#######################################

All RenderSettings prims must live under </Render>, but we stipulate, for
simplicity, that any single render can consume only a **single** RenderSettings
prim. We propose a new piece of string-valued SdfLayer metadata,
:bi:`renderSettingsPrimPath` that identifies the full path to the active/current
RenderSettings prim, though we think it may also be useful for applications
(including API in usdImaging) to be able to specify an **override** to this
embedded-in-the-scene value, so that multiple, differently-configured renders
can happen simultaneously without mutating the UsdStage (which is not
threadsafe).

Artist workflows often involve juggling variations of common settings to quickly
iterate on images or to analyze renders. It is useful for common settings to
have well-known names, as bookmarks that are easy to refer to, and which can be
combined. An example is wanting a quick render by requesting a half-resolution
image, with displacement and motion blur disabled. Our opening position on
dynamically combining settings is that we would require the application that
wants to support it to express it in terms of USD composition arcs, i.e., given
an ordered list of paths-to-RenderSettings prims, the application would create a
**new** RenderSettings prim in the session layer, and reference the input
RenderSettings prims, in order. This keeps the schema and renderer-interface
simpler, though does have several consequences worth mentioning:

    * Such "just in time" authoring will cause recomposition; however, it will
      be tightly scoped to the single, newly introduced prim, and therefore
      should not be expensive.

    * But because combining *does* require scene authoring, we cannot perform it
      for two different renders simultaneously from the same stage - a single
      thread would need to prep RenderSettings for both renders before splitting
      for the actual renders.

    * If combining arbitrary RenderSettings prims, the specific "list operators"
      used in specifying the *products* relationship become more important - for
      example if two RenderSettings each create an **explicit** list of
      products, then combining the two prims by reference will select only the
      products of the stronger, whereas if they each **prepend** or **append**
      products, they will combine reasonably.

Grouping RenderVars and RenderProducts
######################################

Because all RenderProducts and RenderVars are identified by relationship, they
can robustly live anywhere within the </Render> tree. We leave it as a pipeline
decision as to whether further organization is worthwhile (e.g. all RenderVars
live under </Render/Vars>, all RenderSettings under </Render/Settings>, etc),
and stipulate applications should make no assumptions about where under
</Render> any of the Render prims should live.

There *may* be value, in some pipelines, to consider RenderProducts to **belong
to a given RenderSettings prim** ; therefore, if we were to require them to be
grouped under a RenderSettings prim, then if we use USD composition to combine
RenderSettings prims, as suggested in the previous section, then we get the
desirable property that the RenderProducts come along, uniquely, as well, which
means that their member properties can be overridden uniquely for the
combined/composed result RenderSettings prim.

Common RenderVars
*****************

Although not a part of this proposal, we may provide derived RenderVar types for
common types such as P, N, Nn, Z, so that no configuration is required to
consume these basic render variables.

Interpretation of RenderVar Prims in Hydra
******************************************

For the majority of prim types in USD schemas, there will be one or more rprim,
sprim, or bprim objects created in Hydra, associated with the scene prim's path,
and at first one might assume here would be a single bprim (buffer prim)
associated with each RenderVar prim in a UsdStage. However, because we have
opted to allow RenderVars to be shared among multiple active RenderProduct
prims, this association does not hold. A RenderVar targeted by two different
RenderProducts (which may each have different cameras and resolutions) serves
only as a prescriptive input to the RenderProduct to create bprim buffers unique
to itself, with path identifiers scoped to the RenderProduct.

Discovering All Potential Shader-based RenderVars
#################################################

For fast discoverablilty by render-prepping software, we have stipulated that
the RenderVars **consumed by the configured RenderSettings** must be located
under the root-level </Render> location. However, typically, *available* shader
outputs will be published within assets, which may be scattered throughout large
scenes.  We do not consider it practical/robust to require that </Render/Vars>
(or however we choose to organize RenderVars) be perpetually kept in sync as a
complete manifest of the RenderVar contents of the entire scene. Therefore, any
artist or process that needs to browse the *available* RenderVars for a scene is
faced with a more expensive discovery process.

Pixar's experience is that so much pipeline setup is required to expose a shader
output for useful consumption that we typically only have a handful that are
well-known to artists, and therefore a discovery process is not needed. If
other studios have different experience, we can consider further schema to aid
in discovery, for example requiring available shader outputs to be identified at
the UsdShadeMaterial-level, so that traversing shader networks is not required.

Workflow Considerations
=======================

Interactive vs. Batch Rendering
###############################

UsdRender is primarily aimed at describing a batch render process which produces
image files in a filesystem.

However, it does not describe the broader context about how those files are
managed within the filesystem or in a distributed computing environment. The
output files are always assumed, for present discussion purposes, to be "local"
to a renderer's working directory.

It is a later task to describe the broader details of batch processing of images
and how the produced images are shipped around a network.

Similarly, we assume an interactive rendering system will, where sensible, be
able to redirect rendering results to in-memory buffers or viewports as needed.

Examples
========

.. code-block:: usda
   :caption: TestPrims_px

   def Scope "Render"
   {
       def RenderSettings "PrimarySettings" {
           rel products = </Render/PrimaryProduct>
           int2 resolution = (512, 512)
       }
       def RenderProduct "PrimaryProduct" {
           rel camera = </World/main_cam>
           token productName = "/scratch/tmp/render000009.exr"
           rel orderedVars = [
               </Render/Vars/Ci>,
               </Render/Vars/a>,
               </Render/Vars/id>,
               </Render/Vars/id2>
           ]
       }
       def Scope "Vars"
       {
          def RenderVar "Ci" {
               string sourceName = "Ci"
           }
           def RenderVar "a" {
               token dataType = "float"
               string sourceName = "a"
           }
           def RenderVar "emission" {
               string sourceType = "lpe"
               string sourceName = "C[<L.>O]"
           }
           def RenderVar "directDiffuse" {
               string sourceType = "lpe"
               string sourceName = "C<RD>[<L.>O]"
           }
           def RenderVar "indirectDiffuse" {
               string sourceType = "lpe"
               string sourceName = "C<RD>[DS]+[<L.>O]"
           }
           def RenderVar "subsurface" {
               string sourceType = "lpe"
               string sourceName = "C<TD>[DS]*[<L.>O]"
           }
           def RenderVar "directSpecular" {
               string sourceType = "lpe"
               string sourceName = "C<RS>[<L.>O]"
           }
           def RenderVar "indirectSpecular" {
               string sourceType = "lpe"
               string sourceName = "C<RS>[DS]+[<L.>O]"
           }
           def RenderVar "transmissive" {
               string sourceType = "lpe"
               string sourceName = "C<TS>[DS]*[<L.>O]"
           }
          def RenderVar "id" {
               token dataType = "int"
               string sourceName = "id"
               string parameters:ri:filter = "zmin"
           }
           def RenderVar "id2" {
               token dataType = "int"
               string sourceName = "id2"
               string parameters:ri:filter = "zmin"
           }
       }
   }

Discussion and Questions
========================

ID variables
############

ID passes/variables in renders are quite common and enormously useful. However,
we do not seem to yet have standardized way to describe them, and therefore they
tend to be renderer and pipeline specific. If we can standardize on a
description and encoding, we might be able to tackle the problem of providing a
standard RenderVar for ID information. There are a few issues to consider:

    * how to resolve multiple samples per pixel; could use deep images, or could
      only return closest value

    * how to map values in the ID AOV image to objects in the scene; whether
      image metadata is needed to store the mapping

    * how to handle instancing (including nested instancing)

    * how to handle ID's in the presence of participating media and transparent
      surfaces (glass, water)

Cryptomatte offers a compelling option for several of these issues.

Stereo Rendering
################

As long as the eyes are not allowed to see different things (i.e. no differences
in visibility), then stereo output from a single render can be encoded as two
RenderProducts each specifying different cameras.

Camera Exposure Curves
######################

Marquee renderers often allow clients to control the exposure of an image
between the camera's *shutterOpen* and *shutterClose* typically with a curve. 
Since these controls currently tend to be highly renderer-specific, we expect
them to be encoded in applied API schemas, either on a UsdGeomCamera prim (since
that is where shutter information already lives), or on a RenderSettings prim.

Denoising, Color Correction, and Tasks
######################################

We stipulated in the introduction that we are not, at this time, addressing the
needs of compositing with these schemas. However, such "filtering" tasks as
denoising and color correction seem both smaller and more requisite for
producing a usable element than general compositing. Theoretically, one could
encode these behaviors in a RenderProduct's *driver:parameters* , but that does
not represent an interchangeable solution. Addressing these needs is a likely
next step in the evolution of these schemas.

.. _rendersettings-locality:

Why Locality of Overrides is Valuable
#####################################

One goal of this schema proposal is that a USD scene be able to contain
multiple, alternative collections of settings from which different renders can
be initiated without requiring any stage mutations. A second goal, as discussed
in `Selecting and Combining Render Settings <#rendersettings-selecting>`_ , is 
to make it easy to produce variations and combinations of Settings through
composition (e.g. in a stage's session layer). Both of these goals are
facilitated by keeping all required overrides for different configurations **on
render prims** , because requiring overrides on renderable scene objects (even
cameras) limits the number of differing render configurations we can encode in a
scene.

Image-Mapping-Related Options on Camera or RenderSettings?
##########################################################

Several observers have wondered whether concerns such as screen window
conformance, and possibly even image resolution might better be expressed as
concerns of the UsdGeomCamera prim. This would mean we would use regular USD
overrides *on camera prims* to specializeconformance properties. If we want to
preserve `the desirable locality of overrides for RenderSettings
<#rendersettings-locality>`_ , we would create a new Camera prim (under the 
RenderSettings or RenderProduct that uses it) that references the "original" 
camera, thus preserving the ability to combine RenderSettings themselves via 
composition arcs.

Doing so, however, creates confusion for scene consumers, because we have
essentially duplicated any cameras used in rendering. Which should a client
import? How should a client manage overrides on the original vs duplicated
cameras inside a DCC when exporting the results to USD? Further, we believe
there actually :bi:`is` **a logical separation between camera and imaging
concerns.**

    * **A camera** defines a view frustum, describes lens properties and
      exposure, and (for cameras used for rendering) are in the purview of a
      layout/photography department under a director of photography.

    * **Producing an image as viewed through a camera** requires further
      specification of image resolution, and how to map the screen window of the
      camera's view frustum *to* that image. These are generally concerns of
      rendering and/or lighting departments, *and* need to be varied for
      different renders, whereas the cameras concerns, as outlined here, do not.

We conclude that image-related properties, therefore, best reside in
RenderSettings, not on Camera prims. The single concession for usability we
make to "overlapping concerns" is allowing a RenderSetings prim to override a
camera's exposure to be instantaneous, as this is a very common per-render
adjustment.

Crop Windows and Region-of-Interest
###################################

Some rendering toolsets provide separate notions of a crop window and a
region-of-interest. The former is a pipeline- or rendering-centric way to
configure the renderer, and the latter is an interactive user affordance to
temporarily steer render compute resources to a subset of the image. Given that
they have the same effect of truncating the computed data window, and the
cumulative effect is just to take the the intersection of these restricted
regions, we have decided with UsdRender to only provide a single cropWindow
attribute and leave any interactive region-of-interest concerns as a
toolset-side concern. Specifically, we expect an interactive rendering tool
could add a Session-layer override to the crop window to intersect any further
region-of-interest known to the application.

We intend UsdRender to provide utilities to support this and related
calculations.

