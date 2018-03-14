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
#include "gusd/stageEdit.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>
#include <boost/python/implicit.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


template <typename VEC>
VEC _ListToVector(const list& list)
{
    const size_t size = len(list);
    VEC vec(size);
    for(size_t i = 0; i < size; ++i) {
        vec[i] = extract<typename VEC::value_type>(list[i]);
    }
    return vec;
}


template <typename ARRAY>
ARRAY _ListToArray(const list& list)
{
    ARRAY array;
    const size_t size = len(list);
    array.setSize(size);
    for(size_t i = 0; i < size; ++i) {
        array[i] = extract<typename ARRAY::value_type>(list[i]);
    }
    return array;
}


GusdStageBasicEditPtr _New()
{
    return GusdStageBasicEditPtr(new GusdStageBasicEdit);
}


const UT_Array<SdfPath>&
_GetVariants(const GusdStageBasicEdit& self)
{
    return self.GetVariants();
}


void _SetVariants(GusdStageBasicEdit& self, const list& variants)
{
    self.GetVariants() = _ListToArray<UT_Array<SdfPath> >(variants);
}


const std::vector<std::string>&
_GetLayersToMute(const GusdStageBasicEdit& self)
{
    return self.GetLayersToMute();
}


void _SetLayersToMute(GusdStageBasicEdit& self, const list& layers)
{
    self.GetLayersToMute() = _ListToVector<std::vector<std::string> >(layers);
}


} // namespace


PXR_NAMESPACE_OPEN_SCOPE

/// Extract of underlying value in a UT_IntrusivePtr.
/// Required for conversion in boost python.
template <typename T>
T* get_pointer(const UT_IntrusivePtr<T>& p)
{
    return p.get();
}

PXR_NAMESPACE_CLOSE_SCOPE


namespace boost { namespace python {

    /// Boilerplate needed to use UT_IntrusivePtr types in boost python.
    template <typename T>
    struct pointee<UT_IntrusivePtr<T> > {
        typedef T type;
    };

} // namespace python
} // namespace boost


namespace {


void _wrapGusdStageBasicEdit()
{
    using This = GusdStageBasicEdit;
    using ThisPtr = UT_IntrusivePtr<GusdStageBasicEdit>;

    class_<This, ThisPtr, bases<GusdStageEdit> >("StageBasicEdit", no_init)

        .def("New", &_New)
        .staticmethod("New")

        .def("GetVariants", &_GetVariants,
             return_value_policy<TfPySequenceToList>())

        .def("SetVariants", &_SetVariants)

        .def("GetLayersToMute", _GetLayersToMute,
             return_value_policy<TfPySequenceToList>())

        .def("SetLayersToMute", &_SetLayersToMute)
        ;

    implicitly_convertible<GusdStageBasicEditPtr,GusdStageEditPtr>();
}


bool _Apply(const GusdStageEdit& self, const object& obj)
{
    extract<SdfLayerHandle> x(obj);
    if(x.check())
        return self.Apply(x);
    return self.Apply(extract<UsdStagePtr>(obj));
}


} // namespace


void wrapGusdStageEdit()
{
    using This = GusdStageEdit;
    using ThisPtr = GusdStageEditPtr;

    class_<This, ThisPtr, boost::noncopyable>("StageEdit", no_init)

        .def("Apply", &_Apply)

        .def("hash", &This::GetHash)
        .def("__eq__", &This::operator==)
        .def("__ne__", &This::operator!=)
        ;

    _wrapGusdStageBasicEdit();
}
