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
#include "pxr/base/plug/testPlugBase.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

template <class T>
void wrap_TestPlugBase(const std::string & name)
{
    typedef T This;
    typedef TfWeakPtr<T> ThisPtr;
    class_<This, ThisPtr, boost::noncopyable> ( name.c_str(), no_init )
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(&This::New))

        // Expose Manufacture as another initializer.
        .def(TfMakePyConstructor(&This::Manufacture))

        .def("GetTypeName", &This::GetTypeName)

        ;
}

void wrap_TestPlugBase()
{
    wrap_TestPlugBase<_TestPlugBase1>("_TestPlugBase1");
    wrap_TestPlugBase<_TestPlugBase2>("_TestPlugBase2");
    wrap_TestPlugBase<_TestPlugBase3>("_TestPlugBase3");
    wrap_TestPlugBase<_TestPlugBase4>("_TestPlugBase4");
}

PXR_NAMESPACE_CLOSE_SCOPE
