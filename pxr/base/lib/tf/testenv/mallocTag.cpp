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
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/mallocHook.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <mutex>
#include <thread>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// The TfMallocTag code depends upon the Linux memory allocator, ptmalloc3.
// Turning this test off for any other platforms for now.
#if defined(ARCH_OS_LINUX)

using boost::bind;
using boost::function;

using std::vector;
using std::string;

static vector<void*> _requests;
std::mutex _mutex;
int _total = 0;
int _maxTotal = 0;

static void
MyMalloc(size_t n) {
    void* ptr = malloc(n);
    std::lock_guard<std::mutex> lock(_mutex);
    _requests.push_back(ptr);
    _total += n;
    if (_total > _maxTotal) {
        _maxTotal = _total;
    }
}

static void
FreeAll() {
    for (size_t i = 0; i < _requests.size(); i++)
        free(::_requests[i]);
    _requests.clear();
    _total = 0;
}


static void*
FreeTaskNoTag(void*) {
    MyMalloc(100000);
    return 0;
}
    
static void*
FreeTaskWithTag(void*) {
    TfAutoMallocTag noname("freeTaskWithTag");
    MyMalloc(100000);
    return 0;
}

static void
RegularTask(bool tag, int n) {
    if (tag) {
        TfAutoMallocTag noname("threadTag");
        MyMalloc(n);
    }
    else {
        MyMalloc(n);
    }
}

static int
GetBytesForCallSite(const char* name, bool skipRepeated = true)
{
    TfMallocTag::CallTree ct;
    TfMallocTag::GetCallTree(&ct, skipRepeated);
    
    for (size_t i = 0; i < ct.callSites.size(); i++)
        if (ct.callSites[i].name == name)
            return ct.callSites[i].nBytes;

    return -1;
}

static bool CloseEnough(int64_t a1, int64_t a2) {
    /*
     * Account for various small allocations during this test that can get
     * billed to various sites.  We use fairly large allocations so we can drown
     * out the small stuff.
     */
    if (a1 < 2048 && a2 < 2048)
        return true;
    
    return a1 >= (.95 * a2) && a1 <= (1.05 * a2);
}

static bool
MemCheck()
{
    int64_t m = _total,
            current = TfMallocTag::GetTotalBytes();
    bool ok = CloseEnough(m, current);

    printf("Expected about %zd, actual is %zd: %s\n",
           m, current, ok ? "[close enough]" : "[not good]");

    bool maxOk = CloseEnough(::_maxTotal, TfMallocTag::GetMaxTotalBytes());
    printf("Expected max of about %zd, actual is %zd: %s\n",
           m, current, maxOk ? "[close enough]" : "[not good]");

    return ok && maxOk;
}

static void
TestFreeThread()
{
    pthread_attr_t  _detatchedAttr, _joinableAttr;    

    if (pthread_attr_init(&_detatchedAttr) != 0 ||
        pthread_attr_init(&_joinableAttr) != 0) {
        TF_RUNTIME_ERROR("error initializing attributes");
    }
        
    if (pthread_attr_setdetachstate(&_detatchedAttr, PTHREAD_CREATE_DETACHED) != 0 ||
        pthread_attr_setdetachstate(&_joinableAttr,PTHREAD_CREATE_JOINABLE) != 0) {
        TF_RUNTIME_ERROR("error setting detatch state");
    }

    pthread_t id;
    TfAutoMallocTag noname("site3");

    if (pthread_create(&id, &_joinableAttr, FreeTaskNoTag, 0) < 0) {
        TF_RUNTIME_ERROR("pthread create failed");
    }

    void* ignored;
    if (pthread_join(id, &ignored) < 0)
        TF_RUNTIME_ERROR("join failed");

    printf("bytesForSite[site3] = %d\n", GetBytesForCallSite("site3"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site3"), 0));
    FreeAll();
}

static void
TestFreeThreadWithTag()
{
    pthread_attr_t  _detatchedAttr, _joinableAttr;    

    if (pthread_attr_init(&_detatchedAttr) != 0 ||
        pthread_attr_init(&_joinableAttr) != 0) {
        TF_RUNTIME_ERROR("error initializing attributes");
    }
        
    if (pthread_attr_setdetachstate(&_detatchedAttr, PTHREAD_CREATE_DETACHED) != 0 ||
        pthread_attr_setdetachstate(&_joinableAttr,PTHREAD_CREATE_JOINABLE) != 0) {
        TF_RUNTIME_ERROR("error setting detatch state");
    }

    pthread_t id;
    TfAutoMallocTag noname("site4");

    if (pthread_create(&id, &_joinableAttr, FreeTaskWithTag, 0) < 0) {
        TF_RUNTIME_ERROR("pthread create failed");
    }

    void* ignored;
    if (pthread_join(id, &ignored) < 0)
        TF_RUNTIME_ERROR("join failed");

    printf("bytesForSite[freeTaskWithTag] = %d\n",
           GetBytesForCallSite("freeTaskWithTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("freeTaskWithTag"), 100000));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site4"), 0));
    FreeAll();

    printf("bytesForSite[freeTaskWithTag] = %d\n",
           GetBytesForCallSite("freeTaskWithTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("freeTaskWithTag"), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site4"), 0));
}

static void
TestRegularTask()
{
    {
        TfAutoMallocTag noname("name");
        std::thread t(RegularTask, false, 100000);
        t.join();
    }

    printf("bytesForSite[%s] = %d\n", "name", GetBytesForCallSite("name"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("name"), 100000));
    FreeAll();

    printf("bytesForSite[%s] = %d\n", "name", GetBytesForCallSite("name"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("name"), 0));
}

static void
TestRegularTaskWithTag()
{
    {
        TfAutoMallocTag noname("site2");
        std::thread t(RegularTask, true, 100000);
        t.join();
    }

    printf("bytesForSite[%s] = %d\n", "threadTag", GetBytesForCallSite("threadTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("threadTag"), 100000));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site2"), 0));
    FreeAll();

    printf("bytesForSite[%s] = %d\n", "threadTag", GetBytesForCallSite("threadTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("threadTag"), 0));
}

static void
TestRepeated() 
{
    TfAutoMallocTag noname1("site1");
    MyMalloc(100000);
    TfAutoMallocTag noname2("site2");
    MyMalloc(200000);
    TfAutoMallocTag noname3("site1");
    MyMalloc(100000);
    TfAutoMallocTag noname4("site3");
    MyMalloc(100000);

    TF_AXIOM(CloseEnough(GetBytesForCallSite("site2", false), 200000));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site2", true ), 300000));

    TF_AXIOM(CloseEnough(GetBytesForCallSite("site1", true), 100000));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site1", false), 200000));

    TF_AXIOM(CloseEnough(GetBytesForCallSite("site3", true), 100000));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site3", false), 100000));
}

static bool
Test_TfMallocTag()
{
    bool runme = false;

#if defined(ARCH_BITS_64) && defined(ARCH_OS_LINUX)
    runme = true;
#endif

    if (!ArchIsPtmallocActive()) {
        printf("ptmalloc is not the active allocator. Skipping tests for "
                "TfMallocTag.\n");
        runme = false; 
    }
    
    if (!runme)
        return true;

    _requests.reserve(1024);
    TF_AXIOM(TfMallocTag::GetTotalBytes() == 0);
    TF_AXIOM(MemCheck());

    // this won't show up in the accounting
    void* mem1 = malloc(100000);

    string errMsg;
    if (!TfMallocTag::Initialize(&errMsg)) {
        printf("TfMallocTag init error: %s\n", errMsg.c_str());
        TF_AXIOM(0);
    }

    TfAutoMallocTag topTag("myRoot");
    
    // and this free shouldn't either...
    free(mem1);
    printf("total: %zd\n", TfMallocTag::GetTotalBytes());
    TF_AXIOM(TfMallocTag::GetTotalBytes() == 0);

    MyMalloc(300000);
    TF_AXIOM(MemCheck());
    
    FreeAll();
    TF_AXIOM(MemCheck());

    TfMallocTag::Push(string("manualTag"));
    MyMalloc(100000);
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), 100000));
    TfMallocTag::Push("manualTag2");
    MyMalloc(100000);
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), 100000));
    TfMallocTag::Pop(string("manualTag2"));
    TfMallocTag::Pop();
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), 100000));
    FreeAll();
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag2"), 0));
    
    FreeAll();
    TF_AXIOM(MemCheck());

    TestRegularTask();
    TF_AXIOM(MemCheck());

    TestRegularTaskWithTag();
    TF_AXIOM(MemCheck());

    TestFreeThread();
    TF_AXIOM(MemCheck());

    TestFreeThreadWithTag();
    TF_AXIOM(MemCheck());

    FreeAll();
    TF_AXIOM(MemCheck());

    TestRepeated();
    TF_AXIOM(MemCheck());

    return true;
}

TF_ADD_REGTEST(TfMallocTag);

#endif
