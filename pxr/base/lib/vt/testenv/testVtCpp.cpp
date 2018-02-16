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
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/functions.h"

#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/fileUtils.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/fileSystem.h"

#include <boost/scoped_ptr.hpp>

#include <cstdio>
#include <iterator>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::cout;
using std::endl;
PXR_NAMESPACE_USING_DIRECTIVE

static void die(const std::string &msg) {
    TF_FATAL_ERROR("ERROR: %s failed.", msg.c_str());
}


static void testArray() {

    VtDoubleArray da(60);

    double val = 1;
    TF_FOR_ALL(elem, da)
        *elem = val++;

    val = 1;
    for (VtDoubleArray::const_iterator i = da.begin(); i != da.end(); ++i)
        if (*i != val++)
            die("iterator");

    // Do copy-on-write cases.
    VtDoubleArray da2 = da;
    da2[0] = 333.333;

    if (da2[0] != 333.333 || 
        da[0] == 333.333)
        die("copy-on-write");

    // Try swapping
    VtDoubleArray daCopy = da;
    VtDoubleArray da2Copy = da2;

    da.swap(da2);
    TF_AXIOM(da == da2Copy);
    TF_AXIOM(da2 == daCopy);

    using std::swap;
    swap(da, da2);
    TF_AXIOM(da == daCopy);
    TF_AXIOM(da2 == da2Copy);

    {
        // Try default-constructing a VtArray.
        VtDoubleArray def;
        TF_AXIOM(def.size() == 0);
        
        // Try iterating over the array.
        std::vector<double> v(def.begin(), def.end());
        TF_AXIOM(v.empty());

        // Test resizing a default constructed array.
        def.resize(123);
        TF_AXIOM(def.size() == 123);
    }

    {
        // Try creating an empty VtArray.
        VtDoubleArray array(0);
        TF_AXIOM(array.size() == 0);

        // Try iterating over the array.
        std::vector<double> v(array.begin(), array.end());
        TF_AXIOM(v.empty());
    }

    {
        // Array push_back and resize.
        VtDoubleArray array(0);

        // Push back on a rank-1 array.
        TF_AXIOM(array.size() == 0);
        array.push_back(1.234);
        TF_AXIOM(array.size() == 1);
        TF_AXIOM(array[0] == 1.234);

        array.push_back(2.3456);
        TF_AXIOM(array.size() == 2);
        TF_AXIOM(array[0] == 1.234);
        TF_AXIOM(array[1] == 2.3456);

        array.pop_back();
        TF_AXIOM(array.size() == 1);
        TF_AXIOM(array[0] == 1.234);

        // Resize should preserve elements.
        array.resize(100);
        TF_AXIOM(array.size() == 100);
        TF_AXIOM(array[0] == 1.234);
        TF_AXIOM(array[1] == 0.0);
        TF_AXIOM(array[50] == 0.0);
        TF_AXIOM(array[99] == 0.0);

        for (size_t i = 0; i != 100; ++i)
            array[i] = i;

        array.resize(1000);
        TF_AXIOM(array.size() == 1000);
        for (size_t i = 0; i != 1000; ++i) {
            if (i < 100) {
                TF_AXIOM(array[i] == i);
            } else {
                TF_AXIOM(array[i] == 0);
            }
        }

        array.resize(10);
        TF_AXIOM(array.size() == 10);
        for (size_t i = 0; i != 10; ++i) {
            TF_AXIOM(array[i] == i);
        }

        array.pop_back();
        array.pop_back();
        array.pop_back();
        array.pop_back();
        array.pop_back();

        TF_AXIOM(array.size() == 5);

    }

    {
        // Test that mutating shape data doesn't affect copies of an array.
        VtArray<int> a(4);
        a._GetShapeData()->otherDims[0] = 4;
        a._GetShapeData()->otherDims[1] = 0;

        VtArray<int> b = a;
        const auto &ca = a;
        const auto &cb = b;
        TF_AXIOM(ca._GetShapeData()->otherDims[0] ==
                 cb._GetShapeData()->otherDims[0]);
        TF_AXIOM(ca._GetShapeData()->otherDims[1] ==
                 cb._GetShapeData()->otherDims[1]);

        b._GetShapeData()->otherDims[0] = 2;
        b._GetShapeData()->otherDims[1] = 2;
        b._GetShapeData()->otherDims[2] = 0;

        // Check that a's shape data is unchanged
        TF_AXIOM(ca._GetShapeData()->otherDims[0] == 4);
        TF_AXIOM(ca._GetShapeData()->otherDims[1] == 0);

        // and that b's shape data has been updated as expected.
        TF_AXIOM(cb._GetShapeData()->otherDims[0] == 2);
        TF_AXIOM(cb._GetShapeData()->otherDims[1] == 2);
        TF_AXIOM(cb._GetShapeData()->otherDims[2] == 0);
    }
}

static void testArrayOperators() {

    {
        VtDoubleArray a(3), b(3);
        a[0] = 1;
        a[1] = 2;
        a[2] = 3;
        b[0] = 4;
        b[1] = 5;
        b[2] = 6;

        VtDoubleArray c = VtCat(a,b);
        VtDoubleArray d = c * 2.0;
        TF_AXIOM(d[3] == 8);
        VtDoubleArray e = a * b / 2.0;
        TF_AXIOM(e[2] == 9);
        TF_AXIOM(VtAnyTrue(VtEqual(a,VtZero<double>())) == false);
        TF_AXIOM(VtAllTrue(VtEqual(a-a,VtZero<double>())) == true);
        std::string empty = VtZero<std::string>();
        VtStringArray s(4);
        s[0] = empty;
        s[1] = "a";
        s[2] = "test";
        s[3] = "array";
        TF_AXIOM(VtAllTrue(VtNotEqual(s,VtZero<std::string>())) == false);
    }
}

static void testRecursiveDictionaries()
{
    VtDictionary outer;
    VtDictionary mid;
    VtDictionary inner; 

    VtDictionary outerCopy = outer;
   
    inner["one"] = VtValue(1);
    mid["inner"] = VtValue(inner);
    outer["mid"] = VtValue(mid);

    VtDictionary midCopy = outer["mid"].Get<VtDictionary>();
    VtDictionary innerCopy = inner;
    innerCopy["two"] = VtValue(2);
    midCopy["inner"] = innerCopy;
    
    TF_AXIOM(innerCopy != inner);
    TF_AXIOM(midCopy != mid);
    TF_AXIOM(outerCopy != outer);
}


static void testDictionaryKeyPathAPI()
{
    VtDictionary dict1, dict2;

    dict1.SetValueAtPath("foo:bar:baz", VtValue(1.234));
    TF_AXIOM(!dict1.empty());
    TF_AXIOM(dict1.size() == 1);
    TF_AXIOM(dict1.GetValueAtPath("foo:bar:baz"));
    TF_AXIOM(*dict1.GetValueAtPath("foo:bar:baz") == VtValue(1.234));
    TF_AXIOM(dict1.GetValueAtPath("foo:bar")->IsHolding<VtDictionary>());

    dict2["baz"] = VtValue(1.234);
    TF_AXIOM(*dict1.GetValueAtPath("foo:bar") == dict2);

    dict1.SetValueAtPath("foo:foo", VtValue(dict2));
    TF_AXIOM(dict1.GetValueAtPath("foo:foo:baz")->IsHolding<double>());
    TF_AXIOM(dict1.GetValueAtPath("foo:foo:baz")->Get<double>() == 1.234);
    TF_AXIOM(*dict1.GetValueAtPath("foo:foo") == dict2);

    TF_AXIOM(dict1.GetValueAtPath("does:not:exist") == NULL);

    dict1.SetValueAtPath("top", VtValue("level"));
    TF_AXIOM(*dict1.GetValueAtPath("top") == dict1["top"]);

    TF_AXIOM(dict1.size() == 2);

    dict1.EraseValueAtPath("does-not-exist");
    TF_AXIOM(dict1.size() == 2);

    dict1.EraseValueAtPath("top");
    TF_AXIOM(dict1.size() == 1);

    // Set another element in the foo:foo dict, then erase both elements.
    // Removing the last should remove the entire subdict.
    dict1.SetValueAtPath("foo:foo:qux", VtValue(1234));
    dict1.EraseValueAtPath("foo:foo:baz");
    TF_AXIOM(dict1.GetValueAtPath("foo:foo")->Get<VtDictionary>().size() == 1);
    dict1.EraseValueAtPath("foo:foo:qux");
    TF_AXIOM(dict1.GetValueAtPath("foo:foo") == NULL);

    // Remove an entire subdict at once.
    dict1.SetValueAtPath("foo:bar:qux", VtValue(1234));
    dict1.EraseValueAtPath("foo:bar");
    TF_AXIOM(dict1.GetValueAtPath("foo:bar:baz") == NULL);
    TF_AXIOM(dict1.GetValueAtPath("foo:bar:qux") == NULL);
    TF_AXIOM(dict1.GetValueAtPath("foo:bar") == NULL);
}


static void testDictionary() {

    // test recursive dictionaries.
    testRecursiveDictionaries();
    
    double d = 1.5;
    bool b = false;
    string s("a string");

    VtDictionary dictionary;

    // test empty dictionary's erase range method
    dictionary.erase(dictionary.begin(), dictionary.end());
    if ( dictionary != VtDictionary())
        die("VtDictionary::erase range");

    dictionary["key1"] = VtValue(d);
    dictionary["key2"] = VtValue(b);

    // test full dictionary's erase range method
    dictionary.erase(dictionary.begin(), dictionary.end());
    if ( dictionary != VtDictionary())
        die("VtDictionary::erase range");

    dictionary["key1"] = VtValue(d);
    dictionary["key2"] = VtValue(b);

    VtDictionary dictionary2;
    dictionary2["key2"] = VtValue(s);

    // In-place creation and code coverage for equality operator.
    if ( VtMakeDictionary(VtKeyValue("key1", d), VtKeyValue("key2", b)) !=
         dictionary) {
        die("VtMakeDictionary");
    }
    if ( VtMakeDictionary(VtKeyValue("key1", d), VtKeyValue("key2X", b)) ==
         dictionary ) {
        die("VtMakeDictionary");
    }
    if ( VtMakeDictionary(VtKeyValue("key1", d), VtKeyValue("key2", true)) ==
         dictionary ) {
        die("VtMakeDictionary");
    }
    if ( VtMakeDictionary(VtKeyValue("key1", d)) ==
         dictionary ) {
        die("VtMakeDictionary");
    }

    // Composite dictionary2 over dictionary.
    VtDictionaryOver(dictionary2, &dictionary);

    // Make sure the result is the same if we create a new dictionary.
    if ( VtDictionaryOver(dictionary2, dictionary) != dictionary ) {
        die("VtDictionaryOver");
    }

    // Call Over with a NULL pointer.
    fprintf(stderr, "expected error:\n");
    VtDictionaryOver(dictionary2, NULL);
    fprintf(stderr, "end expected error:\n");

    // Look up a value that was there before the composite.
    if ( !VtDictionaryIsHolding<double>(dictionary, "key1") ) {
        die("VtDictionaryIsHolding");
    }
    if (VtDictionaryGet<double>(dictionary, "key1") != d) {
        die("VtDictionaryGet");
    }

    // Look up a value that resulted from the composite.
    if ( !VtDictionaryIsHolding<string>(dictionary, "key2") ) {
        die("VtDictionaryIsHolding");
    }
    if (VtDictionaryGet<string>(dictionary, "key2") != s) {
        die("VtDictionaryGet");
    }

    // Look up a key that's not there.
    if ( VtDictionaryIsHolding<double>(dictionary, "key3") ) {
        die("VtDictionaryIsHolding");
    }

    dictionary["key1"] = VtValue(d);
    dictionary2["key3"] = VtValue(s);

    // Composite dictionary over dictionary2.
    VtDictionaryOver(&dictionary, dictionary2);

    // Make sure the result is the same if we create a new dictionary.
    if ( VtDictionaryOver(dictionary, dictionary2) != dictionary ) {
        die("VtDictionaryOver");
    }
    // Call Over with a NULL pointer.
    fprintf(stderr, "expected error:\n");
    VtDictionaryOver(NULL, dictionary2);
    fprintf(stderr, "end expected error:\n");

    // Look up a value that was there before the composite.
    if ( !VtDictionaryIsHolding<double>(dictionary, "key1") ) {
        die("VtDictionaryIsHolding");
    }
    if (VtDictionaryGet<double>(dictionary, "key1") != d) {
        die("VtDictionaryGet");
    }

    // Look up a value that resulted from the composite.
    if ( !VtDictionaryIsHolding<string>(dictionary, "key3") ) {
        die("VtDictionaryIsHolding");
    }
    if (VtDictionaryGet<string>(dictionary, "key3") != s) {
        die("VtDictionaryGet");
    }
}

static void testDictionaryOverRecursive() {
    double d = 1.5;
    double d2 = 2.5;
    bool b = false;
    bool b2 = true;
    string s("a string");

    VtDictionary subDictA;
    subDictA["key1"] = VtValue(d);
    subDictA["key2"] = VtValue(b);

    VtDictionary subDictB;
    subDictB["key2"] = VtValue(s);
    subDictB["key3"] = VtValue(b2);

    VtDictionary dictionaryA;
    dictionaryA["key1"] = VtValue(d);
    dictionaryA["key2"] = VtValue(b);
    dictionaryA["subDict"] = VtValue(subDictA);

    VtDictionary dictionaryB;
    dictionaryB["key2"] = VtValue(s);
    dictionaryB["key3"] = VtValue(d2);
    dictionaryB["subDict"] = VtValue(subDictB);

    VtDictionary aOverBSubResultRecursive;
    aOverBSubResultRecursive["key1"] = VtValue(d);
    aOverBSubResultRecursive["key2"] = VtValue(b);
    aOverBSubResultRecursive["key3"] = VtValue(b2);

    VtDictionary aOverBResult;
    aOverBResult["key1"] = VtValue(d);
    aOverBResult["key2"] = VtValue(b);
    aOverBResult["key3"] = VtValue(d2);
    aOverBResult["subDict"] = VtValue(subDictA);

    VtDictionary aOverBResultRecursive;
    aOverBResultRecursive["key1"] = VtValue(d);
    aOverBResultRecursive["key2"] = VtValue(b);
    aOverBResultRecursive["key3"] = VtValue(d2);
    aOverBResultRecursive["subDict"] = VtValue(aOverBSubResultRecursive);

    // Check methods that take references for strong and weak.
    //
    if ( VtDictionaryOver(dictionaryA, dictionaryB) != aOverBResult ) {
        die("VtDictionaryOver - two ref version");
    }
    if ( VtDictionaryOverRecursive(dictionaryA, dictionaryB) 
        != aOverBResultRecursive ) {
        die("VtDictionaryOverRecursive - two ref version recursive");
    }

    // Check methods that pointer for strong, reference for weak
    //
    fprintf(stderr, "expected error:\n");
    VtDictionaryOverRecursive(NULL, dictionaryB);
    fprintf(stderr, "end expected error:\n");
    VtDictionary aCopy = dictionaryA;
    VtDictionaryOver(&aCopy, dictionaryB);
    if ( aCopy != aOverBResult ) {
        die("VtDictionaryOver - strong Ptr version");
    }
    aCopy = dictionaryA;
    VtDictionaryOverRecursive(&aCopy, dictionaryB);
    if ( aCopy != aOverBResultRecursive ) {
        die("VtDictionaryOverRecursive - strong Ptr version");
    }

    // Check methods that use reference for strong, pointer for weak
    //
    fprintf(stderr, "expected error:\n");
    VtDictionaryOverRecursive(dictionaryA, NULL);
    fprintf(stderr, "end expected error:\n");
    VtDictionary bCopy = dictionaryB;
    VtDictionaryOver(dictionaryA, &bCopy);
    if ( bCopy != aOverBResult ) {
        die("VtDictionaryOver - strong ref, weak Ptr version");
    }
    bCopy = dictionaryB;
    VtDictionaryOverRecursive(dictionaryA, &bCopy);
    if ( bCopy != aOverBResultRecursive ) {
        die("VtDictionaryOverRecursive - strong ref, weak Ptr version");
    }
}

static void
testDictionaryIterators()
{
    // Test iterator-related things that might break if one were to attempt a
    // copy-on-write implementation for VtDictionary.

    VtKeyValue key1("key1", false);
    VtKeyValue key2("key2", true);
    VtKeyValue key3("key3", VtValue());

    // Check that copy + insertion + destruction does not invalidate iterators.
    {
        VtDictionary a = VtMakeDictionary(key1, key2);
        VtDictionary::iterator i = a.find(key2.GetKey());

        {
            boost::scoped_ptr<VtDictionary> b(new VtDictionary(a));
            a.insert(std::make_pair(key3.GetKey(), key3.GetValue()));
        }

        a.erase(i);

        VtDictionary expected = VtMakeDictionary(key1, key3);
        if (a != expected) {
            die("VtDictionary::erase(Iterator) - failed after copy");
        }
    }

    // Check that copy + insertion does not result in invalid iterators.
    {
        VtDictionary a = VtMakeDictionary(key1, key2);
        VtDictionary::const_iterator i = a.find(key2.GetKey());
        a.insert(std::make_pair(key3.GetKey(), key3.GetValue()));
        VtDictionary::const_iterator j = a.find(key2.GetKey());
        if (i != j) {
            die("VtDictionary - iterators to same element do not compare "
                "equal");
        }
    }

    // Check that iterator distance is preserved across a making a copy and
    // destroying it.
    {
        VtDictionary a = VtMakeDictionary(key1, key2);
        VtDictionary expected = VtMakeDictionary(key1, key2);
        VtDictionary::const_iterator i = a.find(key2.GetKey());
        VtDictionary::const_iterator j = expected.find(key2.GetKey());
        {
            boost::scoped_ptr<VtDictionary> b(new VtDictionary(a));
            VtDictionary::value_type v(key3.GetKey(), key3.GetValue());
            a.insert(v);
            expected.insert(v);
        }
        VtDictionary::const_iterator aEnd = a.end();
        VtDictionary::const_iterator expectedEnd = expected.end();
        if (std::distance(i, aEnd) != std::distance(j, expectedEnd)) {
            die("VtDictionary - incorrect iterator distance after copy");
        }
    }

    // Check that iterators who point to same keys in a container, also
    // dereference to equal values.
    {
        VtDictionary a = VtMakeDictionary(key1, key2);
        VtDictionary::const_iterator i = a.find(key1.GetKey());
        {
            boost::scoped_ptr<VtDictionary> b(new VtDictionary(a));
            a[key1.GetKey()] = VtValue(12);
        }

        VtDictionary::const_iterator j = a.find(key1.GetKey());
        if (i != j) {
            die("VtDictionary - iterators to same item do not compare equal");
        }

        if (*i != *j) {
            die("VtDictionary - dereferenced iterators to same item do not "
                "have equal values.");
        }
    }
}

static void
testDictionaryInitializerList()
{
    const VtDictionary dict{};
    TF_AXIOM(dict.empty());

    const VtDictionary dict2 = {
        { "key_a", VtValue(1) },
        { "key_b", VtValue(2) }
    };
    TF_AXIOM(!dict2.empty());

    int i = 0;
    for (const string& k : {"key_a", "key_b"}) {
        auto it = dict2.find(k);
        TF_AXIOM(it != dict2.end());
        TF_AXIOM(it->first == k);
        TF_AXIOM(it->second.IsHolding<int>());
        TF_AXIOM(it->second.UncheckedGet<int>() == ++i);
    }
}

// dest and source types are flipped so we can allow compiler to infer
// source type
template <class VB, class VA>
static
void _TestVecCast(VA const &vecA)
{
    string  typeNameA = ArchGetDemangled<VA>();
    string  typeNameB = ArchGetDemangled<VB>();
    VtValue  val(vecA);
    
    if (!val.CanCast<VB>()){
        die("Could not cast type " + typeNameA + " to a " + typeNameB);
    }

    TF_AXIOM(!val.Cast<VB>().IsEmpty());

    if (!(val.UncheckedGet<VB>() == VB(vecA)) ){
        die("Unboxed " + typeNameA + " to " + typeNameB + "did no compare equal");
    }
}

template <class VB, class VA>
static
void _FailVecCast(VA const &vecA)
{
    string  typeNameA = ArchGetDemangled<VA>();
    string  typeNameB = ArchGetDemangled<VB>();
    VtValue  val(vecA);
    
    if (val.CanCast<VB>()){
        die("Should not have been able to cast " + typeNameA + " to a " + typeNameB);
    }

    TF_AXIOM( val.Cast<VB>().IsEmpty() );

}

struct _NotStreamable {};
// Equality comparison requirement.
bool operator==(const _NotStreamable &l, const _NotStreamable &r)
{
    return true;
}

struct _NotDefaultConstructible
{
    explicit _NotDefaultConstructible(int x) {}
    bool operator==(const _NotDefaultConstructible &other) const {
        return true;
    }
};

// Currently the default implementation of TfMoveTo is the only thing that
// requires default constructibility for types stored in VtValue, so we provide
// an overload here that doesn't invoke it.
void
TfMoveTo(_NotDefaultConstructible *location, _NotDefaultConstructible &value)
{
    new (location) _NotDefaultConstructible(value);
}

enum Vt_TestEnum {
    Vt_TestEnumVal1,
    Vt_TestEnumVal2
};
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<Vt_TestEnum>();
}
TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(Vt_TestEnumVal1);
    TF_ADD_ENUM_NAME(Vt_TestEnumVal2);
}

static void testValue() {
    {
        // Test that we can create values holding non-streamable types. 
        static_assert(!Vt_IsOutputStreamable<_NotStreamable>::value,
                      "_NotStreamable must not satisfy Vt_IsOutputStreamable.");
        _NotStreamable n;
        VtValue v(n);
        VtValue copy = v;
        copy = n;
    }

    {
        // Test that we can store non-default-constructible objects in VtValue.
        _NotDefaultConstructible n(123);
        VtValue v(n);
        VtValue copy = v;
        copy = n;
    }

    {
        VtValue v(Vt_TestEnumVal1);
        TF_AXIOM(TfStringify(v) == "Vt_TestEnumVal1");
        v = Vt_TestEnumVal2;
        TF_AXIOM(TfStringify(v) == "Vt_TestEnumVal2");
    }

    {
        // Test that floating-point values stream as expected.
        TF_AXIOM(TfStringify(VtValue(0.0)) == "0");
        TF_AXIOM(TfStringify(VtValue(3.14159)) == "3.14159");
        TF_AXIOM(TfStringify(VtValue(0.1)) == "0.1");
        TF_AXIOM(TfStringify(VtValue(-0.000001)) == "-0.000001");
        TF_AXIOM(TfStringify(
             VtValue(std::numeric_limits<double>::infinity())) == "inf");
        TF_AXIOM(TfStringify(
             VtValue(-std::numeric_limits<double>::infinity())) == "-inf");

        TF_AXIOM(TfStringify(VtValue(0.0f)) == "0");
        TF_AXIOM(TfStringify(VtValue(3.14159f)) == "3.14159");
        TF_AXIOM(TfStringify(VtValue(0.1f)) == "0.1");
        TF_AXIOM(TfStringify(VtValue(-0.000001f)) == "-0.000001");
        TF_AXIOM(TfStringify(
             VtValue(std::numeric_limits<float>::infinity())) == "inf");
        TF_AXIOM(TfStringify(
             VtValue(-std::numeric_limits<float>::infinity())) == "-inf");
    }

    VtValue v(1.234);
    if (!v.IsHolding<double>())
        die("IsHolding");
    
    if (v.Get<double>() != 1.234)
        die("Get");

    if (v.GetTypeid() != typeid(double))
        die("GetTypeid");

    if (v.GetType() != TfType::Find<double>())
        die("GetType for unregistered type");

    if (v.GetElementTypeid() != typeid(void))
        die("GetElementTypeid for non-shaped type");

    v = VtValue("hello world");
    if (v.GetElementTypeid() != typeid(void))
        die("GetElementTypeid for non-shaped, non-stack-held type");

    if (v.IsArrayValued())
        die("IsArrayValued for non-array type");

    // Now test with shaped case.
    v = VtValue(VtDoubleArray(9));
    if (v.GetElementTypeid() != typeid(double))
        die("GetElementTypeid");

    // Test casts...

    v = VtValue(2.345);
    if (!v.CanCast<double>())
        die("CanCast to same type");
    if (v != VtValue::Cast<double>(v))
        die("Cast to same type");

    v = VtValue(2.345);
    if (!v.CanCast<int>())
        die("CanCast double to int");
    if (v.Cast<int>() != 2)
        die("Cast double to int");

    v = VtValue(2.345);
    if (!v.CanCast<short>())
        die("CanCast double to short");
    if (v.Cast<short>() != short(2))
        die("Cast double to short");

    v = VtValue(1.25);
    if (!v.CanCast<float>())
        die("CanCast double to float");
    if (v.Cast<float>() != 1.25f)
        die("Cast double to float");

    v = VtValue(1.25);
    if (v.CanCast<GfVec3d>())
        die("CanCast double to Vec3d");
    if (!v.Cast<GfVec3d>().IsEmpty())
        die("Cast to Vec3d type is not empty");

    v = VtValue(1.25);
    if (!v.CanCastToTypeOf(v))
        die("CanCast to same type");
    if (v.CastToTypeOf(v).Get<double>() != 1.25)
        die("Casting to same type got wrong value");
    
    v = VtValue(1.25);
    VtValue v2 = VtValue(3);
    if (!v.CanCastToTypeOf(v2))
        die("CanCast to type of another value");
    if (v2.CastToTypeOf(v).Get<double>() != 3.0)
        die("Could not cast to type of another value");

    v = VtValue(1.25);
    v2 = VtValue(3);
    if (!v.CanCastToTypeOf(v2))
        die("CanCast to type of another value");
    if (VtValue::CastToTypeOf(v2, v).Get<double>() != 3.0)
        die("Could not cast to type of another value");

    v = VtValue(1.25);
    if (!v.CanCastToTypeid(typeid(double)))
        die("CanCast to typeid of same type");
    if (!v.CanCastToTypeid(typeid(int)))
        die("CanCast double to typeid of int");
    if (v.CanCastToTypeid(typeid(GfVec3d)))
        die("CanCast double to typeid of GfVec3d");

    // Check that too large doubles cast to float infinities
    v = VtValue(1e50);
    if (!v.CanCast<float>())
        die("CanCast of too large double to float");
    if (v.Cast<float>() != std::numeric_limits<float>::infinity())
        die("Cast of too large double to float is not +inf");
    
    v = VtValue(-1e50);
    if (!v.CanCast<float>())
        die("CanCast of too small double to float");
    if (v.Cast<float>() != -std::numeric_limits<float>::infinity())
        die("Cast of too small double to float is not -inf");

    // Check that double infinities cast to float infinities
    v = VtValue(std::numeric_limits<double>::infinity());
    if (!v.CanCast<float>())
        die("CanCast of double +inf to float");
    if (v.Cast<float>() != std::numeric_limits<float>::infinity())
        die("Cast of double +inf to float is not +inf");

    v = VtValue(-std::numeric_limits<double>::infinity());
    if (!v.CanCast<float>())
        die("CanCast of double -inf to float");
    if (v.Cast<float>() != -std::numeric_limits<float>::infinity())
        die("Cast of double -inf to float is not -inf");

    // Check that float infinities cast to double infinities
    v = VtValue(std::numeric_limits<float>::infinity());
    if (!v.CanCast<double>())
        die("CanCast of float +inf to double");
    if (v.Cast<double>() != std::numeric_limits<double>::infinity())
        die("Cast of float +inf to double is not +inf");

    v = VtValue(-std::numeric_limits<float>::infinity());
    if (!v.CanCast<double>())
        die("CanCast of float -inf to double");
    if (v.Cast<double>() != -std::numeric_limits<double>::infinity())
        die("Cast of float -inf to double is not -inf");

    // Check that really large long long casts to double
    v = VtValue(1000000000000000000ll);
    if (!v.CanCast<double>())
        die("CanCast of really large long long to double");
    if (v.Cast<double>() != 1e+18)
        die("Cast of really large long long to double");

    // Check that really large long long casts to float
    v = VtValue(1000000000000000000ll);
    if (!v.CanCast<float>())
        die("CanCast of really large long long to float");
    if (v.Cast<float>() != 1e+18f)
        die("Cast of really large long long to float");
        
    // Check that really large long long casts to GfHalf infinity
    v = VtValue(1000000000000000000ll);
    if (!v.CanCast<GfHalf>())
        die("CanCast of really large long long to GfHalf");
    if (v.Cast<GfHalf>() != std::numeric_limits<GfHalf>::infinity())
        die("Cast of really large long long to GfHalf is not +inf");

    // Check that really small long long casts to minus GfHalf infinity
    v = VtValue(-1000000000000000000ll);
    if (!v.CanCast<GfHalf>())
        die("CanCast of really small long long to GfHalf");
    if (v.Cast<GfHalf>() != -std::numeric_limits<GfHalf>::infinity())
        die("Cast of really small long long to GfHalf is not -inf");

    // Check that too large unsigned short casts to GfHalf infinity
    v = VtValue((unsigned short)65535);
    if (!v.CanCast<GfHalf>())
        die("CanCast of too large unsigned short to GfHalf");
    if (v.Cast<GfHalf>() != std::numeric_limits<GfHalf>::infinity())
        die("Cast of too large unsigned short to GfHalf is not +inf");

    // Some sanity checks
    v = VtValue((int)0);
    if (!v.CanCast<double>())
        die("CanCast of integer zero to double");
    if (v.Cast<double>() != 0.0)
        die("Cast of integer zero to double not zero");

    v = VtValue((int)-1);
    if (!v.CanCast<double>())
        die("CanCast of integer -1 to double");
    if (v.Cast<double>() != -1.0)
        die("Cast of integer -1 to double not -1");

    v = VtValue((int)+1);
    if (!v.CanCast<double>())
        die("CanCast of integer one to double");
    if (v.Cast<double>() != +1.0)
        die("Cast of integer one to double not one");

    // Range-checked casts.
    v = VtValue(std::numeric_limits<short>::max());
    v.Cast<short>();
    TF_AXIOM(v.IsHolding<short>() &&
             v.UncheckedGet<short>() == std::numeric_limits<short>::max());
    // Out-of-range should fail.
    v = VtValue(std::numeric_limits<int>::max());
    v.Cast<short>();
    TF_AXIOM(v.IsEmpty());

    v = VtValue(std::numeric_limits<unsigned int>::max());
    v.Cast<int>();
    TF_AXIOM(v.IsEmpty());

    // expected to succeed
    _TestVecCast<GfVec2h>(GfVec2i(1, 2));
    _TestVecCast<GfVec2f>(GfVec2i(1, 2));
    _TestVecCast<GfVec2d>(GfVec2i(1, 2));
    _TestVecCast<GfVec2f>(GfVec2h(1, 2));
    _TestVecCast<GfVec2d>(GfVec2h(1, 2));
    _TestVecCast<GfVec2d>(GfVec2f(1, 2));
    _TestVecCast<GfVec2h>(GfVec2f(1, 2));
    _TestVecCast<GfVec2h>(GfVec2d(1, 2));
    _TestVecCast<GfVec2f>(GfVec2d(1, 2));

    _TestVecCast<GfVec3h>(GfVec3i(1, 2, 3));
    _TestVecCast<GfVec3f>(GfVec3i(1, 2, 3));
    _TestVecCast<GfVec3d>(GfVec3i(1, 2, 3));
    _TestVecCast<GfVec3f>(GfVec3h(1, 2, 3));
    _TestVecCast<GfVec3d>(GfVec3h(1, 2, 3));
    _TestVecCast<GfVec3d>(GfVec3f(1, 2, 3));
    _TestVecCast<GfVec3h>(GfVec3f(1, 2, 3));
    _TestVecCast<GfVec3h>(GfVec3d(1, 2, 3));
    _TestVecCast<GfVec3f>(GfVec3d(1, 2, 3));

    _TestVecCast<GfVec4h>(GfVec4i(1, 2, 3, 4));
    _TestVecCast<GfVec4f>(GfVec4i(1, 2, 3, 4));
    _TestVecCast<GfVec4d>(GfVec4i(1, 2, 3, 4));
    _TestVecCast<GfVec4f>(GfVec4h(1, 2, 3, 4));
    _TestVecCast<GfVec4d>(GfVec4h(1, 2, 3, 4));
    _TestVecCast<GfVec4d>(GfVec4f(1, 2, 3, 4));
    _TestVecCast<GfVec4h>(GfVec4f(1, 2, 3, 4));
    _TestVecCast<GfVec4h>(GfVec4d(1, 2, 3, 4));
    _TestVecCast<GfVec4f>(GfVec4d(1, 2, 3, 4));

    _FailVecCast<GfVec4i>(GfVec4h(1, 2, 3, 4));
    _FailVecCast<GfVec4i>(GfVec4f(1, 2, 3, 4));
    _FailVecCast<GfVec4i>(GfVec4d(1, 2, 3, 4));

    _FailVecCast<GfVec3i>(GfVec3h(1, 2, 3));
    _FailVecCast<GfVec3i>(GfVec3f(1, 2, 3));
    _FailVecCast<GfVec3i>(GfVec3d(1, 2, 3));

    _FailVecCast<GfVec2i>(GfVec2h(1, 2));
    _FailVecCast<GfVec2i>(GfVec2f(1, 2));
    _FailVecCast<GfVec2i>(GfVec2d(1, 2));

    // Equality special cases.

    v = VtValue();
    v2 = VtValue();

    if (!(v == v2))
        die("comparison with empty");

    v = VtValue(1.234);

    if (v == v2)
        die("comparison with empty");

    v2 = VtValue("hello");

    if (v == v2)
        die("comparison of mismatched types");

    v = VtValue(1234.0);
    v2 = VtValue(1234);
    if (v == v2)
        die("comparison of mismatched stack-held types");

    // Coverage

    v = VtValue();
    if (v.IsArrayValued())
        die("IsArrayValued for empty value");

    v = VtValue(1.234);
    if (v.IsArrayValued())
        die("scalar value reports it is shaped");

    v = VtValue(VtDoubleArray());
    if (!v.IsArrayValued())
        die("array value reports it is not an array");


    // Streaming...
    VtDictionary d;
    d["foo"] = 1.234;
    d["bar"] = "baz";

    vector<VtValue> vals;
    vals.push_back(VtValue(1.234));
    vals.push_back(VtValue("hello world"));

    std::ostringstream stream;
    stream << VtValue(d);
    if (stream.str().empty())
        die("couldn't stream value holding dictionary.");

    std::ostringstream stream2;
    stream2 << VtValue(vals);
    if (stream2.str().empty())
        die("couldn't stream value holding vector of values.");


    // Default stuff...
    TF_AXIOM(VtDictionaryGet<double>(d, "foo", VtDefault = 0) == 1.234);
    TF_AXIOM(VtDictionaryGet<double>(d, "noKey", VtDefault = 3.14) == 3.14);
    TF_AXIOM(VtDictionaryGet<string>(d, "bar", VtDefault = "hello") == "baz");
    TF_AXIOM(VtDictionaryGet<string>(d, "noKey", VtDefault = "bye") == "bye");


    // Casting a VtValue holding a TfToken to a string.
    {
        TfToken token("token");
        VtValue val(token);
        TF_AXIOM(val.IsHolding<TfToken>());
        val.Cast<string>();
        TF_AXIOM(val.IsHolding<string>());
        TF_AXIOM(val.Get<string>() == "token");
    }

    // Assignment and equality with string literals.
    {
        VtValue val;
        val = "hello";
        TF_AXIOM(val.IsHolding<string>());
        TF_AXIOM(val.Get<string>() == "hello");
        TF_AXIOM(val == "hello");
        TF_AXIOM("hello" == val);
    }

    // Equality
    {
        double d = 1.234, e = 2.71828;
        VtValue v(d);
        TF_AXIOM(v == d);
        TF_AXIOM(d == v);
        TF_AXIOM(v != e);
        TF_AXIOM(e != v);
    }

    // IsHolding<VtValue>
    {
        VtValue v(1.234);
        TF_AXIOM(v.IsHolding<double>());
        TF_AXIOM(v.IsHolding<VtValue>());
    }

    // Shapeliness and other stuff with non-stack-held arrays.
    {
        VtVec2iArray a(2), b(3);
        VtValue v(a);
        VtValue vclone(v);
        TF_AXIOM(v.Get<VtVec2iArray>().size() == 2);
        v = b;
        TF_AXIOM(v.Get<VtVec2iArray>().size() == 3);
        TF_AXIOM(v.IsArrayValued());
        TF_AXIOM(v.GetElementTypeid() == typeid(GfVec2i));
        TF_AXIOM(vclone.Get<VtVec2iArray>().size() == 2);
    }

    // Precision-casting of VtArrays
    {
        // only testing float <-> double... compound Vec types should
        // be the same
        VtFloatArray  fa(3), fRoundTripped;
        VtDoubleArray  da;

        fa[0] = 1.23456567;
        fa[1] = 4.63256635;
        fa[2] = 123443634.432;

        VtValue  v(fa);
        v.Cast<VtDoubleArray>();
        TF_AXIOM(v.IsHolding<VtDoubleArray>());
        da = v.UncheckedGet<VtDoubleArray>();

        VtValue vv(da);
        vv.Cast<VtFloatArray>();
        TF_AXIOM(vv.IsHolding<VtFloatArray>());
        fRoundTripped = vv.UncheckedGet<VtFloatArray>();
        // verify they compare euqal, but are physically two different arrays
        TF_AXIOM(fRoundTripped == fa);
        TF_AXIOM(!fRoundTripped.IsIdentical(fa));
    }

    // Test swapping VtValues holding dictionaries.
    {
        VtValue a, b;
        VtDictionary d1, d2;

        d1["foo"] = "bar";
        d2["bar"] = "foo";

        a = d1;
        b = d2;

        a.Swap(b);

        TF_AXIOM(a.Get<VtDictionary>().count("bar"));
        TF_AXIOM(b.Get<VtDictionary>().count("foo"));
    }

    // Test creating VtValues by taking contents of objects, and destructively
    // removing contents from objects.
    {
        string s("hello world!");
        VtValue v = VtValue::Take(s);
        TF_AXIOM(s.empty());
        TF_AXIOM(v.IsHolding<string>());
        TF_AXIOM(v.UncheckedGet<string>() == "hello world!");
        v.Swap(s);
        TF_AXIOM(v.IsHolding<string>());
        TF_AXIOM(v.UncheckedGet<string>().empty());
        TF_AXIOM(s == "hello world!");
        
        v.Swap(s);
        TF_AXIOM(v.IsHolding<string>() &&
                 v.UncheckedGet<string>() == "hello world!");
        string t = v.Remove<string>();
        TF_AXIOM(t == "hello world!");
        TF_AXIOM(v.IsEmpty());

        v.Swap(t);
        TF_AXIOM(t.empty());
        TF_AXIOM(v.IsHolding<string>() &&
                 v.UncheckedGet<string>() == "hello world!");

        t = v.UncheckedRemove<string>();
        TF_AXIOM(t == "hello world!");
        TF_AXIOM(v.IsEmpty());
    }

    // Test calling Get with incorrect type.  Should issue an error and produce
    // some "default" value.

    {
        VtValue empty;
        TfErrorMark m;
        TF_AXIOM(empty.Get<bool>() == false);
        TF_AXIOM(!m.IsClean());
    }

    {
        VtValue d(1.234);
        TfErrorMark m;
        TF_AXIOM(d.Get<double>() == 1.234);
        TF_AXIOM(m.IsClean());

        m.SetMark();
        TF_AXIOM(d.Get<int>() == 0);
        TF_AXIOM(!m.IsClean());
        
        m.SetMark();
        TF_AXIOM(d.Get<string>() == string());
        TF_AXIOM(!m.IsClean());
    }

}

struct _Unhashable {};
bool operator==(_Unhashable, _Unhashable) { return true; }

static void
testValueHash()
{
    static_assert(VtIsHashable<int>(), "");
    static_assert(!VtIsHashable<_Unhashable>(), "");

    VtValue vHashable{1};
    VtValue vUnhashable{_Unhashable{}};

    // Test the dynamic hashability check.
    TF_AXIOM(vHashable.CanHash());
    TF_AXIOM(!vUnhashable.CanHash());

    {
        // Test that hashable types can hash without error.
        TfErrorMark m;
        vHashable.GetHash();
        TF_AXIOM(m.IsClean());
    }

    {
        // Test that unhashable types post an error when attempting to hash.
        TfErrorMark m;
        vUnhashable.GetHash();
        TF_AXIOM(!m.IsClean());
        m.Clear();
    }
}

int main(int argc, char *argv[])
{
    testArray();
    testArrayOperators();

    testDictionary();
    testDictionaryKeyPathAPI();
    testDictionaryOverRecursive();
    testDictionaryIterators();
    testDictionaryInitializerList();

    testValue();
    testValueHash();

    printf("Test SUCCEEDED\n");

    return 0;
}
