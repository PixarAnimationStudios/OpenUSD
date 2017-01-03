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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/arch/nap.h"

#include <boost/noncopyable.hpp>

#include <condition_variable>
#include <cstdio>
#include <future>
#include <map>
#include <mutex>
#include <string>

class Lemur : public TfWeakBase {
public:
    void Method1() { }
};


class MonkeyInterface {
  public:

    TF_DECLARE_WEAK_POINTABLE_INTERFACE;
    
    virtual ~MonkeyInterface();
    virtual void SeeAndDo() = 0;
};

MonkeyInterface::~MonkeyInterface() {}


class Human : public TfWeakBase, public MonkeyInterface {
  public:

    TF_IMPLEMENT_WEAK_POINTABLE_INTERFACE;
    
    virtual ~Human();

    virtual void SeeAndDo() {
        printf("See and Do called on Human!\n");
    }

};

Human::~Human() {}


typedef TfWeakPtr<MonkeyInterface> MonkeyInterfaceWeakPtr;


static void InvokeSeeAndDo(MonkeyInterfaceWeakPtr const &monkey) {
    monkey->SeeAndDo();
}

static void _TestComparisons()
{
    typedef  MonkeyInterfaceWeakPtr Ptr;

    Human h1, h2;
    Ptr x(&h1), y(&h2);

    if (y < x)
        std::swap(x, y);

    TF_AXIOM( x <  y);
    TF_AXIOM( x <= y);
    TF_AXIOM( !(x >  y) );
    TF_AXIOM( !(x >= y) );

    TF_AXIOM( !(y <  x) );
    TF_AXIOM( !(y <= x) );
    TF_AXIOM( y >  x );
    TF_AXIOM( y >= x );

    TF_AXIOM( !(x == nullptr) );
    TF_AXIOM( !(nullptr == x) );

    TF_AXIOM( nullptr != x );
    TF_AXIOM( x != nullptr );

    TF_AXIOM( !(x < nullptr) );
    TF_AXIOM( nullptr < x );

    TF_AXIOM( !(nullptr > x) );
    TF_AXIOM( x > nullptr );

    TF_AXIOM( !(x <= nullptr) );
    TF_AXIOM( nullptr <= x );

    TF_AXIOM( !(nullptr >= x) );
    TF_AXIOM( x >= nullptr );

    TF_AXIOM( !(x == NULL) );
    TF_AXIOM( !(NULL == x) );
}

static bool
Test_TfWeakPtr()
{
    Lemur* lemur = new Lemur;
    TfWeakPtr<Lemur> lPtr(lemur);
    TfWeakPtr<Lemur> lPtr2(lemur);
 
    /*
     * The lemur should still exist at this point.
     */
    if (!lPtr)
        TF_FATAL_ERROR("unexpected lemur death");
    else
        lPtr->Method1();

    TF_AXIOM(lPtr);
    TF_AXIOM(lPtr2 == lPtr);

    Lemur* tmp = lemur;
    delete lemur;           // note: this can make lemur undefined
    lemur = tmp;
    
    TF_AXIOM(!lPtr);
    TF_AXIOM(!lPtr2);
    TF_AXIOM(lPtr2 == lPtr);

    /*
     * Expected: one dead lemur.
     */
    if (lPtr)
        TF_FATAL_ERROR("unexpectedly found living lemur");

    TF_AXIOM(lPtr.IsInvalid());
    TF_AXIOM(!lPtr);

    lPtr = TfNullPtr;

    TF_AXIOM(!lPtr);
    TF_AXIOM(!lPtr.IsInvalid());

    TF_AXIOM(lPtr2 != lPtr);

    // Monkey tests.
    Human *human = new Human;
    TfWeakPtr<Human> hPtr(human);
    InvokeSeeAndDo(hPtr);
    delete human;
    TF_AXIOM(!hPtr);
    _TestComparisons();

    return true;
}

////////////////////////////////////////////////////////////////////////
//
// Test TfCreateRefPtrFromProtectedWeakPtr.

// You can comment out this #define to demonstrate the broken behavior
// when you try to naively construct a TfRefPtr from a weak ptr.
#define USE_CREATE_REF_PTR_FROM_PROTECTED_WEAK_PTR 1

TF_DECLARE_WEAK_AND_REF_PTRS(ProtectedBase);

// Singleton registry of instances.
class ProtectedBase_Registry : public boost::noncopyable {
public:
    static ProtectedBase_Registry &GetInstance() {
        return TfSingleton<ProtectedBase_Registry>::GetInstance();
    }
    int GetNumEntries() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _registry.size();
    }
private:
    typedef std::map<int, ProtectedBasePtr> _RegistryMap;
    _RegistryMap _registry;
    static std::mutex _mutex;

    friend class ProtectedBase;
    friend class TfSingleton<ProtectedBase_Registry>;
};
std::mutex ProtectedBase_Registry::_mutex;

// Simple semaphore.
class Semaphore : boost::noncopyable {
public:
    Semaphore() : _count(0) { }

    void Post()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        ++_count;
        _condvar.notify_one();
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_count) {
            _condvar.wait(lock);
        }
        --_count;
    }

private:
    std::mutex _mutex;
    std::condition_variable _condvar;
    size_t _count;
};

// This semaphore is used to carefully control scheduling
// of multi-threaded code to exercise certain paths.
static Semaphore _findOrCreateSema;

TF_INSTANTIATE_SINGLETON(ProtectedBase_Registry);

// A test class whose destructor provides the guarantee needed by
// TfCreateRefPtrFromProtectedWeakPtr.
class ProtectedBase : public TfRefBase, public TfWeakBase {
public:
    ~ProtectedBase() {
        _findOrCreateSema.Post();

        // Lock the registry mutex while removing this entry.
        std::lock_guard<std::mutex> lock(ProtectedBase_Registry::_mutex);

        // Erase the entry, but only if it still maps to this instance.
        ProtectedBase_Registry & reg =
            ProtectedBase_Registry::GetInstance();
        ProtectedBase_Registry::_RegistryMap::iterator it =
            reg._registry.find(_id);
        if (it != reg._registry.end() && it->second == TfCreateWeakPtr(this)) {
            reg._registry.erase(it);
        }
    }
    static ProtectedBaseRefPtr FindOrCreate(int id) {
        // Lock the registry mutex while scanning the registry.
        std::lock_guard<std::mutex> lock(ProtectedBase_Registry::_mutex);

        _findOrCreateSema.Wait();

        ProtectedBase_Registry & reg =
            ProtectedBase_Registry::GetInstance();
        ProtectedBase_Registry::_RegistryMap::iterator it =
            reg._registry.find(id);
        if (it != reg._registry.end()) {
            // Found an entry -- try to convert to a ref ptr.
#ifdef USE_CREATE_REF_PTR_FROM_PROTECTED_WEAK_PTR
            // Use TfCreateRefPtrFromProtectedWeakPtr to safely create
            // a ref ptr from this weak pointer.
            if (ProtectedBaseRefPtr baseRef =
                TfCreateRefPtrFromProtectedWeakPtr(it->second)) {
#else
            // This is the *unsafe* way to do this, and will reliably
            // cause this test to fail.
            if (ProtectedBaseRefPtr baseRef = 
                ProtectedBaseRefPtr(it->second)) {
#endif
                return baseRef;
            } else {
                // This object is expiring!  We need to allocate a
                // new equivalent object, which will replace it
                // in the registry map.
                reg._registry.erase(it);
            }
        }
        ProtectedBaseRefPtr baseRef =
            TfCreateRefPtr(new ProtectedBase(id));
        reg._registry[id] = ProtectedBasePtr(baseRef);
        return baseRef;
    }
    int GetId() const { return _id; }
private:
    ProtectedBase(int id) : _id(id) {
    }

    const int _id;
};

static ProtectedBaseRefPtr
_ThreadFunc()
{
    return ProtectedBase::FindOrCreate(1); // wait
}

static bool
Test_TfCreateRefPtrFromProtectedWeakPtr()
{
    ProtectedBase_Registry & reg = ProtectedBase_Registry::GetInstance();

    // Test basic (non multi-threaded) usage
    {
        TF_VERIFY(reg.GetNumEntries() == 0);
        _findOrCreateSema.Post(); // post
        ProtectedBaseRefPtr b1 = ProtectedBase::FindOrCreate(1); // wait
        TF_VERIFY(reg.GetNumEntries() == 1);
        b1 = TfNullPtr; // post
        TF_VERIFY(reg.GetNumEntries() == 0);
        _findOrCreateSema.Wait(); // wait
    }

    // Now test concurrent destruction during FindOrCreate()
    {
        _findOrCreateSema.Post(); // post
        ProtectedBaseRefPtr b1 = ProtectedBase::FindOrCreate(1); // wait
        ProtectedBasePtr b1_weak(b1);

        // Spawn a thread to call FindOrCreate(1)
        std::future<ProtectedBaseRefPtr> t1 =
            std::async(std::launch::async, _ThreadFunc);

        // Wait for that thread to block on semaphore (janky!)
        ArchNap(25 /* .25 sec */);

        // Now invoke the destructor.  This will post to _findOrCreateSema,
        // unblocking the t1 thread, and then block trying to grab the
        // registry mutex.
        b1 = TfNullPtr; // post

        // Wait for t1 to complete
        t1.wait();
        ProtectedBaseRefPtr b2 = t1.get();
        ProtectedBasePtr b2_weak(b2);

        // The thread will have detected that b1 is expiring, and have
        // returned a new object.  (We use the weak_ptrs to verify this.)
        TF_VERIFY(reg.GetNumEntries() == 1);
        TF_VERIFY(!b1);
        TF_VERIFY(!b1_weak);
        TF_VERIFY(b2);
        TF_VERIFY(b2_weak);
        TF_VERIFY(b1_weak != b2_weak);
    }
    
    return true;
}

namespace {

using std::string;

class Base : public TfWeakBase {};
class Derived : public Base {};
class Unrelated : public TfWeakBase {};

inline string F(const TfWeakPtr<Base> &b) { return "base"; }
inline string F(const TfWeakPtr<Unrelated> &b) { return "unrelated"; }

} // anon

static bool
Test_TfWeakPtrConversion() {
    Derived d;
    Unrelated u;

    TfWeakPtr<Derived> wd(&d);
    TfWeakPtr<Unrelated> wu(&u);

    // Invoking F with TfWeakPtr<Derived> used to be ambiguous.  Verify that
    // this compiles unambiguously and invokes the correct overload.
    TF_AXIOM(F(wd) == "base");
    TF_AXIOM(F(wu) == "unrelated");

    return true;
}

////////////////////////////////////////////////////////////////////////

TF_ADD_REGTEST(TfWeakPtr);
TF_ADD_REGTEST(TfCreateRefPtrFromProtectedWeakPtr);
TF_ADD_REGTEST(TfWeakPtrConversion);

////////////////////////////////////////////////////////////////////////

// Compile-time testing of the Tf_SUPPORTS_WEAKPTR mechanism.
namespace
{
    struct Tf_TestHasGetWeakBase
    {
        TfWeakBase const &__GetTfWeakBase__() const;
    };

    struct Tf_TestHasGetWeakBaseDerived : public Tf_TestHasGetWeakBase
    {
    };

    struct Tf_TestHasGetWeakBaseNot
    {
    };

    struct Tf_TestIsWeakBase : public TfWeakBase
    {
    };

    struct Tf_TestGetWeakBaseWrongSignature
    {
        void __GetTfWeakBase__() const;
    };

    class Tf_TestGetWeakBasePrivate
    {
    private:
        const TfWeakBase& __GetTfWeakBase__() const;
    };

    class Tf_TestGetWeakBaseProtected
    {
    protected:
        const TfWeakBase& __GetTfWeakBase__() const;
    };

    static_assert(TF_SUPPORTS_WEAKPTR(Tf_TestHasGetWeakBase), "");
    static_assert(TF_SUPPORTS_WEAKPTR(Tf_TestHasGetWeakBaseDerived), "");
    static_assert(!TF_SUPPORTS_WEAKPTR(Tf_TestHasGetWeakBaseNot), "");
    static_assert(!TF_SUPPORTS_WEAKPTR(TfWeakPtr<Tf_TestIsWeakBase>), "");
    static_assert(!TF_SUPPORTS_WEAKPTR(Tf_TestGetWeakBaseWrongSignature), "");
    static_assert(!TF_SUPPORTS_WEAKPTR(Tf_TestGetWeakBasePrivate), "");
    static_assert(!TF_SUPPORTS_WEAKPTR(Tf_TestGetWeakBaseProtected), "");
    static_assert(!TF_SUPPORTS_WEAKPTR(int), "");
}
