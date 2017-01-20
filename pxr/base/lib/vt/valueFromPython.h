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
#ifndef VT_VALUE_FROM_PYTHON_H
#define VT_VALUE_FROM_PYTHON_H

/// \file vt/valueFromPython.h

#include "pxr/pxr.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/singleton.h"

#include <Python.h>
#include <boost/noncopyable.hpp>
#include "pxr/base/tf/hashmap.h"

#include <memory>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class Vt_ValueFromPythonRegistry
///
class Vt_ValueFromPythonRegistry : boost::noncopyable {
public:

    static bool HasConversions() {
	return !_GetInstance()._lvalueExtractors.empty() &&
           !_GetInstance()._rvalueExtractors.empty();
    }
    
    static VtValue Invoke(PyObject *obj);

    template <class T>
    static void Register(bool registerRvalue) {
	if (!TfPyIsInitialized()) {
	    TF_FATAL_ERROR("Tried to register a VtValue from python conversion "
			   "but python is not initialized!");
	}
	_GetInstance()._RegisterLValue(_Extractor::MakeLValue<T>());
        if (registerRvalue)
            _GetInstance()._RegisterRValue(_Extractor::MakeRValue<T>());
    }

private:

    friend class TfSingleton<Vt_ValueFromPythonRegistry>;

    class _Extractor {
    public:
        _Extractor() {}

    private:
	class _HolderBase {
	public:
	    virtual ~_HolderBase();
	    virtual VtValue Invoke(PyObject *) const = 0;
	};

        // LValueHolder will attempt to obtain an l-value T from the python
        // object it's passed in Invoke.  This effectively disallows type
        // conversions (other than things like derived-to-base type
        // conversions).
	template <class T>
	class _LValueHolder : public _HolderBase {
	public:
	    virtual ~_LValueHolder();
	    virtual VtValue Invoke(PyObject *) const;
	};

        // RValueHolder will attempt to obtain an r-value T from the python
        // object it's passed in Invoke.  This allows boost.python to invoke
        // type conversions to produce the T.
	template <class T>
	class _RValueHolder : public _HolderBase {
	public:
	    virtual ~_RValueHolder();
	    virtual VtValue Invoke(PyObject *) const;
	};

	typedef std::shared_ptr<_HolderBase> _HolderBasePtr;

    public:

	template <class T>
	static _Extractor MakeLValue() {
	    return _Extractor(_HolderBasePtr(new _LValueHolder<T>()));
	}
	
	template <class T>
	static _Extractor MakeRValue() {
	    return _Extractor(_HolderBasePtr(new _RValueHolder<T>()));
	}
	
	VtValue Invoke(PyObject *obj) const {
	    return _holder->Invoke(obj);
	}

    private:
	_Extractor(_HolderBasePtr const &holder) : _holder(holder) {}
	
	_HolderBasePtr _holder;
	
    };

    static Vt_ValueFromPythonRegistry &_GetInstance() {
	return TfSingleton<Vt_ValueFromPythonRegistry>::GetInstance();
    }
    
    void _RegisterLValue(_Extractor const &e);
    void _RegisterRValue(_Extractor const &e);
    
    std::vector<_Extractor> _lvalueExtractors;
    std::vector<_Extractor> _rvalueExtractors;

    typedef TfHashMap<PyObject *, _Extractor, TfHash> _LValueExtractorCache;
    _LValueExtractorCache _lvalueExtractorCache;

};

// Holders are created and inserted into the registry, and currently never die.
template <class T>
Vt_ValueFromPythonRegistry::_Extractor::_LValueHolder<T>::~_LValueHolder() {}
template <class T>
Vt_ValueFromPythonRegistry::_Extractor::_RValueHolder<T>::~_RValueHolder() {}

template <class T>
VtValue Vt_ValueFromPythonRegistry::
_Extractor::_LValueHolder<T>::Invoke(PyObject *obj) const {
    boost::python::extract<T &> x(obj);
    if (x.check())
	return VtValue(x());
    return VtValue();
}

template <class T>
VtValue Vt_ValueFromPythonRegistry::
_Extractor::_RValueHolder<T>::Invoke(PyObject *obj) const {
    boost::python::extract<T> x(obj);
    if (x.check())
	return VtValue(x());
    return VtValue();
}

template <class T>
void VtValueFromPython() {
    Vt_ValueFromPythonRegistry::Register<T>(/* registerRvalue = */ true);
}

template <class T>
void VtValueFromPythonLValue() {
    Vt_ValueFromPythonRegistry::Register<T>(/* registerRvalue = */ false);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_VALUE_FROM_PYTHON_H
