//
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
#include "pxr/pxr.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/writableAsset.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/systemInfo.h"

PXR_NAMESPACE_USING_DIRECTIVE;

// There was a race condition when simultaneously opening multiple assets for
// write in the same directory when that directory does not yet exist. Each
// asset tried to create the directory and one of them could fail.  Verify that
// this is now a race free condition.

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>

std::mutex threadSyncMutex;
std::mutex errorMutex;
std::vector<std::string> errors;

static void
_CreateAssetInThread(const std::string fullPath)
{
    const ArResolvedPath arPath(fullPath);
    ArResolver& resolver = ArGetResolver();

    // Acquire then release the thread sync mutex
    threadSyncMutex.lock();
    threadSyncMutex.unlock();

    std::shared_ptr<ArWritableAsset> asset =
        resolver.OpenAssetForWrite(arPath, ArResolver::WriteMode::Replace);

    if (asset) {
        // Write some data (the path) into the file
        asset->Write(fullPath.c_str(), fullPath.size(), 0);
        asset->Close();
    } else {
        std::unique_lock<std::mutex> lock(errorMutex);
        errors.push_back("Failed to open asset for write: " + fullPath);
    }
}

static void
_VerifyAsset(const std::string& fullPath)
{
    ArResolver& resolver = ArGetResolver();

    ArResolvedPath arPath(fullPath);
    std::shared_ptr<ArAsset> asset = resolver.OpenAsset(arPath);

    if (!asset) {
        std::cerr << "Failed to open asset for read: " << fullPath << std::endl;
        TF_AXIOM(asset);
    }
    
    TF_AXIOM(asset->GetSize() == fullPath.size());
    std::shared_ptr<const char> data = asset->GetBuffer();
    std::string contents(data.get(), asset->GetSize());
    TF_AXIOM(contents == fullPath);
}

static void
TestThreadedAssetCreation()
{
    // If two assets were created "simultaneously" in a directory which did not
    // already exist, it was possible for one of them to fail when it tried to
    // create a missing directory that had sprung into existence when the other
    // asset was created.
    
    // Figure out where we're going to create our assets
    const std::string tmpDir = ArchMakeTmpSubdir(".", "TestCreateAsset");

    // Create an asset dir several levels below tmpDir to increase the odds of
    // hitting the race condition as multiple threads discover that "g" does not
    // exist and then tries to create the hierarchy.
    const std::string assetDir = tmpDir + "/a/b/c/d/e/f/g/";

    const std::string fullPath1 = assetDir + "Asset1.out";
    const std::string fullPath2 = assetDir + "Asset2.out";

    // Acquire threadSyncMutex to keep the threads from running off without us.
    threadSyncMutex.lock();
    
    std::thread thread1(_CreateAssetInThread, fullPath1);
    std::thread thread2(_CreateAssetInThread, fullPath2);

    // Give the threads time to start up.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // And release them.
    threadSyncMutex.unlock();

    thread1.join();
    thread2.join();

    // Report any errors.
    for(const std::string& error : errors) {
        std::cerr << error << std::endl;
    }

    // Fail if errors was not empty
    TF_AXIOM(errors.empty());

    // Make sure we can read the data back.
    _VerifyAsset(fullPath1);
    _VerifyAsset(fullPath2);

    // Cleanup
    TfRmTree(tmpDir);
}

int main(int argc, char** argv)
{
    // Set the preferred resolver to ArDefaultResolver.
    ArSetPreferredResolver("ArDefaultResolver");

    std::cout << "TestThreadedAssetCreation..." << std::endl;

    TestThreadedAssetCreation();

    std::cout << "Passed!" << std::endl;

    return EXIT_SUCCESS;;
}
