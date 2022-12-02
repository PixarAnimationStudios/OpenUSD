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

#include "unicodeDucetCommon.h"

std::string Trim(const std::string& input)
{
    size_t leftIndex = input.find_first_not_of(' ');
    if (leftIndex == std::string::npos)
    {
        // if we didn't find any whitespace at all
        // and already hit the end of the string, just return
        return input;
    }

    size_t rightIndex = input.find_last_not_of(' ');
    return input.substr(leftIndex, (rightIndex - leftIndex + 1));
}

std::vector<std::string> Split(const std::string& input, char delimeter)
{
    std::vector<std::string> result;
    std::string leftToProcess = input;
    size_t delimeterIndex = leftToProcess.find_first_of(delimeter);
    while (delimeterIndex != std::string::npos)
    {
        result.push_back(leftToProcess.substr(0, delimeterIndex));
        leftToProcess = leftToProcess.substr(delimeterIndex + 1, leftToProcess.length() - delimeterIndex - 1);
        delimeterIndex = leftToProcess.find_first_of(delimeter);
    }

    result.push_back(leftToProcess);

    return result;
}
 
std::vector<uint64_t> ExtractCollationElements(const std::string& input)
{
    // each collation element consists of a primary, secondary, and tertiary weight
    // we combine that all into a single 32-bit value which we can extract later
    // when we need to form the sort keys
    std::string leftToProcess = input;
    std::vector<uint64_t> collationElements;
    size_t leftBraceIndex = leftToProcess.find_first_of('[');
    while (leftBraceIndex != std::string::npos)
    {
        // find the matching right brace
        size_t rightBraceIndex = leftToProcess.find_first_of(']');
        std::string collationElementString = leftToProcess.substr(leftBraceIndex + 1, rightBraceIndex - leftBraceIndex - 1);

        // the collation element will have the form [.x.x.x] or [*x.x.x]
        // we need to extract the x's (weights) and convert them to a 64-bit unsigned integer
        collationElementString = collationElementString.substr(1, collationElementString.length() - 1);
        size_t firstDotIndex = collationElementString.find_first_of('.');
        size_t lastDotIndex = collationElementString.find_last_of('.');

        // the primary weight can be represented safely in 16-bits, the secondary in 16-bits, and the teriary in 8-bits
        uint16_t primaryWeight = static_cast<uint16_t>(std::stoul(collationElementString.substr(0, firstDotIndex), nullptr, 16));
        uint16_t secondaryWeight = static_cast<uint16_t>(std::stoul(collationElementString.substr(firstDotIndex + 1, lastDotIndex - firstDotIndex - 1), nullptr, 16));
        uint8_t tertiaryWeight = static_cast<uint8_t>(std::stoul(collationElementString.substr(lastDotIndex + 1, collationElementString.length() - lastDotIndex - 1), nullptr, 16));
        uint64_t collationElement = 0;
        collationElement |= static_cast<uint64_t>(primaryWeight) << 24;
        collationElement |= static_cast<uint64_t>(secondaryWeight) << 8;
        collationElement |= static_cast<uint64_t>(tertiaryWeight);
        collationElements.push_back(collationElement);

        leftToProcess = leftToProcess.substr(rightBraceIndex + 1, leftToProcess.length() - rightBraceIndex - 1);
        leftBraceIndex = leftToProcess.find_first_of('[');
    }

    return collationElements;
}