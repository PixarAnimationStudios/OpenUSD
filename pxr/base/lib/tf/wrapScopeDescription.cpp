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

#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/noncopyable.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>

using std::string;
using std::vector;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// This class lets us expose TfScopeDescription to python for use as a "context
// manager" object.  That is, for use with the 'with'-statement.  For example:
//
// with Tf.ScopeDescription("Solving the halting problem"):
//     # Code that solves the halting problem.
//
// This class uses a small helper that holds a TfScopeDescription because
// TfScopeDescription declares, but doesn't define the ordinary new/delete
// operators to help prevent clients from creating them on the heap.
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
    void __exit__(object, object, object) {
        _descriptionHolder.reset();
    }

    void SetDescription(const string& description) {
        _description = description;
        if (_descriptionHolder) {
            _descriptionHolder->_scopeDescription.SetDescription(_description);
        }
    }

private:

    boost::scoped_ptr<_Holder> _descriptionHolder;
    string _description;
};

} // anonymous namespace 

void wrapScopeDescription()
{
    def("GetCurrentScopeDescriptionStack",
        TfGetCurrentScopeDescriptionStack,
        return_value_policy<TfPySequenceToList>());

    typedef Tf_PyScopeDescription This;
    
    class_<This, boost::noncopyable>("ScopeDescription", init<string>())
        .def("__enter__", &This::__enter__, return_self<>())
        .def("__exit__", &This::__exit__)
        .def("SetDescription", &This::SetDescription)
        ;
}
