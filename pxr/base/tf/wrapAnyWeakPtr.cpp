//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/anyWeakPtr.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python/to_python_converter.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

// Put this in the pxr namespace so that we can declare it a friend in
// anyWeakPtr.h

object Tf_GetPythonObjectFromAnyWeakPtr(TfAnyWeakPtr const &self) {
    return self._GetPythonObject();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct Tf_AnyWeakPtrToPython {

    Tf_AnyWeakPtrToPython() {
        to_python_converter<TfAnyWeakPtr, Tf_AnyWeakPtrToPython>();
    }

    static PyObject *convert(TfAnyWeakPtr const &any) {
        return incref(Tf_GetPythonObjectFromAnyWeakPtr(any).ptr());
    }
};

} // anonymous namespace

void wrapAnyWeakPtr()
{
    to_python_converter<TfAnyWeakPtr, Tf_AnyWeakPtrToPython>();

    TfPyContainerConversions::from_python_sequence<
        std::set<TfAnyWeakPtr>,
        TfPyContainerConversions::set_policy>();
}
