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
import argparse
import os
import sys

from pxr import Ar, Usd


def _Msg(msg):
    sys.stdout.write(msg + '\n')

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _GetChildren(prim, iterType):
    if iterType == 'UsdPrim':
        return prim.GetChildren()
    else:
        return prim.nameChildren

def _GetProperties(prim, iterType):
    if iterType == 'UsdPrim':
        return prim.GetAuthoredProperties()
    else:
        return prim.properties

def _GetMetadata(prim, iterType):
    if iterType == 'UsdPrim':
        return prim.GetAllAuthoredMetadata().keys()
    else:
        return sorted(prim.ListInfoKeys())

def _GetName(prim, iterType):
    if iterType == 'UsdPrim':
        return prim.GetName()
    else:
        return prim.name

def _GetTypeName(prim, iterType):
    if iterType == 'UsdPrim':
        return prim.GetTypeName()
    else:
        return prim.typeName


def PrintPrim(args, prim, iterType, prefix, isLast):
    if not isLast:
        lastStep = ' |--'
        if _GetChildren(prim, iterType):
            attrStep = ' |   |'
        else:
            attrStep = ' |    '
    else:
        lastStep = ' `--'
        if _GetChildren(prim, iterType):
            attrStep = '     |'
        else:
            attrStep = '      '
    if args.types:
        typeName = _GetTypeName(prim, iterType)
        if typeName:
            label = '{}[{}]'.format( _GetName(prim, iterType), typeName)
        else:
            label =  _GetName(prim, iterType)
    else:
        label =  _GetName(prim, iterType)
    _Msg('{}{}{}'.format(prefix, lastStep, label))

    attrs = []
    if args.metadata:
        attrs.extend('({})'.format(md) for md in _GetMetadata(prim, iterType))
    
    if args.attributes:
        attrs.extend('.{}'.format(prop.GetName()) for prop in _GetProperties(prim, iterType))
    
    numAttrs = len(attrs)
    for i, attr in enumerate(attrs):
        if i < numAttrs - 1:
            _Msg('{}{} :--{}'.format(prefix, attrStep, attr))
        else:
            _Msg('{}{} `--{}'.format(prefix, attrStep, attr))


def PrintChildren(args, prim, iterType, prefix):
    children = _GetChildren(prim, iterType)
    numChildren = len(children)
    for i, child in enumerate(children):
        if i < numChildren - 1:
            PrintPrim(args, child, iterType, prefix, isLast=False)
            PrintChildren(args, child, iterType, prefix + ' |  ')
        else:
            PrintPrim(args, child, iterType, prefix, isLast=True)
            PrintChildren(args, child, iterType, prefix + '    ')


def PrintStage(args, stage):
    _Msg('USD')
    PrintChildren(args, stage.GetPseudoRoot(), 'UsdPrim', '')


def PrintLayer(args, layer):
    _Msg('USD')
    PrintChildren(args, layer.pseudoRoot, 'SdfPrimSpec', '')


def main():
    parser = argparse.ArgumentParser(
        description='Writes the tree structure of a USD file and all its references and payloads composed')

    parser.add_argument('inputPath')
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
        '--types', '-t', action='store_true',
        dest='types',
        help='Display prim types')
    parser.add_argument(
        '-f', '--flatten', action='store_true', help='Compose stages with the '
        'input files as root layers and write their flattened content.')
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
    
    exitCode = 0

    resolver = Ar.GetResolver()

    popMask = (None if args.populationMask is None else Usd.StagePopulationMask())
    if popMask:
        for path in args.populationMask:
            popMask.Add(path)

    try:
        resolver.ConfigureResolverForAsset(args.inputPath)
        resolverContext = resolver.CreateDefaultContextForAsset(args.inputPath)
        with Ar.ResolverContextBinder(resolverContext):
            resolved = resolver.Resolve(args.inputPath)
            if not resolved or not os.path.exists(resolved):
                _Err('Cannot resolve inputPath %r'%resolved)
                exitCode = 1
                return exitCode
            if args.flatten:
                if popMask:
                    if args.unloaded:
                        stage = Usd.Stage.OpenMasked(resolved, popMask, Usd.Stage.LoadNone)
                    else:
                        stage = Usd.Stage.OpenMasked(resolved, popMask)
                else:
                    if args.unloaded:
                        stage = Usd.Stage.Open(resolved, Usd.Stage.LoadNone)
                    else:
                        stage = Usd.Stage.Open(resolved)
                if args.flattenLayerStack:
                    from pxr import UsdUtils
                    stage = UsdUtils.FlattenLayerStack(stage)
                PrintStage(args, stage)
            else:
                from pxr import Sdf
                layer = Sdf.Layer.FindOrOpen(resolved)
                PrintLayer(args, layer)
    except Exception as e:
        _Err("Failed to process '%s' - %s" % (args.inputPath, e))
        exitCode = 1
        raise

    return exitCode

if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    import platform
    if platform.system() != 'Windows':
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    sys.exit(main())
