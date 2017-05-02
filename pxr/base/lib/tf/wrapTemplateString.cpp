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

#include "pxr/base/tf/templateString.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/return_by_value.hpp>

#include <string>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string __repr__(TfTemplateString const &self)
{
    return TF_PY_REPR_PREFIX + "TemplateString(" +
        (self.GetTemplate().empty() ? string() : TfPyRepr(self.GetTemplate())) +
        ")";
}

static string _Substitute(TfTemplateString const &self, dict const &d)
{
    TfTemplateString::Mapping m;
    list items = d.items();
    for (int i = 0; i < len(items); ++i)
        m[extract<string>(items[i][0])] = extract<string>(items[i][1]);
    return self.Substitute(m);
}

static string _SafeSubstitute(TfTemplateString const &self, dict const &d)
{
    TfTemplateString::Mapping m;
    list items = d.items();
    for (int i = 0; i < len(items); ++i)
        m[extract<string>(items[i][0])] = extract<string>(items[i][1]);
    return self.SafeSubstitute(m);
}

} // anonymous namespace 

void wrapTemplateString()
{
    typedef TfTemplateString This;

    class_<This>("TemplateString")
        .def(init<string>())
        .def("__repr__", __repr__)
        .add_property("template",
                      make_function(&This::GetTemplate,
                                    return_value_policy<return_by_value>()))
        .def("Substitute", _Substitute)
        .def("SafeSubstitute", _SafeSubstitute)
        .def("GetEmptyMapping", &This::GetEmptyMapping,
             return_value_policy<TfPyMapToDictionary>())
        .def("GetParseErrors", &This::GetParseErrors,
             return_value_policy<TfPySequenceToList>())
        .add_property("valid", &This::IsValid)
        ;
}
