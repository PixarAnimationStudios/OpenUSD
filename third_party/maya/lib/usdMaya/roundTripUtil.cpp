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

#include "usdMaya/roundTripUtil.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/primvar.h"


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // We just store everything in a top level dictionary named "Maya".
    (Maya)

        // This annotates data that was exported to maya but should not be
        // re-imported into Maya when trying to "roundtrip" data.
        (generated)

        (clamped)

        // This annotates if the attribute used to be a maya array.  The index
        // of the array is likely encoded in the attribute name though we could
        // extend this to store the name and index.
        (arrayIndex)
);


template <typename T>
static
bool
_GetMayaDictValue(const UsdAttribute& attr, const TfToken& key, T* outVal)
{
    VtValue data = attr.GetCustomDataByKey(_tokens->Maya);
    if (data.IsEmpty()) {
        return false;
    }

    if (!data.IsHolding<VtDictionary>()) {
        TF_WARN("Expected %s on <%s> to be a dictionary.",
                _tokens->Maya.GetText(),
                attr.GetPath().GetText());
        return false;
    }

    VtValue val;
    if (TfMapLookup(data.UncheckedGet<VtDictionary>(), key, &val)) {
        if (val.IsHolding<T>()) {
            *outVal = val.UncheckedGet<T>();
            return true;
        }
        else {
            TF_WARN("Unexpected type for %s[%s] on <%s>.",
                    _tokens->Maya.GetText(),
                    key.GetText(),
                    attr.GetPath().GetText());
        }
    }

    return false;
}

template <typename T>
static
void
_SetMayaDictValue(
        const UsdAttribute& attr,
        const TfToken& flagName,
        const T& val)
{
    VtValue data = attr.GetCustomDataByKey(_tokens->Maya);

    VtDictionary newDict;
    if (!data.IsEmpty()) {
        if (data.IsHolding<VtDictionary>()) {
            newDict = data.UncheckedGet<VtDictionary>();
        }
        else {
            TF_WARN("Expected to get %s on <%s> to be a dictionary.",
                    _tokens->Maya.GetText(),
                    attr.GetPath().GetText());
            return;
        }
    }

    newDict[flagName] = VtValue(val);
    attr.SetCustomDataByKey(_tokens->Maya, VtValue(newDict));
}

/* static */
bool
UsdMayaRoundTripUtil::IsAttributeUserAuthored(const UsdAttribute& attr)
{
    return attr.HasAuthoredValueOpinion() && !IsAttributeMayaGenerated(attr);
}

/* static */
bool
UsdMayaRoundTripUtil::IsAttributeMayaGenerated(const UsdAttribute& attr)
{
    bool ret = false;
    return _GetMayaDictValue(attr, _tokens->generated, &ret) && ret;
}

/* static */
void
UsdMayaRoundTripUtil::MarkAttributeAsMayaGenerated(const UsdAttribute& attr)
{
    _SetMayaDictValue(attr, _tokens->generated, true);
}

/* static */
bool
UsdMayaRoundTripUtil::IsPrimvarClamped(const UsdGeomPrimvar& primvar)
{
    bool ret = false;
    return _GetMayaDictValue(primvar.GetAttr(), _tokens->clamped, &ret) && ret;
}

/* static */
void
UsdMayaRoundTripUtil::MarkPrimvarAsClamped(const UsdGeomPrimvar& primvar)
{
    _SetMayaDictValue(primvar.GetAttr(), _tokens->clamped, true);
}

/* static */
bool
UsdMayaRoundTripUtil::GetAttributeArray(
        const UsdAttribute& attr,
        unsigned int* index)
{
    return _GetMayaDictValue(attr, _tokens->arrayIndex, index);
}

/* static */
void
UsdMayaRoundTripUtil::MarkAttributeAsArray(
        const UsdAttribute& attr,
        const unsigned int index)
{
    _SetMayaDictValue(attr, _tokens->arrayIndex, index);
}


PXR_NAMESPACE_CLOSE_SCOPE
