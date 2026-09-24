//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pySafePython.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stackTrace.h"

#include <mutex>
#include <vector>

// Compile-time option to help debug identity issues.
//#define DEBUG_IDENTITY

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

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

    // Return a new reference to the referent, or null if it has expired.
    PyObject *GetRef() const;

    // Return the referent without owning a reference to it, or null if it has
    // expired.  Only valid for comparison and diagnostics; the result may
    // expire as soon as the caller drops the GIL.
    PyObject *PeekPtr() const;

    // The reference acquired by Acquire(), or null when not acquired.
    mutable PyObject *_acquired;
    PyObject *_weakRef;
};



Tf_PyIdHandle::Tf_PyIdHandle() :
    _acquired(nullptr), _weakRef(nullptr)
{
}

Tf_PyIdHandle::Tf_PyIdHandle(PyObject *obj) :
    _acquired(nullptr), _weakRef(nullptr)
{
    TfPyLock lock;
    _weakRef = PyWeakref_NewRef(obj, 0);
    Acquire();
}

Tf_PyIdHandle::Tf_PyIdHandle(Tf_PyIdHandle const &other) :
    _acquired(nullptr), _weakRef(nullptr)
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
        if (other._acquired) {
            Acquire();
        }
    }
    return *this;
}

Tf_PyIdHandle::~Tf_PyIdHandle() {
    CleanUp();
}

void Tf_PyIdHandle::CleanUp() {
    if (_acquired) {
        Release();
    }
    TfPyLock lock;
    Py_CLEAR(_weakRef);
}

void Tf_PyIdHandle::Release() const {
    if (_weakRef && !_acquired) {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Releasing while not acquired!");
        return;
        // CODE_COVERAGE_ON
    }
    // Holding the acquired reference keeps the referent alive, so there is no
    // need to consult the weak reference to find it.
    TfPyLock lock;
    Py_CLEAR(_acquired);
}

void Tf_PyIdHandle::Acquire() const {
    if (_acquired) {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Acquiring while already acquired!");
        return;
        // CODE_COVERAGE_ON
    }
    TfPyLock lock;
    // The reference returned here becomes the acquired reference.
    _acquired = GetRef();
    if (!_acquired) {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        TF_CODING_ERROR("Acquiring Python identity with expired Python "
                        "object!");
        TfLogStackTrace("Acquiring Python identity with expired Python "
                        "object!");
        // CODE_COVERAGE_ON
    }
}

PyObject *
Tf_PyIdHandle::GetRef() const {
    if (!_weakRef) {
        return nullptr;
    }
    TfPyLock lock;
    PyObject *obj;
    if (Tf_PyWeakrefGetRef(_weakRef, &obj) < 0) {
        // CODE_COVERAGE_OFF _weakRef always comes from PyWeakref_NewRef, so
        // this can only happen if there's a bug.  Note that obj is unspecified
        // on this path and must not be read.
        TF_CODING_ERROR("Expected a Python weak reference");
        PyErr_Clear();
        return nullptr;
        // CODE_COVERAGE_ON
    }
    return obj;
}

PyObject *
Tf_PyIdHandle::PeekPtr() const {
    TfPyLock lock;
    PyObject *obj = GetRef();
    Py_XDECREF(obj);
    return obj;
}


typedef TfHashMap<void const *, Tf_PyIdHandle, TfHash> _IdentityMap;

static _IdentityMap& _GetIdentityMap()
{
    static _IdentityMap* _identityMap = new _IdentityMap();
    return *_identityMap;
}


static void _WeakBaseDied(void const *key) {
    // Python may be already shut down (like in exit()).  If so, do nothing.
    if (!Py_IsInitialized()) {
        return;
    }    
    // Erase python identity.
    Tf_PyIdentityHelper::Erase(key);
};

static std::string _GetTypeName(PyObject *obj) {
    using namespace pxr_boost::python;
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
    
    if (!key || !obj)
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
    } else if (i->second.PeekPtr() != obj) {
        // CODE_COVERAGE_OFF Can only get here if there's a bug.
        // Hold a reference to the existing object while we report on it, since
        // _GetTypeName calls back into Python.
        pxr_boost::python::handle<> existing {
            pxr_boost::python::allow_null(i->second.GetRef()) };
        TF_CODING_ERROR("Multiple Python objects for C++ object %p: "
                        "(Existing python object id %p with type %s, "
                        "new python object id %p with type %s)",
                        key, existing.get(),
                        existing ? _GetTypeName(existing.get()).c_str()
                                 : "expired",
                        obj, _GetTypeName(obj).c_str());
        _IssueMultipleIdentityErrorStacks(key);
        i->second = Tf_PyIdHandle(obj);
        // CODE_COVERAGE_ON
    }
}

// Return a new reference to the python object associated with ptr.  If
// there is none, return 0.
PyObject *Tf_PyIdentityHelper::Get(void const *key) {
    if (!key) {
        return 0;
    }

    TfPyLock lock;
    _IdentityMap& _identityMap = _GetIdentityMap();
    _IdentityMap::iterator i = _identityMap.find(key);
    if (i == _identityMap.end()) {
        return 0;
    }

    // Returns a new reference, or null if the Python object has expired.
    return i->second.GetRef();
}


void
Tf_PyIdentityHelper::Erase(void const *key) {
    if (!key)
        return;
    TfPyLock lock;
    _GetIdentityMap().erase(key);
    _EraseEstablishedIdentityStack(key);
}


void Tf_PyIdentityHelper::Acquire(void const *key) {
    if (!key)
        return;

    TfPyLock lock;

    _IdentityMap& _identityMap = _GetIdentityMap();
    _IdentityMap::iterator i = _identityMap.find(key);
    if (i == _identityMap.end())
        return;

    i->second.Acquire();
}

void Tf_PyIdentityHelper::Release(void const *key) {
    if (!key)
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
    if (!Py_IsInitialized())
        return;

    void const *uniqueId = Tf_PyOwnershipPtrMap::Lookup(refBase);

    if (!uniqueId) {
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

PXR_NAMESPACE_CLOSE_SCOPE
