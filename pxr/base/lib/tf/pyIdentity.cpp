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
#include <Python.h>

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stackTrace.h"

#include <mutex>
#include <vector>

// Compile-time option to help debug identity issues.
//#define DEBUG_IDENTITY


using std::string;
using std::vector;

Tf_PyOwnershipPtrMap::_CacheType Tf_PyOwnershipPtrMap::_cache;


struct Tf_PyIdHandle {

    TF_MALLOC_TAG_NEW("Tf", "Tf_PyIdHandle");
    
    Tf_PyIdHandle();
    explicit Tf_PyIdHandle(PyObject *obj);
    Tf_PyIdHandle(Tf_PyIdHandle const &other);
    Tf_PyIdHandle &operator=(Tf_PyIdHandle const &other);
    ~Tf_PyIdHandle();
    void CleanUp();
    void Release() const;
    void Acquire() const;
    PyObject *Ptr() const;
    mutable bool _isAcquired;
    PyObject *_weakRef;
};



Tf_PyIdHandle::Tf_PyIdHandle() :
    _isAcquired(false), _weakRef(0)
{
}

Tf_PyIdHandle::Tf_PyIdHandle(PyObject *obj) :
    _isAcquired(false), _weakRef(0)
{
    TfPyLock lock;
    _weakRef = PyWeakref_NewRef(obj, 0);
    Acquire();
}

Tf_PyIdHandle::Tf_PyIdHandle(Tf_PyIdHandle const &other) :
    _isAcquired(false), _weakRef(0)
{
    *this = other;
}

Tf_PyIdHandle &
Tf_PyIdHandle::operator=(Tf_PyIdHandle const &other)
{
    CleanUp();
    if (other._weakRef) {
        _weakRef = other._weakRef;
        TfPyLock lock;
        Py_INCREF(_weakRef);
        if (other._isAcquired)
            Acquire();
    }
    return *this;
}

Tf_PyIdHandle::~Tf_PyIdHandle() {
    CleanUp();
}

void Tf_PyIdHandle::CleanUp() {
    if (_isAcquired)
        Release();
    TfPyLock lock;
    Py_XDECREF(_weakRef);
}

void Tf_PyIdHandle::Release() const {
    if (_weakRef and not _isAcquired) {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Releasing while not acquired!");
        return;
        // CODE_COVERAGE_ON
    }            
    if (PyObject *ptr = Ptr()) {
        _isAcquired = false;
        TfPyLock lock;
        Py_DECREF(ptr);
    } else {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Acquiring Python identity with "
                        "expired Python object!");
        TfLogStackTrace("Acquiring Python identity with "
                        "expired Python object!");
        // CODE_COVERAGE_ON
    }
}

void Tf_PyIdHandle::Acquire() const {
    if (_isAcquired) {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Acquiring while already acquired!");
        return;
        // CODE_COVERAGE_ON
    }
    if (PyObject *ptr = Ptr()) {
        _isAcquired = true;
        TfPyLock lock;
        Py_INCREF(ptr);
    } else {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Acquiring Python identity with expired Python "
                        "object!");
        TfLogStackTrace("Acquiring Python identity with expired Python "
                        "object!");
        // CODE_COVERAGE_ON
    }
}

PyObject *
Tf_PyIdHandle::Ptr() const {
    if (_weakRef) {
    TfPyLock lock;
        return PyWeakref_GetObject(_weakRef);
    }
    return 0;
}





typedef TfHashMap<void const *, Tf_PyIdHandle, TfHash> _IdentityMap;

static _IdentityMap& _GetIdentityMap()
{
    static _IdentityMap* _identityMap = new _IdentityMap();
    return *_identityMap;
}


static void _WeakBaseDied(void const *key) {
    // Erase python identity.
    Tf_PyIdentityHelper::Erase(key);
};

static std::string _GetTypeName(PyObject *obj) {
    using namespace boost::python;
    TfPyLock lock;
    handle<> typeHandle( borrowed<>( PyObject_Type(obj) ) );
    if (typeHandle) {
        object classObj(typeHandle);
        object nameObj(classObj.attr("__name__"));
        extract<string> name(nameObj);
        if (name.check())
            return name();
    }
    return "unknown";
}


#ifdef DEBUG_IDENTITY

#include <map>
#include <utility>
using std::map;
using std::make_pair;

TfStaticData<map<void const *, string> > _establishedIdentityStacks;

static void _RecordEstablishedIdentityStack(void const *key)
{
    _establishedIdentityStacks->insert(make_pair(key, TfGetStackTrace()));
}

static void _EraseEstablishedIdentityStack(void const *key)
{
    _establishedIdentityStacks->erase(key);
}

static void _IssueMultipleIdentityErrorStacks(void const *key)
{
    fprintf(stderr, "****** Original identity for %p established here:\n", key);
    fprintf(stderr, "%s\n",
            _establishedIdentityStacks->find(key)->second.c_str());
    fprintf(stderr, "****** Currently:\n");
    fprintf(stderr, "%s\n", TfGetStackTrace().c_str());
}

#else

static void _RecordEstablishedIdentityStack(void const *)
{
}

static void _EraseEstablishedIdentityStack(void const *)
{
}

static void _IssueMultipleIdentityErrorStacks(void const *)
{
}

#endif


// Set the identity of ptr (which derives from TfWeakBase) to be the
// python object \a obj.  
void Tf_PyIdentityHelper::Set(void const *key, PyObject *obj) {

    TfAutoMallocTag2 tag("Tf", "Tf_PyIdentityHelper::Set");
    
    static std::once_flag once;
    std::call_once(once, [](){
        Tf_ExpiryNotifier::SetNotifier(_WeakBaseDied);
    });
    
    if (not key or not obj)
        return;

    // printf("Setting Python object id %zu with "
    //        "type %s to have identity %p\n",
    //        (size_t) obj, _GetTypeName(obj).c_str(), key);

    TfPyLock lock;

    _IdentityMap& _identityMap = _GetIdentityMap();
    _IdentityMap::iterator i = _identityMap.find(key);

    if (i == _identityMap.end()) { 
        _identityMap[key] = Tf_PyIdHandle(obj);
        _RecordEstablishedIdentityStack(key);
    } else if (i->second.Ptr() != obj) {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Multiple Python objects for C++ object %p: "
                        "(Existing python object id %p with type %s, "
                        "new python object id %p with type %s)",
                        key, i->second.Ptr(),
                        _GetTypeName(i->second.Ptr()).c_str(),
                        obj, _GetTypeName(obj).c_str());
        _IssueMultipleIdentityErrorStacks(key);
        i->second = Tf_PyIdHandle(obj);
        // CODE_COVERAGE_ON
    }
}

// Return a new reference to the python object associated with ptr.  If
// there is none, return 0.
PyObject *Tf_PyIdentityHelper::Get(void const *key) {
    if (not key) {
        return 0;
    }

    TfPyLock lock;
    _IdentityMap& _identityMap = _GetIdentityMap();
    _IdentityMap::iterator i = _identityMap.find(key);
    if (i == _identityMap.end()) {
        return 0;
    }

    // use boost::python::xincref here, because it returns the increfed ptr.
    return boost::python::xincref(i->second.Ptr());
}


void
Tf_PyIdentityHelper::Erase(void const *key) {
    if (not key)
        return;
    TfPyLock lock;
    _GetIdentityMap().erase(key);
    _EraseEstablishedIdentityStack(key);
}


void Tf_PyIdentityHelper::Acquire(void const *key) {
    if (not key)
        return;

    TfPyLock lock;

    _IdentityMap& _identityMap = _GetIdentityMap();
    _IdentityMap::iterator i = _identityMap.find(key);
    if (i == _identityMap.end())
        return;

    i->second.Acquire();
}

void Tf_PyIdentityHelper::Release(void const *key) {
    if (not key)
        return;

    TfPyLock lock;

    _IdentityMap& _identityMap = _GetIdentityMap();
    _IdentityMap::iterator i = _identityMap.find(key);
    if (i == _identityMap.end())
        return;

    i->second.Release();
}


static TfStaticData<vector<PyGILState_STATE> > _pyLocks;
static void _LockPython() {
    // Python may already be shut down -- if so, don't do anything.
    if (Py_IsInitialized()) {
        _pyLocks->push_back(PyGILState_Ensure());
    }
}
static void _UnlockPython() {
    // Python may already be shut down -- if so, don't do anything.
    if (Py_IsInitialized()) {
        PyGILState_STATE state = _pyLocks->back();
        _pyLocks->pop_back();
        PyGILState_Release(state);
    }
}

void
Tf_PyOwnershipPtrMap::Insert(TfRefBase *refBase, void const *uniqueId)
{
    TfAutoMallocTag2 tag("Tf", "Tf_PyOwnershipPtrMap::Insert");
    static std::once_flag once;
    std::call_once(once, [](){
            TfRefBase::UniqueChangedListener l;
            l.lock = _LockPython;
            l.unlock = _UnlockPython;
            l.func = Tf_PyOwnershipRefBaseUniqueChanged;
            TfRefBase::SetUniqueChangedListener(l);
        });

    // Make sure we get called when the object's refcount changes from 2 ->
    // 1 or from 1 -> 2.
    refBase->SetShouldInvokeUniqueChangedListener(true);
    _cache[refBase] = uniqueId;
}

void const *
Tf_PyOwnershipPtrMap::Lookup(TfRefBase const *refBase)
{
    _CacheType::iterator i = _cache.find(refBase);
    if (i != _cache.end())
        return i->second;
    // CODE_COVERAGE_OFF Can only happen if there's a bug.
    return 0;
    // CODE_COVERAGE_ON
}

void
Tf_PyOwnershipPtrMap::Erase(TfRefBase *refBase)
{
    // Stop listening to when ptr's uniqueness changed.
    refBase->SetShouldInvokeUniqueChangedListener(false);
    _cache.erase(refBase);
}

void Tf_PyOwnershipRefBaseUniqueChanged(TfRefBase const *refBase,
                                        bool isNowUnique)
{
    // Python may already be shut down -- if so, don't do anything.
    if (not Py_IsInitialized())
        return;

    void const *uniqueId = Tf_PyOwnershipPtrMap::Lookup(refBase);

    if (not uniqueId) {
        // CODE_COVERAGE_OFF Can only happen if there's a bug.
        TF_CODING_ERROR("Couldn't get uniqueId associated with refBase!");
        TfLogStackTrace("RefBase Unique Changed Error");
        // CODE_COVERAGE_ON
    } else {
        if (isNowUnique)
            Tf_PyIdentityHelper::Release(uniqueId);
        else
            Tf_PyIdentityHelper::Acquire(uniqueId);
    }
}
