#!/usr/bin/env python
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
# A script for generating the ascii parser files for Sdf.
# This takes the flex and bison sources in Sdf and generates C++
# source files using flex and bison. 

from distutils.spawn import find_executable
from tempfile import mkdtemp
from argparse import ArgumentParser
from sys import exit, stdout
from os import listdir, mkdir, getcwd, chdir, rename, path, access, W_OK
from os.path import abspath, basename, join, splitext, isfile, dirname, exists
import platform
from subprocess import call, Popen, PIPE
from shutil import rmtree, copyfile
from difflib import unified_diff

# -----------------------------------------------------------------------------
# Validation functions.
# This section contains code that will gather/report diffs between the generated
# code and the installed files as well as installing generating files which
# pass validation.
# -----------------------------------------------------------------------------
def _compareFiles(installedFiles, generatedFiles, configuration):
    failOnDiff = configuration[VALIDATE]

    if len(installedFiles) != len(generatedFiles):
        installedNames = set(map(basename, installedFiles))
        generatedNames = set(map(basename, generatedFiles))

        if (generatedNames - installedNames):
            exit('*** Unknown files generated, please add them to the list of '
                 'base files names in sdfGenAsciiParsers.py')

        exit('*** Missing files:\n' + '\n'.join(installedNames - generatedNames))

    diffs = {}
    for i in xrange(0, len(installedFiles)):
        with open(installedFiles[i], 'r') as installedFile,\
             open(generatedFiles[i], 'r') as generatedFile:
            
            installedContent = installedFile.read()
            generatedContent = generatedFile.read()

            if installedContent != generatedContent:
                diff = '\n'.join(unified_diff(installedContent.split('\n'),
                                              generatedContent.split('\n'),
                                              'Source ' + installedFile.name,
                                              'Generated ' + generatedFile.name))
                diffs[basename(installedFile.name)] = diff

            if diffs and failOnDiff:
                exit('*** Differing Generated Files:\n' +
                     '\n'.join(diffs.values()))

    return diffs

def _copyGeneratedFiles(installedFiles, generatedFiles, diffs):
    baseNames = map(basename, installedFiles)
    for baseName, generatedFile, installedFile in zip(baseNames, 
                                                      generatedFiles, 
                                                      installedFiles):
        if baseName in diffs:
            print('Changed: ' + baseName)
            print(diffs[baseName])
            if not access(installedFile, W_OK):
                print('Cannot author ' + installedFile + ', (no write access).')
            else:
                copyfile(generatedFile, installedFile) 
        else:
            print('Unchanged: ' + baseName)

# -----------------------------------------------------------------------------
# Code generation functions.
# This section contains the code that will actually generate the C++ source
# and headers from the yy/ll source files as well as the code which will
# curate the generated code.
# -----------------------------------------------------------------------------
def _runBisonAndFlexCommands(configuration):
    # build up collections of all relevant files, these include 
    # yy/ll source files, as well as the generated C++ header
    # and source files.
    srcDir   = configuration[SRC_DIR]
    destDir  = configuration[DEST_DIR]
    bases    = configuration[BASES]

    bisonFiles      = [join(srcDir, base + '.yy') for base in bases] 
    flexFiles       = [join(srcDir, base + '.ll') for base in bases]
    bisonGenSources = [join(destDir, base + '.tab.cpp') for base in bases]
    bisonGenHeaders = [join(destDir, base + '.tab.hpp') for base in bases]
    flexGenSources  = [join(destDir, base + '.lex.cpp') for base in bases]

    sourceFiles    = bisonFiles + flexFiles
    generatedFiles = bisonGenHeaders + bisonGenSources + flexGenSources

    # generate all components of a flex/bison command, these
    # include the desired executables, flag settings 
    bisonFlags = lambda base: ['-d', '-p', base + 'Yy', '-o']
    flexFlags  = lambda base: ['-P'+ base + "Yy", '-t']  

    bisonExecutable = configuration[BISON_EXE]
    flexExecutable  = configuration[FLEX_EXE]
    
    bisonCommand = lambda index: ([bisonExecutable]
                                  + bisonFlags(base) 
                                  + [bisonGenSources[index]] 
                                  + [bisonFiles[index]])

    flexCommand  = lambda index: ([flexExecutable] 
                                  + flexFlags(base)
                                  + [flexFiles[index]])
    
    for index, base in enumerate(bases):
        print 'Running bison on %s' % (base + '.yy')
        call(bisonCommand(index))

        print 'Running flex on %s' % (base + '.ll')
        with open(flexGenSources[index], 'w') as outputFile:
            call(flexCommand(index), stdout=outputFile)

    # prepend license header to all generated files.
    licenseText = '\n'.join([
    "//"
    , "// Copyright 2016 Pixar"
    , "//"
    , "// Licensed under the Apache License, Version 2.0 (the \"Apache License\")"
    , "// with the following modification; you may not use this file except in"
    , "// compliance with the Apache License and the following modification to it:"
    , "// Section 6. Trademarks. is deleted and replaced with:"
    , "//"
    , "// 6. Trademarks. This License does not grant permission to use the trade"
    , "//    names, trademarks, service marks, or product names of the Licensor"
    , "//    and its affiliates, except as required to comply with Section 4(c) of"
    , "//    the License and to reproduce the content of the NOTICE file."
    , "//"
    , "// You may obtain a copy of the Apache License at"
    , "//"
    , "//     http://www.apache.org/licenses/LICENSE-2.0"
    , "//"
    , "// Unless required by applicable law or agreed to in writing, software"
    , "// distributed under the Apache License with the above modification is"
    , "// distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY"
    , "// KIND, either express or implied. See the Apache License for the specific"
    , "// language governing permissions and limitations under the Apache License."
    , "//\n"])

    for generatedFile in generatedFiles:
        with open(generatedFile, 'r') as f:
            lines = f.read()
        with open(generatedFile, 'w') as f:
            f.write(licenseText)
            f.write(lines)

    return sourceFiles, generatedFiles

def _canonicalizeFiles(sourceFiles, generatedFiles):
    PXR_PREFIX_PATH = 'pxr/usd/sdf'

    # by default, bison will output hpp header files, we don't want this
    # as it goes against our convention of .h for headers. More recent
    # versions of bison support this option directly.
    #
    # We also need to update the paths in the generated #line directives
    # so we can easily diff the generated and installed files.

    # 'renamed' represents the renamed files on disk, whereas the identifiers
    # are altered paths that will be used in #line directives in the source
    renamed = list(generatedFiles)
    identifiers = list(generatedFiles)

    # rename hpp files to h, also update our index of renamed files
    # and identifiers(these will be used when scrubbing the files' contents)
    for index, fileName in enumerate(generatedFiles):
        if 'hpp' in fileName:
            newName = fileName.replace('.hpp', '.h')
            rename(fileName, newName)
            renamed[index] = newName
            identifiers[index] = newName

    # identifiers includes the sourceFiles(yy,ll files) because 
    # they are also referred to in line directives
    identifiers += sourceFiles
    for index, fileName in enumerate(list(renamed + sourceFiles)):
        if '/' in fileName:
            identifiers[index] = join(PXR_PREFIX_PATH, basename(fileName))

    # create a list of pairs, representing the things to replace in our
    # generated files
    replacements = [] 
    for index, fileName in enumerate(list(generatedFiles+sourceFiles)):
        oldFileName = fileName
        newFileName = identifiers[index]
        replacements.append((oldFileName, newFileName))

    for renamedFile in renamed:
        print 'Fixing line directives in ' + basename(renamedFile)

        with open(renamedFile, 'r+') as inputFile:
            data = inputFile.read()
            
            # find and replace all generated file names
            for oldFileName, newFileName in replacements:
                data = data.replace(oldFileName, newFileName)
            
            # we seek to 0 and truncate as we intend 
            # to overwrite the existing data in the file
            inputFile.seek(0)
            inputFile.write(data)
            inputFile.truncate()

    return renamed

# -----------------------------------------------------------------------------
# Configuration info.
# This section of code discerns all the necessary info the run the parser
# and lexical analyzer generators over the source files.
# -----------------------------------------------------------------------------
def _parseArguments():
    parser = ArgumentParser(description='Generate Ascii File Parsers for Sdf')
    parser.add_argument('--srcDir', required=False, default=getcwd(),
                        help='The source directory for sdf.')
    parser.add_argument('--bison', required=False, 
                        help='The location of the bison executable to be used.')
    parser.add_argument('--flex', required=False,                         
                        help='The location of the flex executable to be used.')
    parser.add_argument('--validate', action='store_true',
                        help='Verify that the source files are unchanged.')
    parser.add_argument('--bases', nargs='+',
                        help='Base file identifiers used for generation,'
                             'for example, textFileFormat')
    return parser.parse_args()

# Configuration constants
DEST_DIR   = 0
SRC_DIR    = 1
VALIDATE   = 2
BASES      = 3
BISON_EXE  = 4
FLEX_EXE   = 5

def _getConfiguration():
    arguments = _parseArguments()

    config = { VALIDATE  : arguments.validate,
               SRC_DIR   : arguments.srcDir,
               DEST_DIR  : mkdtemp(),
               BISON_EXE : arguments.bison,
               FLEX_EXE  : arguments.flex,
               BASES     : arguments.bases }
               
    # Ensure all optional arguments get properly populated
    if not arguments.flex:
        config[FLEX_EXE] = _getFlex(config) 

    if not arguments.bison:
        config[BISON_EXE] = _getBison(config)

    if not arguments.bases:
        allFiles = listdir(arguments.srcDir)
        validExts = ['.yy', '.ll']
        relevantFiles = filter(lambda f: splitext(f)[1] in validExts, allFiles)
        bases = list(set(map(lambda f: splitext(f)[0], relevantFiles)))

        if not bases:
            exit('*** Unable to find source files for parser. Ensure that they '
                 'are in the source directory(--srcDir). If unspecified, the '
                 'source directory is assumed to be the current directory.')

        config[BASES] = bases 

    _validateSourceDirectory(config)

    return config

def _validateSourceDirectory(configuration):
    bases = configuration[BASES]
    srcDir = configuration[SRC_DIR]

    allFiles = ([join(srcDir, base + '.yy') for base in bases] 
                + [join(srcDir, base + '.ll') for base in bases]
                + [join(srcDir, base + '.tab.cpp') for base in bases]
                + [join(srcDir, base + '.tab.h') for base in bases]
                + [join(srcDir, base + '.lex.cpp') for base in bases])

    if not all(isfile(f) for f in allFiles):
        exit('*** Invalid source directory. This directory must '
             'contain all necessary flex/bison sources.')

# -----------------------------------------------------------------------------
# Build system info. 
# This is needed for discerning which flex/bison to use when 
# running sdfGenAsciiParsers. This works in the context of either a 
# SCons build configuration or a CMake build configuration.
# -----------------------------------------------------------------------------
SCONS = 0
CMAKE = 1

def _determineBuildSystem(configuration):
    srcDir = configuration[SRC_DIR]
    if isfile(join(srcDir, 'SConscript')):
        return SCONS
    elif isfile(join(srcDir, 'CMakeLists.txt')):
        return CMAKE
    else:
        exit('*** Unable to determine build system.')

def _getSconsBuildEnvSetting(environmentVariable, configuration):
    command = [find_executable('scons'), '-u', 
               '-Qq', '--echo=' + environmentVariable]
    line, _ = Popen(command, stdout=PIPE).communicate()
    _, envSettingValue = line.strip().split(' = ')

    if not envSettingValue:
        exit('*** Unable to determine ' + environmentVariable + 'from '
             'SCons build system. Try supplying it through the command line '
             'options.')

    return envSettingValue


def _getCMakeBuildEnvSetting(environmentVariable, configuration):
    # Gather the root of our source directory, we'll need to 
    # point CMake to it to find the cached variables.
    srcRootDir = configuration[SRC_DIR]

    foundPxr = lambda d: 'pxr' in map(basename, listdir(d))
    while exists(srcRootDir) and (not foundPxr(srcRootDir)):
        srcRootDir = dirname(srcRootDir)

        if srcRootDir is None:
            exit('*** Unable to find root of this source tree, '
                 'this information is needed for obtaining build environment '
                 'information from CMake.')

    command = [find_executable('cmake'), '-LA', '-N', srcRootDir]
    output, _ = Popen(command, stdout=PIPE).communicate()
    line = filter(lambda o: environmentVariable in o, output.split('\n'))[0]
    _, envSettingValue = line.strip().split('=')

    if not envSettingValue:
        exit('*** Unable to determine ' + environmentVariable + 'from '
             'CMake build system. Try supplying it through the command line '
             'options.')

    return envSettingValue

def _getFlex(configuration):
    buildSystem = _determineBuildSystem(configuration)    

    if buildSystem == SCONS:
        return _getSconsBuildEnvSetting('LEX', configuration)
    elif buildSystem == CMAKE:
        return _getCMakeBuildEnvSetting('FLEX_EXECUTABLE', configuration)

def _getBison(configuration):
    buildSystem = _determineBuildSystem(configuration)    

    if buildSystem == SCONS:
        return _getSconsBuildEnvSetting('YACC', configuration)
    elif buildSystem == CMAKE:
        return _getCMakeBuildEnvSetting('BISON_EXECUTABLE', configuration) 

# -----------------------------------------------------------------------------

def _printSection(sectionInfo):
    print '+-------------------------------------------------+'
    print sectionInfo
    print '+-------------------------------------------------+'

if __name__ == '__main__':
    configuration = _getConfiguration()

    _printSection('Running flex and bison on sources')
    sourceFiles, generatedFiles = _runBisonAndFlexCommands(configuration)

    _printSection('Canonicalizing generated files')
    generatedFiles = _canonicalizeFiles(sourceFiles, generatedFiles)
    
    diffSectionMsg = 'Checking for diffs'
    if configuration[VALIDATE] and not any(platform.win32_ver()):
        diffSectionMsg = diffSectionMsg + '(validation on)'

    _printSection(diffSectionMsg)
    installedFiles = [join(configuration[SRC_DIR], basename(f)) 
                      for f in generatedFiles]

    diffs = _compareFiles(installedFiles, generatedFiles, configuration)
    _copyGeneratedFiles(installedFiles, generatedFiles, diffs) 
    # If validation passed, clean up the generated files
    rmtree(configuration[DEST_DIR])
