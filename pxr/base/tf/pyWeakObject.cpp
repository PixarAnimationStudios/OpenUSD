//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/pyWeakObject.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE

struct Tf_PyWeakObjectRegistry
{
    typedef Tf_PyWeakObjectRegistry This;
    
    /// Return the singleton instance.
    static This &GetInstance();
    void Insert(PyObject *obj, Tf_PyWeakObjectPtr const &weakObj);
    Tf_PyWeakObjectPtr Lookup(PyObject *obj) const;
    void Remove(PyObject *obj);

  private:
    Tf_PyWeakObjectRegistry() = default;
    friend class TfSingleton<This>;

    TfHashMap<PyObject *, Tf_PyWeakObjectPtr, TfHash> _weakObjects;
};

TF_INSTANTIATE_SINGLETON(Tf_PyWeakObjectRegistry);

Tf_PyWeakObjectRegistry &
Tf_PyWeakObjectRegistry::GetInstance()
{
    return TfSingleton<This>::GetInstance();
}

void
Tf_PyWeakObjectRegistry::Insert(PyObject *obj,
                                Tf_PyWeakObjectPtr const &weakObj)
{
    _weakObjects[obj] = weakObj;
}

Tf_PyWeakObjectPtr
Tf_PyWeakObjectRegistry::Lookup(PyObject *obj) const
{
    auto iter = _weakObjects.find(obj);
    return iter == _weakObjects.end() ? Tf_PyWeakObjectPtr() : iter->second;
}

void
Tf_PyWeakObjectRegistry::Remove(PyObject *obj)
{
    _weakObjects.erase(obj);
}

Tf_PyWeakObjectPtr
Tf_PyWeakObject::GetOrCreate(nanobind::handle obj)
{
    // If it's in the registry, return it.
    if (Tf_PyWeakObjectPtr p =
        Tf_PyWeakObjectRegistry::GetInstance().Lookup(obj.ptr()))
        return p;
    // Otherwise, make sure we can create a python weak reference to the
    // object.
    if (PyObject *weakRef = PyWeakref_NewRef(obj.ptr(), NULL)) {
        Py_DECREF(weakRef);
        return TfCreateWeakPtr(new Tf_PyWeakObject(obj));
    }
    // Cannot create a weak reference to obj -- return a null pointer.
    PyErr_Clear();
    return Tf_PyWeakObjectPtr();
}


nanobind::object
Tf_PyWeakObject::GetObject() const
{
    PyObject *obj;
    if (Tf_PyWeakrefGetRef(_weakRef.ptr(), &obj) != 1) {
        // The referent has expired; report it as None.
        return nanobind::none();
    }
    return nanobind::steal(obj);
}

void
Tf_PyWeakObject::Delete()
{
    // Use the cached key rather than GetObject(): our weak reference has
    // already been cleared by the time this callback runs.
    Tf_PyWeakObjectRegistry::GetInstance().Remove(_refObjKey);
    delete this;
}

Tf_PyWeakObject::Tf_PyWeakObject(nanobind::handle obj)
    : _refObjKey(obj.ptr())
{
    Tf_PyWeakObjectPtr self(this);
    nanobind::object deleter = nanobind::cpp_function(
        [self](nanobind::handle) {
            if (self) {
                self->Delete();
            }
        });
    _weakRef = nanobind::steal(PyWeakref_NewRef(obj.ptr(), deleter.ptr()));

    // Set our python identity, but release it immediately, since we are a weak
    // reference and will expire as soon as the python object does.
    Tf_PyReleasePythonIdentity(self, _refObjKey);

    // Install us in the registry.
    Tf_PyWeakObjectRegistry::GetInstance().Insert(_refObjKey, self);
}

PXR_NAMESPACE_CLOSE_SCOPE
