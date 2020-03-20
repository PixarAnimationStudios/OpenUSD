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

#include "pxr/base/tf/pyWeakObject.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <boost/python/class.hpp>

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

// A deleter instance is passed to PyWeakref_NewRef as the callback object
// so that when the python object we have the weak ref to dies, we can
// delete the corresponding weak object.
struct Tf_PyWeakObjectDeleter {
    static int WrapIfNecessary();
    explicit Tf_PyWeakObjectDeleter(Tf_PyWeakObjectPtr const &self);
    void Deleted(PyObject * /* weakRef */);
private:
    Tf_PyWeakObjectPtr _self;
};

int
Tf_PyWeakObjectDeleter::WrapIfNecessary()
{
    if (TfPyIsNone(TfPyGetClassObject<Tf_PyWeakObjectDeleter>())) {
        boost::python::class_<Tf_PyWeakObjectDeleter>
            ("Tf_PyWeakObject__Deleter", boost::python::no_init)
            .def("__call__", &Tf_PyWeakObjectDeleter::Deleted);
    }
    return 1;
}

Tf_PyWeakObjectDeleter::Tf_PyWeakObjectDeleter(Tf_PyWeakObjectPtr const &self)
    : _self(self)
{
    static int ensureWrapped = WrapIfNecessary();
    (void)ensureWrapped;
}

void
Tf_PyWeakObjectDeleter::Deleted(PyObject * /* weakRef */)
{
    _self->Delete();
}

Tf_PyWeakObjectPtr
Tf_PyWeakObject::GetOrCreate(boost::python::object const &obj)
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


boost::python::object
Tf_PyWeakObject::GetObject() const
{
    return boost::python::object
        (boost::python::handle<>
         (boost::python::borrowed(PyWeakref_GetObject(_weakRef.get()))));
}

void
Tf_PyWeakObject::Delete()
{
    Tf_PyWeakObjectRegistry::GetInstance().Remove(GetObject().ptr());
    delete this;
}
    
Tf_PyWeakObject::Tf_PyWeakObject(boost::python::object const &obj)
    : _weakRef(
        PyWeakref_NewRef(
            obj.ptr(), boost::python::
            object(Tf_PyWeakObjectDeleter(TfCreateWeakPtr(this))).ptr()))
{
    Tf_PyWeakObjectPtr self(this);
    
    // Set our python identity, but release it immediately, since we are a weak
    // reference and will expire as soon as the python object does.
    Tf_PyReleasePythonIdentity(self, GetObject().ptr());
    
    // Install us in the registry.
    Tf_PyWeakObjectRegistry::GetInstance().Insert(GetObject().ptr(), self);
}

PXR_NAMESPACE_CLOSE_SCOPE
