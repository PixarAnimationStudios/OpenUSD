//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
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
    // Extract VtValue from python object.
    VtValue val;
    {
        TfPyLock lock;
        VtValueFromPythonObject(pyVal.ptr(), &val);
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
    SdfSchema const &schema = SdfSchema::GetInstance();

    SdfSchema::FieldDefinition const *fieldDef = schema.GetFieldDefinition(key);

    if (!fieldDef) {
        TF_CODING_ERROR("Unregistered metadata key: %s", key.GetText());
        return false;
    }

    VtValue value;
    VtValueFromPythonObject(pyVal.ptr(), &value);

    // Empty values are always considered valid.
    if (value.IsEmpty()) {
        *result = value;
        return true;
    }

    // Attempt to obtain a fallback value.
    VtValue fallback = fieldDef->GetFallbackValue();

    if (value.IsHolding<VtDictionary>()) {
        VtDictionary inDict;
        value.UncheckedSwap(inDict);
        std::string errMsg;
        if (!SdfConvertToValidMetadataDictionary(&inDict, &errMsg)) {
            TfPyThrowValueError(
                TfStringPrintf(
                    "Invalid value type for %s%s%s: %s.",
                    key.GetText(), keyPath.IsEmpty() ? "" : ":",
                    keyPath.GetText(), errMsg.c_str()));
        }
        value.UncheckedSwap(inDict);
    }

    if (!keyPath.IsEmpty() && fallback.IsHolding<VtDictionary>()) {
        if (!fieldDef->IsValidMapValue(value)) {
            TfPyThrowValueError(
                TfStringPrintf(
                    "Invalid value type for dictionary key-path '%s:%s': '%s'.",
                    key.GetString().c_str(),
                    keyPath.GetText(),
                    TfPyRepr(pyVal.Get()).c_str()));
        }
        // Clear out the fallback here, since we allow any scene desc type in
        // dicts.
        fallback = VtValue();
    }

    // We have to handle a few things as special cases to disambiguate
    // types from Python.
    if (!fallback.IsEmpty()) {
        if (fallback.IsHolding<TfTokenVector>()) {
            value = nanobind::cast<TfTokenVector>(pyVal.Get());
        }
        else if (fallback.IsHolding< std::vector<std::string> >()) {
            std::vector<std::string> vec;
            VtStringArray a;
            if (nanobind::try_cast<std::vector<std::string>>(pyVal.Get(), vec)) {
                value = std::move(vec);
            } else if (nanobind::try_cast<VtStringArray>(pyVal.Get(), a)) {
                value = std::vector<std::string>(a.begin(), a.end());
            }
        }
        else {
            value.CastToTypeOf(fallback);
        }
    }
    // If we failed to produce a value, or if we don't have a fallback and the
    // value we do have is not valid for either the field definition or the
    // schema, then complain.
    if (value.IsEmpty() ||
        (fallback.IsEmpty() &&
         (!fieldDef->IsValidValue(value) || !schema.IsValidValue(value)))) {
        VtValue origValue;
        VtValueFromPythonObject(pyVal.ptr(), &origValue);
        TfPyThrowValueError(
            TfStringPrintf(
                "Invalid value '%s' (type '%s') for key '%s%s'.%s",
                TfPyRepr(pyVal.Get()).c_str(),
                origValue.GetTypeName().c_str(),
                key.GetText(),
                keyPath.IsEmpty() ? "" :
                TfStringPrintf(":%s", keyPath.GetText()).c_str(),
                fallback.IsEmpty() ? "" :
                TfStringPrintf(" Expected type '%s'", fallback.GetType().
                               GetTypeName().c_str()).c_str()));
    }
    result->Swap(value);
    return true;
}

static std::type_info const *_objTypeInfos[Usd_NumObjTypes];
static Usd_ObjectSubclass::DowncastFn _downcastFns[Usd_NumObjTypes];

PyObject *
Usd_ObjectSubclassConvertToPython(const UsdObject *obj)
{
    UsdObjType dynType = Usd_GetObjType(*obj);
    return NB_CALL(nb_type_put)(
        NB_CTX, _objTypeInfos[dynType], nullptr,
        const_cast<void *>(_downcastFns[dynType](obj)),
        nanobind::rv_policy::copy, nullptr, nullptr);
}

void
Usd_ObjectSubclass::_ReplaceConverter(
    std::type_info const &ti,
    UsdObjType objType,
    DowncastFn downcast)
{
    _objTypeInfos[objType] = &ti;
    _downcastFns[objType] = downcast;
}

static PyObject *(*_primRangeToPython)(const UsdPrimRange &) = nullptr;
static bool (*_primRangeFromPython)(PyObject *, UsdPrimRange *) = nullptr;

void
Usd_RegisterPrimRangePythonConversions(
    PyObject *(*toPy)(const UsdPrimRange &),
    bool (*fromPy)(PyObject *, UsdPrimRange *))
{
    _primRangeToPython = toPy;
    _primRangeFromPython = fromPy;
}

PyObject *
Usd_PrimRangeToPython(const UsdPrimRange &primRange)
{
    return _primRangeToPython ? _primRangeToPython(primRange) : nullptr;
}

bool
Usd_PrimRangeFromPython(PyObject *obj, UsdPrimRange *out)
{
    return _primRangeFromPython ? _primRangeFromPython(obj, out) : false;
}

PXR_NAMESPACE_CLOSE_SCOPE

