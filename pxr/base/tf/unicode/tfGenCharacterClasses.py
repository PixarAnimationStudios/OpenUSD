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
# A script for generating the character class sets for XID_Start and XID_Continue
# character classes.  This takes a source UnicodeDatabase.txt from the Unicode standard
# and generates C++ source files that populate unordered sets with the appropriate
# code points.

import os

from argparse import ArgumentParser
from ctypes import c_uint

UNICODE_DATABASE_FILE = "UnicodeDatabase.txt"
SPECIAL_CASES_FILE = "SpecialCasing.txt"
CPP_FILE_NAME = "unicodeCharacterClasses.cpp"
XID_START_CLASS = ["Lu", "Ll", "Lt", "Lm", "Lo", "Nl"]
XID_CONTINUE_CLASS = ["Nd", "Mn", "Mc", "Pc"]

CPP_FILE_HEADER = """
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
"""

INCLUDE_HEADERS = """
#include "pxr/pxr.h"
#include "pxr/base/tf/unicodeUtils.h"

#include <unordered_set>
#include <vector>
#include <utility>

"""

xid_start_class = []
xid_continue_class = []
xid_start_range_pairs = []
xid_continue_range_pairs = []

def _write_cpp_files(destination_directory : str):
    """
    Writes the C++ code file that will initialize character class
    sets with the values read by this script.

    Args:
        destination_directory: A string defining the path at which the generated cpp file will be written to.
                               If the specified directory does not exist, it will be created.
    """
    if not os.path.exists(destination_directory):
        os.mkdir(destination_directory)

    generated_cpp_file_name = os.path.join(destination_directory, CPP_FILE_NAME)
    with open(generated_cpp_file_name, 'w') as generated_cpp_file:
        # write the header comment
        generated_cpp_file.write(CPP_FILE_HEADER)

        # write includes
        generated_cpp_file.write(INCLUDE_HEADERS)

        # open the namespace
        generated_cpp_file.write("PXR_NAMESPACE_OPEN_SCOPE\n\n")

        # generate the sets
        generated_cpp_file.write("std::unordered_set<uint32_t> TfUnicodeUtils::xidStartClass = {")
        generated_cpp_file.write(','.join(xid_start_class))
        generated_cpp_file.write("};\n\n")
        generated_cpp_file.write("std::unordered_set<uint32_t> TfUnicodeUtils::xidContinueClass = {")
        generated_cpp_file.write(','.join(xid_continue_class))
        generated_cpp_file.write("};\n\n")
        generated_cpp_file.write("std::vector<std::pair<uint32_t, uint32_t>> TfUnicodeUtils::xidStartRangeClass = {")
        if len(xid_start_range_pairs) > 0:
            generated_cpp_file.write(','.join('{' + x[0] + ',' + x[1] + '}' for x in xid_start_range_pairs))
        generated_cpp_file.write("};\n\n")
        generated_cpp_file.write("std::vector<std::pair<uint32_t, uint32_t>> TfUnicodeUtils::xidContinueRangeClass = {")
        if len(xid_continue_range_pairs) > 0:
            generated_cpp_file.write(','.join('{' + x[0] + ',' + x[1] + '}' for x in xid_continue_range_pairs))
        generated_cpp_file.write("};\n\n")

        # close the namespace
        generated_cpp_file.write("PXR_NAMESPACE_CLOSE_SCOPE\n")

def _parseArguments():
    """
    Parses the arguments sent to the script.

    Returns:
        An object containing the parsed arguments as accessible fields.
    """
    parser = ArgumentParser(description='Generate character class sets for Unicode characters.')
    parser.add_argument('--srcDir', required=False, default=os.getcwd(),
                        help='The source directory where the UnicodeDatabase.txt file exists.')
    parser.add_argument('--destDir', required=False, default=os.getcwd(),
                        help='The destination directory where the processed cpp file will be written to.')

    return parser.parse_args()

if __name__ == '__main__':
    # read in the UnicodeDatabase.txt file
    arguments = _parseArguments()

    # parse the UnicodeDatabase.txt file
    # each line represents a single code point with information about the character
    # represented by that code point
    # for now we are only interested in the code point itself and
    # the character class, which reside in columns 0 and 2 respectively
    unicode_database_file_name = os.path.join(arguments.srcDir, UNICODE_DATABASE_FILE)
    if not os.path.exists(unicode_database_file_name):
        raise RuntimeError(f"Error in script: Could not find 'UnicodeDatabase.txt' at path {arguments.srcDir}!")

    # the UnicodeDatabase is the primary source of mappings
    # this will give us the character classes as well as:
    # (some) case mapping information (the rest is in SpecialCasing.txt)
    first_pair = None
    with open(unicode_database_file_name, 'r') as unicode_database_file:
       for line in unicode_database_file:
            # split the line 
            tokens = line.split(';')
            code_point = int(tokens[0], 16)
            character_name = tokens[1]
            character_class = tokens[2]
            if '<' in character_name:
                # this is an indication that the character is a group of characters
                # that fall in a range of code points that all have the same character class
                # with more specific properties given elsewhere
                # we don't need those, but we do need to account for ranges
                # the first part of the range is always before the last part in the UnicodeDatabase.txt file
                # and are always separated by a single line, so we can track it very simply
                if ', First' in character_name:
                    # it's the first character in the range
                    first_pair = str(code_point)
                elif ', Last' in character_name:
                    # it's the second character in the range
                    if character_class in XID_START_CLASS:
                        xid_start_range_pairs.append((first_pair, str(code_point)))
                    elif character_class in XID_CONTINUE_CLASS: 
                        xid_continue_range_pairs.append((first_pair, str(code_point)))

                    first_pair = None
            else:
                if character_class in XID_START_CLASS:
                    xid_start_class.append(str(code_point))
                elif character_class in XID_CONTINUE_CLASS:
                    xid_continue_class.append(str(code_point))
            
            if code_point == 95:
                # special case is underscore, which we will add to the XID_Start class because
                # C++ / Python allow it specifically (it's a separate if because it's part of
                # the "Pc" class, meaning it is considered XID_Continue)
                xid_start_class.append(str(code_point))

    _write_cpp_files(arguments.destDir)