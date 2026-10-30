#include "nb_internals.h"
#include "nb_ft.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

struct int64_hash {
    size_t operator()(const int64_t value) const {
        return (size_t) fmix64((uint64_t) value);
    }
};

// This data structure is used to map Python instances to integers as well as
// the inverse. We're reusing the type to avoid generating essentially the same
// code for two template instantiations. The key/value types are big enough to
// hold both.
using enum_map = tsl::robin_map<int64_t, int64_t, int64_hash>;

static PyObject *enum_create_impl(nb_internals *p, const enum_data_init *ed) {
    // Update hash table that maps from std::type_info to Python type
    bool success;
    nb_type_map_slow::iterator it;

    PyObject *existing = nullptr;
    {
        lock_internals guard(p);
        std::tie(it, success) = p->type_c2p_slow.try_emplace(ed->type, nullptr);
        if (!success) {
            existing = (PyObject *) it->second->type_py;
            NB_INCREF_ENUM(existing);
        }
    }

    if (!success) {
        // Warn only after releasing the lock: PyErr_WarnFormat can run
        // arbitrary Python code, and the internals mutex is non-reentrant
        if (PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
                             "nanobind: type '%s' was already registered!\n",
                             ed->name) != 0)
            warning_failed();
        return existing;
    }

    handle scope(ed->scope);

    bool is_arithmetic = ed->flags & (uint32_t) enum_flags::is_arithmetic;
    bool is_flag = ed->flags & (uint32_t) enum_flags::is_flag;
    bool is_str = ed->flags & (uint32_t) enum_flags::is_str;

    if (is_str && (is_flag || is_arithmetic))
        fail("nanobind: is_str cannot be combined with is_flag or "
             "is_arithmetic (enumeration \"%s\")", ed->name);

    str name(ed->name), qualname = name;
    object modname;

    if (PyModule_Check(ed->scope)) {
        modname = str_getattr_def(p, scope, "__name__");
    } else {
        modname = getattr(scope, NB_INTERNED(p, __module__), handle());

        object scope_qualname = str_getattr_def(p, scope, "__qualname__");
        if (scope_qualname.is_valid())
            qualname = steal<str>(
                PyUnicode_FromFormat("%U.%U", scope_qualname.ptr(), name.ptr()));
    }

    // Python's enum module requires the name to be present
    check(modname.is_valid(),
          "nanobind::detail::enum_create(\"%s\"): the scope lacks a "
          "'__name__' or '__module__' attribute, could not determine the "
          "enumeration's module name!", ed->name);

    const char *factory_name = "Enum";

    if (is_arithmetic && is_flag)
        factory_name = "IntFlag";
    else if (is_flag)
        factory_name = "Flag";
    else if (is_arithmetic)
        factory_name = "IntEnum";
    else if (is_str)
        factory_name = "StrEnum";

    object enum_mod = module_::import_("enum");

    bool str_fallback = false;
#if PY_VERSION_HEX < 0x030B0000
    // enum.StrEnum was added in Python 3.11. On earlier versions, fall back to
    // bare Enum with type=str, which produces an equivalent class derived from (str, Enum).
    str_fallback = is_str;
#endif

    // Call, e.g., enum.IntEnum(name, (), module=.., qualname=.. [, type=str])
    object factory = str_getattr(p, enum_mod,
                                 str_fallback ? "Enum" : factory_name);
    object empty = steal(raise_if_null(PyTuple_New(0)));
    object kwnames = steal(raise_if_null(
        str_fallback ? Py_BuildValue("(sss)", "module", "qualname", "type")
                     : Py_BuildValue("(ss)", "module", "qualname")));

    PyObject *call_args[] = { nullptr, name.ptr(), empty.ptr(), modname.ptr(),
                              qualname.ptr(), (PyObject *) &PyUnicode_Type };

    object result = steal(raise_if_null(PyObject_Vectorcall(
        factory.ptr(), call_args + 1, 2 | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames.ptr())));

    setattr(scope, name, result);
    str_setattr(p, result, "__doc__",
                ed->docstr ? (object) str(ed->docstr) : (object) none());

    object enum_str = str_getattr(
        p, str_getattr(p, enum_mod, is_flag ? factory_name : "Enum"),
        "__str__");
    str_setattr(p, result, "__str__", enum_str);
    str_setattr(p, result, "__repr__", enum_str);

    enum_type_data *t = new enum_type_data{};
    t->internals = p;
    t->name = strdup_check(ed->name);
    t->type = ed->type;
    t->type_py = (PyTypeObject *) result.ptr();
    t->flags = NB_ABI_FLAGS(ed->flags);
    t->enum_tbl.fwd = new enum_map();
    t->enum_tbl.rev = new enum_map();
    t->scope = ed->scope;

    {
        lock_internals guard(p);
        p->type_c2p_slow[ed->type] = t;

        #if !defined(NB_FREE_THREADED)
            p->type_c2p_fast[(void *) ed->type] = t;
        #endif
    }

    make_immortal(result.ptr());

    str_setattr(p, result, "__nb_enum__",
                capsule(t, [](void *td) noexcept {
                    enum_type_data *t = (enum_type_data *) td;
                    delete (enum_map *) t->enum_tbl.fwd;
                    delete (enum_map *) t->enum_tbl.rev;
                    nb_type_unregister(t);
                    free((char*) t->name);
                    delete t;
                }));

    return result.release().ptr();
}

PyObject *enum_create(nb_internals *p, const enum_data_init *ed) noexcept {
    try {
        return enum_create_impl(p, ed);
    } catch (...) {
        fail_exception("nanobind::detail::enum_create", ed->name);
    }
}

static enum_type_data *enum_get_type_data(handle tp) {
    object c = steal(raise_if_null(
        PyObject_GetAttrString(tp.ptr(), "__nb_enum__")));
    return (enum_type_data *) (borrow<capsule>(c)).data();
}

static void enum_append_impl(PyObject *tp_, const char *name_, int64_t value_,
                             const char *str_value_, const char *doc) {
    handle tp(tp_),
           val_tp(&PyLong_Type),
           str_tp((PyObject *) &PyUnicode_Type),
           obj_tp((PyObject *) &PyBaseObject_Type);

    type_data *t = enum_get_type_data(tp);
    nb_internals *p = t->internals;
    bool is_str = (t->flags & (uint32_t) enum_flags::is_str);

    if (is_str && !str_value_)
        fail("enum_append(): StrEnum member \"%s.%s\" must be added with "
             "str_value() instead of value().", t->name, name_);

    if (!is_str && str_value_)
        fail("enum_append(): str_value() can only be used on enumerations "
             "declared with nb::is_str() (member \"%s.%s\").",
             t->name, name_);

    object val;
    if (is_str) {
        val = steal(PyUnicode_InternFromString(str_value_));
        if (!val.is_valid())
            fail("enum_append(): unable to intern string value for \"%s.%s\"",
                 t->name, name_);
    } else if (t->flags & (uint32_t) enum_flags::is_signed) {
        val = steal(PyLong_FromLongLong((long long) value_));
    } else {
        val = steal(PyLong_FromUnsignedLongLong((unsigned long long) value_));
    }

    dict value_map = borrow<dict>(str_getattr(p, tp, "_value2member_map_")),
         member_map = borrow<dict>(str_getattr(p, tp, "_member_map_"));
    list member_names = borrow<list>(str_getattr(p, tp, "_member_names_"));
    str name(name_);

    if (member_map.contains(name))
        fail("refusing to add duplicate key \"%s\" to enumeration \"%s\"!",
             name_, type_name(tp).c_str());

    # if PY_VERSION_HEX >= 0x030B0000
    // In Python 3.11+, update the flag and bit masks by hand,
    // since enum._proto_member.__set_name__ is not called in this code path.
    if (t->flags & (uint32_t) enum_flags::is_flag) {
        object flag_mask = str_getattr(p, tp, "_flag_mask_");
        flag_mask = steal(
            raise_if_null(PyNumber_InPlaceOr(flag_mask.ptr(), val.ptr())));
        str_setattr(p, tp, "_flag_mask_", flag_mask);

        bool is_single_bit = (value_ != 0) && (value_ & (value_ - 1)) == 0;
        if (is_single_bit && str_hasattr(p, tp, "_singles_mask_")) {
            object singles_mask = str_getattr(p, tp, "_singles_mask_");
            singles_mask = steal(raise_if_null(
                PyNumber_InPlaceOr(singles_mask.ptr(), val.ptr())));
            str_setattr(p, tp, "_singles_mask_", singles_mask);
        }

        int_ bit_length =
            int_(obj_call(p, str_getattr(p, flag_mask, "bit_length")));
        str_setattr(p, tp, "_all_bits_", (int_(2) << bit_length) - int_(1));
    }
    #endif

    object el;
    if (issubclass(tp, str_tp))
        el = obj_call(p, getattr(str_tp, NB_INTERNED(p, __new__)), tp, val);
    else if (issubclass(tp, val_tp))
        el = obj_call(p, getattr(val_tp, NB_INTERNED(p, __new__)), tp, val);
    else
        el = obj_call(p, getattr(obj_tp, NB_INTERNED(p, __new__)), tp);

    str_setattr(p, el, "_name_", name);
    str_setattr(p, el, "__objclass__", tp);
    obj_call(p, getattr(el, NB_INTERNED(p, __init__)), val);
    str_setattr(p, el, "_sort_order_",
                steal(raise_if_null(
                    PyLong_FromSize_t(len(member_names)))));
    str_setattr(p, el, "_value_", val);
    str_setattr(p, el, "__doc__", doc ? (object) str(doc) : (object) none());

    // Compatibility with nanobind 1.x
    str_setattr(p, el, "__name__", name);

    setattr(tp, name, el);

    if (!value_map.contains(val)) {
        member_names.append(name);
        value_map[val] = el;
    }

    member_map[name] = el;

    enum_map *fwd = (enum_map *) t->enum_tbl.fwd;
    fwd->emplace(value_, (int64_t) (uintptr_t) el.ptr());

    enum_map *rev = (enum_map *) t->enum_tbl.rev;
    rev->emplace((int64_t) (uintptr_t) el.ptr(), value_);
}

void enum_append(PyObject *tp, const char *name, int64_t value,
                 const char *str_value, const char *doc) noexcept {
    try {
        enum_append_impl(tp, name, value, str_value, doc);
    } catch (...) {
        fail_exception("nanobind::detail::enum_append", name);
    }
}

bool enum_from_python(nb_internals *p, const std::type_info *tp, PyObject *o,
                      int64_t *out, uint32_t flags) noexcept {
    type_data *t = nb_type_c2p(p, tp);
    if (!t)
        return false;

    if ((t->flags & (uint32_t) enum_flags::is_flag) != 0 && Py_TYPE(o) == t->type_py) {
        PyObject *value_o =
                PyObject_GetAttr(o, NB_INTERNED(p, value));
        if (value_o == nullptr) {
            PyErr_Clear();
            return false;
        }
        if ((t->flags & (uint32_t) enum_flags::is_signed)) {
            long long value = PyLong_AsLongLong(value_o);
            Py_DECREF(value_o);
            if (value == -1 && PyErr_Occurred()) {
                PyErr_Clear();
                return false;
            }
            *out = (int64_t) value;
            return true;
        } else {
            unsigned long long value = PyLong_AsUnsignedLongLong(value_o);
            Py_DECREF(value_o);
            if (value == (unsigned long long) -1 && PyErr_Occurred()) {
                PyErr_Clear();
                return false;
            }
            *out = (int64_t) value;
            return true;
        }
    }

    enum_map *rev = (enum_map *) t->enum_tbl.rev;
    enum_map::iterator it = rev->find((int64_t) (uintptr_t) o);

    if (it != rev->end()) {
        *out = it->second;
        return true;
    }

    if (flags & cast_flags::convert) {
        enum_map *fwd = (enum_map *) t->enum_tbl.fwd;

        if (t->flags & (uint32_t) enum_flags::is_str) {
            if (!isinstance<str>(o))
                return false;
            PyObject *vmap = PyObject_GetAttrString(
                (PyObject *) t->type_py, "_value2member_map_");
            if (vmap) {
                bool error;
                PyObject *member = dict_getitem_ref(vmap, o, &error);
                Py_DECREF(vmap);
                if (member) {
                    enum_map::iterator it3 =
                        rev->find((int64_t) (uintptr_t) member);
                    Py_DECREF(member);
                    if (it3 != rev->end()) {
                        *out = it3->second;
                        return true;
                    }
                } else if (error) {
                    PyErr_Clear();
                }
            } else {
                PyErr_Clear();
            }
            return false;
        }

        if (!PyLong_CheckExact(o) && !PyIndex_Check(o))
            return false;

        if (t->flags & (uint32_t) enum_flags::is_signed) {
            long long value = PyLong_AsLongLong(o);
            if (value == -1 && PyErr_Occurred()) {
                PyErr_Clear();
                return false;
            }
            enum_map::iterator it2 = fwd->find((int64_t) value);
            if (it2 != fwd->end()) {
                *out = (int64_t) value;
                return true;
            }
        } else {
            unsigned long long value = PyLong_AsUnsignedLongLong(o);
            if (value == (unsigned long long) -1 && PyErr_Occurred()) {
                PyErr_Clear();
                return false;
            }
            enum_map::iterator it2 = fwd->find((int64_t) value);
            if (it2 != fwd->end()) {
                *out = (int64_t) value;
                return true;
            }
        }

    }

    return false;
}

PyObject *enum_from_cpp(nb_internals *p, const std::type_info *tp,
                        int64_t key) noexcept {
    type_data *t = nb_type_c2p(p, tp);
    if (!t)
        return nullptr;

    enum_map *fwd = (enum_map *) t->enum_tbl.fwd;

    enum_map::iterator it = fwd->find(key);
    if (it != fwd->end())
        return Py_NewRef((PyObject *) it->second);

    uint32_t flags = t->flags;
    if ((flags & (uint32_t) enum_flags::is_flag) != 0) {
        PyObject *enum_tp = (PyObject *) t->type_py;

        object val;
        if (flags & (uint32_t) enum_flags::is_signed)
            val = steal(PyLong_FromLongLong((long long) key));
        else
            val = steal(PyLong_FromUnsignedLongLong((unsigned long long) key));
        if (!val.is_valid())
            return nullptr;

        object new_fn = steal(PyObject_GetAttr(
            enum_tp, NB_INTERNED(p, __new__)));
        if (!new_fn.is_valid())
            return nullptr;

        // May fail, e.g. for out-of-range bits with a STRICT flag boundary
        PyObject *args[2] = { enum_tp, val.ptr() };
        return PyObject_Vectorcall(new_fn.ptr(), args, 2, nullptr);
    }

    if (flags & (uint32_t) enum_flags::is_signed)
        PyErr_Format(PyExc_ValueError, "%lld is not a valid %s.",
                     (long long) key, t->name);
    else
        PyErr_Format(PyExc_ValueError, "%llu is not a valid %s.",
                     (unsigned long long) key, t->name);

    return nullptr;
}

void enum_export(PyObject *tp) {
    enum_type_data *t = enum_get_type_data(tp);
    nb_internals *p = t->internals;

    handle scope = t->scope;
    for (handle item: handle(tp))
        setattr(scope, str_getattr(p, item, "name"), item);
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
