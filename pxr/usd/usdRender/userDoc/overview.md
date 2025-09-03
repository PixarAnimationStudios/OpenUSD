(usdRender_overview)=
# Overview

While OpenUSD can be used to describe many kinds of data, its primary and 
most-used domain is describing 3D scenes, and visualizing or rendering those 
scenes is a key process. Interactive "viewport" or browser renders can often 
give a reasonable approximation of the scene's content with minimal instruction 
to a renderer. But for producing "film quality" or "final quality" renders of 
complicated scenes with large numbers of interacting elements, special 
geometric, volumetric, and shading effects, studios routinely need to provide 
much more detailed instructions to a "final quality" renderer. Such instructions
might include complex configuration needed to decompose the scene for 
a number of different "render passes" that each produce outputs that are then 
composited, and post-processed into a "final frame". 

UsdRender provides schemas for specifying these instructions such that they can 
be delivered to renderers in a standardized way. Renderers are responsible for 
applying your render configurations to the final render to the best of their 
ability. Renderers that support OpenUSD should have reasonable defaults that 
will be applied if render configuration isn't authored in your scene.

UsdRender provides the following schemas.

- **RenderSettings**: Encapsulates all the settings and components that tell 
a renderer what render settings to use, and what render output to produce (via
specified RenderProducts), for a single invocation of rendering the scene.

- **RenderProduct**: Represents a single render output artifact, such as a 
rendered image file or an output depth buffer. Includes render output 
information (e.g. artifact type and name) and which output channels (via
specified RenderVars) should be incorporated into the artifact.

- **RenderVar**: Represents a quantity or "channel" of computed output data that 
can vary across an output artifact, sometimes referred to in other rendering
pipelines as an arbitrary output variable (AOV).

- **RenderPass**: Represents the renderer and scene configuration for a single 
render pass in a multi-pass rendering workflow. For example, you might have a 
workflow that uses separate render passes to render the foreground and 
background portions of a scene, and a third pass that composites the foreground 
and background render output into the final frame. 

(usdRender_best_practices)=
## Best Practices 

The following are some general best practices for using UsdRender schemas.

(usdRender_which_schemas_to_use)=
### Understand Which Schemas to Use

When configuring how to render your scene, it's important to understand which
UsdRender schemas you will need to use. Some schemas rely on other schemas to
fully describe the desired configuration. For example, to fully configure the 
final render output artifact, you need to provide a RenderProduct, and also 
provide a set of RenderVars that the RenderProduct uses.

Additionally, some schemas might not be needed for your rendering workflow. 
For example, if your render pipeline does not use multi-pass rendering, you 
might not need to utilize RenderPass.

When planning out your use of UsdRender schemas, here are some things to keep
in mind.

- Always provide a 'default' RenderSettings. This RenderSettings prim will act
as the default global render configuration for your scene. See 
{ref}`usdRender_default_rendersettings` for how to set the default 
RenderSettings path in layer metadata.

- Determine if you will need to use a multi-pass rendering workflow or not. This
will determine if you'll need to define RenderPass prims. Many animation 
production workflows benefit from the added flexibility multi-pass workflows 
provide, or may require multi-pass rendering to apply certain effects. However, 
multi-pass workflows increase the complexity of the render process and may also 
increase storage requirements for the output from multiple passes. See 
{ref}`usdRender_configuring_multipass_renders` for more details on using 
RenderPass.

- Renderers that support OpenUSD should produce a default output artifact
(typically an RGB image) for a renderer invocation if no RenderProduct is
specified. Renderers should also provide API schemas that apply to and extend 
UsdRender schemas such that a single default RenderSettings can be usefully 
consumed by any renderer. However, you may need to override configuration 
specified in your RenderSettings for a particular output, or specify output 
channels (color, normal, alpha, etc.) for your output, or even specify 
renderer-specific configuration for the output (special output compression 
settings, etc.). If you need this level of output configuration, you'll need to 
provide RenderProduct and RenderVar prims with the appropriate configuration 
values. See {ref}`usdRender_working_with_render_product` and 
{ref}`usdRender_understanding_rendervars` for more details on using 
RenderProduct and RenderVar.

(usdRender_group_render_prims)=
### Group UsdRender Prims

As a general best practice, group all your UsdRender prims (RenderSettings, 
RenderProduct, RenderVar, RenderPass) in a scene under a common root prim named 
"Render". This encapsulates render-related specifications from the rest of your 
scene data so that, even in large scenes, render specifications can be accessed 
efficiently using features like UsdStage masking. The following example has two 
RenderSettings and a RenderProduct grouped under a Scope "Render" prim.

```{code-block} usda
def Scope "Render"
{
    def RenderSettings "PrimarySettings" {
        rel products = </Render/PrimaryProduct>
        int2 resolution = (512, 512)
    }
    def RenderSettings "PrimarySettingsRaw" {
        rel products = </Render/PrimaryProduct>
        int2 resolution = (1024, 1024)
        uniform token renderingColorSpace = "raw"
    }
    def RenderProduct "PrimaryProduct" {
        token productName = "/scratch/tmp/render000009.exr"
    }
}
```

(usdRender_default_rendersettings)=
### Provide a Default RenderSettings 

It's possible to have multiple RenderSettings prims in a given stage. You 
might have a stage that needs to provide separate RenderSettings for renders
for different purposes (preview, final, etc.). As RenderSettings provide the 
global render configuration for a scene, it's important to clarify what the 
"default" RenderSettings is.

Communicate to a renderer what the default RenderSettings is by using the 
`renderSettingsPrimPath` metadata in your stage's root layer, providing a 
path to a RenderSettings prim. The following example sets the default 
RenderSettings to "/Render/PrimarySettings".

```{code-block} usda
#usda 1.0
(
    renderSettingsPrimPath = "/Render/PrimarySettings"
)
```

(usdRender_designate_camera)=
### Designate a Render Camera

RenderProduct and RenderSettings can designate the 
{ref}`render camera <render_camera>` used for rendering via setting the 
`camera` relationship to a Camera prim. The Camera prim determines the 
visual composition of the scene as an image, and represents creative choices 
distinct from the technical render settings used to configure image generation. 
This is why some attributes originate from the camera and others (such as pixel 
resolution) are expressed separately as render settings, and may vary per 
RenderProduct.

The following example has a "PrimarySettings" RenderSetting with 
<span class="mono sdfpath">/World/main_cam</span> configured as the render 
camera, and a second RenderSettings, "StereoSettings" (also using 
<span class="mono sdfpath">/World/main_cam</span>). Additionally, one of the 
RenderProducts ("StereoProductLeft") that "StereoSettings" includes overrides 
the render camera from "StereoSettings" to use 
<span class="mono sdfpath">/World/stereo_cam/cam_left</span> just for that 
product output.

```{code-block} usda
def Scope "Render" {
    def RenderSettings "PrimarySettings" {
        rel camera = </World/main_cam>
        # ... other settings ...
    }
    def RenderSettings "StereoSettings" {
        # stereo render products will override camera as needed
        rel camera = </World/main_cam>
        rel products = [ </Render/StereoProductRight>, </Render/StereoProductLeft> ]
        # ... other settings ...
    }

    def RenderProduct "StereoProductRight" {
        rel camera = </World/stereo_cam/cam_right>
        # ... other settings ....
    }
    def RenderProduct "StereoProductLeft" {
        rel camera = </World/stereo_cam/cam_left>
        # ... other settings ....
    }
}
```

(usdRender_controlling_rendered_prims)=
## Using UsdRender to Control How Prims Are Rendered

UsdRender schemas provide several properties to control which prims get 
rendered for a render invocation or a render pass, as well as properties to
control how prims get rendered, e.g. controlling which material bindings are
used during a render.

(usdRender_using_imageable_purpose)=
### Configuring Imageable Purpose

USD provides several ways to {ref}`control prim visibility <render_visibility>` 
in a scene. One approach specifically designed to be used at render-time is
{ref}`imageable purpose <render_purpose>`, which lets you author a 
`purpose` attribute on an imageable prim, and then configure which 
purposes are used at render-time.

To configure which purposes to use for a render, use the 
`includedPurposes` attribute on RenderSettings to list the purposes that
will be rendered.

```{note}
Prims that have no authored or inherited purpose (i.e. the prim has the 
fallback "default" purpose) are always rendered. Renderers are expected to 
render default purpose prims for all render situations.
```

The following example has `includedPurposes` set to `["proxy", "render"]`, 
so only prims with `purpose` set to "proxy" or "render", or prims that 
have {ref}`"inherited" <render_purpose_inheritance>` either of those imageable 
purposes, will be rendered. Note that prims with no authored or inherited 
purpose (i.e. the "default" fallback purpose) will always be rendered.

```{code-block} usda
def RenderSettings "PrimarySettings" {
    uniform token[] includedPurposes = ["proxy", "render"]
}
```

(usdRender_using_materialbindingpurposes)=
### Configuring materialBindingPurposes

Material binding purposes let you specify, for the same Gprim, different
material bindings for different purposes, e.g. a material binding for a
"full" render, vs a lightweight material binding for a "preview" render.
See {ref}`material_binding_purpose` for more details. 

Configure which material binding purposes are used for a render via the
`materialBindingPurposes` RenderSettings attribute. For example, you might 
have separate RenderSettings for a final render, using the "full" purpose, and 
for your DCC tool, using the "preview" purpose.

```{code-block} usda
def RenderSettings "FinalRenderSettings"
{
    uniform token[] materialBindingPurposes = ["full"]
    ...
}
def RenderSettings "DefaultRenderSettings"
{
    uniform token[] materialBindingPurposes = ["preview"]
    ...
}
```

In most scenarios, `materialBindingPurposes` should be set to a single 
value. Note that if the specified material binding purpose is not found, 
renderers should attempt to fall back to an all purpose binding, if present.

(usdRender_using_renderpass_collections)=
### Using RenderPass Collections

If you are using a multi-pass rendering workflow, RenderPass provides additional
properties to control how prims are rendered through the 
`renderVisibility`, `cameraVisibility`, `prune`, and 
`matte` collections. 

- `renderVisibility`: This collection describes which prims are visible to 
the renderer for this pass. Use this collection if you have separate passes for 
different sets of objects in the stage (e.g. separate foreground and background 
passes), or passes that only apply to specific types of objects. Note that 
this collection acts as a "mask" and does not override authored 
`visibility` values -- for example, if an imageable prim in the scene 
has (or inherits) ``token visibility = "invisible"``, including this prim in 
this collection will not cause the prim to be visible.
- `cameraVisibility`: Similar to `renderVisibility`, however this 
collection describes which prims are visible to the main render camera (set
in RenderSettings or RenderProduct) for this pass. Note that objects that are 
not in this collection should still participate in other light paths such as 
shadowing, reflections, and refraction. Similar to `renderVisibility`, 
this collection does not override authored `Imageable.visibility` values. Also,
note that this collection is applied after `renderVisibility`, so prims in this 
list, but *not* in `renderVisibility` will not be visible to the renderer.
- `prune`: This collection defines which prims should be *removed* during
this render pass. This is provided as an alternative to collections like 
`renderVisibility` and `cameraVisibility`, to support scenarios such as batch 
rendering, or when renderers can't support visibility toggles. The tradeoff is 
that pruning is likely to be a more expensive operation than visibility toggling
and therefore not appropriate for certain renderer scenarios, such as renders 
for interactive workflows.
- `matte`: This collection describes which prims should be rendered as
"matte objects" (objects with zero alpha) during this render pass. Note that 
this list is applied after `renderVisibility`, so prims in this list, but 
*not* in `renderVisibility` will not be visible to the renderer, and therefore
not rendered as matte.

Note that all of these collections are 
{ref}`USD collections <collections_and_patterns>` and therefore can be defined 
using relationship-mode or pattern-based collections. The following example 
defines two RenderPasses, a "foreground" pass that uses a relationship-mode 
collection for `renderVisibility`, and a "background" pass that uses a 
pattern-based collection for `renderVisibility`.

```{code-block} usda
def RenderPass "foreground" 
{
    rel renderSource = </Render/PrimarySettings>
    uniform bool collection:renderVisibility:includeRoot = false
    prepend rel collection:renderVisibility:includes = [
        </World/characters>,
        </World/sets/Kitchen/Table_grp>,
    ]
}
def RenderPass "background" 
{
    rel renderSource = </Render/PrimarySettings>
    pathExpression collection:renderVisibility:membershipExpression = "/World/Background* + /World/OLD_Background*"
}
```

(usdRender_working_with_render_product)=
## Working with RenderProduct

A RenderProduct represents a single render output artifact, such as a rendered 
image file or an output depth buffer. RenderProducts can override some of the 
configuration in a RenderSetting (such as the 
{ref}`camera <usdRender_designate_camera>`), but also have product-specific 
settings, such as the output "product name" (for a rendered image, the image 
filename).

The following example RenderProduct configures `productType` and 
`productName`, but also overrides any authored RenderSettings values
in use for `camera` and `resolution`.

```{code-block} usda
def RenderProduct "PrimaryProduct" {

    # --- product-specific ---
    uniform token productType = "deepRaster"
    uniform token productName = "/output/render000009.png"

    # --- overrides ---

    # For this product, override our RenderSettings camera and resolution 
    rel camera = </Cameras/stereo_cam>
    uniform int2 resolution = (3840, 2160)

    # ...other settings...
}
```

```{note}
Overriding settings such as `camera` and `resolution` may, for some 
renderers, necessitate multiple independent renders. Other renderers may be 
able to vary these settings over RenderProducts computed in a single render.
```

A RenderProduct must also specify one or more RenderVars, each of which 
represent a channel of computed output data, as described in 
{ref}`usdRender_understanding_rendervars` below.

(usdRender_understanding_rendervars)=
### Understanding RenderVars

A RenderVar represents a quantity or "channel" of computed output data that can 
vary across an output artifact. A RenderProduct may contain multiple channels of 
data representing related values or variables sampled by a render process. The 
RenderVar prim specifies what values the renderer should output and how the 
renderer should produce them. 

Examples of render variables include geometric measurements such as 
camera-space depth, quantities emitted by material shaders, light path 
expressions (LPE's), and quantities intrinsic to the renderer such as 
computation time per pixel. Note that USD does not yet enforce a set of 
universal RenderVar names and formats, so renderer-specific RenderVars are 
expected. In the following example, the "PrimaryProduct" RenderProduct 
specifies four RenderVars representing channels for color, alpha, directDiffuse, 
and a general ID value.

```{code-block} usda
    def RenderProduct "PrimaryProduct" {
        rel camera = </World/main_cam>
        token productName = "/scratch/tmp/render000009.exr"
        rel orderedVars = [
            </Render/Vars/color>,
            </Render/Vars/alpha>,
            </Render/Vars/directDiffuse>,
            </Render/Vars/id>
        ]
    }
    def Scope "Vars"
    {
        def RenderVar "color" {
            string sourceName = "Ci"
        }
        def RenderVar "alpha" {
            token dataType = "float"
            string sourceName = "a"
        }
        def RenderVar "directDiffuse" {
            string sourceName = "C<RD>[<L.>O]"
            token sourceType = "lpe"
        }
        def RenderVar "id" {
            token dataType = "int"
            string sourceName = "id"
        }
    }
```

(usdRender_configuring_multipass_renders)=
## Configuring Multi-pass Renders with RenderPass

Use RenderPass prims to configure multi-pass renders. RenderPass prims 
encapsulate individual passes in a multi-pass rendering workflow, letting you 
specify a different RenderSetting for each render pass. For example, you might 
have a workflow that uses separate render passes and settings to render the 
foreground and background portions of a scene, and a third pass that composites 
the foreground and background render output into the final frame. RenderPass can 
point to a RenderSettings for render configuration for the pass, or point to 
product-specific configuration for external renderers that may not describe a 
render in terms of RenderSettings (e.g., compositing applications). In addition 
to organizing the different renders and processes (such as denoising and 
compositing) that collectively produce a "final frame", RenderPass codifies the 
dependencies between passes. A single pass generally represents not just a 
single set of products, but a sequence of temporally varying frames of outputs 
that depend on temporally varying inputs.

RenderPass also lets you use 
{ref}`collections of prims <collections_and_patterns>` to control prim 
visibility or behavior for the given pass. See 
{ref}`usdRender_using_renderpass_collections` for more details on these 
collections.

The following example shows three RenderPasses. A "foreground" pass and a 
"background" pass are specified that use RenderMan and the "PrimarySettings" 
RenderSettings configuration, but specify different parts of the stage to render 
using the RenderPass `renderVisibility` collection. A final "composite" 
pass is also specified that uses Nuke and takes the results from the other two 
passes as `inputPasses`. Note that the `nuke:writeNode` attribute 
and Nuke `renderSource` are hypothetical examples that would be associated 
with a Nuke-supplied API schema applied to the "composite" RenderPass prim -- 
USD does not provide any default Nuke render configuration support. 

```{code-block} usda
def Scope "Render"
{
    ...settings and products...

    def Scope "Passes"
    {
        def RenderPass "foreground" 
        {
            token passType = "prman"
            rel renderSource = <Render/PrimarySettings>
            string[] command = ["prman"]
            uniform bool collection:renderVisibility:includeRoot = false
            prepend rel collection:renderVisibility:includes = [
                </World/characters>,
                </World/sets/Kitchen/Table_grp>,
            ]
        }
        def RenderPass "background" 
        {
            token passType = "prman"
            rel renderSource = <Render/PrimarySettings>
            string[] command = ["prman"]
            uniform bool collection:renderVisibility:includeRoot = true
            prepend rel collection:renderVisibility:excludes = [
                </World/characters>,
                </World/sets/Kitchen/Table_grp>,
            ]
        }
        def RenderPass "composite"
        {
            token passType = "nuke"
            asset fileName = @composite.nk@
            # this nuke-namespaced property might come from a hypothetical Nuke-supplied API schema
            string nuke:writeNode = "WriteFinalComposite"
            rel renderSource = </Render/Passes/composite.nuke:writeNode>
            string[] command = ["nuke", "-x", "-c", "32G"]
            rel inputPasses = [
                </Render/Passes/foreground>,
                </Render/Passes/background>
            ]
        }
    }
}
```


