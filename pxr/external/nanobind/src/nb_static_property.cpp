#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/// `nb_static_property.__get__()`: Always pass the class instead of the instance.
static PyObject *nb_static_property_descr_get(PyObject *self, PyObject *, PyObject *cls) {
    return NB_TYPE_SLOT(PyProperty_Type, tp_descr_get)(self, cls, cls);
}

/// `nb_static_property.__set__()`: Just like the above `__get__()`.
static int nb_static_property_descr_set(PyObject *self, PyObject *obj, PyObject *value) {
    PyObject *cls = PyType_Check(obj) ? obj : (PyObject *) Py_TYPE(obj);
    return NB_TYPE_SLOT(PyProperty_Type, tp_descr_set)(self, cls, value);
}

PyTypeObject *nb_static_property_tp(nb_internals *p) noexcept {
    PyTypeObject *tp = p->nb_static_property.load_acquire();

    if (NB_UNLIKELY(!tp)) {
        lock_internals guard(p);

        tp = p->nb_static_property.load_relaxed();
        if (tp)
            return tp;

        PyMemberDef *members =
            (PyMemberDef *) PyType_GetSlot(&PyProperty_Type, Py_tp_members);

        PyType_Slot slots[] = {
            { Py_tp_base, &PyProperty_Type },
            { Py_tp_descr_get, (void *) nb_static_property_descr_get },
            { Py_tp_members, members },
            { 0, nullptr }
        };

        PyType_Spec spec = {
            /* .name = */ "nanobind.nb_static_property",
            /* .basicsize = */ 0,
            /* .itemsize = */ 0,
            /* .flags = */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
            /* .slots = */ slots
        };

        tp = new_type(p, &spec);
        check(tp, "nb_static_property type creation failed!");

        p->nb_static_property_descr_set = nb_static_property_descr_set;
        p->nb_static_property.store_release(tp);
    }

    return tp;
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
