//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/object.hpp>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


// XXX: This function is no longer required -- remove.
TfPyObjWrapper
UsdVtValueToPython(const VtValue &value)
{
    // Convert to python.
    TfPyLock lock;
    return TfPyObjWrapper(TfPyObject(value));
}

VtValue 
UsdPythonToSdfType(TfPyObjWrapper pyVal, SdfValueTypeName const &targetType)
{
    using namespace boost::python;

    // Extract VtValue from python object.
    VtValue val;
    {
        TfPyLock lock;
        val = extract<VtValue>(pyVal.Get())();
    }

    // Attempt to cast the value to what we want.  Get a default value for this
    // attribute's type name.
    VtValue defVal = targetType.GetDefaultValue();

    // Attempt to cast the given value to the default value's type -- this
    // will convert python buffer protocol objects (e.g. numpy arrays) to the
    // appropriate typed VtArray when possible.  If casting fails, attempt to
    // continue with the given value.  Deeper in the 'Set()' implementation,
    // we'll issue a detailed type mismatch error.
    VtValue cast = VtValue::CastToTypeOf(val, defVal);
    if (!cast.IsEmpty())
        cast.Swap(val);

    return val;
}

bool
UsdPythonToMetadataValue(
    const TfToken &key, const TfToken &keyPath, 
    TfPyObjWrapper pyVal, VtValue *result)
{
    using namespace boost::python;

    VtValue fallback;
    if (!SdfSchema::GetInstance().IsRegistered(key, &fallback)) {
        TF_CODING_ERROR("Unregistered metadata key: %s", key.GetText());
        return false;
    }
    if (!keyPath.IsEmpty() && fallback.IsHolding<VtDictionary>()) {
        // Extract fallback element from fallback dict if present.
        if (VtValue const *fb = fallback.UncheckedGet<VtDictionary>().
            GetValueAtPath(keyPath.GetString())) {
            fallback = *fb;
        } else {
            fallback = VtValue();
        }
    }
    VtValue value = extract<VtValue>(pyVal.Get())();
    if (value.IsEmpty()) {
        *result = value;
        return true;
    }
    // We have to handle a few things as special cases to disambiguate
    // types from Python.
    if (!fallback.IsEmpty()) {
        if (fallback.IsHolding<SdfPath>()) {
            value = extract<SdfPath>(pyVal.Get())();
        }
        else if (fallback.IsHolding<TfTokenVector>()) {
            value = extract<TfTokenVector>(pyVal.Get())();
        }
        else if (fallback.IsHolding<SdfVariantSelectionMap>()) {
            value = extract<SdfVariantSelectionMap>(pyVal.Get())();
        }
        else if (fallback.IsHolding< std::vector<std::string> >()) {
            extract<std::vector<std::string> > getVecString(pyVal.Get());
            extract<VtStringArray> getStringArray(pyVal.Get());
            if (getVecString.check()) {
                value = getVecString();
            } else if (getStringArray.check()) {
                VtStringArray a = getStringArray();
                value = std::vector<std::string>(a.begin(), a.end());
            }
        }
        else {
            value.CastToTypeOf(fallback);
        }
    }
    if (value.IsEmpty()) {
        TfPyThrowValueError(
            TfStringPrintf(
                "Invalid type for key '%s'. Expected '%s', got '%s'",
                key.GetString().c_str(),
                fallback.GetType().GetTypeName().c_str(),
                TfPyRepr(pyVal.Get()).c_str()));
    }
    result->Swap(value);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

