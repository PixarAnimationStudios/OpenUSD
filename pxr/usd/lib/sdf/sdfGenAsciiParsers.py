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
# This takes the lex and yacc sources in Sdf and generates C++
# source files using flex and bison. 

from argparse import ArgumentParser
from sys import exit, stdout
from os import listdir, mkdir, getcwd, chdir, rename, path
from subprocess import call
from shutil import rmtree

def _validateGeneratedFiles(installedFiles, generatedFiles):
    assert len(installedFiles) == len(generatedFiles),\
            "Did not generate the correct number of files"

    for i in xrange(0, len(installedFiles)):
        with open(installedFiles[i], 'r') as installedFile,\
             open(generatedFiles[i], 'r') as generatedFile:
            
            if not installedFile.read() == generatedFile.read():
                exit('Generated file %s did not match installed file %s' \
                     % (generatedFiles[i], installedFiles[i]))

def _canonicalizeFiles(sourceFiles, generatedFiles):
    PXR_PREFIX_PATH = "pxr/usd/sdf"

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
            identifiers[index] = path.join(PXR_PREFIX_PATH, 
                                           path.basename(fileName))

    # create a list of pairs, representing the things to replace in our
    # generated files
    replacements = [] 
    for index, fileName in enumerate(list(generatedFiles+sourceFiles)):
        oldFileName = fileName
        newFileName = identifiers[index]
        replacements.append((oldFileName, newFileName))

    for renamedFile in renamed:
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

def _parseArguments():
    parser = ArgumentParser(description='Generate Ascii File Parsers for Sdf')
    parser.add_argument('--srcDir', required=True,
                        help='The target directory for the source files.')
    parser.add_argument('--bison', required=True,
                        help='The location of the bison executable to be used.')
    parser.add_argument('--flex', required=True,
                        help='The location of the flex executable to be used.')
    parser.add_argument('--validate', action='store_true',
                        help='Generate files in a temporary directory and '
                             'compare against currently installed files.')
    parser.add_argument('--bases', required=True, nargs='+',
                        help='Base file identifiers used for generation,'
                             'for example, textFileFormat')
    arguments = parser.parse_args()

    srcDir = path.abspath(arguments.srcDir)

    if arguments.validate:
        import tempfile
        destDir = tempfile.mkdtemp()
    else:
        destDir = srcDir 

    return {'bison': arguments.bison, 
            'flex' : arguments.flex,
            'validate': arguments.validate,
            'srcDir': srcDir,
            'destDir': destDir,
            'bases': list(arguments.bases)}

def _runYaccAndLexCommands(configuration):
    # build up collections of all relevant files, these include 
    # yy/ll source files, as well as the generated C++ header
    # and source files.
    srcDir   = configuration['srcDir']
    destDir  = configuration['destDir']
    bases  = configuration['bases']

    yaccFiles      = [path.join(srcDir, base + '.yy') for base in bases] 
    lexFiles       = [path.join(srcDir, base + '.ll') for base in bases]
    yaccGenSources = [path.join(destDir, base + '.tab.cpp') for base in bases]
    yaccGenHeaders = [path.join(destDir, base + '.tab.hpp') for base in bases]
    lexGenSources  = [path.join(destDir, base + '.lex.cpp') for base in bases]

    sourceFiles    = yaccFiles + lexFiles
    generatedFiles = yaccGenHeaders + yaccGenSources + lexGenSources

    # generate all components of a lex/yacc command, these
    # include the desired executables, flag settings 
    yaccFlags = lambda base: ['-d', '-p', base + 'Yy', '-o']
    lexFlags  = lambda base: ['-P'+ base + "Yy", '-t']  

    yaccExecutable = configuration['bison']
    lexExecutable  = configuration['flex']
    
    yaccCommand = lambda index: ([yaccExecutable]
                                + yaccFlags(base) 
                                + [yaccGenSources[index]] 
                                + [yaccFiles[index]])

    lexCommand  = lambda index: ([lexExecutable] 
                                + lexFlags(base)
                                + [lexFiles[index]])
    
    for index, base in enumerate(bases):
        print 'Running %s on %s' %(yaccExecutable, base)
        call(yaccCommand(index))

        print 'Running %s on %s' %(lexExecutable, base)
        with open(lexGenSources[index], 'w') as outputFile:
            call(lexCommand(index), stdout=outputFile)

    # prepend license header to all generated files.
    licenseText = '''\
    //
    // Copyright 2016 Pixar
    //
    // Licensed under the Apache License, Version 2.0 (the "Apache License")
    // with the following modification; you may not use this file except in
    // compliance with the Apache License and the following modification to it:
    // Section 6. Trademarks. is deleted and replaced with:
    //
    // 6. Trademarks. This License does not grant permission to use the trade
    //    names, trademarks, service marks, or product names of the Licensor
    //    and its affiliates, except as required to comply with Section 4(c) of
    //    the License and to reproduce the content of the NOTICE file.
    //
    // You may obtain a copy of the Apache License at
    //
    //     http://www.apache.org/licenses/LICENSE-2.0
    //
    // Unless required by applicable law or agreed to in writing, software
    // distributed under the Apache License with the above modification is
    // distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    // KIND, either express or implied. See the Apache License for the specific
    // language governing permissions and limitations under the Apache License.
    //
    '''
    import textwrap
    licenseText = textwrap.dedent(licenseText)

    for generatedFile in generatedFiles:
        with open(generatedFile, 'r') as f:
            lines = f.read()
        with open(generatedFile, 'w') as f:
            f.write(licenseText)
            f.write(lines)

    return sourceFiles, generatedFiles


if __name__ == '__main__':
    configuration = _parseArguments()

    print 'Running lex and yacc on sources'
    sourceFiles, generatedFiles = _runYaccAndLexCommands(configuration)

    print 'Canonicalizing generated files'
    generatedFiles = _canonicalizeFiles(sourceFiles, generatedFiles)
    
    if configuration['validate']:
        print 'Validating generated files'
        installedFiles = [path.join(configuration['srcDir'], path.basename(f)) 
                          for f in generatedFiles]
        _validateGeneratedFiles(installedFiles, generatedFiles)

        # if validation passed, we can clean up our temp dir 
        rmtree(configuration['destDir'])
