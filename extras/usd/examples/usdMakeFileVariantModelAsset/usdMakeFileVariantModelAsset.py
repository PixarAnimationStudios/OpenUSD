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

'''
Creates a top-level, referenceable asset USD file from one or more
'variant' files, each of which can contain arbitrary scene description.
When supplying multiple files, one must also provide the name for a 
variantSet that will be constructed to switch between the files.

The asset file will place the variant files behind a "payload", which will
enable consumers to defer loading and processing of the data when composed
onto a UsdStage. 

The names of the created variations will be taken directly from the basename
of their corresponding input file.
'''

from pxr import Tf, Kind, Sdf, Usd

# ToDo:
# - handle multiple variantSets
# - layer multiple kinds of files (e.g. shading.usd over geom.usd)
# - allow output filename to be independently specifiable? (Breaks with Pixar
#   convention)
# - allow variant names to be specified independently of variant file names
# - Compute and present (per-variant) UsdGeomModelAPI.extentsHint
# - Compute and author UsdModelAPI::SetPayloadAssetDependencies()
def CreateModelStage(assetName, 
                     assetIdentifier=None, 
                     kind=Kind.Tokens.component,
                     filesToReference=None,
                     variantSetName=None,
                     defaultVariantSelection=None):
    # Preconditions....
    if not Tf.IsValidIdentifier(assetName):
        print "assetName '%s' must be a valid identifier. Aborting." % \
            assetName
        return None
    if variantSetName and not Tf.IsValidIdentifier(variantSetName):
        print "variantSetName '%s' must be a valid identifier. Aborting." % \
            variantSetName
        return None
    if filesToReference and len(filesToReference) > 1 and not variantSetName:
        # For now, we only allow multiple files to reference if we're switching
        # them with a variantSet.  We can relax this restriction when we can
        # make internal payload arcs (bug #119960)
        print "Cannot create multiple-file-reference without a variantSet. Aborting"
        return None
    if not Kind.Registry.IsA(kind, Kind.Tokens.model):
        print "kind '%s' is not a valid model kind, which must be one of:" % \
            kind
        print Kind.Registry.GetAllKinds()
        return None
    
    # Create the root file for the stage, and make it ASCII text.
    # We need some nicer sugar for this.
    fileName = assetName + ".usd"
    rootLayer = Sdf.Layer.CreateNew(fileName, args = {'format':'usda'})
    stage = Usd.Stage.Open(rootLayer)

    # Name the root prim after the asset.  Don't give it a type, since we
    # want that to come from referenced files.  Make it be the "default prim"
    # so that we can reference the resulting file without specifiying a 
    # prim path
    rootPath = Sdf.Path.absoluteRootPath
    modelRootPrim = stage.DefinePrim(rootPath.AppendChild(assetName))
    stage.SetDefaultPrim(modelRootPrim)
    modelAPI = Usd.ModelAPI(modelRootPrim)
    modelAPI.SetKind(kind)
    # See http://openusd.org/docs/api/class_usd_model_a_p_i.html#details
    # for more on assetInfo
    modelAPI.SetAssetName(assetName)
    modelAPI.SetAssetIdentifier(assetIdentifier or fileName)
    
    # Add a class named after the asset, and make the asset inherit from it.
    # This is not necessary for a valid asset, and the class-naming is a Pixar
    # convention.  But always having a class associated with each asset is
    # extremely useful for non-destructively editing many referenced or
    # instanced assets of the same type.
    classPrim = stage.CreateClassPrim(rootPath.AppendChild("_class_"+assetName))
    modelRootPrim.GetInherits().AddInherit(classPrim.GetPath())
    
    if not filesToReference:
        # weird edge case... we're done
        return stage
    elif len(filesToReference) == 1 and not variantSetName:
        # The other, more plausible edge case: we're just wrapping
        # some other file (e.g. alembic) in order to give it a payload
        # and other proper USD trappings - no variants
        modelRootPrim.GetPayloads().AddPayload(Sdf.Payload(filesToReference[0]))
        return stage

    # OK, we're making a variantSet, and we are going to vary the payload
    # in each variant
    varSet = modelRootPrim.GetVariantSet(variantSetName)
    for variantFile in filesToReference:
        import os
        variantName = os.path.splitext(os.path.basename(variantFile))[0]
        # If we didn't specify a default selection, choose the first one
        if not defaultVariantSelection:
            defaultVariantSelection = variantName
        varSet.AddVariant(variantName)
        varSet.SetVariantSelection(variantName)
        # The context object makes all edits "go inside" the variant we
        # just created.
        with varSet.GetVariantEditContext():
            modelRootPrim.GetPayloads().AddPayload(Sdf.Payload(variantFile))
    # Now put the variantSet into the state we want it to be in by default
    varSet.SetVariantSelection(defaultVariantSelection)
    
    return stage

if __name__ == "__main__":
    import argparse, os, sys
    descr = __doc__.strip()
    parser = argparse.ArgumentParser(prog=os.path.basename(sys.argv[0]),
                                     description=descr)
    parser.add_argument('assetName')
    parser.add_argument('variantFiles', nargs='+')
    parser.add_argument(
        '-k', '--kind', default='component', action='store', metavar='kind',
        help="Model kind, one of: component, group, or assembly")
    parser.add_argument(
        '-v', '--variantSet', default='', action='store', metavar='variantSet',
        help="Variantset to create to modulate variantFiles. Can be elided "
        "if only one file is supplied")
    parser.add_argument(
        '-i', '--identifier', default='', action='store', metavar='identifier',
        help="The identifier you would expect your Ar asset-resolver plugin "
        "to resolve to the (installed) assetName.usd file this script creates. "
        " If unspecified, defaults to assetName.usd")
    parser.add_argument(
        '-d', '--defaultVariantSelection', default='', action='store', 
        metavar='defaultVariantSelection',
        help="This variant will be selected by default when the asset is "
        "added to a composition.  If unspecified, will be the variant for "
        "'variantFile1'")

    args = parser.parse_args()
    
    if not args.assetName or args.assetName == '':
        parser.error("No assetName specified")
    
    stage = CreateModelStage(args.assetName,
                             assetIdentifier=args.identifier,
                             kind=args.kind,
                             filesToReference=args.variantFiles,
                             variantSetName=args.variantSet,
                             defaultVariantSelection=args.defaultVariantSelection)
    
    if stage:
        stage.GetRootLayer().Save()
        exit(0)
    else:
        exit(1)

                             
    
