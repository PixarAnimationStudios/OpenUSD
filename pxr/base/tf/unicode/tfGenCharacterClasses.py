#!/usr/bin/env python
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# A script for generating the character class sets for XID_Start and
# XID_Continue character classes.  This takes a source UnicodeData.txt
# from the Unicode standard and generates C++ source files that populate
# data structures with the appropriate code points.
'''This script reads the DerivedCoreProperties.txt from a versioned set of
Unicode collateral and generates appropriate data structures for the XID_Start
and XID_Continue character classes used to process UTF-8 encoded strings in
the Tf library.'''

import os

from argparse import ArgumentParser

DERIVEDCOREPROPERTIES_FILE = "DerivedCoreProperties.txt"
TEMPLATE_FILE_NAME = "unicodeCharacterClasses.template.cpp"
CPP_FILE_NAME = "unicodeCharacterClasses.cpp"

xid_start_range_pairs = []
xid_continue_range_pairs = []

def _write_cpp_file(source_template_path : str, destination_directory : str):
    """
    Writes the C++ code file that will initialize character class
    sets with the values read by this script.
    Args:
        source_template_path : A string defining the path at which the source
                               template file exists.
        destination_directory: A string defining the path at which the 
                               generated cpp file will be written to.
                               If the specified directory does not exist,
                               it will be created.
    """
    if not os.path.exists(source_template_path):
        raise ValueError(f"Provided source template file \
{source_template_path} does not exist!")

    source_template_content = None
    with open(source_template_path, 'r') as source_template_file:
        source_template_content = source_template_file.read()

    if not os.path.exists(destination_directory):
        os.mkdir(destination_directory)

    generated_cpp_file_name = os.path.join(destination_directory,
                                           CPP_FILE_NAME)
    with open(generated_cpp_file_name, 'w') as generated_cpp_file:
        # we need to replace markers {xid_start_ranges} and
        # {xid_continue_ranges} (along with their sizes) with the content we
        # derived from DerivedCoreProperties.txt
        xid_start_range_expression = "\n".join(
            "        {{{}, {}}},".format(str(x[0]), str(x[1]))
            for x in xid_start_range_pairs)

        xid_continue_range_expression = "\n".join(
            "        {{{}, {}}},".format(str(x[0]), str(x[1]))
            for x in xid_continue_range_pairs)

        destination_template_content = source_template_content.replace(
            r"{xid_start_ranges}", xid_start_range_expression)
        destination_template_content = destination_template_content.replace(
            r"{xid_continue_ranges}", xid_continue_range_expression)
        destination_template_content = destination_template_content.replace(
            r"{xid_start_ranges_size}", str(len(xid_start_range_pairs)))
        destination_template_content = destination_template_content.replace(
            r"{xid_continue_ranges_size}", str(len(xid_continue_range_pairs)))

        generated_cpp_file.write(destination_template_content)

def _parseArguments():
    """
    Parses the arguments sent to the script.
    Returns:
        An object containing the parsed arguments as accessible fields.
    """
    parser = ArgumentParser(
        description='Generate character class sets for Unicode characters.')
    parser.add_argument('--srcDir', required=False, default=os.getcwd(),
        help='The source directory where the DerivedCoreProperties.txt \
file exists.')
    parser.add_argument('--destDir', required=False, default=os.getcwd(),
        help='The destination directory where the processed cpp file will \
be written to.')
    parser.add_argument("--srcTemplate", required=False, 
        default=os.path.join(os.getcwd(), TEMPLATE_FILE_NAME),
        help='The full path to the source template file to use.')

    return parser.parse_args()

if __name__ == '__main__':
    arguments = _parseArguments()

    # parse the DerivedCoreProperties.txt file
    # sections of that file contain the derived properties XID_Start
    # and XID_Continue based on the allowed character classes and code points
    # sourced from UnicodeData.txt each line in the file that we are interested
    # in is of one of two forms:
    # codePoint ; XID_Start # character class Character Name
    # codePointRangeStart..codePointRangeEnd ; XID_Start 
    # # character class [# of elements in range] Character Name 
    file_name = os.path.join(arguments.srcDir, DERIVEDCOREPROPERTIES_FILE)
    if not os.path.exists(file_name):
        raise RuntimeError(f"Error in script: Could not find \
'DerivedCoreProperties.txt' at path {arguments.srcDir}!")

    with open(file_name, 'r') as file:
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

                xid_start_range_pairs.append((start_code_point, 
                                              end_code_point))
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

                xid_continue_range_pairs.append((start_code_point, 
                                                 end_code_point))

    _write_cpp_file(arguments.srcTemplate, arguments.destDir)