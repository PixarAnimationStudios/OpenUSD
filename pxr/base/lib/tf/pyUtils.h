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
#ifndef TF_PYUTILS_H
#define TF_PYUTILS_H

///
/// \file pyUtils.h
/// \brief Miscellaneous Utilities for dealing with script.
///

#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/api.h"

#include <typeinfo>
#include <string>
#include <vector>

#include <boost/python/object.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/type_id.hpp>

/// A macro which expands to the proper __repr__ prefix for a library.  This is
/// the "canonical" name of the module that the system uses to identify it
/// followed by a '.'.  This can be used in the implementation of __repr__
#define TF_PY_REPR_PREFIX \
    std::string(BOOST_PP_STRINGIZE(MFB_PACKAGE_MODULE) ".")

//! \brief Returns true if python is initialized.
TF_API bool TfPyIsInitialized();

/// \brief Raises a python index error and throws a C++ exception.  Intended to
/// be used in wrapper code.
TF_API void TfPyThrowIndexError(std::string const &msg);

/// \brief Raises a python runtime error and throws a C++ exception.  Intended
/// to be used in wrapper code.
TF_API void TfPyThrowRuntimeError(std::string const &msg);

/// \brief Raises a python StopIteration exception and throws a C++ exception.
/// Intended to be used in wrapper code.
TF_API void TfPyThrowStopIteration(std::string const &msg);

/// \brief Raises a python key error and throws a C++ exception.  Intended for
/// use in wrapper code.
TF_API void TfPyThrowKeyError(std::string const &msg);

/// \brief Raises a python value error and throws a C++ exception.  Intended
/// to be used in wrapper code.
TF_API void TfPyThrowValueError(std::string const &msg);

/// \brief Raises a python type error and throws a C++ exception.  Intended
/// to be used in wrapper code.
TF_API void TfPyThrowTypeError(std::string const &msg);

/// \brief Return true iff \a obj is None.
TF_API bool TfPyIsNone(boost::python::object const &obj);

/// \brief Return true iff \a obj is None.
TF_API bool TfPyIsNone(boost::python::handle<> const &obj);

// Helper for \c TfPyObject().
TF_API void Tf_PyObjectError(bool printError);

/// \brief Return a python object for the given C++ object, loading the
/// appropriate wrapper code if necessary.  Spams users if complainOnFailure is true and conversion fails.
template <typename T>
boost::python::object TfPyObject(T const &t, bool complainOnFailure = true) {
    // initialize python if it isn't already, so at least we can try to
    // return an object
    if (!TfPyIsInitialized()) {
        // CODE_COVERAGE_OFF
        TF_CODING_ERROR("Called TfPyObject without python being initialized!");
        TfPyInitialize();
        // CODE_COVERAGE_ON
    }

    TfPyLock pyLock;

    // will only be able to return objects which have been wrapped.
    // returns None otherwise
    try {
        return boost::python::object(t);
    } catch (boost::python::error_already_set const &) {
        Tf_PyObjectError(complainOnFailure);
        return boost::python::object();
   }
}

inline
boost::python::object TfPyObject(PyObject* t, bool complainOnFailure = true) {
    TfPyLock pyLock;
    return boost::python::object(boost::python::handle<>(t));
}

/// \brief Return repr(t).
//
// Calls PyObject_Repr on the given python object.
TF_API std::string TfPyObjectRepr(boost::python::object const &t);

/// \brief Return repr(t).
//
// Converts t to its equivalent python object and then calls
// PyObject_Repr on that.
template <typename T>
std::string TfPyRepr(T const &t) {
    if (!TfPyIsInitialized())
        return "<python not initialized>";
    TfPyLock lock;
    return TfPyObjectRepr(TfPyObject(t));
}

/// \brief Return repr(t) for a vector as a python list.
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

/// \brief Evaluate python expression \a expr with all the known script modules
/// imported under their standard names. Additional globals may be provided in
/// the \p extraGlobals dictionary.
boost::python::object
TfPyEvaluate(
    std::string const &expr,
    boost::python::dict const &extraGlobals = boost::python::dict());

/// \brief Return a positive index in the range [0,size).  If \a throwError is
// true, this will throw an index error if the resulting index is out of range.
TF_API 
int TfPyNormalizeIndex(int index, unsigned int size, bool throwError = false);

/// \brief Return the name of the class of \a obj.
TF_API std::string TfPyGetClassName(boost::python::object const &obj);


/// \brief Return the python class object for \a type if \a type has been
/// wrapped.  Otherwise return None.
TF_API boost::python::object
TfPyGetClassObject(std::type_info const &type);

/// \brief Return the python class object for T if T has been wrapped.
/// Otherwise return None.
template <typename T>
boost::python::object
TfPyGetClassObject() {
    return TfPyGetClassObject(typeid(T));
}

/// \brief Invokes \p wrapFunc to wrap type \c T if \c T is not already
///        wrapped.
///
/// Executing \p wrapFunc *must* register \c T with boost python.  Otherwise,
/// \p wrapFunc may be executed more than once.
///
/// TfPyWrapOnce will acquire the GIL prior to invoking \p wrapFunc.
/// Does not invoke \p wrapFunc if Python has not been initialized.
template <typename T>
void
TfPyWrapOnce(boost::function<void()> const &wrapFunc)
{
    // Don't try to wrap if python isn't initialized.
    if (not TfPyIsInitialized()) {
        return;
    }

    static bool isTypeWrapped = false;
    if (isTypeWrapped) {
        return;
    }

    TF_API void Tf_PyWrapOnceImpl(
        boost::python::type_info const &,
        boost::function<void()> const&,
        bool *);

    Tf_PyWrapOnceImpl(boost::python::type_id<T>(), wrapFunc, &isTypeWrapped);
}

/// \brief Load the python module \a moduleName.  This is used by some low-level
/// infrastructure code to load python wrapper modules corresponding to C++
/// shared libraries when they are needed.  It should generally not need to be
/// called from normal user code.
void Tf_PyLoadScriptModule(std::string const &name);

/// \brief Creates a python dictionary from a std::map
template <class Map>
boost::python::dict TfPyCopyMapToDictionary(Map const &map) {
    TfPyLock lock;
    boost::python::dict d;
    for (typename Map::const_iterator i = map.begin(); i != map.end(); ++i)
        d[i->first] = i->second;
    return d;
}

template<class Seq>
boost::python::list TfPyCopySequenceToList(Seq const &seq) {
    TfPyLock lock;
    boost::python::list l;
    for (typename Seq::const_iterator i = seq.begin();
         i != seq.end(); ++i)
        l.append(*i);
    return l; 
}

template<class Seq>
boost::python::tuple TfPyCopySequenceToTuple(Seq const &seq) {
    return boost::python::tuple(TfPyCopySequenceToList(seq));
}

///
/// \brief Return a vector of strings containing the current python traceback.
///
/// The vector contains the same strings that python's traceback.format_stack()
/// returns.
std::vector<std::string> TfPyGetTraceback();

/// Populates the vector passed in with pointers to strings containing the
/// python interpreter stack frames. 
/// Note that TfPyGetStackFrames allocates these strings on the heap and its 
/// the caller's responsibility to free them.
void TfPyGetStackFrames(std::vector<uintptr_t> *frames);

///
/// \brief Print the current python traceback to stdout.
void TfPyDumpTraceback();

/// \brief Set an environment variable in \c os.environ.
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
bool TfPySetenv(const std::string & name, const std::string & value);

/// \brief Remove an environment variable from \c os.environ.
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
bool TfPyUnsetenv(const std::string & name);

// Private helper method to TfPyEvaluateAndExtract.
//
TF_API bool Tf_PyEvaluateWithErrorCheck(
    const std::string & expr, boost::python::object * obj);

/// Safely evaluates \p expr and extracts the return object of type T.
/// If successful, returns \c true and sets *t to the return value, otherwise
/// returns \c false.
///
template <typename T>
bool TfPyEvaluateAndExtract(const std::string & expr, T * t)
{
    if (expr.empty())
        return false;

    // Take the lock before doing anything with boost::python.
    TfPyLock lock;

    // Though TfPyEvaluate (called by Tf_PyEvaluateWithErroCheck) takes the
    // python lock, it is important that we lock before we initialize the 
    // boost::python::object, since it will increment and decrement ref counts 
    // outside of the call to TfPyEvaluate.
    boost::python::object obj;
    if (not Tf_PyEvaluateWithErrorCheck(expr, &obj))
        return false;

    boost::python::extract<T> extractor(obj);

    if (not extractor.check())
        return false;

    *t = extractor();

    return true;
}

/// Print a standard traceback to sys.stderr and clear the error indicator.
/// If the error is a KeyboardInterrupt then this does nothing.  Call this
/// function only when the error indicator is set.
void TfPyPrintError();

#endif // TF_PYUTILS_H
