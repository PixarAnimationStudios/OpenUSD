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
#include <boost/python/class.hpp>
#include <boost/python/object.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include <memory>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class _PyResolverContextBinder
    : public boost::noncopyable
{
public:
    _PyResolverContextBinder(const ArResolverContext& context)
        : _context(context)
    {
    }

    void Enter()
    {
        _binder.reset(new ArResolverContextBinder(_context));
    }

    bool Exit(
        boost::python::object& /* exc_type */,
        boost::python::object& /* exc_val  */,
        boost::python::object& /* exc_tb   */)
    {
        _binder.reset(0);
        // Re-raise exceptions.
        return false;
    }

private:
    ArResolverContext _context;
    std::unique_ptr<ArResolverContextBinder> _binder;
};

} // anonymous namespace 

void
wrapResolverContextBinder()
{
    typedef _PyResolverContextBinder This;

    class_<This, boost::noncopyable>
        ("ResolverContextBinder", init<const ArResolverContext&>())
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit)
        ;
}
