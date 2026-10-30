//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_PY_RESOLVER_CONTEXT_H
#define PXR_USD_AR_PY_RESOLVER_CONTEXT_H

/// \file ar/pyResolverContext.h
/// Macros for creating Python bindings for objects used with 
/// ArResolverContext.

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/resolverContext.h"

PXR_NAMESPACE_OPEN_SCOPE

class ArAsset;
class Ar_PyAsset {
public:
    explicit Ar_PyAsset(std::shared_ptr<ArAsset> asset)
        : _asset(std::move(asset)) {}
    std::shared_ptr<ArAsset> _asset;
};

/// Register the specified type as a context object that may be converted from
/// Python into a ArResolverContext object in C++.  This typically would be
/// called in the source file where the Python wrapping for the context object
/// is defined.
template <class Context>
void 
ArWrapResolverContextForPython();

#ifndef doxygen

using Ar_ConvertContextFromPythonFn =
    bool (*)(PyObject*, ArResolverContext*);

AR_API
void Ar_RegisterResolverContextForPython(Ar_ConvertContextFromPythonFn fn);

AR_API
bool Ar_ConvertFromPythonToResolverContext(
    PyObject* obj, ArResolverContext* ctx);

template <class Context>
bool Ar_ConvertContextFromPython(PyObject* obj, ArResolverContext* ctx)
{
    Context* ptr = nullptr;
    if (NB_CALL(nb_type_get)(NB_CTX, &typeid(Context), obj, 0, nullptr,
                             reinterpret_cast<void**>(&ptr)) && ptr) {
        if (ctx) {
            *ctx = ArResolverContext(*ptr);
        }
        return true;
    }
    return false;
}

template <class Context>
void 
ArWrapResolverContextForPython()
{
    Ar_RegisterResolverContextForPython(&Ar_ConvertContextFromPython<Context>);
}

#endif //doxygen

PXR_NAMESPACE_CLOSE_SCOPE

namespace nanobind {
namespace detail {

template <typename T>
struct type_caster<T, enable_if_t<std::is_same_v<T, PXR_NS::ArResolverContext>>>
    : type_caster_base_tag {
    using Value = T;
    template <typename T_>
    using Cast = precise_cast_t<T_>;
    static constexpr auto Name = const_name<T>();

    bool from_python(handle src, uint32_t flags,
                     cleanup_list *cleanup) noexcept {
        if (!src.is_valid()) {
            return false;
        }
        if (src.is_none()) {
            if (flags & static_cast<uint32_t>(cast_flags::construct)) {
                return false;
            }
            holder_.emplace();
            ptr_ = &*holder_;
            return true;
        }
        if (NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(T), src.ptr(),
                                 flags, cleanup,
                                 reinterpret_cast<void **>(&ptr_))) {
            return true;
        }
        if ((flags & static_cast<uint32_t>(cast_flags::construct)) ||
            !(flags & static_cast<uint32_t>(cast_flags::convert))) {
            return false;
        }
        T ctx;
        if (PXR_NS::Ar_ConvertFromPythonToResolverContext(src.ptr(), &ctx)) {
            holder_.emplace(std::move(ctx));
            ptr_ = &*holder_;
            return true;
        }
        return false;
    }

    template <typename T_>
    static handle from_cpp(T_ &&src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        return type_caster_base<T>::from_cpp(std::forward<T_>(src), policy,
                                             cleanup);
    }

    template <typename T_>
    bool can_cast() const noexcept {
        return std::is_pointer_v<T_> || (ptr_ != nullptr);
    }

    operator T *() { return ptr_; }
    operator T &() { return *ptr_; }
    operator T &&() && { return std::move(*ptr_); }

private:
    T *ptr_ = nullptr;
    std::optional<T> holder_;
};

template <>
struct type_caster<std::shared_ptr<PXR_NS::ArAsset>> {
    NB_TYPE_CASTER(std::shared_ptr<PXR_NS::ArAsset>, const_name("Ar.Ar_PyAsset"))

    bool from_python(handle src, uint8_t flags,
                     cleanup_list *cleanup) noexcept {
        if (src.is_none()) {
            value.reset();
            return true;
        }
        PXR_NS::Ar_PyAsset *ptr = nullptr;
        if (!NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(PXR_NS::Ar_PyAsset),
                                  src.ptr(), flags, cleanup,
                                  reinterpret_cast<void **>(&ptr)) ||
            !ptr) {
            return false;
        }
        value = ptr->_asset;
        return true;
    }

    static handle from_cpp(const std::shared_ptr<PXR_NS::ArAsset> &src,
                           rv_policy, cleanup_list *cleanup) noexcept {
        if (!src) {
            return none().release();
        }
        return type_caster_base<PXR_NS::Ar_PyAsset>::from_cpp(
            PXR_NS::Ar_PyAsset(src), rv_policy::move, cleanup);
    }
};

template <>
struct has_arg_defaults<PXR_NS::ArResolverContext> : std::true_type {};

} // namespace detail
} // namespace nanobind

#endif
