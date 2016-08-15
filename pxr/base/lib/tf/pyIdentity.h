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
#ifndef TF_PYIDENTITY_H
#define TF_PYIDENTITY_H

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/weakPtr.h"

#include <boost/python/class.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility.hpp>

#include "pxr/base/tf/hashmap.h"

// Specializations for boost::python::pointee and get_pointer for TfRefPtr and
// TfWeakPtr.
namespace boost { namespace python {

// TfWeakPtrFacade
template <template <class> class X, class Y>
struct pointee< TfWeakPtrFacade<X, Y> > {
    typedef Y type;
};

// TfRefPtr
template <typename T>
struct pointee< TfRefPtr<T> > {
    typedef T type;
};

}}

struct Tf_PyIdentityHelper
{
    // Set the identity of ptr (which derives from TfPtrBase) to be the
    // python object \a obj.  
    static void Set(void const *id, PyObject *obj);

    // Return a new reference to the python object associated with ptr.  If
    // there is none, return 0.
    static PyObject *Get(void const *id);

    static void Erase(void const *id);

    // Acquire a reference to the python object associated with ptrBase
    // if not already acquired.
    static void Acquire(void const *id);

    // Release a reference to the python object associated with ptrBase
    // if we own a reference.
    static void Release(void const *id);
    
};

template <class Ptr>
void Tf_PyReleasePythonIdentity(Ptr const &ptr, PyObject *obj)
{
    Tf_PySetPythonIdentity(ptr, obj);
    Tf_PyIdentityHelper::Release(ptr.GetUniqueIdentifier());
}

void Tf_PyOwnershipRefBaseUniqueChanged(TfRefBase const *refBase,
                                        bool isNowUnique);

struct Tf_PyOwnershipPtrMap
{
    typedef TfHashMap<TfRefBase const *, void const *, TfHash>
    _CacheType;
    static void Insert(TfRefBase *refBase, void const *uniqueId);
    static void const *Lookup(TfRefBase const *refBase);
    static void Erase(TfRefBase *refBase);
  private:
    static _CacheType _cache;
};


// Doxygen generates files whose names are mangled typenames.  This is fine
// except when the filenames get longer than 256 characters.  This is one case
// of that, so we'll just disable doxygen.  There's no actual doxygen doc here,
// so this is fine.  If/when this gets solved for real, we can remove this
// (6/06)
#ifndef doxygen


template <class Ptr, typename Enable = void>
struct Tf_PyOwnershipHelper {
    template <typename U>
    static void Add(U const &, const void *, PyObject *) {}
    template <typename U>
    static void Remove(U const &, PyObject *) {}
};

template <typename Ptr>
struct Tf_PyOwnershipHelper<Ptr,
    typename boost::enable_if<
        boost::mpl::and_<boost::is_same<TfRefPtr<typename Ptr::DataType>, Ptr>,
            boost::is_base_of<TfRefBase, typename Ptr::DataType> > >::type>
{
    struct _RefPtrHolder {
        static boost::python::object
        Get(Ptr const &refptr) {
            TfPyLock pyLock;
            _WrapIfNecessary();
            return boost::python::object(_RefPtrHolder(refptr));
        }
        static void _WrapIfNecessary() {
            TfPyLock pyLock;
            if (TfPyIsNone(TfPyGetClassObject<_RefPtrHolder>())) {
                std::string name =
                    "__" + ArchGetDemangled(typeid(typename Ptr::DataType)) +
                    "__RefPtrHolder";
                name = TfStringReplace(name, "<", "_");
                name = TfStringReplace(name, ">", "_");
                name = TfStringReplace(name, "::", "_");
                boost::python::class_<_RefPtrHolder>(name.c_str(),
                                                     boost::python::no_init);
            }
        }
      private:
        explicit _RefPtrHolder(Ptr const &refptr) : _refptr(refptr) {}
        Ptr _refptr;
    };
    
    static void Add(Ptr ptr, const void *uniqueId, PyObject *self) {

        TfPyLock pyLock;

        // Make the python object keep the c++ object alive.
        int ret = PyObject_SetAttrString(self, "__owner",
                                         _RefPtrHolder::Get(ptr).ptr());
        if (ret == -1) {
            // CODE_COVERAGE_OFF
            TF_WARN("Could not set __owner attribute on python object!");
            PyErr_Clear();
            return;
            // CODE_COVERAGE_ON
        }
        TfRefBase *refBase =
            static_cast<TfRefBase *>(get_pointer(ptr));
        Tf_PyOwnershipPtrMap::Insert(refBase, uniqueId);
    }
    
    static void Remove(Ptr ptr, PyObject *obj) {
        TfPyLock pyLock;

        if (not ptr) {
            // CODE_COVERAGE_OFF Can only happen if there's a bug.
            TF_CODING_ERROR("Removing ownership from null/expired ptr!");
            return;
            // CODE_COVERAGE_ON
        }
        
        if (PyObject_HasAttrString(obj, "__owner")) {
            // We are guaranteed that ptr is not unique at this point,
            // as __owner has a reference and ptr is itself a
            // reference.  This also guarantees us that the object owns
            // a reference to its python object, so we don't need to
            // explicitly acquire a reference here.
            TF_AXIOM(not ptr->IsUnique());
            // Remove this object from the cache of refbase to uniqueId
            // that we use for python-owned things.
            Tf_PyOwnershipPtrMap::Erase(get_pointer(ptr));
            // Remove the __owner attribute.
            if (PyObject_DelAttrString(obj, "__owner") == -1) {
                // CODE_COVERAGE_OFF It's hard to make this occur.
                TF_WARN("Undeletable __owner attribute on python object!");
                PyErr_Clear();
                // CODE_COVERAGE_ON
            }
        }
    }
};

#endif // doxygen -- see comment above.


template <typename Ptr>
struct Tf_PyIsRefPtr {
    static const bool value = false;
};

template <typename T>
struct Tf_PyIsRefPtr<TfRefPtr<T> > {
    static const bool value = true;
};


template <class Ptr>
typename boost::enable_if<Tf_PyIsRefPtr<Ptr> >::type
Tf_PySetPythonIdentity(Ptr const &, PyObject *)
{
}

template <class Ptr>
typename boost::disable_if<Tf_PyIsRefPtr<Ptr> >::type
Tf_PySetPythonIdentity(Ptr const &ptr, PyObject *obj)
{
    if (ptr.GetUniqueIdentifier()) {
        Tf_PyIdentityHelper::Set(ptr.GetUniqueIdentifier(), obj);
        // Make sure we hear about it when this weak base dies so we can remove
        // it from the map.
        ptr.EnableExtraNotification();
    }
}

template <class Ptr>
PyObject *Tf_PyGetPythonIdentity(Ptr const &ptr)
{
    PyObject *ret = Tf_PyIdentityHelper::Get(ptr.GetUniqueIdentifier());
    return ret;
}

template <class Ptr>
void Tf_PyRemovePythonOwnership(Ptr const &t, PyObject *obj)
{
    Tf_PyOwnershipHelper<Ptr>::Remove(t, obj);
}

template <class Ptr>
void Tf_PyAddPythonOwnership(Ptr const &t, const void *uniqueId, PyObject *obj)
{
    Tf_PyOwnershipHelper<Ptr>::Add(t, uniqueId, obj);
}

#endif // TF_PYIDENTITY_H
