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
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/gf/range1d.h"
#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/range2f.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/range3f.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

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

namespace {
    // Convenience function to convert from a vector of vt values
    // which we recieve from python to a vector of reified types.
    //
    // This function assumes our output type 'VtArray' is correct and 
    // will do UncheckedGet(s) based on this.
    //
    // This function also assumes the output vector is newly initialized
    template <typename VtArrayType>
    void _ToVtArray(VtValue* output) {
        using ItemType = typename VtArrayType::value_type;
        using StdVecType = typename std::vector<VtValue>;

        const auto& input = output->UncheckedGet<StdVecType>();
        VtArrayType result;
        result.reserve(input.size());
        for (const auto& item : input) {
            result.push_back(item.template UncheckedGet<ItemType>());
        }

        output->Swap(result);
    }
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

    // We need to convert from python list types, which come in
    // as std::vector<VtValue>s to proper VtArray types which can
    // be authored in metadata.
    // 
    // Note that we don't convert all types, such as an array of 
    // GfHalf, as its not possible for a python script to author these.
    if (value.IsHolding<std::vector<VtValue>>()) {
        const auto& vRef = value.UncheckedGet<std::vector<VtValue>>();
        if (vRef.size() == 0) {
            TF_WARN("Invalid metadata authored, cannot author empty list");
            return false;
        }
        
        const auto& firstElement = vRef[0];

        // base types
        if (firstElement.IsHolding<int>()) {
            _ToVtArray<VtIntArray>(&value);
        } else if (firstElement.IsHolding<double>()) {
            _ToVtArray<VtDoubleArray>(&value);
        } else if (firstElement.IsHolding<std::string>()) {
            _ToVtArray<VtStringArray>(&value);
        } else if (firstElement.IsHolding<bool>()) {
            _ToVtArray<VtBoolArray>(&value);
        } 

        // gf vec2 types 
        else if (firstElement.IsHolding<GfVec2i>()) {
            _ToVtArray<VtVec2iArray>(&value);
        } else if (firstElement.IsHolding<GfVec2f>()) {
            _ToVtArray<VtVec2fArray>(&value); 
        } else if (firstElement.IsHolding<GfVec2d>()) {
            _ToVtArray<VtVec2dArray>(&value);
        } else if (firstElement.IsHolding<GfVec2h>()) {
            _ToVtArray<VtVec2hArray>(&value);
        } 

        // gf vec3 types
        else if (firstElement.IsHolding<GfVec3i>()) {
            _ToVtArray<VtVec3iArray>(&value);
        } else if (firstElement.IsHolding<GfVec3f>()) {
            _ToVtArray<VtVec3fArray>(&value); 
        } else if (firstElement.IsHolding<GfVec3d>()) {
            _ToVtArray<VtVec3dArray>(&value);
        } else if (firstElement.IsHolding<GfVec3h>()) {
            _ToVtArray<VtVec3hArray>(&value);
        } 

        // gf vec4 types
        else if (firstElement.IsHolding<GfVec4i>()) {
            _ToVtArray<VtVec4iArray>(&value);
        } else if (firstElement.IsHolding<GfVec4f>()) {
            _ToVtArray<VtVec4fArray>(&value); 
        } else if (firstElement.IsHolding<GfVec4d>()) {
            _ToVtArray<VtVec4dArray>(&value);
        } else if (firstElement.IsHolding<GfVec4h>()) {
            _ToVtArray<VtVec4hArray>(&value);
        } 

        // gf matrix types
        else if (firstElement.IsHolding<GfMatrix2d>()) {
            _ToVtArray<VtMatrix2dArray>(&value);
        }
        else if (firstElement.IsHolding<GfMatrix3d>()) {
            _ToVtArray<VtMatrix3dArray>(&value);
        }
        
        // gf quat types
        else if (firstElement.IsHolding<GfQuatf>()) {
            _ToVtArray<VtQuatfArray>(&value); 
        } else if (firstElement.IsHolding<GfQuatd>()) {
            _ToVtArray<VtQuatdArray>(&value);
        } else if (firstElement.IsHolding<GfQuath>()) {
            _ToVtArray<VtQuathArray>(&value);
        } 

        // error handling, unknown type authored
        else {
            TF_WARN("Invalid metadata authoring at %s, no known conversion for "
                    "list containing unknown type.", keyPath.GetText());
            return false;
        }
    } 

    result->Swap(value);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

