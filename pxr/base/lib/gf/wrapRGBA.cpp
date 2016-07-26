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
#include <boost/python.hpp>

#include "pxr/base/gf/rgba.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

using namespace boost::python;

static GfRGBA *__init__() {
    // Default ctor in python zero-initializes
    return new GfRGBA(0.0f);
}

static tuple _GetHSV(const GfRGBA &self) {
    float hue, saturation, value;
    self.GetHSV(&hue, &saturation, &value);
    return make_tuple(hue, saturation, value);
}

static std::string __repr__(GfRGBA const &self)
{
    return TF_PY_REPR_PREFIX + "RGBA(" +
        TfPyRepr(self[0]) + ", " +
        TfPyRepr(self[1]) + ", " +
        TfPyRepr(self[2]) + ", " +
        TfPyRepr(self[3]) + ")";
}

static int
_NormalizeIndex(int index) {
    return TfPyNormalizeIndex(index, 4, true /*throw error*/);
}

static int __len__(GfRGBA const &self) {
    return 4;
}

static float __getitem__(const GfRGBA &self, int index) {
    index = _NormalizeIndex(index);
    return self[index];
}

static void __setitem__(GfRGBA &self, int index, float value) {
    index = _NormalizeIndex(index);
    self[index] = value;
}

static bool __contains__(const GfRGBA &self, float value) {
    for (int i = 0; i < 4; ++i)
        if (self[i] == value)
            return true;
    return false;
}


template <size_t Component>
static float _GetComponent(GfRGBA const &self) {
    return self[Component];
}

template <size_t Component>
static void _SetComponent(GfRGBA &self, float value) {
    self[Component] = value;
}


struct GfRGBAFromPythonTuple {
    GfRGBAFromPythonTuple() {
        converter::registry::
            push_back(&_convertible, &_construct,
                      boost::python::type_id<GfRGBA>());
    }

  private:
    
    static void* _convertible(PyObject *obj_ptr) {
	if (not PyTuple_Check(obj_ptr))
	    return 0;
        size_t size = PyTuple_GET_SIZE(obj_ptr);
        if (size != 3 and size != 4)
            return 0;
	for (size_t i = 0; i < size; ++i)
	    if (not extract<double>(PyTuple_GET_ITEM(obj_ptr, i)).check())
		return 0;
	return obj_ptr;
    }

    static void _construct(PyObject *obj_ptr, converter::
                           rvalue_from_python_stage1_data *data) {
        void* storage = ((converter::rvalue_from_python_storage<GfRGBA>*)data)
	    ->storage.bytes;
        size_t size = PyTuple_GET_SIZE(obj_ptr);

        double r = extract<double>(PyTuple_GET_ITEM(obj_ptr, 0));
        double g = extract<double>(PyTuple_GET_ITEM(obj_ptr, 1));
        double b = extract<double>(PyTuple_GET_ITEM(obj_ptr, 2));
        double a = (size == 4) ?
                   extract<double>(PyTuple_GET_ITEM(obj_ptr, 4)) :
                   1.0;

        new (storage)
            GfRGBA(r, g, b, a);

        data->convertible = storage;
    }
};

// This adds support for python's builtin pickling library
// This is used by our Shake plugins which need to pickle entire classes
// (including code), which we don't support in pxml.
struct RGBA_Pickle_Suite : boost::python::pickle_suite
{
    static boost::python::tuple getinitargs(const GfRGBA &c)
    {
        return boost::python::make_tuple(c[0], c[1], c[2], c[3]);
    }
};

bool GfIsClose(const GfRGBA &v1, const GfRGBA &v2, double tolerance);


void wrapRGBA()
{    
    typedef GfRGBA This;

    def("IsClose", (bool (*)(const GfRGBA &,
                             const GfRGBA &, double)) GfIsClose);
    
    class_<This>("RGBA", no_init)

        .def(TfTypePythonClass())
        .def_pickle(RGBA_Pickle_Suite())
        
        .def("__init__", make_constructor(__init__))
        .def(init<float>())
        .def(init<float, float, float, optional<float> >())
        .def(init<GfVec4f>())
        .def(init<GfRGB, optional<float> >())

        .def("Clamp", &This::Clamp,
             (arg("min")=0.0f, arg("max")=1.0f), return_self<>())
        .def("IsBlack", &This::IsBlack)
        .def("IsWhite", &This::IsWhite)
        .def("IsTransparent", &This::IsTransparent)
        .def("IsOpaque", &This::IsOpaque)
        .def("Transform", &This::Transform)
        .def("GetComplement", &This::GetComplement)
        .def("GetVec", &This::GetVec, return_value_policy<return_by_value>())
        .def("GetHSV", _GetHSV)
        .def("SetHSV", &This::SetHSV)

        .add_property("r", _GetComponent<0>, _SetComponent<0>)
        .add_property("g", _GetComponent<1>, _SetComponent<1>)
        .add_property("b", _GetComponent<2>, _SetComponent<2>)
        .add_property("a", _GetComponent<3>, _SetComponent<3>)

        .def("__repr__", __repr__)

        .def("__len__", __len__)
        .def("__contains__", __contains__)
        .def("__getitem__", __getitem__)
        .def("__setitem__", __setitem__)

        .def(self_ns::str(self))

        .def(self == self)
        .def(self != self)

        .def(double() * self)
        .def(self * double())
        .def(self * self)
        .def(self *= double())
        .def(self *= self)
        .def(self + self)
        .def(self += self)
        .def(self - self)
        .def(self -= self)
        .def(self / double())
        .def(self / self)
        .def(self /= double())
        .def(self /= self)

        .def(self * GfMatrix4d())
        
        ;
    
    // Allow appropriate tuples to be passed where GfRGBAs are expected.
    GfRGBAFromPythonTuple();
}
