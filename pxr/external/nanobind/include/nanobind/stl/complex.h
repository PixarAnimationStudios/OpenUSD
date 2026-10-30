/*
    nanobind/stl/complex.h: type caster for std::complex<...>

    Copyright (c) 2023 Degottex Gilles and Wenzel Jakob
    Copyright (c) 2025 High Performance Kernels LLC

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <complex>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename T> struct type_caster<std::complex<T>> {
    NB_TYPE_CASTER(std::complex<T>, const_name("complex"))

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        // The boundary function writes (real, imaginary) to two doubles;
        // std::complex<double> is layout-compatible
        std::complex<double> cmplx;
        if (!NB_CALL(load_cmplx)(NB_CTX_C(cleanup), src.ptr(), flags,
                                 (double *) &cmplx))
            return false;
        if constexpr (std::is_same_v<T, double>) {
            value = cmplx;
            return true;
        } else {
            T re = (T) cmplx.real();
            T im = (T) cmplx.imag();
            if ((flags & cast_flags::convert)
                    ||  (((double) re == cmplx.real()
                             || (re != re && cmplx.real() != cmplx.real()))
                      && ((double) im == cmplx.imag()
                             || (im != im && cmplx.imag() != cmplx.imag())))) {
                value = std::complex<T>(re, im);
                return true;
            }
            return false;
        }
    }

    static handle from_cpp(const std::complex<T>& value, rv_policy,
                           cleanup_list*) noexcept {
        return PyComplex_FromDoubles((double) value.real(),
                                     (double) value.imag());
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
