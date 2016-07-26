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
#ifndef TF_PYWEAKOBJECT_H
#define TF_PYWEAKOBJECT_H

#include "pxr/base/tf/pyIdentity.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"

#include <boost/python/class.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>

#include "pxr/base/tf/hashmap.h"


typedef TfWeakPtr<class Tf_PyWeakObject> Tf_PyWeakObjectPtr;

struct Tf_PyWeakObjectRegistry
{
    typedef Tf_PyWeakObjectRegistry This;
    
    //! \brief Return the singleton instance.
    static This &GetInstance() {
        return TfSingleton<This>::GetInstance();
    } 

    void Insert(PyObject *obj, Tf_PyWeakObjectPtr const &weakObj) {
        _weakObjects[obj] = weakObj;
    }

    Tf_PyWeakObjectPtr Lookup(PyObject *obj) {
        if (_weakObjects.count(obj))
            return _weakObjects[obj];
        return Tf_PyWeakObjectPtr();
    }

    void Remove(PyObject *obj) {
        _weakObjects.erase(obj);
    }

  private:

    Tf_PyWeakObjectRegistry() {}
    virtual ~Tf_PyWeakObjectRegistry();
    friend class TfSingleton<This>;

    TfHashMap<PyObject *, Tf_PyWeakObjectPtr, TfHash> _weakObjects;

};


// A weak pointable weak reference to a python object.
struct Tf_PyWeakObject : public TfWeakBase
{
    typedef Tf_PyWeakObject This;

    // A deleter instance is passed to PyWeakref_NewRef as the callback object
    // so that when the python object we have the weak ref to dies, we can
    // delete the corresponding weak object.
    struct Deleter {
        static void WrapIfNecessary() {
            if (TfPyIsNone(TfPyGetClassObject<Deleter>()))
                boost::python::class_<Deleter>
                    ("Tf_PyWeakObject__Deleter", boost::python::no_init)
                    .def("__call__", &This::Deleter::Deleted);
        }
        explicit Deleter(Tf_PyWeakObjectPtr const &self)
            : _self(self) {
            WrapIfNecessary();
        }
        void Deleted(PyObject * /* weakRef */) {
            _self->Delete();
        }
      private:
        Tf_PyWeakObjectPtr _self;
    };

    static Tf_PyWeakObjectPtr GetOrCreate(boost::python::object const &obj) {
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
    
    boost::python::object GetObject() const {
        return boost::python::object
            (boost::python::handle<>
             (boost::python::borrowed(PyWeakref_GetObject(_weakRef.get()))));
    }

    void Delete() {
        Tf_PyWeakObjectRegistry::GetInstance().Remove(GetObject().ptr());
        delete this;
    }
    
  private:
    explicit Tf_PyWeakObject(boost::python::object const &obj) :
        _weakRef(PyWeakref_NewRef(obj.ptr(), boost::python::
                                  object(Deleter(TfCreateWeakPtr(this))).ptr()))
    {
        Tf_PyWeakObjectPtr self(this);
        
        // Set our python identity, but release it immediately, since we are a
        // weak reference and will expire as soon as the python object does.
        Tf_PyReleasePythonIdentity(self, GetObject().ptr());

        // Install us in the registry.
        Tf_PyWeakObjectRegistry::GetInstance().Insert(GetObject().ptr(), self);
    }
    
    boost::python::handle<> _weakRef;
    
};

#endif // TF_PYWEAKOBJECT_H
