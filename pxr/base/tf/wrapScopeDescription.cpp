//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/pyResultConversions.h"


#include <memory>
#include <string>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {

// This class exposes TfScopeDescription to Python clients as a context
// manager and a decorator.
//
// This class uses a small helper that holds a TfScopeDescription because
// TfScopeDescription declares, but doesn't define, the ordinary new/delete
// operators to help prevent clients from creating them on the heap.
//
// It's wrapped as a private class because we extend it using the
// ContextDecorator mixin on the Python side (see __init__.py).
class Tf_PyScopeDescription
{
    // This is used to avoid new/delete on TfScopeDescription directly, which is
    // disallowed.
    struct _Holder {
        explicit _Holder(string const &description)
            : _scopeDescription(description) {}
        TfScopeDescription _scopeDescription;
    };

public:

    // Construct with a description string.
    Tf_PyScopeDescription(string const &description) :
        _description(description) {}

    // Enter creates a description object, pushing onto the stack.
    void __enter__() {
        _descriptionHolder.reset(new _Holder(_description));
    }

    // Exit destroys the scope description, popping from the stack.
    void __exit__() {
        _descriptionHolder.reset();
    }

    void SetDescription(const string& description) {
        _description = description;
        if (_descriptionHolder) {
            _descriptionHolder->_scopeDescription.SetDescription(_description);
        }
    }

private:

    std::unique_ptr<_Holder> _descriptionHolder;
    string _description;
};

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

void wrapScopeDescription(nanobind::module_& m)
{
    namespace nb = nanobind;
    m.def("GetCurrentScopeDescriptionStack", TfGetCurrentScopeDescriptionStack);

    typedef Tf_PyScopeDescription This;

    nb::class_<This>(m, "_ScopeDescription")
        .def(nb::init<string>())
        .def("__enter__", [](nb::object self) {
            nb::cast<This&>(self).__enter__();
            return self;
        })
        .def("__exit__", [](This& self, nb::args) {
            self.__exit__();
        })
        .def("SetDescription", &This::SetDescription)
        ;
}
