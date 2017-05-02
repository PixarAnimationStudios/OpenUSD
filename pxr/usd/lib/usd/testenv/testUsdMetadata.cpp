//
// Copyright 2017 Pixar
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
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE
using std::string;

// Tests to ensure the templated API for Metadata is checked.
// The semantics of the underlying functionality for both the templated
// API and the VtValue/TfAny API is more thoroughly tested via the 
// testUsdMetadata.py test.

namespace {
template <typename T>
void
_VerifyMetadata(const UsdObject& obj,
                const T& expected, 
                const TfToken& field)
{
    TF_VERIFY(!obj.HasMetadata(field));
    TF_VERIFY(obj.SetMetadata<T>(field, expected)); 
    TF_VERIFY(obj.HasMetadata(field));
    VtValue tmp;
    TF_VERIFY(obj.GetMetadata(field, &tmp));
    TF_VERIFY(tmp.IsHolding<T>());
    TF_VERIFY(tmp.Get<T>() == expected);

    T typed;
    TF_VERIFY(obj.GetMetadata<T>(field, &typed));
    TF_VERIFY(typed == expected);

    TF_VERIFY(obj.ClearMetadata(field));
    TF_VERIFY(!obj.GetMetadata<T>(field, &typed));
}

template <typename T>
void 
_VerifyMetadataByDictKey(const UsdObject& obj,
                         const T& expected, 
                         const TfToken& field,
                         const TfToken& keyPath)
{
    TF_VERIFY(!obj.HasMetadataDictKey(field, keyPath));
    TF_VERIFY(obj.SetMetadataByDictKey<T>(field, keyPath, expected));
    TF_VERIFY(obj.HasMetadataDictKey(field, keyPath));

    T typed;
    VtValue untyped;
    TF_VERIFY(obj.GetMetadataByDictKey(field, keyPath, &untyped));
    TF_VERIFY(obj.GetMetadataByDictKey<T>(field, keyPath, &typed));
    TF_VERIFY(untyped.UncheckedGet<T>() == expected);
    TF_VERIFY(typed == expected);

    TF_VERIFY(obj.ClearMetadataByDictKey(field, keyPath));
    TF_VERIFY(!obj.GetMetadataByDictKey(field, keyPath, &untyped));
    TF_VERIFY(!obj.GetMetadataByDictKey<T>(field, keyPath, &typed));
}
} // end anonymous namespace

int main() {
    for (const auto& fmt : {"usda", "usdc"}) {
        auto stage = UsdStage::CreateNew(string("test.") + fmt);
        auto prim = stage->DefinePrim(SdfPath("/World"));
        auto attr = prim.CreateAttribute(TfToken("a"), 
                                         SdfValueTypeNames->String,
                                         /* custom */ false);
        auto rel = prim.CreateRelationship(TfToken("r"), /*custom*/false);

        // Test typed value lookups/sets
        
        // prim metadata
        _VerifyMetadata(prim, string("hello"), SdfFieldKeys->Comment);
        _VerifyMetadata(prim, true, SdfFieldKeys->Active);
        _VerifyMetadata(prim, true, SdfFieldKeys->Hidden);

        // attribute metadata
        _VerifyMetadata(attr, string("hello"), SdfFieldKeys->Comment);
        VtTokenArray allowed;
        allowed.push_back(TfToken("a"));
        _VerifyMetadata(attr, allowed, SdfFieldKeys->AllowedTokens);

        // relationship metadata
        _VerifyMetadata(rel, true, SdfFieldKeys->NoLoadHint);

        // Test typed dictionary lookups/sets
        TfToken cd = SdfFieldKeys->CustomData;
        VtDictionary testDict { {"foo", VtValue(5)} };

        // prim metadata
        _VerifyMetadata(prim, testDict, cd);
        _VerifyMetadataByDictKey<int>(prim, 5, cd, TfToken("in"));
        _VerifyMetadataByDictKey<string>(prim, string("hello"), cd, TfToken("str"));
        _VerifyMetadataByDictKey<double>(prim, 5.5, cd, TfToken("dbl"));

        // attribute metadata
        _VerifyMetadata(attr, testDict, cd);
        _VerifyMetadataByDictKey<int>(attr, 10, cd, TfToken("aIn"));
        _VerifyMetadataByDictKey<string>(attr, string("aHello"), cd, TfToken("aStr"));
        _VerifyMetadataByDictKey<double>(attr, 10.10, cd, TfToken("aDbl"));

        // relationship metadata
        _VerifyMetadata(rel, testDict, cd);
        _VerifyMetadataByDictKey<int>(rel, 20, cd, TfToken("rIn"));
        _VerifyMetadataByDictKey<string>(rel, string("rHello"), cd, TfToken("rStr"));
        _VerifyMetadataByDictKey<double>(rel, 20.20, cd, TfToken("rDbl"));
    }
    
    printf("\n\n>>> Test SUCCEEDED\n");
}
