// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/usd/usdUtils/dependencies.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <type_traits>

#include <pthread.h>
#include <emscripten.h>
#include <emscripten/bind.h>

PXR_NAMESPACE_USING_DIRECTIVE

using RequestFunc = std::add_pointer<void(const std::string&, std::string*)>::type;

struct Request {
    std::string url;
    std::string* result;
    RequestFunc func;
    bool complete = false;
};

pthread_t workerThread;
std::mutex _mutex;
std::condition_variable _workerCondition;
std::queue<Request*> _queue;

// Processes asyncronous requests on the worker thread as to not block the
// browser's UI thread when layers need to be fetched from the server.
static void* 
_WorkerThreadFunc(
    void* arg) 
{
    for (;;) {
        Request* request = nullptr;

        {
            std::unique_lock<std::mutex> lock(_mutex);
            _workerCondition.wait(lock,[](){
                return !_queue.empty();
            });

            request = _queue.front();
            _queue.pop();
        }


        request->func(request->url, request->result);
        request->complete = true;
    }

    return nullptr;
}

// Sets the url and function to run on the worker thread and waits for that 
// operation to return.
static bool 
_ProcessRequest(
    const std::string& url, 
    RequestFunc func, 
    std::string* result) 
{
    Request request;
    request.url = url;
    request.func = func;
    request.result = result;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(&request);
        _workerCondition.notify_one();
    }

    while (!request.complete) {
        // This call yields the UI thread to the browser allowing it to remain
        // responsive.  We are not allowed to wait for fetch operations to
        // complete on this thread.
        emscripten_sleep(16);
    }

    return true;
}


void 
_PrintPrim(
    const UsdPrim &prim, 
    const std::string &prefix, 
    std::ostream& out) 
{
    out << prefix << prim.GetName() << "\n";

    std::vector<std::string> attrs;
    for (const auto &prop : prim.GetAuthoredProperties()) {
        attrs.emplace_back("." + prop.GetName().GetString());
    }

    for (auto it = attrs.begin(); it != attrs.end(); ++it) {
        out << prefix << "  " << *it << "\n";
    }
}

void 
_PrintChildren(
    const UsdPrim &prim, 
    const std::string &prefix,
    std::ostream& out) 
{
    auto children = prim.GetAllChildren();
    for (auto child = children.begin(); child != children.end(); ++child) {
        _PrintPrim(*child, prefix, out);
        _PrintChildren(*child, prefix + "    ", out);
    }
}

// Loads the stage at url and run UsdTree output on it.
static void 
_ProcessShowTreeRequest(
    const std::string& url, 
    std::string* result)
{
    std::ostringstream str;

    auto stage = UsdStage::Open(url);
    if (stage) {
        str << "/" << "\n";
        _PrintChildren(stage->GetPseudoRoot(), "    ", str);
    } else {
        str << "Unable to open url: " << url << "\n";
    }

    *result = str.str();
}

// Initiates a ShowTree request. This function is only called from the
// browser's UI thread.
std::string 
ShowTree(
    const std::string& url)
{
    std::string result;
    _ProcessRequest(url, &_ProcessShowTreeRequest, &result);

    return result;
}


// Loads the stage at url and runs UsdUtilsComputeAllDependencies on it
static void 
_ProcessComputeAllDependenciesRequest(
    const std::string& url, 
    std::string* result)
{
    std::vector<SdfLayerRefPtr> layers;
    std::vector<std::string> assets;
    std::vector<std::string> unresolvedPaths;

    SdfAssetPath assetPath(url);
    UsdUtilsComputeAllDependencies(assetPath, &layers, &assets, &unresolvedPaths);

    std::ostringstream str;

    str << "Layers (" << layers.size() << "):" << std::endl;
    for (const auto& layer : layers) {
        str << "    " << layer->GetRealPath() << std::endl;
    }

    str << std::endl;

    str << "Assets (" << assets.size() << "):" << std::endl;
    for (const auto& asset : assets) {
        str << "    " << asset << std::endl;
    }

    str << std::endl;

    str << "UnresolvedPaths (" << unresolvedPaths.size() << "):" << std::endl;
    for (const auto& unresolvedPath : unresolvedPaths) {
        str << "    " << unresolvedPath << std::endl;
    }

    *result = str.str();
}

// Initiates a UsdUtilsComputeAllDependencies request. This function is only
// called from the browser's UI thread.
std::string 
ComputeAllDependencies(
    const std::string& url)
{
    std::string result;
    _ProcessRequest(url, &_ProcessComputeAllDependenciesRequest, &result);

    return result;
}

// Loads the stage at url and creates a new Usdz package from it. The package
// is stored in the virtual filesystem.
static void 
_ProcessCreateNewUsdzPackageRequest(
    const std::string& url, 
    std::string* result)
{
    const std::string usdzFileName = "/tmp/" +
        TfStringGetBeforeSuffix(TfGetBaseName(url)) + ".usdz";

    if (ArGetResolver().Resolve(url).IsEmpty()) {
        *result = "";
        return;
    }

    SdfAssetPath assetPath(url);
    UsdUtilsCreateNewUsdzPackage(assetPath, usdzFileName);

    *result = usdzFileName;
}

// Initiates a UsdUtilsCreateNewUsdzPackage request. This function is only
// called from the browser's UI thread.
std::string 
CreateNewUsdzPackage(
    const std::string& url)
{
    std::string result;
    _ProcessRequest(url, &_ProcessCreateNewUsdzPackageRequest, &result);

    return result;
}

// Initializes the worker thread which will process the ansynchronous requests
// exposed by the API in this file.
void 
InitWorkerThread()
{
    ArSetPreferredResolver("FetchResolver");
    pthread_create(&workerThread, nullptr, _WorkerThreadFunc, nullptr);
}

EMSCRIPTEN_BINDINGS(wasmFetchResolver)
{
    emscripten::function("InitWorkerThread", &InitWorkerThread);
    emscripten::function("ShowTree", &ShowTree);
    emscripten::function("ComputeAllDependencies", &ComputeAllDependencies);
    emscripten::function("CreateNewUsdzPackage", &CreateNewUsdzPackage);
}

