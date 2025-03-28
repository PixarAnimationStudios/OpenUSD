#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

import argparse, sys, os

from pxr import Ar

def _HasCreateIdentifier():
    return hasattr(Ar.Resolver, "CreateIdentifier")

def _HasConfigureResolverForAsset():
    return hasattr(Ar.Resolver, "ConfigureResolverForAsset")

def _Msg(msg):
    sys.stdout.write(msg + '\n')

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _ConfigureAssetResolver(args, resolver):
    if _HasConfigureResolverForAsset():
        configurePath = args.configureAssetPath or args.inputPath
        resolver.ConfigureResolverForAsset(configurePath)
    else:
        if args.createContextForAsset:
            return resolver.CreateDefaultContextForAsset(
                args.createContextForAsset)
        elif args.createContextFromString:
            configStrs = []
            for s in args.createContextFromString:
                try:
                    uriScheme, config = s.split(":", 1)
                    configStrs.append((uriScheme, config))
                except ValueError:
                    configStrs.append(("", s))
            return resolver.CreateContextFromStrings(configStrs)

    return resolver.CreateDefaultContextForAsset(args.inputPath)

def _AnchorRelativePath(args, resolver):
    if args.anchorPath:
        if _HasCreateIdentifier():
            return resolver.CreateIdentifier(
                args.inputPath, Ar.ResolvedPath(args.anchorPath))
        elif resolver.IsRelativePath(args.inputPath):
            return resolver.AnchorRelativePath(args.anchorPath, args.inputPath)
    return args.inputPath


def main():
    parser = argparse.ArgumentParser(
        description='Resolves an asset path using a fully configured USD Asset Resolver.')

    parser.add_argument('inputPath',
        help="An asset path to be resolved by the USD Asset Resolver.")

    if _HasConfigureResolverForAsset():
        parser.add_argument(
            '--configureAssetPath',
            help="Run ConfigureResolverForAsset on the given asset path.")
    else:
        subparser = parser.add_mutually_exclusive_group()
        subparser.add_argument(
            '--createContextForAsset',
            help=("Run CreateDefaultContextForAsset with the given asset path "
                  "to create the context used for resolution."))
        subparser.add_argument(
            '--createContextFromString', action='append',
            help=("Run CreateContextFromString with the given string to create "
                  "the context used for resolution. This accepts strings like "
                  "[<URI Scheme>:]<Configuration String> and may be specified "
                  "multiple times.\n\n"
                  "ex: usdresolve --createContextFromString 'config_primary' "
                  "--createContextFromString 'my_uri_scheme:config_uri'"))

    if _HasCreateIdentifier():
        parser.add_argument(
            '--anchorPath',
            help=("Run CreateIdentifier with the input path and this anchor "
                  "asset path and resolve the result.\n\n"
                  "ex: usdresolve --anchorPath /asset/asset.usd sublayer.usd"))
    else:
        parser.add_argument(
            '--anchorPath',
            help="Run AnchorRelativePath on the given asset path.")
        

    args = parser.parse_args()

    exitCode = 0

    resolver = Ar.GetResolver()

    try:
        resolverContext = _ConfigureAssetResolver(args, resolver)
        with Ar.ResolverContextBinder(resolverContext):
            inputPath = _AnchorRelativePath(args, resolver)
            resolved = resolver.Resolve(inputPath)
    except Exception as e:
        _Err("Failed to resolve '%s' - %s" % (args.inputPath, e))
        exitCode = 1

    if not resolved:
        _Err("Failed to resolve '%s'" % args.inputPath)
        exitCode = 1
    else:
        print(resolved)
    
    return exitCode

if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    import platform
    if platform.system() != 'Windows':
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    sys.exit(main())
