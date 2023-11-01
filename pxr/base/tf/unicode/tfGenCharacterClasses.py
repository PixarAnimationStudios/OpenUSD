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
# character classes.  This takes a source UnicodeData.txt from the Unicode standard
# and generates C++ source files that populate unordered sets with the appropriate
# code points.
'''This script reads the DerivedCoreProperties.txt from a versioned set of Unicode
collateral and generates appropriate data structures for the XID_Start and XID_Continue
character classes used to process UTF-8 encoded strings in the Tf library.'''

import os

from argparse import ArgumentParser
from ctypes import c_uint

DERIVEDCOREPROPERTIES_FILE = "DerivedCoreProperties.txt"
CPP_FILE_NAME = "unicodeCharacterClasses.cpp"

CPP_FILE_HEADER = """//
// Copyright 2023 Pixar
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
#include "pxr/base/tf/unicodeCharacterClasses.h"
#include "pxr/base/work/loops.h"

"""

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
        generated_cpp_file.write("namespace TfUnicodeUtils {\n")
        generated_cpp_file.write("namespace Impl {\n\n")

        # generate the TfStaticData objects
        generated_cpp_file.write("TfStaticData<UnicodeXidStartRangeData> xidStartRangeData;\n")
        generated_cpp_file.write("TfStaticData<UnicodeXidContinueRangeData> xidContinueRangeData;\n")
        generated_cpp_file.write("TfStaticData<UnicodeXidStartFlagData> xidStartFlagData;\n")
        generated_cpp_file.write("TfStaticData<UnicodeXidContinueFlagData> xidContinueFlagData;\n\n")

        # generate the constructors
        generated_cpp_file.write("UnicodeRangeData::UnicodeRangeData()\n")
        generated_cpp_file.write("{\n")
        generated_cpp_file.write("}\n\n")

        generated_cpp_file.write("UnicodeXidStartRangeData::UnicodeXidStartRangeData()\n")
        generated_cpp_file.write("{\n")
        generated_cpp_file.write("    ranges = {\n")

        for x in xid_start_range_pairs:
            range_expression = "{" + str(x[0]) + ", " + str(x[1]) + "}"
            generated_cpp_file.write(f"        {range_expression},\n")

        generated_cpp_file.write("    };\n")
        generated_cpp_file.write("}\n\n")

        generated_cpp_file.write("UnicodeXidContinueRangeData::UnicodeXidContinueRangeData()\n")
        generated_cpp_file.write("{\n")
        generated_cpp_file.write("    ranges = {\n")
        
        for x in xid_continue_range_pairs:
            range_expression = "{" + str(x[0]) + ", " + str(x[1]) + "}"
            generated_cpp_file.write(f"        {range_expression},\n")

        generated_cpp_file.write("    };\n")
        generated_cpp_file.write("}\n\n")

        generated_cpp_file.write("UnicodeFlagData::UnicodeFlagData()\n")
        generated_cpp_file.write("{\n")
        generated_cpp_file.write("}\n\n")

        generated_cpp_file.write("UnicodeXidStartFlagData::UnicodeXidStartFlagData()\n")
        generated_cpp_file.write("{\n")
        generated_cpp_file.write("    // set all of the bits corresponding to the code points in the range\n")
        generated_cpp_file.write("    WorkParallelForEach(xidStartRangeData->ranges.begin(), xidStartRangeData->ranges.end(),\n")
        generated_cpp_file.write("        [this](const std::pair<uint32_t, uint32_t>& entry) {\n")
        generated_cpp_file.write("            for(uint32_t i = entry.first; i <= entry.second; i++) {\n")
        generated_cpp_file.write("                this->flags[static_cast<size_t>(i)] = true;\n")
        generated_cpp_file.write("            }\n")
        generated_cpp_file.write("        });\n")
        generated_cpp_file.write("}\n\n")

        generated_cpp_file.write("UnicodeXidContinueFlagData::UnicodeXidContinueFlagData()\n")
        generated_cpp_file.write("{\n")
        generated_cpp_file.write("    // set all of the bits corresponding to the code points in the range\n")
        generated_cpp_file.write("    WorkParallelForEach(xidContinueRangeData->ranges.begin(), xidContinueRangeData->ranges.end(),\n")
        generated_cpp_file.write("        [this](const std::pair<uint32_t, uint32_t>& entry) {\n")
        generated_cpp_file.write("            for(uint32_t i = entry.first; i <= entry.second; i++) {\n")
        generated_cpp_file.write("                this->flags[static_cast<size_t>(i)] = true;\n")
        generated_cpp_file.write("            }\n")
        generated_cpp_file.write("        });\n")
        generated_cpp_file.write("}\n\n")

        # close the namespace
        generated_cpp_file.write("} // namespace Impl\n")
        generated_cpp_file.write("} // namespace TfUnicodeUtils\n\n")
        generated_cpp_file.write("PXR_NAMESPACE_CLOSE_SCOPE\n")

def _parseArguments():
    """
    Parses the arguments sent to the script.

    Returns:
        An object containing the parsed arguments as accessible fields.
    """
    parser = ArgumentParser(description='Generate character class sets for Unicode characters.')
    parser.add_argument('--srcDir', required=False, default=os.getcwd(),
                        help='The source directory where the DerivedCoreProperties.txt file exists.')
    parser.add_argument('--destDir', required=False, default=os.getcwd(),
                        help='The destination directory where the processed cpp file will be written to.')

    return parser.parse_args()

if __name__ == '__main__':
    arguments = _parseArguments()

    # parse the DerivedCoreProperties.txt file
    # sections of that file contain the derived properties XID_Start and XID_Continue
    # based on the allowed character classes and code points sourced from UnicodeData.txt
    # each line in the file that we are interested in is of one of two forms:
    # codePoint                                 ; XID_Start # character class Character Name
    # codePointRangeStart..codePointRangeEnd    ; XID_Start # character class [# of elements in range] Character Name 
    file_name = os.path.join(arguments.srcDir, DERIVEDCOREPROPERTIES_FILE)
    if not os.path.exists(file_name):
        raise RuntimeError(f"Error in script: Could not find 'DerivedCoreProperties.txt' at path {arguments.srcDir}!")

    with open(file_name, 'r') as file:
        seen_underscore_xid_start = False
        for line in file:
            if "; XID_Start" in line:
                # this is an XID_Start single code point or range
                tokens = line.split(';')
                code_points = tokens[0].strip()
                if ".." in code_points:
                    # this is a ranged code point
                    code_point_ranges = code_points.split("..")
                    start_code_point = int(code_point_ranges[0], 16)
                    end_code_point = int(code_point_ranges[1], 16)
                else:
                    # this is a single code point
                    start_code_point = int(code_points, 16)
                    end_code_point = start_code_point

                # special case for underscore, which is not in XID_Start
                # but which is allowed in C++ / Python identifiers
                if not seen_underscore_xid_start:
                    if start_code_point > 95:
                        # we are at a code point range that is bigger than
                        # the code point value of '_', so we add
                        # underscore here
                        xid_start_range_pairs.append((95, 95))
                        seen_underscore_xid_start = True

                xid_start_range_pairs.append((start_code_point, end_code_point))
            elif "; XID_Continue" in line:
                # this is an XID_Continue single code point or range
                tokens = line.split(';')
                code_points = tokens[0].strip()
                if ".." in code_points:
                    # this is a ranged code point
                    code_point_ranges = code_points.split("..")
                    start_code_point = int(code_point_ranges[0], 16)
                    end_code_point = int(code_point_ranges[1], 16)
                else:
                    # this is a single code point
                    start_code_point = int(code_points, 16)
                    end_code_point = start_code_point

                xid_continue_range_pairs.append((start_code_point, end_code_point))

    _write_cpp_files(arguments.destDir)
