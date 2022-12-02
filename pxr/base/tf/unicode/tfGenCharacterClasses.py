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
BLOCK_FILE = "Blocks.txt"
SPECIAL_CASES_FILE = "SpecialCasing.txt"
ALLKEYS_FILE = "allkeys.txt"
CPP_FILE_NAME = "unicodeCharacterClasses.cpp"
XID_START_CLASS = ["Lu", "Ll", "Lt", "Lm", "Lo", "Nl"]
XID_CONTINUE_CLASS = ["Nd", "Mn", "Mc", "Pc"]
TANGUT = "Tangut"
TANGUT_COMPONENTS = "Tangut Components"
TANGUT_SUPPLEMENT = "Tangut Supplement"
NUSHU = "Nushu"
KHITAN_SMALL_SCRIPT = "Khitan Small Script"
CJK_UNIFIED_IDEOGRAPHS = "CJK Unified Ideographs"
CJK_COMPATIBILITY_IDEOGRAPHS = "CJK Compatibility Ideographs"
CJK_UNIFIED_IDEOGRAPHS_A = "CJK Unified Ideographs Extension A"
CJK_UNIFIED_IDEOGRAPHS_B = "CJK Unified Ideographs Extension B"
CJK_UNIFIED_IDEOGRAPHS_C = "CJK Unified Ideographs Extension C"
CJK_UNIFIED_IDEOGRAPHS_D = "CJK Unified Ideographs Extension D"
CJK_UNIFIED_IDEOGRAPHS_E = "CJK Unified Ideographs Extension E"
CJK_UNIFIED_IDEOGRAPHS_F = "CJK Unified Ideographs Extension F"
CJK_UNIFIED_IDEOGRAPHS_G = "CJK Unified Ideographs Extension G"

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
#include <unordered_map>
#include <vector>
#include <utility>

"""

xid_start_class = []
xid_continue_class = []
xid_start_range_pairs = []
xid_continue_range_pairs = []
tangut_code_points = None
tangut_component_code_points = None
tangut_supplement_code_points = None
nushu_code_points = None
khitan_small_script_code_points = None
cjk_unified_ideographs_code_points = None
cjk_compatibility_ideographs_code_points = None
cjk_unified_ideographs_a_code_points = None
cjk_unified_ideographs_b_code_points = None
cjk_unified_ideographs_c_code_points = None
cjk_unified_ideographs_d_code_points = None
cjk_unified_ideographs_e_code_points = None
cjk_unified_ideographs_f_code_points = None
cjk_unified_ideographs_g_code_points = None
lower_case_mappings = {}
upper_case_mappings = {}
lower_case_multi_mappings = {}
upper_case_multi_mappings = {}
canonical_combining_class_mappings = {}
decomposition_mappings = {}
ducet_mapping = []

def _write_cpp_files(destination_directory):
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

        # generate the block ranges
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::tangutCodePoints = std::make_pair(")
        generated_cpp_file.write(tangut_code_points[0] + ', ' + tangut_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::tangutComponentCodePoints = std::make_pair(")
        generated_cpp_file.write(tangut_component_code_points[0] + ', ' + tangut_component_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::tangutSupplementCodePoints = std::make_pair(")
        generated_cpp_file.write(tangut_supplement_code_points[0] + ', ' + tangut_supplement_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::nushuCodePoints = std::make_pair(")
        generated_cpp_file.write(nushu_code_points[0] + ', ' + nushu_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::khitanSmallScriptCodePoints = std::make_pair(")
        generated_cpp_file.write(khitan_small_script_code_points[0] + ', ' + khitan_small_script_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsCodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_code_points[0] + ', ' + cjk_unified_ideographs_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkCompatibilityIdeographsCodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_compatibility_ideographs_code_points[0] + ', ' + cjk_compatibility_ideographs_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsACodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_a_code_points[0] + ', ' + cjk_unified_ideographs_a_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsBCodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_b_code_points[0] + ', ' + cjk_unified_ideographs_b_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsCCodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_c_code_points[0] + ', ' + cjk_unified_ideographs_c_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsDCodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_d_code_points[0] + ', ' + cjk_unified_ideographs_d_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsECodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_e_code_points[0] + ', ' + cjk_unified_ideographs_e_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsFCodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_f_code_points[0] + ', ' + cjk_unified_ideographs_f_code_points[1])
        generated_cpp_file.write(");\n\n")
        generated_cpp_file.write("std::pair<uint32_t, uint32_t> TfUnicodeUtils::cjkUnifiedIdeographsGCodePoints = std::make_pair(")
        generated_cpp_file.write(cjk_unified_ideographs_g_code_points[0] + ', ' + cjk_unified_ideographs_g_code_points[1])
        generated_cpp_file.write(");\n\n")

        # generate the case mappings
        generated_cpp_file.write("std::unordered_map<uint32_t, uint32_t> TfUnicodeUtils::unicodeCaseMapUpperToLower = {")
        generated_cpp_file.write(','.join('{' + code_point + ', ' + lower_case_mappings[code_point] + '}' for code_point in lower_case_mappings))
        generated_cpp_file.write("};\n\n")
        generated_cpp_file.write("std::unordered_map<uint32_t, uint32_t> TfUnicodeUtils::unicodeCaseMapLowerToUpper = {")
        generated_cpp_file.write(','.join('{' + code_point + ', ' + upper_case_mappings[code_point] + '}' for code_point in upper_case_mappings))
        generated_cpp_file.write("};\n\n")
        generated_cpp_file.write("std::unordered_map<uint32_t, std::vector<uint32_t>> TfUnicodeUtils::unicodeCaseMapUpperToMultiLower = {")
        generated_cpp_file.write(','.join('{' + code_point + ', {' + ','.join(x for x in lower_case_multi_mappings[code_point]) + '} }' for code_point in lower_case_multi_mappings))
        generated_cpp_file.write("};\n\n")
        generated_cpp_file.write("std::unordered_map<uint32_t, std::vector<uint32_t>> TfUnicodeUtils::unicodeCaseMapLowerToMultiUpper = {")
        generated_cpp_file.write(','.join('{' + code_point + ', {' + ','.join(x for x in upper_case_multi_mappings[code_point]) + '} }' for code_point in upper_case_multi_mappings))
        generated_cpp_file.write("};\n\n")

        # generate canonical combining classes
        generated_cpp_file.write("std::unordered_map<uint32_t, uint16_t> TfUnicodeUtils::unicodeCanonicalCombiningClass = {")
        generated_cpp_file.write(','.join('{' + code_point + ',' + canonical_combining_class_mappings[code_point] + '}' for code_point in canonical_combining_class_mappings))
        generated_cpp_file.write("};\n\n")

        # generated normalization decomposition mappings
        generated_cpp_file.write("std::unordered_map<uint32_t, std::vector<uint32_t>> TfUnicodeUtils::unicodeDecompositionMapping = {")
        generated_cpp_file.write(','.join('{' + code_point + ', {' + ','.join(x for x in decomposition_mappings[code_point]) + '} }' for code_point in decomposition_mappings))
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
        raise RuntimeError("Error in script: Could not find 'UnicodeDatabase.txt' at path %s!" % (arguments.srcDir))

    # the UnicodeDatabase is the primary source of mappings
    # this will give us the character classes as well as:
    # (some) case mapping information (the rest is in SpecialCasing.txt)
    # canonical combining class values
    # character decomposition maps for normalization
    first_pair = None
    with open(unicode_database_file_name, 'r') as unicode_database_file:
       for line in unicode_database_file:
            # split the line 
            tokens = line.split(';')
            code_point = int(tokens[0], 16)
            character_name = tokens[1];
            character_class = tokens[2]
            canonical_combining_class = tokens[3]
            character_decomposition_mapping = tokens[5].strip()
            lowercase_mapping = tokens[13]
            uppercase_mapping = tokens[12]
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

            # now process case mappings
            # note that the mappings in this file are all single
            # the special case mappings that expand / contract are
            # processed later in when we read that file
            if lowercase_mapping != "":
                # this has a lower case mapping
                lower_case_mappings.update({str(code_point): str(int(lowercase_mapping, 16))})

            if uppercase_mapping != "":
                # this has an upper case mapping
                upper_case_mappings.update({str(code_point): str(int(uppercase_mapping, 16))})

            # process canonical combining classes (but only if it's a non-zero value)
            if canonical_combining_class != "0":
                canonical_combining_class_mappings.update({str(code_point): canonical_combining_class})

            # and finally the decomposition
            if character_decomposition_mapping != "" and '<' not in character_decomposition_mapping:
                # the presence of a <tag> indicates its a compatibility mapping, not a canonical mapping
                # but we only handle canonical mappings
                if ' ' in character_decomposition_mapping:
                    # multi-decomposition
                    character_decomposition = []
                    decomposition_tokens = character_decomposition_mapping.split(' ')
                    for decomposition_token in decomposition_tokens:
                        character_decomposition.append(str(int(decomposition_token, 16)))
                    decomposition_mappings.update({str(code_point): character_decomposition})
                else:
                    # single decomposition
                    decomposition_mappings.update({str(code_point): [str(int(character_decomposition_mapping, 16))]})

    block_file_name = os.path.join(arguments.srcDir, BLOCK_FILE)
    if not os.path.exists(block_file_name):
        raise RuntimeError("Error in script: Could not find 'Blocks.txt' at path %s!" % (arguments.srcDir))

    with open(block_file_name, 'r') as unicode_block_file:
        for line in unicode_block_file:
            
            # we only care about the lines formatted as:
            # x..y; block name
            if '..' in line and not line.startswith('#'):
                tokens = line.split(';')
                ranges = tokens[0].split('..')
                if TANGUT_COMPONENTS in line:
                    tangut_component_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif TANGUT_SUPPLEMENT in line:
                    tangut_supplement_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif TANGUT in line:
                    tangut_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif NUSHU in line:
                    nushu_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif KHITAN_SMALL_SCRIPT in line:
                    khitan_small_script_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS_A in line:
                    cjk_unified_ideographs_a_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS_B in line:
                    cjk_unified_ideographs_b_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS_C in line:
                    cjk_unified_ideographs_c_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS_D in line:
                    cjk_unified_ideographs_d_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS_E in line:
                    cjk_unified_ideographs_e_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS_F in line:
                    cjk_unified_ideographs_f_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS_G in line:
                    cjk_unified_ideographs_g_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_UNIFIED_IDEOGRAPHS in line:
                    cjk_unified_ideographs_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))
                elif CJK_COMPATIBILITY_IDEOGRAPHS in line:
                    cjk_compatibility_ideographs_code_points = (str(int(ranges[0], 16)), str(int(ranges[1], 16)))

    special_cases_file_name = os.path.join(arguments.srcDir, SPECIAL_CASES_FILE)
    if not os.path.exists(special_cases_file_name):
        raise RuntimeError("Error in script: Could not find 'SpecialCases.txt' at path %s!" % (arguments.srcDir))

    # now we have to adjust some of our mappings
    # specifically this file will give us a multi char map in certain cases
    # when these cases are encountered, we remove them from the original map
    # and add to a special case map - but we only process up until
    # the Conditional mapping section, which is language / context specific
    # and which we cannot support
    with open(special_cases_file_name, 'r') as unicode_special_cases_file:
        for line in unicode_special_cases_file:
            
            if 'Conditional' in line:
                break

            if ';' in line and not line.startswith('#'):
                tokens = line.split(';')
                for token in tokens:
                    token = token.strip()

                # most are the same as the code point itself, meaning
                # no real lower case mapping, but the upper case mapping
                # can be multiple code points
                code_point = str(int(tokens[0], 16))
                lower_case = tokens[1].strip()
                upper_case = tokens[3].strip()

                # handle lower case first, but only if there really is a mapping
                if ' ' in lower_case:
                    lower_case_tokens = lower_case.split(' ')
                    final_tokens = []
                    for lower_case_token in lower_case_tokens:
                        final_tokens.append(str(int(lower_case_token, 16)))

                    lower_case_multi_mappings.update({code_point: final_tokens})
                else:
                    lower_case = str(int(lower_case, 16))
                    if code_point != lower_case:
                        # if we already have a mapping replace it
                        lower_case_mappings.update({code_point: lower_case})

                # now upper case - most are multi-mappings, but some are single
                if ' ' in upper_case:
                    # multi-mapping (note, we can't go backward because we have no context)
                    # that is, consider the German es-zed, which maps to SS upper case, but
                    # those are normal latin S code points, meaning when going to lower case
                    # we have no real way (independent of the current locale) to understand
                    # whether we need to go to ss or es-zed
                    upper_case_tokens = upper_case.split(' ')
                    final_tokens = []
                    for upper_case_token in upper_case_tokens:
                        final_tokens.append(str(int(upper_case_token, 16)))

                    upper_case_multi_mappings.update({code_point: final_tokens})
                else:
                    # single mapping
                    upper_case = str(int(upper_case, 16))
                    if code_point != upper_case:
                        upper_case_mappings.update({code_point: str(int(upper_case, 16))})

    # finally, process the DUCET table from allkeys.txt
    all_keys_file_name = os.path.join(arguments.srcDir, ALLKEYS_FILE)
    if not os.path.exists(all_keys_file_name):
        raise RuntimeError("Error in script: Could not find 'allkeys.txt' at path %s!" % (arguments.srcDir))

    with open(all_keys_file_name, 'r') as unicode_allkeys_file:
        for line in unicode_allkeys_file:
            if ';' in line and not line.startswith('#') and not line.startswith('@'):
                tokens = line.split(';')
                unicode_substring = tokens[0].strip()
                substring_elements = []
                if ' ' in unicode_substring:
                    # multi-character mapping
                    substring_tokens = unicode_substring.split(' ')
                    for substring_token in substring_tokens:
                        substring_elements.append(int(substring_token, 16))
                else:
                    # single-character mapping
                    substring_elements.append(int(unicode_substring, 16))

                # isolate each of the [.0000.0000.0000]
                collation_element_table = []
                hash_position = tokens[1].find('#')
                tokens[1] = tokens[1][0:hash_position].strip()
                left_brace_index = tokens[1].find('[')
                while left_brace_index != -1:
                    # get corresponding ']'
                    right_brace_index = tokens[1].find(']')

                    # inside the brace set is something of the form
                    # [.xxxx.xxxx.xxxx] or
                    # [*xxxx.xxxx.xxxx]
                    collation_elements = tokens[1][left_brace_index:right_brace_index]
                    collation_elements = collation_elements.replace('[', '')
                    collation_elements = collation_elements.replace(']', '')
                    collation_elements = collation_elements[1:]
                    collation_elements = collation_elements.split('.')

                    # now we have three weights, append the 3-tuple
                    weight_0 = str(int(collation_elements[0], 16))
                    weight_1 = str(int(collation_elements[1], 16))
                    weight_2 = str(int(collation_elements[2], 16))
                    collation_element_table.append((weight_0, weight_1, weight_2))

                    tokens[1] = tokens[1][right_brace_index + 1:]
                    left_brace_index = tokens[1].find('[')

                # store the collation element table
                ducet_mapping.append((substring_elements, collation_element_table))


    _write_cpp_files(arguments.destDir)