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

def _Err(msg):
    sys.stderr.write(msg + '\n')

def GetUsdData(filePath):
    from pxr import Sdf
    return Sdf.Layer.FindOrOpen(filePath)

def GetFlattenedUsdData(filePath, populationMaskPaths):
    from pxr import Ar, Usd
    Ar.GetResolver().ConfigureResolverForAsset(filePath)
    popMask = (None if populationMaskPaths is None
               else Usd.StagePopulationMask())
    if popMask:
        for path in populationMaskPaths:
            popMask.Add(path)
        return Usd.Stage.OpenMasked(filePath, popMask)
    else:
        return Usd.Stage.Open(filePath)

def GetFlattenedLayerStack(filePath):
    from pxr import Ar, Sdf, Pcp, Usd, UsdUtils
    Ar.GetResolver().ConfigureResolverForAsset(filePath)
    stage = Usd.Stage.Open(filePath, Usd.Stage.LoadNone)
    return UsdUtils.FlattenLayerStack(stage)

def main():
    parser = argparse.ArgumentParser(
        description='Write usd file(s) either as text to stdout or to a '
        'specified output file.')

    parser.add_argument('inputFiles', nargs='+')
    parser.add_argument(
        '-o', '--out', metavar='file', action='store',
        help='Write a single input file to this output file instead of stdout.')
    parser.add_argument(
        '--usdFormat', metavar='usda|usdb|usdc', action='store',
        help="Use this underlying file format "
        "for output files with the extension 'usd'.  For example, passing "
        "'-o output.usd --usdFormat usda' will create output.usd as a text "
        "file.  The USD_DEFAULT_FILE_FORMAT environment variable is another "
        "way to achieve this.")
    parser.add_argument(
        '-f', '--flatten', action='store_true', help='Compose stages with the '
        'input files as root layers and write their flattened content.')
    parser.add_argument(
        '--flattenLayerStack', action='store_true',
        help='Flatten the layer stack with the given root layer, and write '
        'out the result.  Unlike --flatten, this does not flatten composition '
        'arcs (such as references).')
    parser.add_argument('--mask', action='store',
                        dest='populationMask',
                        metavar='PRIMPATH[,PRIMPATH...]',
                        help='Limit stage population to these prims, '
                        'their descendants and ancestors.  To specify '
                        'multiple paths, either use commas with no spaces '
                        'or quote the argument and separate paths by '
                        'commas and/or spaces.  Requires --flatten.')


    args = parser.parse_args()

    formatArgsDict = {}

    # If --out was specified, it must either not exist or must be writable, the
    # extension must correspond to a known Sdf file format, and we must have
    # exactly one input file.
    if args.out:
        if os.path.isfile(args.out) and not os.access(args.out, os.W_OK):
            _Err("%s: error: no write permission for existing output file '%s'"
                 % (parser.prog, args.out))
            return 1
        if len(args.inputFiles) != 1:
            _Err("%s: error: must supply exactly one input file with -o/--out" %
                 parser.prog)
            return 1
        ext = os.path.splitext(args.out)[1][1:]
        if args.usdFormat:
            if ext != 'usd':
                _Err("%s: error: use of --usdFormat requires output file end "
                     "with '.usd' extension" % parser.prog)
                return 1
            formatArgsDict.update(dict(format=args.usdFormat))
        from pxr import Sdf
        if Sdf.FileFormat.FindByExtension(ext) is None:
            _Err("%s: error: unknown output file extension '.%s'"
                 % (parser.prog, ext))
            return 1
    # If --out was not specified, then --usdFormat must be unspecified or must
    # be 'usda'.
    elif args.usdFormat and args.usdFormat != 'usda':
        _Err("%s: error: can only write 'usda' format to stdout; specify an "
             "output file with -o/--out to write other formats" % parser.prog)
        return 1

    # split args.populationMask into paths.
    if args.populationMask:
        if not args.flatten:
            # You can only mask a stage, not a layer.
            _Err("%s: error: --mask requires --flatten" % parser.prog)
            return 1
        args.populationMask = args.populationMask.replace(',', ' ').split()

    from pxr import Usd

    exitCode = 0

    for inputFile in args.inputFiles:
        # Ignore nonexistent or empty files.
        if not os.path.isfile(inputFile) or os.path.getsize(inputFile) == 0:
            _Err("Ignoring nonexistent/empty file '%s'" % inputFile)
            exitCode = 1
            continue

        # Ignore unrecognized file types.
        if not Usd.Stage.IsSupportedFile(inputFile):
            _Err("Ignoring file with unrecognized type '%s'" % inputFile)
            exitCode = 1
            continue

        # Either open a layer or compose a stage, depending on whether or not
        # --flatten was specified.  Note that 'usdData' will be either a
        # Usd.Stage or an Sdf.Layer.
        try:
            if args.flatten:
                usdData = GetFlattenedUsdData(inputFile, args.populationMask)
            elif args.flattenLayerStack:
                usdData = GetFlattenedLayerStack(inputFile)
            else:
                usdData = GetUsdData(inputFile)
            if not usdData:
                raise Exception("Unknown error")
        except Exception as e:
            _Err("Failed to open '%s' - %s" % (inputFile, e))
            exitCode = 1
            continue

        # Write to either stdout or the specified output file
        if args.out:
            try:
                usdData.Export(args.out, args=formatArgsDict)
            except Exception as e:
                # Let the user know an error occurred.
                _Err("Error exporting '%s' to '%s' - %s" %
                     (inputFile, args.out, e))

                # If the output file exists, let's try to rename it with
                # '.quarantine' appended and let the user know.  Do this
                # after the above error report because os.rename() can
                # fail and we don't want to lose the above error.
                if os.path.isfile(args.out):
                    newName = args.out + '.quarantine'
                    try:
                        os.rename(args.out, newName)
                        _Err("Possibly corrupt output file renamed to %s" %
                            (newName, ))
                    except Exception as e:
                        _Err("Failed to rename possibly corrupt output " +
                             "file from %s to %s" % (args.out, newName))
                exitCode = 1
        else:
            try:
                sys.stdout.write(usdData.ExportToString())
            except Exception as e:
                _Err("Error writing '%s' to stdout; %s" % (inputFile, e))
                exitCode = 1

    return exitCode

if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    import platform
    if platform.system() != 'Windows':
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    sys.exit(main())
