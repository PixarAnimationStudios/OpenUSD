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
#
# Usage: testWrapper.py <options> <cmd>
#
# Wrapper for test commands which allows us to control the test environment
# and provide additional conditions for passing.
#
# CTest only cares about the eventual return code of the test command it runs
# with 0 being success and anything else indicating failure. This script allows
# us to wrap a test command and provide additional conditions which must be met
# for that test to pass.
#
# Current features include:
# - specifying non-zero return codes
# - comparing output files against a baseline
#
from __future__ import print_function

import argparse
import glob
import os
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile

def _parseArgs():
    parser = argparse.ArgumentParser(description='USD test wrapper')
    parser.add_argument('--stdout-redirect', type=str,
            help='File to redirect stdout to')
    parser.add_argument('--stderr-redirect', type=str,
            help='File to redirect stderr to')
    parser.add_argument('--diff-compare', default=[], type=str,
            action='append',
            help=('Compare output file with a file in the baseline-dir of the '
                  'same name'))
    parser.add_argument('--image-diff-compare', default=[], type=str,
                        action='append',
                        help=('Compare output image with an image in the baseline '
                              'of the same name'))
    parser.add_argument('--fail', type=str,
                        help='The threshold for the acceptable difference of a pixel for failure')
    parser.add_argument('--failpercent', type=str,
                        help='The percentage of pixels that can be different before failure')
    parser.add_argument('--hardfail', type=str,
                        help='Triggers a failure if any pixels are above this threshold')
    parser.add_argument('--warn', type=str,
                        help='The threshold for the acceptable difference of a pixel for a warning')
    parser.add_argument('--warnpercent', type=str,
                        help='The percentage of pixels that can be different before a warning')
    parser.add_argument('--hardwarn', type=str,
                        help='Triggers a warning if any pixels are above this threshold')
    parser.add_argument('--perceptual', action='store_true',
                        help='Performs a test to see if two images are visually different')
    parser.add_argument('--files-exist', nargs='*',
            help=('Check that a set of files exist.'))
    parser.add_argument('--files-dont-exist', nargs='*',
            help=('Check that a set of files do not exist.'))
    parser.add_argument('--clean-output-paths', nargs='*',
            help=('Path patterns to remove from the output files being diff\'d.'))
    parser.add_argument('--post-command', type=str,
            help=('Command line action to run after running COMMAND.'))
    parser.add_argument('--pre-command', type=str,
            help=('Command line action to run before running COMMAND.'))
    parser.add_argument('--pre-command-stdout-redirect', type=str, 
            help=('File to redirect stdout to when running PRE_COMMAND.'))
    parser.add_argument('--pre-command-stderr-redirect', type=str, 
            help=('File to redirect stderr to when running PRE_COMMAND.'))
    parser.add_argument('--post-command-stdout-redirect', type=str, 
            help=('File to redirect stdout to when running POST_COMMAND.'))
    parser.add_argument('--post-command-stderr-redirect', type=str, 
            help=('File to redirect stderr to when running POST_COMMAND.'))
    parser.add_argument('--testenv-dir', type=str,
            help='Testenv directory to copy into test run directory')
    parser.add_argument('--baseline-dir',
            help='Baseline directory to use with --diff-compare')
    parser.add_argument('--failures-dir',
            help=('If either --diff-compare or --image-diff-compare fail, then '
                  'copy both the generated and baseline images into this '
                  'directory; if <PXR_CTEST_RUN_ID>" is in the value, it is '
                  'replaced with a timestamp identifying a given ctest '
                  'invocation'))

    parser.add_argument('--expected-return-code', type=int, default=0,
            help='Expected return code of this test.')
    parser.add_argument('--env-var', dest='envVars', default=[], type=str, 
            action='append',
            help=('Variable to set in the test environment, in KEY=VALUE form. '
                  'If "<PXR_TEST_DIR>" is in the value, it is replaced with the '
                  'absolute path of the temp directory the tests are run in'))
    parser.add_argument('--pre-path', dest='prePaths', default=[], type=str, 
            action='append', help='Path to prepend to the PATH env var.')
    parser.add_argument('--post-path', dest='postPaths', default=[], type=str, 
            action='append', help='Path to append to the PATH env var.')
    parser.add_argument('--verbose', '-v', action='store_true',
            help='Verbose output.')
    parser.add_argument('cmd', metavar='CMD', type=str, 
            help='Test command to run')
    return parser.parse_args()

def _resolvePath(baselineDir, fileName):
    # Some test envs are contained within a non-specific subdirectory, if that
    # exists then use it for the baselines
    nonSpecific = os.path.join(baselineDir, 'non-specific')
    if os.path.isdir(nonSpecific):
        baselineDir = nonSpecific

    return os.path.normpath(os.path.join(baselineDir, fileName))

def _stripPath(f, pathPattern):
    import re
    import platform

    # Paths on Windows may have backslashes but test baselines never do.
    # On Windows, we rewrite the pattern so slash and backslash both match
    # either a slash or backslash.  In addition, we match the remainder of
    # the path and rewrite it to have forward slashes so it matches the
    # baseline.
    repl = ''
    if platform.system() == 'Windows':
        def _windowsReplacement(m):
            return m.group(1).replace('\\', '/')
        pathPattern = pathPattern.replace('\\', '/')
        pathPattern = pathPattern.replace('/', '[/\\\\]')
        pathPattern = pathPattern + '(\S*)'
        repl = _windowsReplacement

    # Read entire file and perform substitution.
    with open(f, 'r+') as inputFile:
        data = inputFile.read()
        data = re.sub(pathPattern, repl, data)
        inputFile.seek(0)
        inputFile.write(data)
        inputFile.truncate()

def _addFilenameSuffix(path, suffix):
    base, ext = os.path.splitext(path)
    return base + suffix + ext

def _cleanOutput(pathPattern, fileName, verbose):
    if verbose:
        print("stripping path pattern {0} from file {1}".format(pathPattern, 
                                                                fileName))
    _stripPath(fileName, pathPattern)
    return True

def _diff(fileName, baselineDir, verbose, failuresDir=None):
    # Use the diff program or equivalent, rather than filecmp or similar
    # because it's possible we might want to specify other diff programs
    # in the future.
    import platform
    if platform.system() == 'Windows':
        diff = 'fc.exe'
    else:
        diff = '/usr/bin/diff'

    filesToDiff = glob.glob(fileName)
    if not filesToDiff:
        sys.stderr.write(
            "Error: could not files matching {0} to diff".format(fileName))
        return False

    for fileToDiff in filesToDiff:
        baselineFile = _resolvePath(baselineDir, fileToDiff)
        cmd = [diff, baselineFile, fileToDiff]
        if verbose:
            print("diffing with {0}".format(cmd))

        # This will print any diffs to stdout which is a nice side-effect
        if subprocess.call(cmd) != 0:
            if failuresDir is not None:
                _copyFailedDiffFiles(failuresDir, baselineFile, fileToDiff)
            return False

    return True

def _imageDiff(fileName, baseLineDir, verbose, env, warn=None, warnpercent=None,
               hardwarn=None, fail=None, failpercent=None, hardfail=None,
               perceptual=None, failuresDir=None):
    import platform
    if platform.system() == 'Windows':
        imageDiff = 'idiff.exe'
    else:
        imageDiff = 'idiff'

    cmdArgs = []
    if warn is not None:
        cmdArgs.extend(['-warn', warn])

    if warnpercent is not None:
        cmdArgs.extend(['-warnpercent', warnpercent])

    if hardwarn is not None:
        cmdArgs.extend(['-hardwarn', hardwarn])

    if fail is not None:
        cmdArgs.extend(['-fail', fail])

    if failpercent is not None:
        cmdArgs.extend(['-failpercent', failpercent])

    if hardfail is not None:
        cmdArgs.extend(['-hardfail', hardfail])

    if perceptual:
        cmdArgs.extend(['-p'])

    filesToDiff = glob.glob(fileName)
    if not filesToDiff:
        sys.stderr.write(
            "Error: could not files matching {0} to diff".format(fileName))
        return False

    for image in filesToDiff:
        cmd = [imageDiff]
        cmd.extend(cmdArgs)
        baselineImage = _resolvePath(baseLineDir, image)
        cmd.extend([baselineImage, image])

        if verbose:
            print("image diffing with {0}".format(cmd))

        # This will print any diffs to stdout which is a nice side-effect
        # 0: OK: the images match within the warning and error thresholds.
        # 1: Warning: the errors differ a little, but within error thresholds.
        # 2: Failure: the errors differ a lot, outside error thresholds.
        # 3: The images were not the same size and could not be compared.
        # 4: File error: could not find or open input files, etc.
        if subprocess.call(cmd, shell=False, env=env) not in (0, 1):
            if failuresDir is not None:
                _copyFailedDiffFiles(failuresDir, baselineImage, image)
            return False

    return True

def _copyTree(src, dest):
    ''' Copies the contents of src into dest.'''
    if not os.path.exists(dest):
        os.makedirs(dest)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dest, item)
        if os.path.isdir(s):
            shutil.copytree(s, d)
        else:
            shutil.copy2(s, d)

def _copyFailedDiffFiles(failuresDir, baselineFile, resultFile):
    baselineName = _addFilenameSuffix(os.path.basename(baselineFile),
                                      ".baseline")
    resultName = _addFilenameSuffix(os.path.basename(resultFile), ".result")
    baselineOutpath = os.path.join(failuresDir, baselineName)
    resultOutpath = os.path.join(failuresDir, resultName)

    if not os.path.isdir(failuresDir):
        os.makedirs(failuresDir)

    shutil.copy(baselineFile, baselineOutpath)
    shutil.copy(resultFile, resultOutpath)

    print("Image diff failure:\n"
          "  Copied:\n"
          "    {baselineName}\n"
          "    {resultName}\n"
          "  Into:\n"
          "    {failuresDir}".format(**locals()))

# subprocess.call returns -N if the process raised signal N. Convert this
# to the standard positive error code matching that signal. e.g. if the
# process encounters an SIGABRT signal it will return -6, but we really
# want the exit code 134 as that is what the script would return when run
# from the shell. This is well defined to be 128 + (signal number).
def _convertRetCode(retcode):
    if retcode < 0:
        return 128 + abs(retcode)
    return retcode

def _getRedirects(out_redir, err_redir):
    return (open(out_redir, 'w') if out_redir else None,
            open(err_redir, 'w') if err_redir else None)
    
def _runCommand(raw_command, stdout_redir, stderr_redir, env,
                expected_return_code):
    cmd = shlex.split(raw_command)
    fout, ferr = _getRedirects(stdout_redir, stderr_redir)
    try:
        print("cmd: %s" % (cmd, ))
        retcode = _convertRetCode(subprocess.call(cmd, shell=False, env=env,
                                  stdout=(fout or sys.stdout), 
                                  stderr=(ferr or sys.stderr)))
    finally:
        if fout:
            fout.close()
        if ferr:
            ferr.close()

    # The return code didn't match our expected error code, even if it
    # returned 0 -- this is a failure.
    if retcode != expected_return_code:
        if args.verbose:
            sys.stderr.write(
                "Error: return code {0} doesn't match "
                "expected {1} (EXPECTED_RETURN_CODE).".format(retcode, 
                                                        expected_return_code))
        sys.exit(1)

if __name__ == '__main__':
    args = _parseArgs()

    if args.diff_compare and not args.baseline_dir:
        sys.stderr.write("Error: --baseline-dir must be specified with " 
                         "--diff-compare.")
        sys.exit(1)

    if args.image_diff_compare and not args.baseline_dir:
        sys.stderr.write("Error: --baseline-dir must be specified with "
                         "--image-diff-compare.")
        sys.exit(1)

    if args.clean_output_paths and not args.diff_compare:
        sys.stderr.write("Error: --diff-compare must be specified with "
                         "--clean-output-paths.")
        sys.exit(1)

    testDir = tempfile.mkdtemp()
    os.chdir(testDir)
    if args.verbose:
        print("chdir: {0}".format(testDir))

    # Copy the contents of the testenv directory into our test run directory so
    # the test has it's own copy that it can reference and possibly modify.
    if args.testenv_dir and os.path.isdir(args.testenv_dir):
        if args.verbose:
            print("copying testenv dir: {0}".format(args.testenv_dir))
        try:
            _copyTree(args.testenv_dir, os.getcwd())
        except Exception as e:
            sys.stderr.write("Error: copying testenv directory: {0}".format(e))
            sys.exit(1)

    # Add any envvars specified with --env-var options into the environment
    env = os.environ.copy()
    for varStr in args.envVars:
        try:
            k, v = varStr.split('=', 1)
            if k == 'PATH':
                sys.stderr.write("Error: use --pre-path or --post-path to edit PATH.")
                sys.exit(1)
            v = v.replace('<PXR_TEST_DIR>', testDir)
            env[k] = v
        except IndexError:
            sys.stderr.write("Error: envvar '{0}' not of the form "
                             "key=value".format(varStr))
            sys.exit(1)

    # Modify the PATH env var.  The delimiter depends on the platform.
    pathDelim = ';' if platform.system() == 'Windows' else ':'
    paths = env.get('PATH', '').split(pathDelim)
    paths = args.prePaths + paths + args.postPaths
    env['PATH'] = pathDelim.join(paths)

    # Avoid the just-in-time debugger where possible when running tests.
    env['ARCH_AVOID_JIT'] = '1'

    if args.pre_command:
        _runCommand(args.pre_command, args.pre_command_stdout_redirect,
                    args.pre_command_stderr_redirect, env, 0)
        
    _runCommand(args.cmd, args.stdout_redirect, args.stderr_redirect,
                env, args.expected_return_code)

    if args.post_command:
        _runCommand(args.post_command, args.post_command_stdout_redirect,
                    args.post_command_stderr_redirect, env, 0)

    if args.clean_output_paths:
        for path in args.clean_output_paths:
            for diff in args.diff_compare:
                _cleanOutput(path, diff, args.verbose)

    if args.files_exist:        
        for f in args.files_exist:
            if args.verbose:
                print('checking if {0} exists.'.format(f))
            if not os.path.exists(f):
                sys.stderr.write('Error: {0} does not exist '
                                 '(FILES_EXIST).'.format(f))
                sys.exit(1)
        
    if args.files_dont_exist:
        for f in args.files_dont_exist:
            if args.verbose:
                print('checking if {0} does not exist.'.format(f))
            if os.path.exists(f):
                sys.stderr.write('Error: {0} does exist '
                                 '(FILES_DONT_EXIST).'.format(f))
                sys.exit(1)

    # If desired, diff the provided file(s) (must be generated by the test somehow)
    # with a file of the same name in the baseline directory
    failuresDir = None
    if args.failures_dir:
        failuresDir = args.failures_dir.replace('<PXR_CTEST_RUN_ID>',
            os.environ.get('PXR_CTEST_RUN_ID', 'NOT_RUN_FROM_CTEST'))
    if args.diff_compare:
        for diff in args.diff_compare:
            if not _diff(diff, args.baseline_dir, args.verbose,
                         failuresDir=failuresDir):
                sys.stderr.write('Error: diff for {0} failed '
                                 '(DIFF_COMPARE).'.format(diff))
                sys.exit(1)

    if args.image_diff_compare:
        converted = vars(args)
        params = {key: converted[key] for key in
                  ('warn', 'warnpercent', 'hardwarn',
                   'fail', 'failpercent', 'hardfail', 'perceptual')
                  if key in converted}
        params["failuresDir"] = failuresDir

        for image in args.image_diff_compare:
            if not _imageDiff(image, args.baseline_dir, args.verbose, env, **params):
                sys.stderr.write('Error: image diff for {0} failed '
                                 '(IMAGE_DIFF_COMPARE).\n'.format(image))
                sys.exit(1)

    sys.exit(0)
