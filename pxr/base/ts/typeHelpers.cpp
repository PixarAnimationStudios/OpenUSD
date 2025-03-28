//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/typeHelpers.h"

PXR_NAMESPACE_OPEN_SCOPE

#define _MAKE_CLAUSE(unused, tuple)                                     \
template <>                                                             \
TfType Ts_GetType<TS_SPLINE_VALUE_CPP_TYPE(tuple)>()                    \
{                                                                       \
    static const TfType tfType =                                        \
        TfType::Find<TF_PP_TUPLE_ELEM(1, tuple)>();                     \
    return tfType;                                                      \
}
TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES)
#undef _MAKE_CLAUSE

TfType Ts_GetTypeFromTypeName(const std::string &typeName)
{
    if (typeName == "double")
    {
        return Ts_GetType<double>();
    }
    if (typeName == "float")
    {
        return Ts_GetType<float>();
    }
    if (typeName == "half")
    {
        return Ts_GetType<GfHalf>();
    }
    return TfType();
}

std::string Ts_GetTypeNameFromType(const TfType valueType)
{
    if (valueType == Ts_GetType<double>())
    {
        return "double";
    }
    if (valueType == Ts_GetType<float>())
    {
        return "float";
    }
    if (valueType == Ts_GetType<GfHalf>())
    {
        return "half";
    }
    return "";
}

template <>
bool Ts_IsFinite(const GfHalf value)
{
    return value.isFinite();
}


PXR_NAMESPACE_CLOSE_SCOPE
