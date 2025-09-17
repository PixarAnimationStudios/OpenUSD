#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""
This script helps set up build necessities for schema libraries. It will 
generate templates for the following files if they do not already exist:
 * CMakeLists.txt 
 * __init__.py
 * schema.usda
 * module.cpp
 * schemaUserDoc.usda

"""

import sys, os, inspect
from argparse import ArgumentParser

from jinja2 import Environment, FileSystemLoader
from jinja2.exceptions import TemplateNotFound, TemplateSyntaxError

from pxr import Plug
from pxr.Usd.usdGenSchema import _Printer, _WriteFile

Print = _Printer()

# Name of script, e.g. "usdInitSchema"
PROGRAM_NAME = os.path.splitext(os.path.basename(sys.argv[0]))[0]

def _WriteFileIfNotPresent(fileName, filePath, renderedTemplate, validate):
    if os.path.exists(filePath):
        Print("\t{0} is already present, skipping generation.".format(fileName))
    else:
        _WriteFile(filePath, renderedTemplate, validate)    

if __name__ == '__main__':
    #
    # Parse Command-line
    #
    parser = ArgumentParser(description='Set up build necessities for schema classes. This will generate '
             'templates for the CMakeLists.txt, __init__.py, schema.usda, '
             'module.cpp, and a user doc if they do not already exist.')
    parser.add_argument('codeGenPath',
        type=str,
        help='The target directory where the code should be generated. '
        'Can be \'.\' for the current directory.')
    parser.add_argument('-v', '--validate',
        action='store_true',
        help='Verify that the source files are unchanged.')
    parser.add_argument('-q', '--quiet',
        action='store_true',
        help='Do not output text during execution.')
    parser.add_argument('libraryPath',
        type=str,
        help='The partial path with which to prefix \'#include\' statements of '
             'generated files for this module. For external (non-Pixar) '
             'plugins, we recommend setting libraryPath to ".", so that the '
             'headers inside src directory are included in the generated files.')
    parser.add_argument('pxrPrefix',
        nargs='?',
        type=str,
        default="",
        help='Prefix to the path from pxr to this library. For pxr/usd/usd, '
             'the prefix would be pxr/usd. Omit this argument for non-OpenUSD '
             'build systems.')
    parser.add_argument('--genModuleDeps',
        action='store_true',
        help='Generates a template for the moduleDeps.cpp. This template will '
             'need to be filled in and kept up to date with library '
             'dependencies if you are not using the OpenUSD build system.')

    defaultTemplateDir = os.path.join(
        os.path.dirname(
            os.path.abspath(inspect.getfile(inspect.currentframe()))),
        'codegenTemplates')

    instTemplateDir = os.path.join(
        os.path.abspath(Plug.Registry().GetPluginWithName('usd').resourcePath),
        'codegenTemplates')

    parser.add_argument('-t', '--templates',
        dest='templatePath',
        type=str,
        help=('Directory containing schema class templates. '
              '[Default: first directory that exists from this list: {0}]'
              .format(os.pathsep.join([defaultTemplateDir, instTemplateDir]))))

    args = parser.parse_args()
    codeGenPath = os.path.abspath(args.codeGenPath)
    libraryName = os.path.basename(codeGenPath)

    if args.templatePath:
        templatePath = os.path.abspath(args.templatePath)
    else:
        if os.path.isdir(defaultTemplateDir):
            templatePath = defaultTemplateDir
        else:
            templatePath = instTemplateDir

    Print.SetQuiet(args.quiet)

    env = Environment(loader=FileSystemLoader(templatePath),
                            trim_blocks=True)

    # File names
    initFileName = '__init__.py'
    moduleFileName = 'module.cpp'
    schemaFileName = 'schema.usda'
    schemaDocFileName = 'schemaUserDoc.usda'
    cmakeFileName = 'CMakeLists.txt'
    genModuleFileName = 'generatedSchema.module.h'
    genClassesFileName = 'generatedSchema.classes.txt'
    moduleDepsFileName = 'moduleDeps.cpp'

    # Load templates
    Print('Loading Templates from {0}'.format(templatePath))
    try:
        initTemplate = env.get_template(initFileName)
        moduleTemplate = env.get_template(moduleFileName)
        schemaTemplate = env.get_template(schemaFileName)
        schemaDocTemplate = env.get_template(schemaDocFileName)
        cmakeTemplate = env.get_template(cmakeFileName)
        genModuleTemplate = env.get_template(genModuleFileName)
        genClassesTemplate = env.get_template(genClassesFileName)
        if args.genModuleDeps:
            genModuleDepsTemplate = env.get_template(moduleDepsFileName)
    except TemplateNotFound as tnf:
        raise RuntimeError("Template not found: {0}".format(str(tnf)))
    except TemplateSyntaxError as tse:
        raise RuntimeError("Syntax error in template {0} at line {1}: {2}"
                        .format(tse.filename, tse.lineno, tse))

    # Create directories for the codegen and user docs
    if not os.path.exists(os.path.dirname(codeGenPath)):
        os.makedirs(os.path.dirname(codeGenPath))

    userDocFolderPath = os.path.join(codeGenPath, 'userDoc')
    if not os.path.exists(userDocFolderPath):
        os.makedirs(userDocFolderPath)

    # Generate an __init__.py if not present
    initPath = os.path.join(codeGenPath, initFileName)
    _WriteFileIfNotPresent(initFileName, initPath, initTemplate.render(), args.validate)

    # Generate a schema.usda if not present
    schemaPath = os.path.join(codeGenPath, schemaFileName)
    _WriteFileIfNotPresent(schemaFileName, schemaPath,
                           schemaTemplate.render(libraryName=libraryName, 
                                                libraryPath=args.libraryPath), args.validate)

    # Generate a schema doc if not present
    schemaDocPath = os.path.join(userDocFolderPath, schemaDocFileName)
    _WriteFileIfNotPresent(schemaDocFileName, schemaDocPath, 
                           schemaDocTemplate.render(), args.validate)

    # Generate a module.cpp if not present
    filePath = os.path.join(codeGenPath, moduleFileName)
    if os.path.exists(filePath):
        # Check whether the module.cpp includes the generated module file
        with open(filePath, 'r') as f:
            foundModuleCpp = False
            for line in f.readlines():
                if "#include \"" + genModuleFileName + "\"" in line.strip():
                    foundModuleCpp = True
            
            if not foundModuleCpp:
                Print("WARNING:", genModuleFileName, "not included in "
                    "module.cpp. This should be included to avoid "
                    "listing entries manually.")
    else:        
        _WriteFile(filePath, moduleTemplate.render(), args.validate)

    # Generate a moduleDeps.cpp if requested and not present
    if args.genModuleDeps:
        moduleDepsPath = os.path.join(codeGenPath, moduleDepsFileName)
        _WriteFileIfNotPresent(moduleDepsFileName, moduleDepsPath, 
                               genModuleDepsTemplate.render(), args.validate)

    # Generate empty files to be filled in by usdGenSchema after the schema.usda
    # is filled in
    _WriteFileIfNotPresent(genModuleFileName, os.path.join(codeGenPath, 
                genModuleFileName), genModuleTemplate.render(), args.validate)
    _WriteFileIfNotPresent(genClassesFileName, os.path.join(codeGenPath, 
                genClassesFileName), genClassesTemplate.render(), args.validate)

    # Check whether the CMakeLists.txt call to pxr_library includes the 
    # INCLUDE_SCHEMA_FILES flag
    cmakePath = os.path.join(codeGenPath, cmakeFileName)
    if os.path.exists(cmakePath):
        with open(cmakePath, 'r') as f:
            if "INCLUDE_SCHEMA_FILES" not in f.read():
                Print("WARNING: INCLUDE_SCHEMA_FLAGS not present in pxr_library "
                      "in ", cmakeFileName, ". Pass this flag to pxr_library to "
                      "have schema classes automatically built.")
    else:
        _WriteFile(cmakePath, cmakeTemplate.render(pxrPrefix=args.pxrPrefix, 
                    libraryName=libraryName), args.validate)
    
