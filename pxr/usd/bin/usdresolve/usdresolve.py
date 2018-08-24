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

def _Msg(msg):
    sys.stdout.write(msg + '\n')

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _ConfigureAssetResolver(args, resolver):
    if args.configureAssetPath:
        resolver.ConfigureResolverForAsset(args.configureAssetPath)
        resolverContext = resolver.CreateDefaultContextForAsset(args.configureAssetPath)
    else:
        resolver.ConfigureResolverForAsset(args.inputPath)
        resolverContext = resolver.CreateDefaultContextForAsset(args.inputPath)
    return resolverContext

def _AnchorRelativePath(args, resolver):
    if resolver.IsRelativePath(args.inputPath):
        if args.anchorPath:
            anchorPath = resolver.Resolve(args.anchorPath)
        else:
            anchorPath = resolver.Resolve(os.getcwd() + os.path.sep)
        return resolver.AnchorRelativePath(anchorPath, args.inputPath)
    else:
        return args.inputPath


def main():
    parser = argparse.ArgumentParser(
        description='Write usd file(s) either as text to stdout or to a '
        'specified output file.')

    parser.add_argument('inputPath')
    parser.add_argument(
        '--configureAssetPath',
        help="Run ConfigureResolverForAsset on the given asset path.")
    parser.add_argument(
        '--anchorPath',
        help="Run AnchorRelativePath on the given asset path.")

    args = parser.parse_args()
    
    exitCode = 0

    from pxr import Ar, Usd

    resolver = Ar.GetResolver()

    try:
        resolverContext = _ConfigureAssetResolver(args, resolver)
        resolverContextBinder = Ar.ResolverContextBinder(resolverContext)
        inputPath = _AnchorRelativePath(args, resolver)
        resolved = resolver.Resolve(inputPath)
        del resolverContextBinder
    except Exception as e:
        _Err("Failed to resolve '%s' - %s" % (args.inputPath, e))
        exitCode = 1

    if not resolved:
        _Err("Failed to resolve '%s'" % args.inputPath)
        exitCode = 1
    else:
        print resolved
    
    return exitCode

if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    import platform
    if platform.system() != 'Windows':
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    sys.exit(main())
