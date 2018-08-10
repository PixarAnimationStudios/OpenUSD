//
// Copyright 2018 Pixar
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

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/arch/fileSystem.h"

#include <iostream>
#include <memory>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE;

// Test that calling ArResolver::OpenAsset on a file within a .usdz
// file produces the expected result.
static void
TestOpenAsset()
{
    std::cout << "TestOpenAsset..." << std::endl;

    ArResolver& resolver = ArGetResolver();

    std::shared_ptr<ArAsset> usdzAsset = 
        resolver.OpenAsset("test.usdz[bogus.file]");
    TF_AXIOM(!usdzAsset);

    auto testAsset = [&resolver](
        const std::string& packageRelativePath,
        const std::string& srcFilePath,
        size_t expectedSize, size_t expectedOffset) {

        std::cout << "  - " << packageRelativePath << std::endl;

        // Verify that we can open the file within the .usdz file and the
        // size is what we expect.
        std::shared_ptr<ArAsset> asset = 
            resolver.OpenAsset(packageRelativePath);
        TF_AXIOM(asset);
        TF_AXIOM(asset->GetSize() == expectedSize);

        // Read in the file data from the asset in various ways and ensure
        // they match the source file.
        ArchConstFileMapping srcFile = ArchMapFileReadOnly(srcFilePath);
        TF_AXIOM(srcFile);
        TF_AXIOM(ArchGetFileMappingLength(srcFile) == expectedSize);

        std::shared_ptr<const char> buffer = asset->GetBuffer();
        TF_AXIOM(buffer);
        TF_AXIOM(std::equal(buffer.get(), buffer.get() + expectedSize,
                            srcFile.get()));

        std::unique_ptr<char[]> arr(new char[expectedSize]);
        TF_AXIOM(asset->Read(arr.get(), expectedSize, 0) == expectedSize);
        TF_AXIOM(std::equal(arr.get(), arr.get() + expectedSize, 
                            srcFile.get()));

        size_t offset = 100;
        size_t numToRead = expectedSize - offset;
        arr.reset(new char[expectedSize - offset]);
        TF_AXIOM(asset->Read(arr.get(), numToRead, offset) == numToRead);
        TF_AXIOM(std::equal(arr.get(), arr.get() + numToRead,
                            srcFile.get() + offset));
 
        std::pair<FILE*, size_t> fileAndOffset = asset->GetFileUnsafe();
        TF_AXIOM(fileAndOffset.first != nullptr);
        TF_AXIOM(fileAndOffset.second == expectedOffset);

        ArchConstFileMapping file = ArchMapFileReadOnly(fileAndOffset.first);
        TF_AXIOM(file);
        TF_AXIOM(std::equal(file.get() + fileAndOffset.second,
                            file.get() + fileAndOffset.second + expectedSize,
                            srcFile.get()));
    };

    testAsset(
        "test.usdz[file_1.usdc]", "src/file_1.usdc",
        /* expectedSize = */ 680, /* expectedOffset = */ 64);
    testAsset(
        "test.usdz[nested.usdz]", "src/nested.usdz",
        /* expectedSize = */ 2376, /* expectedOffset = */ 832);
    testAsset(
        "test.usdz[nested.usdz[file_1.usdc]]", "src/file_1.usdc",
        /* expectedSize = */ 680, /* expectedOffset = */ 896);
    testAsset(
        "test.usdz[nested.usdz[file_2.usdc]]", "src/file_2.usdc",
        /* expectedSize = */ 621, /* expectedOffset = */ 1664);
    testAsset(
        "test.usdz[nested.usdz[subdir/file_3.usdc]]", 
        "src/subdir/file_3.usdc",
        /* expectedSize = */ 640, /* expectedOffset = */ 2368);
    testAsset(
        "test.usdz[file_2.usdc]", "src/file_2.usdc",
        /* expectedSize = */ 621, /* expectedOffset = */ 3264);
    testAsset(
        "test.usdz[subdir/file_3.usdc]", "src/subdir/file_3.usdc",
        /* expectedSize = */ 640, /* expectedOffset = */ 3968);


}

int main(int argc, char** argv)
{
    TestOpenAsset();

    std::cout << "Passed!" << std::endl;

    return EXIT_SUCCESS;
}
