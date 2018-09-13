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
import argparse, sys, os
from pxr import Ar, Usd

def _Msg(msg):
    sys.stdout.write(msg + '\n')

def _Err(msg):
    sys.stderr.write(msg + '\n')


def PrintPrim(args, prim, prefix, isLast):
    if not isLast:
        lastStep = ' |--'
        if prim.GetChildren():
            attrStep = ' |   |'
        else:
            attrStep = ' |    '
    else:
        lastStep = ' `--'
        if prim.GetChildren():
            attrStep = '     |'
        else:
            attrStep = '      '
    if args.types:
        typeName = prim.GetTypeName()
        if typeName:
            label = '{}[{}]'.format(prim.GetName(), typeName)
        else:
            label = prim.GetName()
    else:
        label = prim.GetName()
    _Msg('{}{}{}'.format(prefix, lastStep, label))

    attrs = []
    if args.metadata:
        attrs.extend('({})'.format(md) for md in prim.GetAllAuthoredMetadata().keys())
    
    if args.attributes:
        attrs.extend('.{}'.format(prop.GetName()) for prop in prim.GetAuthoredProperties())
    
    numAttrs = len(attrs)
    for i, attr in enumerate(attrs):
        if i < numAttrs - 1:
            _Msg('{}{} :--{}'.format(prefix, attrStep, attr))
        else:
            _Msg('{}{} `--{}'.format(prefix, attrStep, attr))


def PrintChildren(args, prim, prefix):
    children = prim.GetChildren()
    numChildren = len(children)
    for i, child in enumerate(children):
        if i < numChildren - 1:
            PrintPrim(args, child, prefix, isLast=False)
            PrintChildren(args, child, prefix + ' |  ')
        else:
            PrintPrim(args, child, prefix, isLast=True)
            PrintChildren(args, child, prefix + '    ')


def PrintTree(args, stage):
    _Msg('USD')
    PrintChildren(args, stage.GetPseudoRoot(), '')


def main():
    parser = argparse.ArgumentParser(
        description='Writes the tree structure of a USD file and all its references and payloads composed')

    parser.add_argument('inputPath')
    parser.add_argument('--unloaded', action='store_true',
                        dest='unloaded',
                        help='Do not load payloads')
    parser.add_argument('--attributes', '-a', action='store_true',
                        dest='attributes',
                        help='Display authored attributes')
    parser.add_argument('--metadata', '-m', action='store_true',
                        dest='metadata',
                        help='Display authored metadata')
    parser.add_argument('--types', '-t', action='store_true',
                        dest='types',
                        help='Display prim types')

    args = parser.parse_args()
    
    exitCode = 0

    resolver = Ar.GetResolver()

    try:
        resolver.ConfigureResolverForAsset(args.inputPath)
        resolverContext = resolver.CreateDefaultContextForAsset(args.inputPath)
        with Ar.ResolverContextBinder(resolverContext):
            resolved = resolver.Resolve(args.inputPath)
            if args.unloaded:
                stage = Usd.Stage.Open(resolved, Usd.Stage.LoadNone)
            else:
                stage = Usd.Stage.Open(resolved)
            PrintTree(args, stage)
    except Exception as e:
        _Err("Failed to process '%s' - %s" % (args.inputPath, e))
        exitCode = 1

    return exitCode

if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    import platform
    if platform.system() != 'Windows':
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    sys.exit(main())
