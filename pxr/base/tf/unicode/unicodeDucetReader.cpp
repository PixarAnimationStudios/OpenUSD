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
# A script for generating the code point element caches for Unicode collation.

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

#include "unicodeDucetCommon.h"

std::string licenseBlock = "//\n"
"// Licensed under the Apache License, Version 2.0 (the \"Apache License\")\n"
"// with the following modification; you may not use this file except in\n"
"// compliance with the Apache License and the following modification to it:\n"
"// Section 6. Trademarks. is deleted and replaced with:\n"
"//\n"
"// 6. Trademarks. This License does not grant permission to use the trade\n"
"//    names, trademarks, service marks, or product names of the Licensor\n"
"//    and its affiliates, except as required to comply with Section 4(c) of\n"
"//    the License and to reproduce the content of the NOTICE file.\n"
"//\n"
"// You may obtain a copy of the Apache License at\n"
"//\n"
"//     http://www.apache.org/licenses/LICENSE-2.0\n"
"//\n"
"// Unless required by applicable law or agreed to in writing, software\n"
"// distributed under the Apache License with the above modification is\n"
"// distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY\n"
"// KIND, either express or implied. See the Apache License for the specific\n"
"// language governing permissions and limitations under the Apache License.\n"
"//\n";

///
/// Computes a single hash value from a set of individual code points. 
/// 
/// This algorithm is taken from https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
/// 
uint32_t ComputeHash(const std::vector<uint32_t>& codePoints)
{
    uint32_t hash = codePoints.size();
    for (auto codePoint : codePoints)
    {
        codePoint = ((codePoint >> 16) ^ codePoint) * 0x45d9f3b;
        codePoint = ((codePoint >> 16) ^ codePoint) * 0x45d9f3b;
        codePoint = (codePoint >> 16) ^ codePoint;
        hash ^= codePoint + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    return hash;
}

int main()
{
    std::string unicodeDucetFilename = "allkeys.txt";
    std::ifstream fileStream(unicodeDucetFilename, std::ios::in);
    if (!fileStream.is_open())
    {
        std::cerr << "File 'allkeys.txt' could not be opened!" << std::endl;
        return 1;
    }
 
    size_t multiCodePointSingleCollationElementCount = 0;
    size_t singleCodePointMultiCollationElementCount = 0;
    size_t multiCodePointMultiCollationElementCount = 0;
    std::uint32_t BLOCK_SIZE = 1024;
    std::uint32_t MULTI_BLOCK_SIZE = 512;
    std::string line;
    std::getline(fileStream, line);
    std::map<uint16_t, std::map<uint32_t, uint64_t>*> blockMap;
    std::map<uint32_t, std::vector<uint64_t>> unicodeDucetMultipleMapping;
    std::map<uint16_t, std::map<uint32_t, std::vector<uint64_t>>*> multiMap;
    while (!fileStream.eof())
    {
        // trim the whitespace from the line
        line = Trim(line);

        // skip empty lines
        if (line.length() == 0)
        {
            std::getline(fileStream, line);
            continue;
        }

        // skip commented lines and lines with @ directives
        if (line.length() > 0 && (line[0] == '#' || line[0] == '@'))
        {
            std::cout << line << std::endl;
            std::getline(fileStream, line);
            continue;
        }

        // all other lines should have the form <codePoint ; collationElements comment>
        size_t commentIndex = line.find_first_of('#');
        if (commentIndex == std::string::npos)
        {
            std::cerr << "Could not process DUCET table, unrecognized line format!" << std::endl;
            fileStream.close();
            return -1;
        }

        // remove the comment
        line = line.substr(0, commentIndex);

        // split the line on ';'
        size_t semiColonIndex = line.find_first_of(';');
        if (semiColonIndex == std::string::npos)
        {
            std::cerr << "Could not process DUCET table, unrecognized line format!" << std::endl;
            fileStream.close();
            return -1;
        }

        // is it a single code point or multiple code points?
        std::string codePointString = Trim(line.substr(0, semiColonIndex));
        std::string collationElementString = Trim(line.substr(semiColonIndex + 1, line.length() - semiColonIndex + 1));
        if (codePointString.find_first_of(' ') != std::string::npos)
        {
            // there is whitespace in the middle, so this is a multi-code point mapping
            std::vector<std::string> codePointStrings = Split(codePointString, ' ');
            std::vector<uint32_t> codePoints;
            for (std::vector<std::string>::iterator it = codePointStrings.begin(); it != codePointStrings.end(); it++)
            {
                codePoints.push_back(static_cast<uint32_t>(std::stoul(*it, nullptr, 16)));
            }

            // no matter how many collation elements we have here, this goes
            // in our multi-mapping table because we already have a multi-code point mapping
            std::vector<uint64_t> collationElements = ExtractCollationElements(collationElementString);
            uint32_t codePointHash = ComputeHash(codePoints);

            uint16_t multiBucket = static_cast<uint16_t>(codePointHash / MULTI_BLOCK_SIZE);
            if (multiMap.find(multiBucket) == multiMap.end())
            {
                multiMap[multiBucket] = new std::map<uint32_t, std::vector<uint64_t>>();
            }

            multiMap[multiBucket]->operator[](codePointHash) = collationElements;

            if (collationElements.size() > 1)
            {
                multiCodePointMultiCollationElementCount++;
            }
            else
            {
                multiCodePointSingleCollationElementCount++;
            }
        }
        else
        {
            // single code point mapping, turn it into a 32-bit unsigned integer
            uint32_t codePoint = static_cast<uint32_t>(std::stoul(codePointString, nullptr, 16));

            // collation element string represents one or more collation elements, each of which can
            // be represented as a 32-bit unsigned integer with 16-bits representing the primary weight,
            // 16-bits representing the secondary weight and 8-bitsreperesnting the tertiary weights
            // need to extract one or more of them
            std::vector<uint64_t> collationElements = ExtractCollationElements(collationElementString);
            if (collationElements.size() == 1)
            {
                // now we need to break down the code points into a set of sub-tables
                // for the compiler's sake, which means we need to determine ranges for
                // the code points to fall into one table or another
                // to make things easy, we can bucket the code points into tables of
                // relatively equal size - this does have the disadvantage of not bucketing
                // them for block locality at the advantage of a simple mod over a range check
                // we arbitrarily choose blocks of size = 1024

                // single code point -> single collation element mapping
                // this would go in our regular block tables
                uint16_t bucket = static_cast<uint16_t>(codePoint / BLOCK_SIZE);
                if (blockMap.find(bucket) == blockMap.end())
                {
                    // need a new map
                    blockMap[bucket] = new std::map<uint32_t, uint64_t>();
                }

                // insert code point into existing map
                blockMap[bucket]->operator[](codePoint) = collationElements[0];
            }
            else
            {
                singleCodePointMultiCollationElementCount++;

                // single code point -> multiple collation element mapping
                // this goes in the special multi-mapping table
                uint16_t multiBucket = static_cast<uint16_t>(codePoint / MULTI_BLOCK_SIZE);
                if (multiMap.find(multiBucket) == multiMap.end())
                {
                    multiMap[multiBucket] = new std::map<uint32_t, std::vector<uint64_t>>();
                }

                multiMap[multiBucket]->operator[](codePoint) = collationElements;
            }
        }

        std::getline(fileStream, line);
    }

    fileStream.close();

    // write out the static information associated with the collation element table
    std::string outputDucetMappingFileName = "unicodeDucetMapping.cpp";
    std::ofstream outputStream(outputDucetMappingFileName, std::ios::out);
    if (!outputStream.is_open())
    {
        std::cerr << "File 'unicodeDucetMapping.cpp' could not be opened!" << std::endl;
        return 1;
    }

    // write the header and includes
    outputStream << licenseBlock << std::endl;
    outputStream << "#include \"pxr/pxr.h\"" << std::endl;
    outputStream << "#include \"pxr/base/tf/unicodeUtils.h\"" << std::endl;
    outputStream << std::endl;
    outputStream << "#include <unordered_map>" << std::endl;
    outputStream << std::endl;

    outputStream << "PXR_NAMESPACE_OPEN_SCOPE" << std::endl;

    // first, the collation element tables themselves have to be output
    // for this we need to keep track of the generated names
    outputStream << "// each table here represents a (non-equal) block of Unicode code points" << std::endl;
    outputStream << "// and their mapping to collation elements (condensed as a 32-bit unsigned integer)" << std::endl;
    outputStream << "// derived from the Unicode DUCET table and used to form sort keys required to" << std::endl;
    outputStream << "// properly order unicode strings." << std::endl;
    outputStream << "// DUCET_BLOCK_SIZE is used to obtain a single unsigned short value used to" << std::endl;
    outputStream << "// index into a map to retrieve the address of a map containing a set of" << std::endl;
    outputStream << "// codepoints and their corresponding collation element value." << std::endl;
    outputStream << "// there are three special cases to consider:" << std::endl;
    outputStream << "// 1. Multiple code points (i.e. a substring) map to a single collation element" << std::endl;
    outputStream << "//    in this case, the mapping is stored in a special table (unicodeDucetMultipleMapping)" << std::endl;
    outputStream << "// 2. A single code point maps to multiple collation elements" << std::endl;
    outputStream << "//    in this case, the collation elements are a vector of uin32_t values" << std::endl;
    outputStream << "// 3. Multiple code points (i.e. a substring) map to multiple collation elements" << std::endl;
    outputStream << "//" << std::endl;
    outputStream << "// in all 3 cases, the information is stored in unicodeDucetMultipleMapping" << std::endl;
    outputStream << "// the key is either the code point (in the case of single -> multiple mappings)" << std::endl;
    outputStream << "// or a hash of the multiple code points - all values are vectors (even in the multiple -> single case)." << std::endl;
    outputStream << "const uint16_t TfUnicodeUtils::DUCET_BLOCK_SIZE = 1024;" << std::endl;
    std::map<uint16_t, std::string> tableNames;
    for (std::map<uint16_t, std::map<uint32_t, uint64_t>*>::iterator it = blockMap.begin(); it != blockMap.end(); it++)
    {
        std::string bucketName = "collationElementTable" + std::to_string(it->first);
        tableNames[it->first] = bucketName;

        //  now output the table
        outputStream << "std::unordered_map<uint32_t, uint64_t> " << bucketName << " = {" << std::endl;
        for (std::map<uint32_t, uint64_t>::iterator iit = (*(it->second)).begin(); iit != (*(it->second)).end(); iit++)
        {
            outputStream << "    { " << iit->first << ", " << iit->second << " }," << std::endl;
        }
        outputStream << "};" << std::endl;
    }

    // now write out the references to the tables
    outputStream << "std::unordered_map<uint16_t, std::unordered_map<uint32_t, uint64_t>*> TfUnicodeUtils::unicodeDucetMap = {" << std::endl;
    for (std::map<uint16_t, std::string>::iterator it = tableNames.begin(); it != tableNames.end(); it++)
    {
        outputStream << "    { " << it->first << ", &" << it->second << " }," << std::endl;
    }
    outputStream << "};" << std::endl;

    outputStream << "PXR_NAMESPACE_CLOSE_SCOPE" << std::endl;
    outputStream.close();

    // the compiler can't handle the file if we add the multi-mapping table
    // so put that in another compilation unit
    std::string outputDucetMultiMappingFileName = "unicodeDucetMultiMapping.cpp";
    std::ofstream outputMultiStream(outputDucetMultiMappingFileName, std::ios::out);
    if (!outputMultiStream.is_open())
    {
        std::cerr << "File 'unicodeDucetMultiMapping.cpp' could not be opened!" << std::endl;
        return 1;
    }

    // write the header and includes
    outputMultiStream << licenseBlock << std::endl;
    outputMultiStream << "#include \"pxr/pxr.h\"" << std::endl;
    outputMultiStream << "#include \"pxr/base/tf/unicodeUtils.h\"" << std::endl;
    outputMultiStream << std::endl;
    outputMultiStream << "#include <unordered_map>" << std::endl;
    outputMultiStream << "#include <vector>" << std::endl;
    outputMultiStream << std::endl;

    outputMultiStream << "PXR_NAMESPACE_OPEN_SCOPE" << std::endl;

    outputMultiStream << "// this map contains the mappings from code points to collation elements" << std::endl;
    outputMultiStream << "// for all values that were either multi-code point (in which case they are" << std::endl;
    outputMultiStream << "// hashed to a single value) or multi-collation element." << std::endl;
    outputMultiStream << "// the same strategy is used here to break up the compile-time tables" << std::endl;
    outputMultiStream << "// as that used in unicodeDucetMapping.cpp" << std::endl;

    outputMultiStream << "const uint16_t TfUnicodeUtils::DUCET_MULTI_BLOCK_SIZE = 512;" << std::endl;
    std::map<uint16_t, std::string> multiTableNames;
    for (std::map<uint16_t, std::map<uint32_t, std::vector<uint64_t>>*>::iterator it = multiMap.begin(); it != multiMap.end(); it++)
    {
        std::string bucketName = "multiMapTable" + std::to_string(it->first);
        multiTableNames[it->first] = bucketName;

        // output the individual table
        outputMultiStream << "std::unordered_map<uint32_t, std::vector<uint64_t>> " << bucketName << " = {" << std::endl;
        for (std::map<uint32_t, std::vector<uint64_t>>::iterator iit = (*(it->second)).begin(); iit != (*(it->second)).end(); iit++)
        {
            outputMultiStream << "    { " << iit->first << ", " << "{ ";
            for (size_t i = 0; i < iit->second.size(); i++)
            {
                outputMultiStream << iit->second[i] << ", ";
            }
            outputMultiStream << "} }, " << std::endl;
        }
        outputMultiStream << "};" << std::endl;
    }

    // now write out the references to the tables
    outputMultiStream << "std::unordered_map<uint16_t, std::unordered_map<uint32_t, std::vector<uint64_t>>*> TfUnicodeUtils::unicodeDucetMultiMap = {" << std::endl;
    for (std::map<uint16_t, std::string>::iterator it = multiTableNames.begin(); it != multiTableNames.end(); it++)
    {
        outputMultiStream << "    { " << it->first << ", &" << it->second << " }," << std::endl;
    }
    outputMultiStream << "};" << std::endl;

    outputMultiStream << "PXR_NAMESPACE_CLOSE_SCOPE" << std::endl;
    outputMultiStream.close();

    // delete all maps we created
    for (std::map<uint16_t, std::map<uint32_t, uint64_t>*>::iterator it = blockMap.begin(); it != blockMap.end(); it++)
    {
        delete it->second;
    }

    for (std::map<uint16_t, std::map<uint32_t, std::vector<uint64_t>>*>::iterator it = multiMap.begin(); it != multiMap.end(); it++)
    {
        delete it->second;
    }

    return 0;
}