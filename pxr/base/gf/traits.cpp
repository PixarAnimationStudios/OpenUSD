//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
GfIsFloatingPointType(const TfType& type)
{
    static const TfType doubleType = TfType::Find<double>();
    static const TfType floatType = TfType::Find<float>();
    static const TfType halfType = TfType::Find<GfHalf>();
    return type == doubleType || type == floatType || type == halfType;
}

bool
GfIsFloatingPointType(const std::type_info& typeInfo)
{
    return typeInfo == typeid(double) || typeInfo == typeid(float) ||
           typeInfo == typeid(GfHalf);
}

PXR_NAMESPACE_CLOSE_SCOPE
