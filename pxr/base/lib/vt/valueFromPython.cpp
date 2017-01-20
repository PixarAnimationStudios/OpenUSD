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
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Vt_ValueFromPythonRegistry);

// Holders are created and inserted into the registry, and currently never die.
Vt_ValueFromPythonRegistry::_Extractor::_HolderBase::~_HolderBase()
{
}

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
            self._lvalueExtractorCache[PyObject_Type(obj)] =
                self._lvalueExtractors[i-1];
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
