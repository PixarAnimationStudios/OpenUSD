#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

"""
This script adds shading to the models at:

    models/
      Ball/

This script will change the texture path to point to different textures (though
we only ship textures for '1', '9', '8', '4').  We also set the displayColor in
the shadingVariants.

"""
import os
ASSET_BASE = os.path.join(os.getcwd(), 'models')

def main():
    from pxr import Usd
    stage = Usd.Stage.CreateNew(os.path.join(
        ASSET_BASE, 
        'Ball/Ball.shadingVariants.usda'))
    stage.GetRootLayer().defaultPrim = 'Ball'
    _AddShadingToBall(stage)
    stage.GetRootLayer().Save()

def _AddShadingToBall(stage):
    from pxr import Sdf, UsdRi
    model = stage.OverridePrim('/Ball')
    texDir =  os.path.join(ASSET_BASE, 'Ball/tex')
    mesh = stage.OverridePrim('/Ball/mesh')

    ballTextureNode = UsdRi.RisObject(stage.OverridePrim(
        model.GetPath().AppendPath('Looks/BallMaterial/BallTexture')))

    # now we'll show adding some shading variants to the ball as well.
    shadingVariantsInfo = [
        ('Cue', '', _Color(0.996, 0.992, 0.874)), # white

        ('Ball_1', '', _Color(1.000, 0.929, 0.184)), # yellow
        ('Ball_2', '', _Color(0.157, 0.243, 0.631)), # blue
        ('Ball_3', '', _Color(0.976, 0.212, 0.141)), # red
        ('Ball_4', '', _Color(0.250, 0.156, 0.400)), # purple
        ('Ball_5', '', _Color(0.980, 0.498, 0.184)), # orange
        ('Ball_6', '', _Color(0.050, 0.255, 0.239)), # green
        ('Ball_7', '', _Color(0.607, 0.059, 0.094)), # darkred

        ('Ball_8', '', _Color(0.122, 0.118, 0.110)), # black

        ('Ball_9',  'striped', _Color(1.000, 0.929, 0.184)), # yellow
        ('Ball_10', 'striped', _Color(0.157, 0.243, 0.631)), # blue
        ('Ball_11', 'striped', _Color(0.976, 0.212, 0.141)), # red
        ('Ball_12', 'striped', _Color(0.250, 0.156, 0.400)), # purple
        ('Ball_13', 'striped', _Color(0.980, 0.498, 0.184)), # orange
        ('Ball_14', 'striped', _Color(0.050, 0.255, 0.239)), # green
        ('Ball_15', 'striped', _Color(0.607, 0.059, 0.094)), # darkred
    ]


    # create the shadingVariant variantSet
    shadingVariant = model.GetVariantSets().AddVariantSet('shadingVariant')
    for variantName, decoration, color in shadingVariantsInfo:
        # creates a variant inside 'shadingVariant'
        shadingVariant.AddVariant(variantName)

        # switch to that variant
        shadingVariant.SetVariantSelection(variantName)

        # this 'with' construct here tells Usd that authoring operations should
        # write to the shadingVariant.
        with shadingVariant.GetVariantEditContext():
            whichBall = variantName.split('_')[-1]
            texPath = os.path.join(texDir, 'ball%s.tex' % whichBall)
            # in the current variant, modify the color
            _SetParameters(ballTextureNode, [
                ('filename', Sdf.ValueTypeNames.String, texPath),
            ])

            # set the display color for hydra
            _SetDisplayColor(mesh, color)

            # currently not doing anything with decoration, but we could maybe
            # use this to make the solid vs. stripes.

    # now make the variant selection 'Cue' instead of the last variant that we
    # created above.
    shadingVariant.SetVariantSelection('Cue')

def _SetParameters(shadingNode, params):
    """
    shadingNode is a RisObject
    params are (paramName, paramType, value) tuples
    """

    for paramName, paramType, value in params:
        shadingNode.CreateInput(paramName, paramType).Set(value)

def _Color(r, g, b):
    # for this tutorial, the colors i got are not in linear space.
    from pxr import Gf
    return Gf.ConvertDisplayToLinear(Gf.Vec3f(r, g, b))

def _SetDisplayColor(mesh, color):
    from pxr import UsdGeom

    # DisplayColor is actually an array.  Here, we just author one color which
    # applies to the whole mesh.
    UsdGeom.Gprim(mesh).CreateDisplayColorAttr([color])

if __name__ == '__main__':
    main()
