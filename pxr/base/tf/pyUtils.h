//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_UTILS_H
#define PXR_BASE_TF_PY_UTILS_H

/// \file tf/pyUtils.h
/// Miscellaneous Utilities for dealing with script.

#include "pxr/pxr.h"

#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/pySafePython.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/api.h"

#include <functional>
#include <typeinfo>
#include <string>
#include <vector>

#include <nanobind/nanobind.h>

PXR_NAMESPACE_OPEN_SCOPE

/// A macro which expands to the proper __repr__ prefix for a library.  This is
/// the "canonical" name of the module that the system uses to identify it
/// followed by a '.'.  This can be used in the implementation of __repr__
///
/// \hideinitializer
#define TF_PY_REPR_PREFIX \
    std::string(TF_PP_STRINGIZE(MFB_PACKAGE_MODULE) ".")

/// Returns true if python is initialized.
TF_API bool TfPyIsInitialized();

/// Raises a Python \c IndexError with the given error \p msg and throws a
/// nanobind::python_error exception. Callers must hold the GIL
/// before calling this function.
TF_API void TfPyThrowIndexError(const char *msg);

/// \overload
inline void TfPyThrowIndexError(std::string const &msg)
{
    TfPyThrowIndexError(msg.c_str());
}

/// Raises a Python \c RuntimeError with the given error \p msg and throws a
/// nanobind::python_error exception. Callers must hold the GIL
/// before calling this function.
TF_API void TfPyThrowRuntimeError(const char *msg);

/// \overload
inline void TfPyThrowRuntimeError(std::string const &msg)
{
    TfPyThrowRuntimeError(msg.c_str());
}

/// Raises a Python \c StopIteration with the given error \p msg and throws a
/// nanobind::python_error exception. Callers must hold the GIL
/// before calling this function.
TF_API void TfPyThrowStopIteration(const char *msg);

/// \overload
inline void TfPyThrowStopIteration(std::string const &msg)
{
    TfPyThrowStopIteration(msg.c_str());
}

/// Raises a Python \c KeyError with the given error \p msg and throws a
/// nanobind::python_error exception. Callers must hold the GIL
/// before calling this function.
TF_API void TfPyThrowKeyError(const char *msg);

/// \overload
inline void TfPyThrowKeyError(std::string const &msg)
{
    TfPyThrowKeyError(msg.c_str());
}

/// Raises a Python \c ValueError with the given error \p msg and throws a
/// nanobind::python_error exception. Callers must hold the GIL
/// before calling this function.
TF_API void TfPyThrowValueError(const char *msg);

/// \overload
inline void TfPyThrowValueError(std::string const &msg)
{
    TfPyThrowValueError(msg.c_str());
}

/// Raises a Python \c TypeError with the given error \p msg and throws a
/// nanobind::python_error exception. Callers must hold the GIL
/// before calling this function.
TF_API void TfPyThrowTypeError(const char *msg);

/// \overload
inline void TfPyThrowTypeError(std::string const &msg)
{
    TfPyThrowTypeError(msg.c_str());
}

/// Return true iff \a obj is None.
inline bool TfPyIsNone(nanobind::handle const &obj)
{
    return !obj.ptr() || obj.is_none();
}

/// Get a strong reference to the object referred to by the weak reference
/// \a ref.
///
/// This behaves exactly like PyWeakref_GetRef, which was added in Python 3.13:
/// on success, store a new strong reference to the referent in \p *pobj and
/// return 1; if the reference is dead, store null in \p *pobj and return 0; on
/// error, raise an exception and return -1.  Callers must hold the GIL.
///
/// As with PyWeakref_GetRef, \p *pobj is unspecified when this returns -1.
/// Callers must consult the return value and must not inspect \p *pobj unless
/// it returned 1 or 0.
///
/// PyWeakref_GetRef supersedes PyWeakref_GetObject, which is deprecated in 3.13
/// and slated for removal in 3.15.  This function exists only to provide
/// PyWeakref_GetRef on Python versions before 3.13.  Once the minimum supported
/// Python is 3.13, every call can be replaced with a direct call to
/// PyWeakref_GetRef and this function deleted.
inline int
Tf_PyWeakrefGetRef(PyObject *ref, PyObject **pobj)
{
#if PY_VERSION_HEX >= 0x030D0000
    return PyWeakref_GetRef(ref, pobj);
#else
    PyObject *obj = PyWeakref_GetObject(ref);
    if (!obj) {
        // Not a weak reference; PyWeakref_GetObject has set an exception.
        // Leave *pobj alone: PyWeakref_GetRef does not specify its value on
        // error, so neither do we.
        return -1;
    }
    if (obj == Py_None) {
        // The referent has expired.  None itself is not weak-referenceable, so
        // this cannot be a live referent.
        *pobj = nullptr;
        return 0;
    }
    Py_INCREF(obj);
    *pobj = obj;
    return 1;
#endif
}

// Helper for \c TfPyObject().
TF_API void Tf_PyObjectError(bool printError);

/// Return a python object for the given C++ object, loading the appropriate
/// wrapper code if necessary. Spams users if complainOnFailure is true and
/// conversion fails.
template <typename T>
nanobind::object TfPyObject(T const &t, bool complainOnFailure = true) {
    // initialize python if it isn't already, so at least we can try to return
    // an object
    if (!TfPyIsInitialized()) {
        TF_CODING_ERROR("Called TfPyObject without python being initialized!");
        TfPyInitialize();
    }

    TfPyLock pyLock;

    // Will only be able to return objects which have been wrapped.
    // Returns None otherwise
    try {
        nanobind::object nbObj = nanobind::cast(t);
        if (nbObj.is_valid()) {
            return nbObj;
        }
    } catch (nanobind::cast_error const &) {
    } catch (nanobind::python_error &e) {
        e.restore();
    }
    Tf_PyObjectError(complainOnFailure);
    return nanobind::none();
}

inline
nanobind::object TfPyObject(PyObject* t, bool complainOnFailure = true) {
    TfPyLock pyLock;
    return t ? nanobind::steal<nanobind::object>(t) : nanobind::none();
}

/// Return repr(t).
///
/// Calls PyObject_Repr on the given python object.
TF_API std::string TfPyObjectRepr(nanobind::handle const &t);

/// Return repr(t).
///
/// Converts t to its equivalent python object and then calls PyObject_Repr on
/// that.
template <typename T>
std::string TfPyRepr(T const &t) {
    if (!TfPyIsInitialized())
        return "<python not initialized>";
    TfPyLock lock;
    return TfPyObjectRepr(TfPyObject(t));
}

/// Return repr(t) for a vector as a python list.
template <typename T>
std::string TfPyRepr(const std::vector<T> &v) {
    std::string result("[");
    typename std::vector<T>::const_iterator i = v.begin();
    if (i != v.end()) {
        result += TfPyRepr(*i);
        ++i;
    }
    while (i != v.end()) {
        result += ", " + TfPyRepr(*i);
        ++i;
    }
    result += "]";
    return result;
}

/// Evaluate python expression \a expr with all the known script modules
/// imported under their standard names. Additional globals may be provided in
/// the \p extraGlobals dictionary.
TF_API
nanobind::object
TfPyEvaluate(
    std::string const &expr,
    nanobind::dict const &extraGlobals = nanobind::dict());

/// Return a positive index in the range [0,size).  If \a throwError is true,
/// this will throw an index error if the resulting index is out of range.
TF_API 
int64_t
TfPyNormalizeIndex(int64_t index, uint64_t size, bool throwError = false);

/// Return the name of the class of \a obj.
TF_API std::string TfPyGetClassName(nanobind::handle const &obj);


/// Return the python class object for \a type if \a type has been wrapped.
/// Otherwise return None.
TF_API nanobind::object
TfPyGetClassObject(std::type_info const &type);

/// Return the python class object for T if T has been wrapped.
/// Otherwise return None.
template <typename T>
nanobind::object
TfPyGetClassObject() {
    return TfPyGetClassObject(typeid(T));
}

TF_API
void
Tf_PyWrapOnceImpl(std::type_info const &,
                  std::function<void()> const&,
                  bool *);

/// Invokes \p wrapFunc to wrap type \c T if \c T is not already wrapped.
///
/// Executing \p wrapFunc *must* register \c T with nanobind.
/// Otherwise, \p wrapFunc may be executed more than once.
///
/// TfPyWrapOnce will acquire the GIL prior to invoking \p wrapFunc. Does not
/// invoke \p wrapFunc if Python has not been initialized.
template <typename T>
void
TfPyWrapOnce(std::function<void()> const &wrapFunc)
{
    // Don't try to wrap if python isn't initialized.
    if (!TfPyIsInitialized()) {
        return;
    }

    static bool isTypeWrapped = false;
    if (isTypeWrapped) {
        return;
    }

    Tf_PyWrapOnceImpl(typeid(T), wrapFunc, &isTypeWrapped);
}

/// Load the python module \a moduleName.  This is used by some low-level
/// infrastructure code to load python wrapper modules corresponding to C++
/// shared libraries when they are needed.  It should generally not need to be
/// called from normal user code.
TF_API
void Tf_PyLoadScriptModule(std::string const &name);

/// Creates a python dictionary from a std::map.
template <class Map>
nanobind::dict TfPyCopyMapToDictionary(Map const &map) {
    TfPyLock lock;
    nanobind::dict d;
    for (typename Map::const_iterator i = map.begin(); i != map.end(); ++i)
        d[nanobind::cast(i->first)] = nanobind::cast(i->second);
    return d;
}

template<class Seq>
nanobind::list TfPyCopySequenceToList(Seq const &seq) {
    TfPyLock lock;
    nanobind::list l;
    for (typename Seq::const_iterator i = seq.begin();
         i != seq.end(); ++i)
        l.append(nanobind::cast(*i));
    return l; 
}

/// Create a python set from an iterable sequence.
///
/// If Seq::value_type is not hashable, TypeError is raised via throwing
/// nanobind::python_error.
template <class Seq>
nanobind::object TfPyCopySequenceToSet(Seq const &seq) {
    TfPyLock lock;
    PyObject *set = PySet_New(nullptr);
    if (!set) {
        throw nanobind::python_error();
    }
    for (auto const& item : seq) {
        nanobind::object obj = nanobind::cast(item);
        if (PySet_Add(set, obj.ptr()) == -1) {
            Py_DECREF(set);
            throw nanobind::python_error();
        }
    }
    return nanobind::steal<nanobind::object>(set);
}

template<class Seq>
nanobind::tuple TfPyCopySequenceToTuple(Seq const &seq) {
    return nanobind::tuple(TfPyCopySequenceToList(seq));
}

/// Create a python bytearray from an input buffer and size.
///
/// If a size of zero is passed in this function will return a valid python
/// bytearray of size zero.
///
/// An invalid object handle is returned on failure.
TF_API
nanobind::object TfPyCopyBufferToByteArray(const char* buffer, size_t size);

/// Return a vector of strings containing the current python traceback.
///
/// The vector contains the same strings that python's traceback.format_stack()
/// returns.
TF_API
std::vector<std::string> TfPyGetTraceback();

/// Set an environment variable in \c os.environ.
///
/// This function is equivalent to
///
/// \code
///    def PySetenv(name, value):
///        try:
///            import os
///            os.environ[name] = value
///            return True
///        except:
///            return False
/// \endcode
///
/// Calling this function without first initializing Python is an error and
/// returns \c false.
///
/// Note that this function will import the \c os module, causing \c
/// os.environ to be poputated.  All modifications to the environment after \c
/// os has been imported must be made with this function or \c TfSetenv if it
/// important that they appear in \c os.environ.
TF_API
bool TfPySetenv(const std::string & name, const std::string & value);

/// Remove an environment variable from \c os.environ.
///
/// This function is equivalent to
///
/// \code
///    def PyUnsetenv(name):
///        try:
///            import os
///            if name in os.environ:
///                del os.environ[name]
///            return True
///        except:
///            return False
/// \endcode
///
/// Calling this function without first initializing Python is an error and
/// returns \c false.
///
/// Note that this function will import the \c os module, causing \c
/// os.environ to be poputated.  All modifications to the environment after \c
/// os has been imported must be made with this function or \c TfUnsetenv if
/// it important that they appear in \c os.environ.
TF_API
bool TfPyUnsetenv(const std::string & name);

// Private helper method to TfPyEvaluateAndExtract.
//
TF_API bool Tf_PyEvaluateWithErrorCheck(
    const std::string & expr, nanobind::object * obj);

/// Safely evaluates \p expr and extracts the return object of type T. If
/// successful, returns \c true and sets *t to the return value, otherwise
/// returns \c false.
template <typename T>
bool TfPyEvaluateAndExtract(const std::string & expr, T * t)
{
    if (expr.empty())
        return false;

    // Take the lock before doing anything with nanobind::object.
    TfPyLock lock;

    nanobind::object obj;
    if (!Tf_PyEvaluateWithErrorCheck(expr, &obj))
        return false;

    return nanobind::try_cast<T>(obj, *t);
}

/// Print a standard traceback to sys.stderr and clear the error indicator.
/// If the error is a KeyboardInterrupt then this does nothing.  Call this
/// function only when the error indicator is set.
TF_API
void TfPyPrintError();

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/base/tf/pyNanobindBridge.h"

#endif // PXR_BASE_TF_PY_UTILS_H

