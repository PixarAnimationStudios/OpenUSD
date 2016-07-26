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

#include "pxr/base/tf/anyWeakPtr.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python/to_python_converter.hpp>


using namespace boost::python;


// put this in the presto namespace so that we can declare it a friend in
// AnyweakPtr.h

object Tf_GetPythonObjectFromAnyWeakPtr(TfAnyWeakPtr const &self) {
    return self._GetPythonObject();
}





struct Tf_AnyWeakPtrToPython {

    Tf_AnyWeakPtrToPython() {
        to_python_converter<TfAnyWeakPtr, Tf_AnyWeakPtrToPython>();
    }

    static PyObject *convert(TfAnyWeakPtr const &any) {
        return incref(Tf_GetPythonObjectFromAnyWeakPtr(any).ptr());
    }
};

void wrapAnyWeakPtr()
{
    to_python_converter<TfAnyWeakPtr, Tf_AnyWeakPtrToPython>();

    TfPyContainerConversions::from_python_sequence<
        std::set<TfAnyWeakPtr>,
        TfPyContainerConversions::set_policy>();
}


