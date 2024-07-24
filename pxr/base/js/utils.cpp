//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file js/utils.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/utils.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

JsOptionalValue
JsFindValue(
    const JsObject& object,
    const std::string& key,
    const JsOptionalValue& defaultValue)
{
    if (key.empty()) {
        TF_CODING_ERROR("Key is empty");
        return std::nullopt;
    }

    JsObject::const_iterator i = object.find(key);
    if (i != object.end())
        return i->second;

    return defaultValue;
}

PXR_NAMESPACE_CLOSE_SCOPE
