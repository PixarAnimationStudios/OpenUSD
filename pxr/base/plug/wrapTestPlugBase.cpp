//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/testPlugBase.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/noncopyable.hpp>
#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

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

} // anonymous namespace 

void wrap_TestPlugBase()
{
    wrap_TestPlugBase<_TestPlugBase1>("_TestPlugBase1");
    wrap_TestPlugBase<_TestPlugBase2>("_TestPlugBase2");
    wrap_TestPlugBase<_TestPlugBase3>("_TestPlugBase3");
    wrap_TestPlugBase<_TestPlugBase4>("_TestPlugBase4");
}
