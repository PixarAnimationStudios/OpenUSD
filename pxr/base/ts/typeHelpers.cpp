//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/typeHelpers.h"

PXR_NAMESPACE_OPEN_SCOPE

template <>
TfType Ts_GetType<double>()
{
    static const TfType tfType = TfType::Find<double>();
    return tfType;
}

template <>
TfType Ts_GetType<float>()
{
    static const TfType tfType = TfType::Find<float>();
    return tfType;
}

template <>
TfType Ts_GetType<GfHalf>()
{
    static const TfType tfType = TfType::Find<GfHalf>();
    return tfType;
}

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
