{#
//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#}

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix{{ DIM }}d.h"
#include "pxr/base/gf/matrix{{ DIM }}f.h"

#include "pxr/base/gf/pyBufferUtils.h"

{% block customIncludes %}
{% endblock customIncludes %}

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

////////////////////////////////////////////////////////////////////////
// Python buffer protocol support.

// Python's getbuffer interface function.
static int
getbuffer(PyObject *self, Py_buffer *view, int flags) {
    if (view == NULL) {
        PyErr_SetString(PyExc_ValueError, "NULL view in getbuffer");
        return -1;
    }

    // We don't support fortran order.
    if ((flags & PyBUF_F_CONTIGUOUS) == PyBUF_F_CONTIGUOUS) {
        PyErr_SetString(PyExc_ValueError, "Fortran contiguity unsupported");
        return -1;
    }

    {{ MAT }} &mat = *nanobind::inst_ptr<{{ MAT }}>(self);

    view->obj = self;
    view->buf = static_cast<void *>(mat.GetArray());
    view->len = sizeof({{ MAT }});
    view->readonly = 0;
    view->itemsize = sizeof({{ SCL }});
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT) {
        view->format = Gf_GetPyBufferFmtFor<{{ SCL }}>();
    } else {
        view->format = NULL;
    }
    if ((flags & PyBUF_ND) == PyBUF_ND) {
        view->ndim = 2;
        static Py_ssize_t shape[] = { {{ DIM }}, {{ DIM }} };
        view->shape = shape;
    } else {
        view->ndim = 0;
        view->shape = NULL;
    }
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES) {
        static Py_ssize_t strides[] = {
            {{ DIM }} * sizeof({{ SCL }}), sizeof({{ SCL }}) };
        view->strides = strides;
    } else {
        view->strides = NULL;
    }
    view->suboffsets = NULL;
    view->internal = NULL;

    Py_INCREF(self); // need to retain a reference to self.
    return 0;
}

static const PyType_Slot bufferSlots[] = {
    { Py_bf_getbuffer, (void *)getbuffer },
    { 0, nullptr }
};

// End python buffer protocol support.
////////////////////////////////////////////////////////////////////////

static string _Repr({{ MAT }} const &self) {
    static char newline[] = ",\n            ";
    return TF_PY_REPR_PREFIX + "Matrix{{ SUFFIX }}(" +
{% for ROW in range(DIM) %}
        {{ LIST("TfPyRepr(self[%(ROW)s][%%(i)s])" % {'ROW':ROW}, sep=" + \", \" + ") }}
        {%- if not loop.last %}{{ " + newline +\n" }}{% endif %}
{% endfor %} + ")";
}

static {{ MAT }} GetInverseWrapper( const {{ MAT }} &self ) {
    return self.GetInverse();
}

static void
throwIndexErr( const char *msg )
{
    PyErr_SetString(PyExc_IndexError, msg);
    throw nanobind::python_error();
}    

static int
normalizeIndex(int index) {
    return TfPyNormalizeIndex(index, {{ DIM }}, true /*throw error*/);
}

// Return number of rows
static int __len__({{ MAT }} const &self) {
    return {{ DIM }};
}

static {{ SCL }} __getitem__{{ SCL }}({{ MAT }} const &self, nanobind::tuple index) {
    int i1=0, i2=0;
    if (nanobind::len(index) == 2) {
        i1 = normalizeIndex(nanobind::cast<int>(index[0]));
        i2 = normalizeIndex(nanobind::cast<int>(index[1]));
    } else
        throwIndexErr("Index has incorrect size.");

    return self[i1][i2];
}

static GfVec{{ SUFFIX }} __getitem__vector({{ MAT }} const &self, int index) {
    return GfVec{{ SUFFIX }}(self[normalizeIndex(index)]);
}

static void __setitem__{{ SCL }}({{ MAT }} &self, nanobind::tuple index, {{ SCL }} value) {
    int i1=0, i2=0;
    if (nanobind::len(index) == 2) {
        i1 = normalizeIndex(nanobind::cast<int>(index[0]));
        i2 = normalizeIndex(nanobind::cast<int>(index[1]));
    } else
        throwIndexErr("Index has incorrect size.");

    self[i1][i2] = value;
}

static void __setitem__vector( {{ MAT }} &self, int index, GfVec{{ SUFFIX }} value ) {
    int ni = normalizeIndex(index);
    {{ LIST("self[ni][%(i)s] = value[%(i)s];", sep="\n    ") }}
}

static bool __contains__{{ SCL }}( const {{ MAT }} &self, {{ SCL }} value ) {
    for( int i = 0; i < {{ DIM }}; ++i )
        for( int j = 0; j < {{ DIM }}; ++j )
            if( self[i][j] == value )
                return true;
    return false;
}

// Check rows against GfVec
static bool __contains__vector( const {{ MAT }} &self, GfVec{{ SUFFIX }} value ) {
    for( int i = 0; i < {{ DIM }}; ++i )
	if( self.GetRow(i) == value )
	    return true;
    return false;
}

{% block customFunctions %}
{% endblock customFunctions %}

static size_t __hash__({{ MAT }} const &m) { return TfHash{}(m); }

} // anonymous namespace 

void wrapMatrix{{ SUFFIX }}(nanobind::module_ &m)
{    
    typedef {{ MAT }} This;

    m.def("IsClose", (bool (*)(const {{ MAT}} &m1, const {{ MAT }} &m2, double))
        GfIsClose);
    
    nanobind::class_<This> cls(m, "Matrix{{ SUFFIX }}", nanobind::type_slots(bufferSlots));
    cls
        .def("__getinitargs__", [](const This &m) {
            return nanobind::make_tuple(
                {{ MATRIX("m[%(i)s][%(j)s]", indent=16) }});
        })
        .def("__init__", [](This *self) { new (self) This(1); })
        .def(nanobind::init<const GfMatrix{{ DIM }}d &>())
        .def(nanobind::init<const GfMatrix{{ DIM }}f &>())
        .def(nanobind::init<int>())
        .def(nanobind::init<{{ SCL }}>())
        .def(nanobind::init<
             {{ MATRIX(SCL, indent=13) }}
             >())
        .def(nanobind::init<const GfVec{{ SUFFIX }} &>())
        .def(nanobind::init<const vector<vector<double>> &>())
        .def(nanobind::init<const vector<vector<float>> &>())
{% block customInit %}
{% endblock customInit %}

        .def(TfTypePythonClass())

        .def_prop_ro_static("dimension", [](nanobind::handle) {
            return nanobind::make_tuple({{ DIM }}, {{ DIM }});
        })
        .def("__len__", __len__, "Return number of rows")

        .def("__getitem__", __getitem__{{ SCL }})
        .def("__getitem__", __getitem__vector)
        .def("__setitem__", __setitem__{{ SCL }})
        .def("__setitem__", __setitem__vector)
        .def("__contains__", __contains__{{ SCL }})
        .def("__contains__", __contains__vector, "Check rows against GfVec")
{% block customSpecialMethods %}
{% endblock customSpecialMethods %}

        .def("Set", (This &(This::*)({{ MATRIX(SCL, indent=37) }}))&This::Set,
             nanobind::rv_policy::reference)
        
        .def("SetIdentity", &This::SetIdentity, nanobind::rv_policy::reference)
        .def("SetZero", &This::SetZero, nanobind::rv_policy::reference)

        .def("SetDiagonal", 
             (This & (This::*)({{ SCL }}))&This::SetDiagonal, 
             nanobind::rv_policy::reference)
        .def("SetDiagonal", 
             (This & (This::*)(const GfVec{{ SUFFIX }} &))&This::SetDiagonal, 
             nanobind::rv_policy::reference)

        .def("SetRow", &This::SetRow)
        .def("SetColumn", &This::SetColumn)
        .def("GetRow", &This::GetRow)
        .def("GetColumn", &This::GetColumn)

        .def("GetTranspose", &This::GetTranspose)
        .def("GetInverse", GetInverseWrapper)

        .def("GetDeterminant", &This::GetDeterminant)
{% block customDefs %}
{% endblock customDefs %}
        
        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def(nanobind::self == nanobind::self)
{% if SCL == 'float' %}
        .def(nanobind::self == GfMatrix{{ DIM }}d())
{% elif SCL == 'double' %}
        .def(nanobind::self == GfMatrix{{ DIM }}f())
{% endif %}
        .def(nanobind::self != nanobind::self)
{% if SCL == 'float' %}
        .def(nanobind::self != GfMatrix{{ DIM }}d())
{% elif SCL == 'double' %}
        .def(nanobind::self != GfMatrix{{ DIM }}f())
{% endif %}
        .def(nanobind::self *= nanobind::self)
        .def(nanobind::self * nanobind::self)
        .def(nanobind::self *= {{ SCL }}())
        .def(nanobind::self * {{ SCL }}())
        .def({{ SCL }}() * nanobind::self)
        .def(nanobind::self += nanobind::self)
        .def(nanobind::self + nanobind::self)
        .def(nanobind::self -= nanobind::self)
        .def(nanobind::self - nanobind::self)
        .def(-nanobind::self)
        .def(nanobind::self / nanobind::self)
        .def(nanobind::self * GfVec{{ SUFFIX }}())
        .def(GfVec{{ SUFFIX }}() * nanobind::self)
{% if SCL == 'double' %}

        // Provide wrapping that makes up for the fact that, in Python, we
        // don't allow implicit conversion from GfVec3f to GfVec3d (which we
        // do in C++).
        .def(nanobind::self * GfVec{{ DIM }}f())
        .def(GfVec{{ DIM }}f() * nanobind::self)
{% endif %}

{% block customXformDefs %}
{% endblock customXformDefs %}
        .def("__repr__", _Repr)
        .def("__hash__", __hash__)

        ;
}
