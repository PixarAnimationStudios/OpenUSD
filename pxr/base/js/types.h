//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_JS_TYPES_H
#define PXR_BASE_JS_TYPES_H

/// \file js/types.h

#include "pxr/pxr.h"

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class JsValue;
typedef std::map<std::string, JsValue> JsObject;
typedef std::vector<JsValue> JsArray;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_JS_TYPES_H
