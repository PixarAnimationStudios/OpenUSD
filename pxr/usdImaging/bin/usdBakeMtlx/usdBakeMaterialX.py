#!/pxrpythonsubst
#
# Copyright 2022 Pixar
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

from __future__ import print_function

import sys, os, argparse
from pxr import Sdf, Sdr
from pxr import Usd, UsdShade, UsdGeom
from pxr import UsdBakeMtlx


# Returns all MaterialX Materials in the given usdStage
def GetMaterialXMaterials(usdStage):
    mtlxMaterials = []
    for prim in usdStage.Traverse():
        material = UsdShade.Material(prim)
        if material:
            outputs = material.GetOutputs()
            for output in outputs:
                if output.GetBaseName() == ('mtlx:surface'):
                    mtlxMaterials.append(material)
    return mtlxMaterials

# Returns the path to the baked version of the given MaterialX Material
def GetBakedMtlxMaterialPath(usdStage, mtlxMaterial):
    bakedMaterialPath = Sdf.Path(mtlxMaterial.GetPrim().GetParent().GetPath().
        AppendChild(mtlxMaterial.GetPath().name + '_baked'))
    assert usdStage.GetPrimAtPath(bakedMaterialPath)
    return bakedMaterialPath

# Returns a list of prims who have Material Bindings to the given original
# MaterialX material, along with the binding relationship 
def GetMtlxBoundPrims(usdStage, originalMtlxMaterialPath):
    prims = []
    for prim in usdStage.Traverse():
        for purpose in UsdShade.MaterialBindingAPI.GetMaterialPurposes():
            (material, bindingRel) = UsdShade.MaterialBindingAPI(prim).\
                                        ComputeBoundMaterial(purpose)
            if originalMtlxMaterialPath == material.GetPath():
                prims.append((prim, bindingRel))
    return prims



def main():
    print("")
    parser = argparse.ArgumentParser(description='Output an updated USD file '
        'that contains an additional material variant corresponding to a baked '
        'version of each MaterialX material in the input USD file. The baked '
        'materials are generated using the MaterialX::TextureBaker class and '
        'will generate a mtlx file and associated texture(s).')

    # Arguments
    parser.add_argument('--width', dest='width', type=int, default=1024, 
        help='Specify the width of the baked textures.')
    parser.add_argument('--height', dest='height', type=int, default=1024, 
        help='Specify the height of the baked textures.')
    parser.add_argument('--hdr', dest='hdr', action='store_true', 
        help='Save images to hdr format.')
    parser.add_argument('--average', dest='average', action='store_true', 
        help='Average baked images to generate constant values.')

    # Inputs
    parser.add_argument(dest='inputUsdFilename',
        help='Name of the input Usd File.')
    parser.add_argument(dest='outputUsdFilename',
        help='Name of the output Usd File, the generated mtlx File and any '
             'generated images, will be saved to this directory.')
    opts = parser.parse_args()


    # Load the UsdStage
    usdStage = Usd.Stage.Open(opts.inputUsdFilename)
    if not usdStage:
        sys.exit("ERROR: Could not open layer")

    # Get the MaterialX Materials in the UsdStage
    mtlxMaterials = GetMaterialXMaterials(usdStage)
    if not mtlxMaterials:
        print("No MaterialX Materials detected in the given USD file")
        sys.exit()
    
    for mtlxMaterial in mtlxMaterials:
        # Translate the UsdShade mtlxMaterial into a MaterialX Document and Bake
        bakedFilename = UsdBakeMtlx.BakeMaterial(
            mtlxMaterial, os.path.dirname(opts.outputUsdFilename),
            opts.width, opts.height, opts.hdr, opts.average)
        if not bakedFilename:
            continue

        # Read the Baked MaterialX Document into the existing USD Stage
        UsdBakeMtlx.ReadFileToStage(bakedFilename, usdStage)

        # Add Variants to the Prims with Bindings to the mtlxMaterial
        bakedMaterialPath = GetBakedMtlxMaterialPath(usdStage, mtlxMaterial)
        needsVariant = GetMtlxBoundPrims(usdStage, mtlxMaterial.GetPath())
        for prim, bindingRel in needsVariant:

            prim.RemoveProperty(bindingRel.GetName())
            rootPrim = prim.GetParent()
            if not (rootPrim.GetPath().IsPrimPath() or
                    rootPrim.GetPath().IsPrimVariantSelectionPath()):
                rootPrim = prim

            vset = rootPrim.GetVariantSets().AddVariantSet('MtlxShadingVariant')
            vset.AddVariant('original')
            vset.AddVariant('baked')
            vset.SetVariantSelection('original')
            with vset.GetVariantEditContext():
                bindingRel.SetTargets([mtlxMaterial.GetPath()])
            vset.SetVariantSelection('baked')
            with vset.GetVariantEditContext():
                bindingRel.SetTargets([bakedMaterialPath])

    # Save the USD Stage
    usdStage.GetRootLayer().Export(opts.outputUsdFilename)
    print("\nWrote baked USD file: {}".format(opts.outputUsdFilename))


if __name__ == '__main__':
    main()
