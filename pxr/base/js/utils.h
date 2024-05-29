//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_JS_UTILS_H
#define PXR_BASE_JS_UTILS_H

/// \file js/utils.h

#include "pxr/pxr.h"
#include "pxr/base/js/api.h"
#include "pxr/base/js/value.h"

#include <optional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

typedef std::optional<JsValue> JsOptionalValue;

/// Returns the value associated with \p key in the given \p object. If no
/// such key exists, and the supplied default is not supplied, this method
/// returns an uninitialized optional JsValue. Otherwise, the \p 
/// defaultValue is returned.
JS_API
JsOptionalValue JsFindValue(
    const JsObject& object,
    const std::string& key,
    const JsOptionalValue& defaultValue = std::nullopt);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_JS_UTILS_H
