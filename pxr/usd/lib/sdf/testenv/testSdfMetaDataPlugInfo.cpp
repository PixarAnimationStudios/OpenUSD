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

#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace std;

// Generic metadata is only enabled on some spec types
static const SdfSpecType validSpecTypes[] = {
    SdfSpecTypePseudoRoot,
    SdfSpecTypePrim,
    SdfSpecTypeAttribute,
    SdfSpecTypeRelationship,
    SdfSpecTypeVariant
};
static const SdfSpecType invalidSpecTypes[] = {
    SdfSpecTypeConnection,
    SdfSpecTypeExpression,
    SdfSpecTypeMapper,
    SdfSpecTypeMapperArg,
    SdfSpecTypeRelationshipTarget,
    SdfSpecTypeVariantSet
};

void CheckPresent(const TfToken &field, 
                  const std::vector<SdfSpecType>& expected)
{
    const SdfSchema& schema = SdfSchema::GetInstance();
    for (SdfSpecType specType : validSpecTypes) {
        const bool isExpected = 
            (std::find(expected.begin(), expected.end(), specType) 
                != expected.end());
        
        if (isExpected) {
            TF_AXIOM(schema.IsValidFieldForSpec(field, specType));
        }
        else {
            TF_AXIOM(!schema.IsValidFieldForSpec(field, specType));
        }
    }

    for (SdfSpecType specType : invalidSpecTypes) {
        TF_AXIOM(!schema.IsValidFieldForSpec(field, specType));
    }
}

// New fields must be present only in valid spec types
void CheckPresent(const TfToken &field) {
    CheckPresent(field, {std::begin(validSpecTypes), std::end(validSpecTypes)});
}

// Bad fields must be absent in all spec types
void CheckAbsent(const TfToken &field) {
    CheckPresent(field, {});
}

// GetInfo() is specialized for each supported plugin metadata type.
template <typename T>
void GetInfo(VtArray<T> *array, string *name);

// Checks all test fields involving a plugin metadata type.
struct CheckField {
    template <typename SdfValueType>
    void operator () (const SdfValueType &) {
        typedef typename SdfValueType::Type T;

        // Get the test case info
        VtArray<T> array;
        string name;
        GetInfo<T>(&array, &name);
        TfToken single(name + "_single");
        TfToken shaped(name + "_shaped");
        TfToken single_default(name + "_single_default");
        TfToken shaped_default(name + "_shaped_default");

        // Check that the fields are there only when they should be
        cout << "Checking presence of " << single << endl;
        CheckPresent(single);
        cout << "Checking presence of " << shaped << endl;
        CheckPresent(shaped);
        cout << "Checking presence of " << single_default << endl;
        CheckPresent(single_default);
        cout << "Checking presence of " << shaped_default << endl;
        CheckPresent(shaped_default);

        // Check that the default values have the correct types
        const SdfSchema& schema = SdfSchema::GetInstance();
        cout << "Checking type of " << single << endl;
        TF_AXIOM(TfType::Find<T>() == schema.GetFallback(single).GetType());
        cout << "Checking type of " << shaped << endl;
        TF_AXIOM(TfType::Find<VtArray<T> >() == 
                 schema.GetFallback(shaped).GetType());
        cout << "Checking type of " << single_default << endl;
        TF_AXIOM(TfType::Find<T>() == 
                 schema.GetFallback(single_default).GetType());
        cout << "Checking type of " << shaped_default << endl;
        TF_AXIOM(TfType::Find<VtArray<T> >() == 
                 schema.GetFallback(shaped_default).GetType());

        // Check that the default values have the correct contents
        auto getDefaultValueForType = [](const TfType& type) {
            return SdfSchema::GetInstance().FindType(type).GetDefaultValue();
        };

        cout << "Checking default value of " << single << endl;
        TF_AXIOM(getDefaultValueForType(TfType::Find<T>()) == 
                 schema.GetFallback(single).Get<T>());
        cout << "Checking default value of " << shaped << endl;
        TF_AXIOM(getDefaultValueForType(TfType::Find<VtArray<T> >()) == 
                 schema.GetFallback(shaped).Get<VtArray<T> >());
        cout << "Checking default value of " << single_default << endl;
        TF_AXIOM(array[0] == schema.GetFallback(single_default).Get<T>());
        cout << "Checking default value of " << shaped_default << endl;
        TF_AXIOM(array == schema.GetFallback(shaped_default).Get<VtArray<T> >());
    }
};

void CheckDictionary()
{
    const SdfSchema& schema = SdfSchema::GetInstance();
    TfToken key("dictionary_single");

    cout << "Checking presence of " << key << endl;
    CheckPresent(key);

    cout << "Checking type of " << key << endl;
    TF_AXIOM(TfType::Find<VtDictionary>() == schema.GetFallback(key).GetType());

    cout << "Checking default value of " << key << endl;
    TF_AXIOM(VtDictionary() == schema.GetFallback(key).Get<VtDictionary>());
}

template <class ListOp>
void CheckListOp(const TfToken& key)
{
    const SdfSchema& schema = SdfSchema::GetInstance();

    cout << "Checking presence of " << key << endl;
    CheckPresent(key);

    cout << "Checking type of " << key << endl;
    TF_AXIOM(TfType::Find<ListOp>() == schema.GetFallback(key).GetType());

    cout << "Checking default value of " << key << endl;
    TF_AXIOM(ListOp() == schema.GetFallback(key).Get<ListOp>());
}

int main()
{
    // Load a plugin that defines the test fields
    const string pluginPath = TfAbsPath("testSdfMetaDataPlugInfo.testenv");
    cout << "Registering metadata from " << pluginPath << "\n";

    PlugPluginPtrVector plugins =
        PlugRegistry::GetInstance().RegisterPlugins(pluginPath);
    TF_AXIOM(plugins.size() == 1);
    TF_AXIOM(plugins[0]);

    // Load the new metadata fields from the plugin
    TfRegistryManager::GetInstance().SubscribeTo<SdfSchema>();

    // Check all the fields
    boost::mpl::for_each<SdfValueTraitsTypesVector>(CheckField());

    // Check the dictionary field separately
    CheckDictionary();

    // Check all supported list ops separately
    CheckListOp<SdfIntListOp>(TfToken("intlistop_single"));
    CheckListOp<SdfInt64ListOp>(TfToken("int64listop_single"));
    CheckListOp<SdfUIntListOp>(TfToken("uintlistop_single"));
    CheckListOp<SdfUInt64ListOp>(TfToken("uint64listop_single"));
    CheckListOp<SdfStringListOp>(TfToken("stringlistop_single"));
    CheckListOp<SdfTokenListOp>(TfToken("tokenlistop_single"));

    // Check that bad fields weren't loaded
    for (int i = 1; i <= 11; i++) {
        TfToken token(TfStringPrintf("bad_%d", i));
        cout << "Checking absence of " << token << endl;
        CheckAbsent(token);
    }

    // Check that fields are only added to spec types specified by 'appliesTo'
    cout << "Checking \"applies_to_layers\"\n";
    CheckPresent(TfToken("applies_to_layers"), { SdfSpecTypePseudoRoot });

    cout << "Checking \"applies_to_prims\"\n";
    CheckPresent(TfToken("applies_to_prims"), 
                 { SdfSpecTypePrim, SdfSpecTypeVariant });

    cout << "Checking \"applies_to_properties\"\n";
    CheckPresent(TfToken("applies_to_properties"), 
                 { SdfSpecTypeAttribute, SdfSpecTypeRelationship });

    cout << "Checking \"applies_to_attributes\"\n";
    CheckPresent(TfToken("applies_to_attributes"), { SdfSpecTypeAttribute });
    
    cout << "Checking \"applies_to_relationships\"\n";
    CheckPresent(TfToken("applies_to_relationships"), 
                 { SdfSpecTypeRelationship });

    cout << "Checking \"applies_to_variants\"\n";
    CheckPresent(TfToken("applies_to_variants"), { SdfSpecTypeVariant });

    cout << "Checking \"applies_to_prims_and_properties\"\n";
    CheckPresent(TfToken("applies_to_prims_and_properties"), 
                 { SdfSpecTypePrim, SdfSpecTypeVariant,
                   SdfSpecTypeAttribute, SdfSpecTypeRelationship });

    cout << "Passed!" << endl;

    return EXIT_SUCCESS;
}

template <>
void GetInfo<GfMatrix3d>(VtArray<GfMatrix3d> *array, string *name)
{
    *name = "matrix3d";
    *array = VtArray<GfMatrix3d>(3);
    (*array)[0] = GfMatrix3d(0.5, 1.5, 2.5, 
                             3.5, 4.5, 5.5, 
                             6.5, 7.5, 8.5);
    (*array)[1] = GfMatrix3d(9.5, 10.5, 11.5, 
                             12.5, 13.5, 14.5, 
                             15.5, 16.5, 17.5);
    (*array)[2] = GfMatrix3d(18.5, 19.5, 20.5, 
                             21.5, 22.5, 23.5, 
                             24.5, 25.5, 26.5);
}

template <>
void GetInfo<string>(VtArray<string> *array, string *name)
{
    *name = "string";
    *array = VtArray<string>(3);
    (*array)[0] = string("a");
    (*array)[1] = string("b");
    (*array)[2] = string("c");
}

template <>
void GetInfo<TfToken>(VtArray<TfToken> *array, string *name)
{
    *name = "token";
    *array = VtArray<TfToken>(3);
    (*array)[0] = TfToken("a");
    (*array)[1] = TfToken("b");
    (*array)[2] = TfToken("c");
}

template <>
void GetInfo<bool>(VtArray<bool> *array, string *name)
{
    *name = "bool";
    *array = VtArray<bool>(3);
    (*array)[0] = bool(1);
    (*array)[1] = bool(0);
    (*array)[2] = bool(1);
}

template <>
void GetInfo<unsigned char>(VtArray<unsigned char> *array, string *name)
{
    *name = "uchar";
    *array = VtArray<unsigned char>(3);
    (*array)[0] = 1;
    (*array)[1] = 2;
    (*array)[2] = 3;
}

template <>
void GetInfo<int>(VtArray<int> *array, string *name)
{
    *name = "int";
    *array = VtArray<int>(3);
    (*array)[0] = int(1);
    (*array)[1] = int(2);
    (*array)[2] = int(3);
}

template <>
void GetInfo<unsigned int>(VtArray<unsigned int> *array, string *name)
{
    *name = "uint";
    *array = VtArray<unsigned int>(3);
    (*array)[0] = 1;
    (*array)[1] = 2;
    (*array)[2] = 3;
}

template <>
void GetInfo<int64_t>(VtArray<int64_t> *array, string *name)
{
    *name = "int64";
    *array = VtArray<int64_t>(3);
    (*array)[0] = int64_t(1);
    (*array)[1] = int64_t(2);
    (*array)[2] = int64_t(3);
}

template <>
void GetInfo<uint64_t>(VtArray<uint64_t> *array, string *name)
{
    *name = "uint64";
    *array = VtArray<uint64_t>(3);
    (*array)[0] = 1;
    (*array)[1] = 2;
    (*array)[2] = 3;
}

template <>
void GetInfo<GfHalf>(VtArray<GfHalf> *array, string *name)
{
    *name = "half";
    *array = VtArray<GfHalf>(3);
    (*array)[0] = GfHalf(0.5);
    (*array)[1] = GfHalf(1.5);
    (*array)[2] = GfHalf(2.5);
}

template <>
void GetInfo<float>(VtArray<float> *array, string *name)
{
    *name = "float";
    *array = VtArray<float>(3);
    (*array)[0] = float(0.5);
    (*array)[1] = float(1.5);
    (*array)[2] = float(2.5);
}

template <>
void GetInfo<double>(VtArray<double> *array, string *name)
{
    *name = "double";
    *array = VtArray<double>(3);
    (*array)[0] = double(0.5);
    (*array)[1] = double(1.5);
    (*array)[2] = double(2.5);
}

template <>
void GetInfo<GfVec2d>(VtArray<GfVec2d> *array, string *name)
{
    *name = "double2";
    *array = VtArray<GfVec2d>(3);
    (*array)[0] = GfVec2d(0.5, 1.5);
    (*array)[1] = GfVec2d(2.5, 3.5);
    (*array)[2] = GfVec2d(4.5, 5.5);
}

template <>
void GetInfo<GfVec2f>(VtArray<GfVec2f> *array, string *name)
{
    *name = "float2";
    *array = VtArray<GfVec2f>(3);
    (*array)[0] = GfVec2f(0.5, 1.5);
    (*array)[1] = GfVec2f(2.5, 3.5);
    (*array)[2] = GfVec2f(4.5, 5.5);
}

template <>
void GetInfo<GfVec2h>(VtArray<GfVec2h> *array, string *name)
{
    *name = "half2";
    *array = VtArray<GfVec2h>(3);
    (*array)[0] = GfVec2h(0.5, 1.5);
    (*array)[1] = GfVec2h(2.5, 3.5);
    (*array)[2] = GfVec2h(4.5, 5.5);
}

template <>
void GetInfo<GfVec2i>(VtArray<GfVec2i> *array, string *name)
{
    *name = "int2";
    *array = VtArray<GfVec2i>(3);
    (*array)[0] = GfVec2i(0, 1);
    (*array)[1] = GfVec2i(2, 3);
    (*array)[2] = GfVec2i(4, 5);
}

template <>
void GetInfo<GfVec3d>(VtArray<GfVec3d> *array, string *name)
{
    *name = "double3";
    *array = VtArray<GfVec3d>(3);
    (*array)[0] = GfVec3d(0.5, 1.5, 2.5);
    (*array)[1] = GfVec3d(3.5, 4.5, 5.5);
    (*array)[2] = GfVec3d(6.5, 7.5, 8.5);
}

template <>
void GetInfo<GfVec3f>(VtArray<GfVec3f> *array, string *name)
{
    *name = "float3";
    *array = VtArray<GfVec3f>(3);
    (*array)[0] = GfVec3f(0.5, 1.5, 2.5);
    (*array)[1] = GfVec3f(3.5, 4.5, 5.5);
    (*array)[2] = GfVec3f(6.5, 7.5, 8.5);
}

template <>
void GetInfo<GfVec3h>(VtArray<GfVec3h> *array, string *name)
{
    *name = "half3";
    *array = VtArray<GfVec3h>(3);
    (*array)[0] = GfVec3h(0.5, 1.5, 2.5);
    (*array)[1] = GfVec3h(3.5, 4.5, 5.5);
    (*array)[2] = GfVec3h(6.5, 7.5, 8.5);
}

template <>
void GetInfo<GfVec3i>(VtArray<GfVec3i> *array, string *name)
{
    *name = "int3";
    *array = VtArray<GfVec3i>(3);
    (*array)[0] = GfVec3i(0, 1, 2);
    (*array)[1] = GfVec3i(3, 4, 5);
    (*array)[2] = GfVec3i(6, 7, 8);
}

template <>
void GetInfo<GfVec4d>(VtArray<GfVec4d> *array, string *name)
{
    *name = "double4";
    *array = VtArray<GfVec4d>(3);
    (*array)[0] = GfVec4d(0.5, 1.5, 2.5, 3.5);
    (*array)[1] = GfVec4d(4.5, 5.5, 6.5, 7.5);
    (*array)[2] = GfVec4d(8.5, 9.5, 10.5, 11.5);
}

template <>
void GetInfo<GfVec4f>(VtArray<GfVec4f> *array, string *name)
{
    *name = "float4";
    *array = VtArray<GfVec4f>(3);
    (*array)[0] = GfVec4f(1.3,  2.3,  3.3,  4.3);
    (*array)[1] = GfVec4f(5.3,  6.3,  7.3,  8.3);
    (*array)[2] = GfVec4f(9.3, 10.3, 11.3, 12.3);
}

template <>
void GetInfo<GfVec4h>(VtArray<GfVec4h> *array, string *name)
{
    *name = "half4";
    *array = VtArray<GfVec4h>(3);
    (*array)[0] = GfVec4h(1.3,  2.3,  3.3,  4.3);
    (*array)[1] = GfVec4h(5.3,  6.3,  7.3,  8.3);
    (*array)[2] = GfVec4h(9.3, 10.3, 11.3, 12.3);
}

template <>
void GetInfo<GfVec4i>(VtArray<GfVec4i> *array, string *name)
{
    *name = "int4";
    *array = VtArray<GfVec4i>(3);
    (*array)[0] = GfVec4i(1, 2, 3, 4);
    (*array)[1] = GfVec4i(5, 6, 7, 8);
    (*array)[2] = GfVec4i(9, 10, 11, 12);
}

template <>
void GetInfo<GfMatrix4d>(VtArray<GfMatrix4d> *array, string *name)
{
    *name = "matrix4d";
    *array = VtArray<GfMatrix4d>(3);
    (*array)[0] = GfMatrix4d(0.5, 1.5, 2.5, 3.5, 
                             4.5, 5.5, 6.5, 7.5, 
                             8.5, 9.5, 10.5, 11.5, 
                             12.5, 13.5, 14.5, 15.5);
    (*array)[1] = GfMatrix4d(16.5, 17.5, 18.5, 19.5, 
                             20.5, 21.5, 22.5, 23.5, 
                             24.5, 25.5, 26.5, 27.5, 
                             28.5, 29.5, 30.5, 31.5);
    (*array)[2] = GfMatrix4d(32.5, 33.5, 34.5, 35.5, 
                             36.5, 37.5, 38.5, 39.5, 
                             40.5, 41.5, 42.5, 43.5, 
                             44.5, 45.5, 46.5, 47.5);
}

template <>
void GetInfo<GfMatrix2d>(VtArray<GfMatrix2d> *array, string *name)
{
    *name = "matrix2d";
    *array = VtArray<GfMatrix2d>(3);
    (*array)[0] = GfMatrix2d(0.5, 1.5, 2.5, 3.5);
    (*array)[1] = GfMatrix2d(4.5, 5.5, 6.5, 7.5);
    (*array)[2] = GfMatrix2d(8.5, 9.5, 10.5, 11.5);
}

template <>
void GetInfo<GfQuatd>(VtArray<GfQuatd> *array, string *name)
{
    *name = "quatd";
    *array = VtArray<GfQuatd>(3);
    (*array)[0] = GfQuatd(1.0, GfVec3d(1.0));
    (*array)[1] = GfQuatd(2.0, GfVec3d(2.0));
    (*array)[2] = GfQuatd(3.0, GfVec3d(3.0));
}

template <>
void GetInfo<GfQuatf>(VtArray<GfQuatf> *array, string *name)
{
    *name = "quatf";
    *array = VtArray<GfQuatf>(3);
    (*array)[0] = GfQuatf(1.0f, GfVec3f(1.0f));
    (*array)[1] = GfQuatf(2.0f, GfVec3f(2.0f));
    (*array)[2] = GfQuatf(3.0f, GfVec3f(3.0f));
}

template <>
void GetInfo<GfQuath>(VtArray<GfQuath> *array, string *name)
{
    *name = "quath";
    *array = VtArray<GfQuath>(3);
    (*array)[0] = GfQuath(1.0f, GfVec3h(1.0f));
    (*array)[1] = GfQuath(2.0f, GfVec3h(2.0f));
    (*array)[2] = GfQuath(3.0f, GfVec3h(3.0f));
}

template <>
void GetInfo<SdfAssetPath>(VtArray<SdfAssetPath> *array, string *name)
{
    *name = "asset";
    *array = VtArray<SdfAssetPath>(3);
    (*array)[0] = SdfAssetPath("a");
    (*array)[1] = SdfAssetPath("b");
    (*array)[2] = SdfAssetPath("c");
}
