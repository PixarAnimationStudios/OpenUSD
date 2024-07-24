//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/pyBufferUtils.h"
#include "pxr/base/gf/half.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

template <class T> constexpr char PyFmtFor();
template <> constexpr char PyFmtFor<bool>() { return '?'; }
template <> constexpr char PyFmtFor<char>() { return 'b'; }
template <> constexpr char PyFmtFor<unsigned char>() { return 'B'; }
template <> constexpr char PyFmtFor<short>() { return 'h'; }
template <> constexpr char PyFmtFor<unsigned short>() { return 'H'; }
template <> constexpr char PyFmtFor<int>() { return 'i'; }
template <> constexpr char PyFmtFor<unsigned int>() { return 'I'; }
template <> constexpr char PyFmtFor<long>() { return 'l'; }
template <> constexpr char PyFmtFor<unsigned long>() { return 'L'; }
template <> constexpr char PyFmtFor<GfHalf>() { return 'e'; }
template <> constexpr char PyFmtFor<float>() { return 'f'; }
template <> constexpr char PyFmtFor<double>() { return 'd'; }

} // anon

template <class T>
char *Gf_GetPyBufferFmtFor() {
    static char str[2] = { PyFmtFor<T>(), '\0' };
    return str;
}

template GF_API char *Gf_GetPyBufferFmtFor<bool>();
template GF_API char *Gf_GetPyBufferFmtFor<char>();
template GF_API char *Gf_GetPyBufferFmtFor<unsigned char>();
template GF_API char *Gf_GetPyBufferFmtFor<short>();
template GF_API char *Gf_GetPyBufferFmtFor<unsigned short>();
template GF_API char *Gf_GetPyBufferFmtFor<int>();
template GF_API char *Gf_GetPyBufferFmtFor<unsigned int>();
template GF_API char *Gf_GetPyBufferFmtFor<long>();
template GF_API char *Gf_GetPyBufferFmtFor<unsigned long>();
template GF_API char *Gf_GetPyBufferFmtFor<GfHalf>();
template GF_API char *Gf_GetPyBufferFmtFor<float>();
template GF_API char *Gf_GetPyBufferFmtFor<double>();

PXR_NAMESPACE_CLOSE_SCOPE
