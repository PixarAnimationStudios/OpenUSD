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
#ifndef PXR_BASE_VT_VALUE_FROM_PYTHON_H
#define PXR_BASE_VT_VALUE_FROM_PYTHON_H

/// \file vt/valueFromPython.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/singleton.h"

#include "pxr/base/tf/pySafePython.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class Vt_ValueFromPythonRegistry
///
class Vt_ValueFromPythonRegistry {
public:

    static bool HasConversions() {
        return !_GetInstance()._lvalueExtractors.empty() &&
               !_GetInstance()._rvalueExtractors.empty();
    }
    
    VT_API static VtValue Invoke(PyObject *obj);

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

    Vt_ValueFromPythonRegistry(Vt_ValueFromPythonRegistry const&) = delete;
    Vt_ValueFromPythonRegistry& operator=(
        Vt_ValueFromPythonRegistry const&) = delete;

    Vt_ValueFromPythonRegistry(Vt_ValueFromPythonRegistry &&) = delete;
    Vt_ValueFromPythonRegistry& operator=(
        Vt_ValueFromPythonRegistry &&) = delete;

private:
    Vt_ValueFromPythonRegistry() {}
    VT_API ~Vt_ValueFromPythonRegistry();

    friend class TfSingleton<Vt_ValueFromPythonRegistry>;

    class _Extractor {
    private:
        using _ExtractFunc = VtValue (*)(PyObject *);

        // _ExtractLValue will attempt to obtain an l-value T from the python
        // object it's passed.  This effectively disallows type conversions
        // (other than things like derived-to-base type conversions).
        template <class T>
        static VtValue _ExtractLValue(PyObject *);

        // _ExtractRValue will attempt to obtain an r-value T from the python
        // object it's passed.  This allows boost.python to invoke type
        // conversions to produce the T.
        template <class T>
        static VtValue _ExtractRValue(PyObject *);

    public:

        template <class T>
        static _Extractor MakeLValue() {
            return _Extractor(&_ExtractLValue<T>);
        }
        
        template <class T>
        static _Extractor MakeRValue() {
            return _Extractor(&_ExtractRValue<T>);
        }
        
        VtValue Invoke(PyObject *obj) const {
            return _extract(obj);
        }

    private:
        explicit _Extractor(_ExtractFunc extract) : _extract(extract) {}

        _ExtractFunc _extract;
    };

    VT_API static Vt_ValueFromPythonRegistry &_GetInstance() {
        return TfSingleton<Vt_ValueFromPythonRegistry>::GetInstance();
    }
    
    VT_API void _RegisterLValue(_Extractor const &e);
    VT_API void _RegisterRValue(_Extractor const &e);
    
    std::vector<_Extractor> _lvalueExtractors;
    std::vector<_Extractor> _rvalueExtractors;

    typedef TfHashMap<PyObject *, _Extractor, TfHash> _LValueExtractorCache;
    _LValueExtractorCache _lvalueExtractorCache;

};

VT_API_TEMPLATE_CLASS(TfSingleton<Vt_ValueFromPythonRegistry>);

template <class T>
VtValue Vt_ValueFromPythonRegistry::
_Extractor::_ExtractLValue(PyObject *obj) {
    boost::python::extract<T &> x(obj);
    if (x.check())
        return VtValue(x());
    return VtValue();
}

template <class T>
VtValue Vt_ValueFromPythonRegistry::
_Extractor::_ExtractRValue(PyObject *obj) {
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

#endif // PXR_BASE_VT_VALUE_FROM_PYTHON_H
