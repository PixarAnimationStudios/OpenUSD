## Usage

```hdcapture.py``` has three required arguments:

```--input```     The input USD file to render to text.

```--output```    The filename to write the serialized USD to. Output format is USDA.

```--baseline```  The .usda file to diff the output with.

## Output

This tool will record information it receives from Hydra and dump it to a .usda file.  This allows inspection of what Hydra "sees" after data is fed through SceneDelegate/RenderIndex or DataSource/SceneIndex.  It ouputs a format like this:

```
#usda 1.0

def "Example"
{
    def "Geometry"
    {
        def Mesh "mesh_1" (
            prepend apiSchemas = ["MaterialBindingAPI"]
        )
        {
            custom uniform bool displayStyle:displacementEnabled = 1
            custom uniform bool displayStyle:flatShadingEnabled = 0
            custom uniform int displayStyle:refineLevel = 0
            uniform bool doubleSided = 0
            ...
        }
        def Mesh "mesh_2" (
            prepend apiSchemas = ["MaterialBindingAPI"]
        )
        {

            token faceVaryingLinearInterpolation = "cornersPlus1"
            int[] faceVertexCounts = [4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4]
            ...
        }
    }
}

```