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

// Simple test program for iterating Unicode collation conformance
// Since there are still some test case exceptions in the collation test
// (because the conformance isn't 100% yet), this is a simple test script
// that can be used to iterate this functionality without running the Tf suite
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

#include "unicodeDucetCommon.h"
#include "../unicodeUtils.h"

int main()
{
    std::string collationTestFileName = "CollationTest_NON_IGNORABLE_SHORT.txt";
    std::ifstream collationFileStream(collationTestFileName, std::ios::in);
    if (!collationFileStream.is_open())
    {
        std::cerr << "File 'CollationTest_NON_INGORABLE_SHORT.txt' could not be opened!" << std::endl;
        return 1;
    }

    size_t testNum = 0;
    size_t numEqual = 0;
    size_t numNonConformances = 0;
    std::string linex;
    std::string line1;
    std::string line2;
    std::vector<std::pair<std::string, std::string>> nonConformances;
    std::getline(collationFileStream, linex);
    while (!collationFileStream.eof())
    {
        if (linex.length() == 0 && testNum == 0)
        {
            // this signals the start of the test loop
            std::getline(collationFileStream, line1);
            std::getline(collationFileStream, line2);
        }
        else if (testNum == 0)
        {
            std::getline(collationFileStream, linex);
            continue;
        }

        // split the code points
        testNum++;
        line1 = Trim(line1);
        line2 = Trim(line2);

        std::vector<std::string> codePointString1 = Split(line1, ' ');
        std::vector<std::string> codePointString2 = Split(line2, ' ');

        // form the string
        std::string input1;
        std::string input2;
        for (size_t i = 0; i < codePointString1.size(); i++)
        {
            uint32_t codePoint = static_cast<uint32_t>(std::stoul(codePointString1[i], nullptr, 16));
            TfUnicodeUtils::AppendUTF8Char(codePoint, input1);
        }

        for (size_t i = 0; i < codePointString2.size(); i++)
        {
            uint32_t codePoint = static_cast<uint32_t>(std::stoul(codePointString2[i], nullptr, 16));
            TfUnicodeUtils::AppendUTF8Char(codePoint, input2);
        }

        // now compare them
        std::cout << "Comparing " << line1 << " and " << line2 << std::endl;
        bool result = TfUnicodeUtils::TfUTF8UCALessThan()(input1, input2);
        if (!result)
        {
            result = TfUnicodeUtils::TfUTF8UCALessThan()(input2, input1);
            if (result)
            {
                // this means that they are not equal
                numNonConformances++;
                nonConformances.push_back(std::make_pair(line1, line2));
            }
            else
            {
                numEqual++;
            }
        }

        line1 = line2;
        std::getline(collationFileStream, line2);
    }

    std::cout << "Total non conformances: " << numNonConformances << std::endl;
    std::cout << "Total equal: " << numEqual << std::endl;
    std::cout << "Non Conformance Cases: " << std::endl;
    for (std::vector<std::pair<std::string, std::string>>::iterator it = nonConformances.begin(); it != nonConformances.end(); it++)
    {
        std::cout << it->first << " : " << it->second << std::endl;
    }

    return 0;
}