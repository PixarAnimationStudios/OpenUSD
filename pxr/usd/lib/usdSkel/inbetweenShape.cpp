//
// Copyright 2018 Pixar
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
#include "pxr/usd/usdSkel/inbetweenShape.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((inbetweensPrefix, "inbetweens:"))
);



UsdSkelInbetweenShape::UsdSkelInbetweenShape(const UsdAttribute& attr)
    :  _attr(attr)
{}


/* static */
UsdSkelInbetweenShape
UsdSkelInbetweenShape::_Create(const UsdPrim& prim, const TfToken& name)
{
    if(TF_VERIFY(prim)) {
        TfToken attrName = _MakeNamespaced(name);

        if(!attrName.IsEmpty()) {
            return UsdSkelInbetweenShape(
                prim.CreateAttribute(attrName, SdfValueTypeNames->Point3fArray,
                                     /*custom*/ false, SdfVariabilityUniform));
        }
    }
    return UsdSkelInbetweenShape();
}


/* static */
bool
UsdSkelInbetweenShape::IsInbetween(const UsdAttribute& attr)
{
    return attr ? _IsValidInbetweenName(attr.GetName()) : false;
}


/* static */
bool
UsdSkelInbetweenShape::_IsValidInbetweenName(const std::string& name,
                                             bool quiet)
{
    // All properly namespaced attributes are legal inbetweens.
    // However, if we extend the schema to include any special attributes
    // -- say, inbetweens:foo:pointIndices, to allow each inbetween to
    // specify its own point indices -- then we should not consider such
    // names to be invalid.
    return TfStringStartsWith(name, _tokens->inbetweensPrefix);
}


/* static */
bool
UsdSkelInbetweenShape::_IsNamespaced(const TfToken& name)
{
    return TfStringStartsWith(name, _tokens->inbetweensPrefix);
}

    
/* static */
TfToken
UsdSkelInbetweenShape::_MakeNamespaced(const TfToken& name, bool quiet)
{
    TfToken result;
    if(_IsNamespaced(name)){
        result = name;
    } else {
        result = TfToken(_tokens->inbetweensPrefix.GetString() +
                         name.GetString());
    }

    // XXX: All properly namespaced attributes are legal inbetweens.
    // However, if we extend the schema to include any special attributes
    // -- such as a namespaced pointIndices attr on each shape -- then  
    // we must validate that the name does not conflict with those.

    if(!_IsValidInbetweenName(result, quiet)) {
        result = TfToken();
    }
    return result;
}


/* static */
const TfToken&
UsdSkelInbetweenShape::_GetNamespacePrefix()
{
    return _tokens->inbetweensPrefix;
}


bool
UsdSkelInbetweenShape::GetWeight(float* weight) const
{
    return _attr.GetMetadata(UsdSkelTokens->weight, weight);
}


bool
UsdSkelInbetweenShape::SetWeight(float weight)
{
    return _attr.SetMetadata(UsdSkelTokens->weight, weight);
}


bool
UsdSkelInbetweenShape::HasAuthoredWeight() const
{
    return _attr.HasAuthoredMetadata(UsdSkelTokens->weight);
}


bool
UsdSkelInbetweenShape::GetOffsets(VtVec3fArray* offsets) const
{
    return _attr.Get(offsets);
}


bool
UsdSkelInbetweenShape::SetOffsets(const VtVec3fArray& offsets) const
{
    return _attr.Set(offsets);
}


PXR_NAMESPACE_CLOSE_SCOPE
