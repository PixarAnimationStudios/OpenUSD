//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Vt_ValueFromPythonRegistry);

Vt_ValueFromPythonRegistry::~Vt_ValueFromPythonRegistry() = default;

VtValue
Vt_ValueFromPythonRegistry::Invoke(PyObject *obj) {
    TfPyLock lock;
    // Iterate over the extractors in reverse registration order.  We walk the
    // lvalue extractors first, looking for an exact match.  If none match, we
    // walk the rvalue extractors, which allow conversions.
    Vt_ValueFromPythonRegistry &self = _GetInstance();

    // Check to see if we have a cached lvalue extractor for the python type
    // object.
    boost::python::handle<> h(PyObject_Type(obj));
    _LValueExtractorCache::iterator i =
        self._lvalueExtractorCache.find(h.get());
    if (i != self._lvalueExtractorCache.end()) {
        // attempt conversion.
        VtValue result = i->second.Invoke(obj);
        if (!result.IsEmpty())
            return result;
    }

    // Fall back to trying extractors in reverse registration order.
    for (size_t i = self._lvalueExtractors.size(); i != 0; --i) {
        VtValue result = self._lvalueExtractors[i-1].Invoke(obj);
        if (!result.IsEmpty()) {
            // Cache the result.
            self._lvalueExtractorCache.insert(
                {PyObject_Type(obj), self._lvalueExtractors[i-1]});
            return result;
        }
    }

    // No lvalue extraction worked -- try rvalue conversions.
    for (size_t i = self._rvalueExtractors.size(); i != 0; --i) {
        VtValue result = self._rvalueExtractors[i-1].Invoke(obj);
        if (!result.IsEmpty())
            return result;
    }
    return VtValue();
}


void
Vt_ValueFromPythonRegistry::_RegisterLValue(_Extractor const &e) {
    _lvalueExtractors.push_back(e);
}

void
Vt_ValueFromPythonRegistry::_RegisterRValue(_Extractor const &e) {
    _rvalueExtractors.push_back(e);
}

PXR_NAMESPACE_CLOSE_SCOPE
