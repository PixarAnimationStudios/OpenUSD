#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Array, typename Entry, size_t Size> struct array_caster {
    NB_TYPE_CASTER(Array, io_name("collections.abc.Sequence", "list") +
                              const_name("[") + make_caster<Entry>::Name +
                              const_name("]"))

    using Caster = make_caster<Entry>;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        PyObject *temp;

        // Will initialize 'temp' (NULL in the case of a failure.)
        PyObject **o = NB_CALL(seq_get_with_size)(src.ptr(), Size, &temp);

        Caster caster;
        bool success = o != nullptr;

        flags = flags_for_local_caster<Entry>(flags);

        if (success) {
            for (size_t i = 0; i < Size; ++i) {
                if (!caster.from_python(o[i], flags, cleanup) ||
                    !caster.template can_cast<Entry>()) {
                    success = false;
                    break;
                }

                value[i] = caster.operator cast_t<Entry>();
            }
        }

        Py_XDECREF(temp);

        return success;
    }

    template <typename T>
    static handle from_cpp(T &&src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        seq_builder<false> b(Size);

        if (NB_UNLIKELY(!b.valid()))
            return {};

        for (auto &value : src) {
            if (NB_UNLIKELY(b.full()))
                break;

            handle h = Caster::from_cpp(forward_like_<T>(value), policy, cleanup);
            if (NB_UNLIKELY(!h.is_valid()))
                break;

            b.put(h);
        }

        return b.commit();
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
