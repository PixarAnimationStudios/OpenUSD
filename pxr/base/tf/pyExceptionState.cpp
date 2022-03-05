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

#include "pxr/base/tf/pyErrorInternal.h"

#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>


PXR_NAMESPACE_OPEN_SCOPE

std::string 
TfPyExceptionState::GetExceptionString() const
{
    TfPyLock lock;
    std::string s;
    // Save the exception state so we can restore it -- getting the exception
    // string should not affect the exception state.
    TfPyExceptionStateScope exceptionStateScope;
    try {
        boost::python::object tbModule(boost::python::handle<>(PyImport_ImportModule("traceback")));
        boost::python::object exception = tbModule.attr("format_exception")(_type, _value, 
                                                                    _trace);
        boost::python::ssize_t size = boost::python::len(exception);
        for (boost::python::ssize_t i = 0; i < size; ++i) {
            s += boost::python::extract<std::string>(exception[i]);
        }
    } catch (boost::python::error_already_set const &) {
        // Just ignore the exception.
    }
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
