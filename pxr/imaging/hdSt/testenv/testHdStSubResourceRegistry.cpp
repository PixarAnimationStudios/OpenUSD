//
// Copyright 2024 Pixar
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

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hgi/hgi.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_ResourceRegistry final : public HdResourceRegistry
{
public:
    My_ResourceRegistry(
        HdStResourceRegistry* hdStResourceRegistry)
      : _hdStResourceRegistry(hdStResourceRegistry)
      , _commitCount(0)
      , _garbageCollectCount(0)
    {}

    size_t GetCommitCount() const
    {
        return _commitCount;
    }

    size_t GetGarbageCollectCount() const
    {
        return _garbageCollectCount;
    }

    void ResetCounters()
    {
        _commitCount = 0;
        _garbageCollectCount = 0;
    }

protected:
    void _Commit() override
    {
        // Do an operation on the HdStResourceRegistry that lets us determine
        // when HdStResourceRegistry::_Commit is invoked.
        VtArray<GfVec3f> points(3);
        points[0] = GfVec3f(0);
        points[1] = GfVec3f(1);
        points[2] = GfVec3f(2);
        _hdStResourceRegistry->AddSource(
            std::make_shared<HdVtBufferSource>(
                HdTokens->points, VtValue(points)));;

        _commitCount += 1;
    }

    void _GarbageCollect() override
    {
        _garbageCollectCount += 1;
    }

private:
    HdStResourceRegistry* _hdStResourceRegistry;
    size_t _commitCount;
    size_t _garbageCollectCount;
};

static
My_ResourceRegistry*
_GetSubResourceRegistry(
    HdStResourceRegistry* hdStResourceRegistry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    // First time we use FindOrCreateSubResourceRegistry we should get a fresh
    // resource registry.
    static const std::string testHdStResourceRegistryIdentifier =
        "testHdStSubResourceRegistry";
    auto testHdStResourceRegistryFactory =
        [hdStResourceRegistry]() {
            return std::make_unique<My_ResourceRegistry>(hdStResourceRegistry);
        };
    My_ResourceRegistry* myResourceRegistry =
        dynamic_cast<My_ResourceRegistry*>(
            hdStResourceRegistry->FindOrCreateSubResourceRegistry(
                testHdStResourceRegistryIdentifier,
                testHdStResourceRegistryFactory)
        );
    TF_VERIFY(myResourceRegistry != nullptr);
    TF_VERIFY(myResourceRegistry->GetCommitCount() == 0);
    TF_VERIFY(myResourceRegistry->GetGarbageCollectCount() == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->garbageCollected) == 0);

    // If we give the same identifier to FindOrCreateSubResourceRegistry a 2nd
    // time we should get the same resource registry as the 1st time.
    My_ResourceRegistry* myResourceRegistryAgain =
        dynamic_cast<My_ResourceRegistry*>(
            hdStResourceRegistry->FindOrCreateSubResourceRegistry(
                testHdStResourceRegistryIdentifier,
                testHdStResourceRegistryFactory)
        );
    TF_VERIFY(myResourceRegistry == myResourceRegistryAgain);

    return myResourceRegistry;
}

static
void
_CommitTest(
    HdStResourceRegistry* hdStResourceRegistry,
    My_ResourceRegistry* myResourceRegistry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    myResourceRegistry->ResetCounters();

    // Invoking Commit on the parent HdStResourceRegistry should invoke Commit
    // on the sub resource registry.
    hdStResourceRegistry->Commit();
    TF_VERIFY(myResourceRegistry->GetCommitCount() == 1);
    TF_VERIFY(myResourceRegistry->GetGarbageCollectCount() == 0);
    // HdStResourceRegistry::_Commit should get invoked, so a buffer source
    // resolve should happen.
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->garbageCollected) == 0);

    // Invoking Commit on the sub resource registry won't invoke Commit on the
    // parent HdStResourceRegistry (to do so would cause a stack overflow).
    myResourceRegistry->Commit();
    TF_VERIFY(myResourceRegistry->GetCommitCount() == 2);
    TF_VERIFY(myResourceRegistry->GetGarbageCollectCount() == 0);
    // HdStResourceRegistry::_Commit should not get invoked, so no additional
    // buffer source resolve should happen.
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->garbageCollected) == 0);
}

static
void
_GarbageCollectTest(
    HdStResourceRegistry* hdStResourceRegistry,
    My_ResourceRegistry* myResourceRegistry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    myResourceRegistry->ResetCounters();

    // Invoking GarbageCollect on the parent HdStResourceRegistry should invoke
    // GarbageCollect on the sub resource registry.
    hdStResourceRegistry->GarbageCollect();
    TF_VERIFY(myResourceRegistry->GetCommitCount() == 0);
    TF_VERIFY(myResourceRegistry->GetGarbageCollectCount() == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    // GarbageCollect should get invoked on HdStResourceRegistry and
    // My_ResourceRegistry -> 2 calls
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->garbageCollected) == 2);

    // Invoking GarbageCollect on the sub resource registry won't invoke
    // GarbageCollect on the parent HdStResourceRegistry (to do so would cause
    // a stack overflow).
    myResourceRegistry->GarbageCollect();
    TF_VERIFY(myResourceRegistry->GetCommitCount() == 0);
    TF_VERIFY(myResourceRegistry->GetGarbageCollectCount() == 2);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    // GarbageCollect should only get invoked on My_ResourceRegistry -> 1 call
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->garbageCollected) == 3);
}

int main()
{
    TfErrorMark mark;

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // Prepare GL context.
    GarchGLDebugWindow window("Hd Test", 512, 512);
    window.Init();

    // Initialize the resource registry we will test.
    std::unique_ptr<Hgi> hgi = Hgi::CreatePlatformDefaultHgi();
    HdStResourceRegistry hdStResourceRegistry(hgi.get());

    std::cout << "Creation and Retrieval Test\n";
    My_ResourceRegistry* myResourceRegistry =
        _GetSubResourceRegistry(&hdStResourceRegistry);

    std::cout << "Commit Test\n";
    _CommitTest(&hdStResourceRegistry, myResourceRegistry);

    std::cout << "GarbageCollect Test\n";
    _GarbageCollectTest(&hdStResourceRegistry, myResourceRegistry);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

