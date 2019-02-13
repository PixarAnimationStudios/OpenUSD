#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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
import argparse
import os
import sys

from pxr import Usd


METADATA_KEYS_TO_SKIP = ('typeName', 'specifier', 'kind', 'active')


def _Msg(msg):
    sys.stdout.write(msg + '\n')

def _Err(msg):
    sys.stderr.write(msg + '\n')


class USDAccessor(object):
    @staticmethod
    def GetChildren(prim):
        return prim.GetAllChildren()

    @staticmethod
    def GetProperties(prim):
        return prim.GetAuthoredProperties()

    @staticmethod
    def GetPropertyName(prop):
        return prop.GetName()

    @staticmethod
    def GetMetadata(prim):
        return prim.GetAllAuthoredMetadata().keys()

    @staticmethod
    def GetName(prim):
        return prim.GetName()

    @staticmethod
    def GetTypeName(prim):
        return prim.GetTypeName()

    @staticmethod
    def GetSpecifier(prim):
        return prim.GetSpecifier()
    
    @staticmethod
    def GetKind(prim):
        return Usd.ModelAPI(prim).GetKind()

    @staticmethod
    def IsActive(prim):
        return prim.IsActive()


class SdfAccessor(object):
    @staticmethod
    def GetChildren(prim):
        return prim.nameChildren

    @staticmethod
    def GetProperties(prim):
        return prim.properties

    @staticmethod
    def GetPropertyName(prop):
        return prop.name

    @staticmethod
    def GetMetadata(prim):
        return prim.ListInfoKeys()

    @staticmethod
    def GetName(prim):
        return prim.name

    @staticmethod
    def GetTypeName(prim):
        return prim.typeName

    @staticmethod
    def GetSpecifier(prim):
        return prim.specifier

    @staticmethod
    def GetKind(prim):
        return prim.kind

    @staticmethod
    def IsActive(prim):
        return prim.active


def GetPrimLabel(acc, prim):
    spec = acc.GetSpecifier(prim).displayName.lower()
    typeName = acc.GetTypeName(prim)
    if typeName:
        definition = '{} {}'.format(spec, typeName)
    else:
        definition = spec
    label = '{} [{}]'.format(acc.GetName(prim), definition)

    shortMetadata = []
    
    if not acc.IsActive(prim):
        shortMetadata.append('active = false')
    
    kind = acc.GetKind(prim)
    if kind:
        shortMetadata.append('kind = {}'.format(kind))
    
    if shortMetadata:
        label += ' ({})'.format(', '.join(shortMetadata))
    return label


def PrintPrim(args, acc, prim, prefix, isLast):
    if not isLast:
        lastStep = ' |--'
        if acc.GetChildren(prim):
            attrStep = ' |   |'
        else:
            attrStep = ' |    '
    else:
        lastStep = ' `--'
        if acc.GetChildren(prim):
            attrStep = '     |'
        else:
            attrStep = '      '
    
    if args.simple:
        label = acc.GetName(prim)
    else:
        label = GetPrimLabel(acc, prim)

    _Msg('{}{}{}'.format(prefix, lastStep, label))

    attrs = []
    if args.metadata:
        mdKeys = filter(lambda x: x not in METADATA_KEYS_TO_SKIP, sorted(acc.GetMetadata(prim)))
        attrs.extend('({})'.format(md) for md in mdKeys)
    
    if args.attributes:
        attrs.extend('.{}'.format(acc.GetPropertyName(prop)) for prop in acc.GetProperties(prim))
    
    numAttrs = len(attrs)
    for i, attr in enumerate(attrs):
        if i < numAttrs - 1:
            _Msg('{}{} :--{}'.format(prefix, attrStep, attr))
        else:
            _Msg('{}{} `--{}'.format(prefix, attrStep, attr))


def PrintChildren(args, acc, prim, prefix):
    children = acc.GetChildren(prim)
    numChildren = len(children)
    for i, child in enumerate(children):
        if i < numChildren - 1:
            PrintPrim(args, acc, child, prefix, isLast=False)
            PrintChildren(args, acc, child, prefix + ' |  ')
        else:
            PrintPrim(args, acc, child, prefix, isLast=True)
            PrintChildren(args, acc, child, prefix + '    ')


def PrintStage(args, stage):
    _Msg('USD')
    PrintChildren(args, USDAccessor, stage.GetPseudoRoot(), '')


def PrintLayer(args, layer):
    _Msg('USD')
    PrintChildren(args, SdfAccessor, layer.pseudoRoot, '')


def PrintTree(args, path):
    if args.flatten:
        popMask = (None if args.populationMask is None else Usd.StagePopulationMask())
        if popMask:
            for mask in args.populationMask:
                popMask.Add(mask)
        if popMask:
            if args.unloaded:
                stage = Usd.Stage.OpenMasked(path, popMask, Usd.Stage.LoadNone)
            else:
                stage = Usd.Stage.OpenMasked(path, popMask)
        else:
            if args.unloaded:
                stage = Usd.Stage.Open(path, Usd.Stage.LoadNone)
            else:
                stage = Usd.Stage.Open(path)
        PrintStage(args, stage)
    elif args.flattenLayerStack:
        from pxr import UsdUtils
        stage = Usd.Stage.Open(path, Usd.Stage.LoadNone)
        layer = UsdUtils.FlattenLayerStack(stage)
        PrintLayer(args, layer)
    else:
        from pxr import Sdf
        layer = Sdf.Layer.FindOrOpen(path)
        PrintLayer(args, layer)


def main():
    parser = argparse.ArgumentParser(
        description='Writes the tree structure of a USD file. The default is to inspect a single USD file. '
        'Use the --flatten argument to see the flattened (or composed) Stage tree.')

    parser.add_argument('inputPath',
        help='The input file path to usdtree')
    parser.add_argument(
        '--unloaded', action='store_true',
        dest='unloaded',
        help='Do not load payloads')
    parser.add_argument(
        '--attributes', '-a', action='store_true',
        dest='attributes',
        help='Display authored attributes')
    parser.add_argument(
        '--metadata', '-m', action='store_true',
        dest='metadata',
        help='Display authored metadata')
    parser.add_argument(
        '--simple', '-s', action='store_true',
        dest='simple',
        help='Only display prim names: no specifier, kind or active state.')
    parser.add_argument(
        '--flatten', '-f', action='store_true', help='Compose the stage with the '
        'input file as root layer and write the flattened content.')
    parser.add_argument(
        '--flattenLayerStack', action='store_true',
        help='Flatten the layer stack with the given root layer. '
        'Unlike --flatten, this does not flatten composition arcs (such as references).')
    parser.add_argument('--mask', action='store',
                        dest='populationMask',
                        metavar='PRIMPATH[,PRIMPATH...]',
                        help='Limit stage population to these prims, '
                        'their descendants and ancestors.  To specify '
                        'multiple paths, either use commas with no spaces '
                        'or quote the argument and separate paths by '
                        'commas and/or spaces.  Requires --flatten.')

    args = parser.parse_args()
    
    # split args.populationMask into paths.
    if args.populationMask:
        if not args.flatten:
            # You can only mask a stage, not a layer.
            _Err("%s: error: --mask requires --flatten" % parser.prog)
            return 1
        args.populationMask = args.populationMask.replace(',', ' ').split()

    from pxr import Ar
    resolver = Ar.GetResolver()

    try:
        resolver.ConfigureResolverForAsset(args.inputPath)
        resolverContext = resolver.CreateDefaultContextForAsset(args.inputPath)
        with Ar.ResolverContextBinder(resolverContext):
            resolved = resolver.Resolve(args.inputPath)
            if not resolved or not os.path.exists(resolved):
                _Err('Cannot resolve inputPath %r'%resolved)
                return 1
            PrintTree(args, resolved)
    except Exception as e:
        _Err("Failed to process '%s' - %s" % (args.inputPath, e))
        return 1

    return 0


if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    import platform
    if platform.system() != 'Windows':
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    sys.exit(main())
