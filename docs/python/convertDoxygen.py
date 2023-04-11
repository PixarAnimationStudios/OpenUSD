#
# Copyright 2023 Pixar
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
# convertDoxygen.py
#
# A utility to convert Doxygen XML files into another format, such as 
# Python docstrings. 
#
# This utility is designed to make it easy to plug in new output
# formats. This is done by creating new cdWriterXXX.py modules. A
# writer module must provide a class called Writer with methods,
# getDocString() and generate(). See cdWriterDocstring.py for an example.
#

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from doxygenlib.cdParser import *
from doxygenlib.cdUtils import *

#
# Parse the global command line arguments (plugin may have more)
#
xml_file      = GetArgValue(['--input', '-i'])
xml_file_dir  = GetArgValue(['--inputDir'])
xml_index_file  = GetArgValue(['--inputIndex'])
output_file   = GetArgValue(['--output', '-o'])
output_format = GetArgValue(['--format', '-f'], "Docstring")
python_path = GetArgValue(['--pythonPath'])

SetDebugMode(GetArg(['--debug', '-d']))

if not (xml_file or xml_index_file) or not output_file or GetArg(['--help', '-h']):
    Usage()

#
# If caller specified an additional path for python libs (for loading USD 
# modules, for example) add the path to sys.path
#
if (python_path != None):
    sys.path.append(python_path)

#
# Try to import the plugin module that creates the desired output
#
if not Import("from doxygenlib.cdWriter"+output_format+" import Writer as Writer"):
    Error("No writer plugin exists for format '%s'" % output_format)

print("Converting Doxygen comments to %s format..." % output_format)

#
# Create a parser object to read the doxygen XML.
#
parser = Parser()

#
# Parse the XML file, generate the doc structures (the writer
# plugin formats the docs)
#
if xml_index_file != None:
    if not parser.parseDoxygenIndexFile(xml_index_file):
        Error("Could not parse XML index file: %s" % xml_index_file)
else:
    if not parser.parse(xml_file):
        Error("Could not parse XML file: %s" % xml_file)

#
# Traverse the list of DocElements from the parsed XML,
# load provided python module(s) and find matching python 
# entities, and write matches to python docs output
#
packageName = GetArgValue(['--package', '-p'])
modules = GetArgValue(['--module', '-m'])
if not packageName:
    Error("Required option --package not specified")
if not modules:
    Error("Required option --module not specified")
docList = None
# Check if a comma-separated list of modules was provided
if ',' in modules:
    # Processing multiple modules. Writer's constructor will verify
    # provided package and modules can be loaded
    moduleList = modules.split(",")
    # Loop through module list and create a Writer for each module to 
    # load and generate the doc strings for the specific module
    for moduleName in moduleList:
        writer = Writer(packageName, moduleName)
        # Parser.traverse builds the docElement tree for all the 
        # doxygen XML files, so we only need to call it once if we're
        # processing multiple modules
        if (docList == None):
            docList = parser.traverse(writer)
            Debug("Processed %d DocElements from doxygen XML" % len(docList))
        Debug("Processing module %s" % moduleName)
        # For multiple-module use-case, assume output_file is really an
        # output path for the parent dir (e.g. lib/python/pxr)
        module_output_dir = os.path.join(output_file, moduleName)
        module_output_file = os.path.join(module_output_dir, "__DOC.py")
        writer.generate(module_output_file, docList)
else:
    # Processing a single module. Writer's constructor will sanity
    # check module and verify it can be loaded
    writer = Writer()
    docList = parser.traverse(writer)
    writer.generate(output_file, docList)