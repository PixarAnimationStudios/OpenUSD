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
#ifndef USD_CONVERSIONS_H
#define USD_CONVERSIONS_H

#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/usd/usd/api.h"

class VtValue;
class TfToken;
class SdfValueTypeName;

// XXX: DEPRECATED.  This function does nothing except convert \p value to
// python.  Do not call, it will be removed.
USD_API
TfPyObjWrapper UsdVtValueToPython(const VtValue &value);

/// Helper for converting a python value to the target Usd/Sdf type, if
/// possible.  Invokes VtValue::CastToTypeOf() to do the conversion, if
/// required.  This internally handles python buffers (e.g. numpy) -> VtArray
/// and some python tuple/list -> VtArray conversions.  If conversion fails,
/// returns a VtValue extracted from the pyVal, which may produce a VtValue
/// holding a python object.
USD_API
VtValue UsdPythonToSdfType(TfPyObjWrapper pyVal,
                           SdfValueTypeName const &targetType);

/// Helper for converting a python value to a metadata value for metadata
/// known to the SdfSchema.  Generates a coding error if \p key is unknown
/// to the SdfSchema.
///
/// For dictionary-valued metadata, \p keyPath may be specified as the path
/// in the dictionary we are targeting, so that if the dictionary was registered
/// with a fallback for that dictionary subcomponent, we will convert
/// appropriately to its type.
///
/// \return \c true on successful conversion, which can happen even if
/// the converted \p result is an empty VtValue
USD_API
bool UsdPythonToMetadataValue(const TfToken &key, const TfToken &keyPath, 
                              TfPyObjWrapper pyVal, VtValue *result);

#endif // USD_CONVERSIONS_H
