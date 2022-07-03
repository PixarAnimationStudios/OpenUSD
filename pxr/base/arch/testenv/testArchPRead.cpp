//
// Copyright 2021 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/arch/fileSystem.h"

#include <cstdio>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("usage: %s <filename> <N>\n", argv[0]);
        printf("- Uses ArchPRead to read file and print the last N bytes.\n");
        return 1;
    }

    const std::string filename(argv[1]);
    size_t bytesFromEnd = 0;

    try {
        bytesFromEnd = std::strtoul(argv[2], NULL, 10);
    }
    catch (const std::exception& e) {
        printf("ERROR: Invalid number of bytes specified\n");
        return 1;
    }


    auto close_file = [](FILE* f) { fclose(f); };
    std::unique_ptr<FILE, decltype(close_file)> f(
        fopen(filename.c_str(), "r"), close_file);
    if (!f) {
        printf("ERROR: Unable to open %s\n", filename.c_str());
        return 1;
    }

    const int64_t fileSize = ArchGetFileLength(f.get());
    printf("Reading %s (%zu bytes)...\n", filename.c_str(), fileSize);

    std::unique_ptr<char[]> fileContents(new char[fileSize]);
    const int64_t numRead = ArchPRead(f.get(), fileContents.get(), fileSize, 0);
    if (numRead != fileSize) {
        printf("ERROR: Read %zu bytes, expected %zu\n", numRead, fileSize);
        return 1;
    }

    const std::string lastNBytes(
        fileContents.get() + fileSize - bytesFromEnd, bytesFromEnd);
    printf("%s", lastNBytes.c_str());

    return 0;
}
