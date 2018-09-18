//
// Copyright 2018 Pixar
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
#include "usdMaya/blockSceneModificationContext.h"

#include <boost/python.hpp>

#include <memory>

using namespace boost::python;


PXR_NAMESPACE_USING_DIRECTIVE;


namespace {

// This exposes UsdMayaBlockSceneModificationContext as a Python "context
// manager" object that can be used with the "with" statement.
class _PyBlockSceneModificationContext
{
    public:
        void __enter__() {
            _context.reset(new UsdMayaBlockSceneModificationContext());
        }

        void __exit__(object, object, object) {
            _context.reset();
        }

    private:
        std::shared_ptr<UsdMayaBlockSceneModificationContext> _context;
};

} // anonymous namespace


void
wrapBlockSceneModificationContext()
{
    typedef _PyBlockSceneModificationContext Context;
    class_<Context>(
            "BlockSceneModificationContext",
            "Context manager for blocking scene modification status changes")
        .def("__enter__", &Context::__enter__, return_self<>())
        .def("__exit__", &Context::__exit__)
    ;
}
