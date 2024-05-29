//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
