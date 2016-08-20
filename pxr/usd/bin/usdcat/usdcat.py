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
import argparse, sys, signal, os

def _Err(msg):
    sys.stderr.write(msg + '\n')

def GetUsdData(filePath):
    from pxr import Sdf
    layer = Sdf.Layer.FindOrOpen(filePath)
    assert layer, 'Failed to open %s' % filePath
    return layer

def GetFlattenedUsdData(filePath):
    from pxr import Ar, Usd
    Ar.GetResolver().ConfigureResolverForAsset(filePath)
    stage = Usd.Stage.Open(filePath)
    assert stage, 'Failed to open %s' % filePath
    return stage

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
            usdData = (GetFlattenedUsdData(inputFile) if args.flatten
                       else GetUsdData(inputFile))
        except Exception as e:
            _Err("Failed to open '%s' - %s" % (inputFile, e))
            exitCode = 1
            continue

        # Write to either stdout or the specified output file
        if args.out:
            try:
                usdData.Export(args.out, args=formatArgsDict)
            except Exception as e:
                # An error occurred -- if the output file exists, let's try to
                # rename it with '.quarantine' appended, and either way let the
                # user know what happened.
                info = 'no output file produced'
                if os.path.isfile(args.out):
                    newName = args.out + '.quarantine'
                    os.rename(args.out, newName)
                    info = 'possibly corrupt output file renamed to ' + newName
                _Err("Error exporting '%s' to '%s' - %s; %s" %
                     (inputFile, args.out, info, e))
                exitCode = 1
        else:
            try:
                print usdData.ExportToString()
            except Exception as e:
                _Err("Error writing '%s' to stdout; %s" % (inputFile, e))
                exitCode = 1

    return exitCode

if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    sys.exit(main())
