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
///
/// \file testenv/testJsConverter.cpp

#include "pxr/base/js/converter.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/staticData.h"

#include <boost/any.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

using std::string;

namespace {

// Generic types and functions.
typedef boost::any _Any;
typedef std::vector<_Any> _AnyVector;
typedef std::map<string, _Any> _Dictionary;
inline const std::type_info* GetType(const _Any& any)
{
    return &any.type();
}
bool IsEmpty(const _Any& any)
{
    return any.empty();
}
template <class T>
bool IsHolding(const _Any& any)
{
    return boost::any_cast<T>(&any) != 0;
}
template <class T>
T Get(const _Any& any)
{
    return boost::any_cast<const T&>(any);
}

// This is a simplified version of TfIndenter.
struct Indenter
{
    Indenter() { Start(); }
    ~Indenter() { Stop(); }
    static void Start() { ++_indent; }
    static void Stop() { --_indent; }
    static std::ostream& Out() {
        string s(_indent * 2, ' ');
        for (size_t i = 0; i < s.size(); i += 4)
            s[i] = '|';
        return std::cout << s;
    }
private:
    static int _indent;
};
int Indenter::_indent = 0;

struct IndenterScope {
    IndenterScope(Indenter& indenter) : indenter(indenter) { indenter.Start(); }
    ~IndenterScope() { indenter.Stop(); }
    Indenter& indenter;
};

} // anonymouse namespace

TfStaticData<Indenter> indenter;
#define indent indenter->Out()

template <class T>
static void
_CheckArrayOf(
    const JsValue& value)
{
    IndenterScope scope(*indenter);

    TF_AXIOM(value.IsArrayOf<T>());

    const std::vector<T> array = value.GetArrayOf<T>();
    const JsArray& expArray = value.GetJsArray();

    indent << "array.size = " << array.size()
        << ", expArray.size = " << expArray.size() << std::endl;

    for (size_t i = 0; i < array.size(); ++i) {
        TF_AXIOM(array[i] == expArray[i].Get<T>());
    }    
}

static void
_CheckArray(
    const _AnyVector& array,
    const JsArray& expArray)
{
    IndenterScope scope(*indenter);

    indent << "array.size = " << array.size()
        << ", expArray.size = " << expArray.size() << std::endl;
    TF_AXIOM(array.size() == expArray.size());

    for (size_t i = 0; i < array.size(); ++i) {
        IndenterScope scope(*indenter);

        const std::type_info* ti = GetType(array[i]);
        indent << "array element " << i << " typename is "
            << (ti ? ArchGetDemangled(*ti).c_str() : "nil") << std::endl;

        IndenterScope scope2(*indenter);

        switch (expArray[i].GetType()) {
        case JsValue::ObjectType: {
            indent << "checking object conversion" << std::endl;
            TF_AXIOM(IsHolding<_Dictionary>(array[i]));
            break;
        }
        case JsValue::ArrayType: {
            indent << "checking array conversion" << std::endl;
            TF_AXIOM(IsHolding<_AnyVector>(array[i]));
            break;
        }
        case JsValue::BoolType: {
            indent << "checking bool conversion" << std::endl;
            TF_AXIOM(IsHolding<bool>(array[i]));
            TF_AXIOM(Get<bool>(array[i]) == expArray[i].GetBool());
            break;
        }
        case JsValue::StringType: {
            indent << "checking string conversion" << std::endl;
            TF_AXIOM(IsHolding<string>(array[i]));
            TF_AXIOM(Get<string>(array[i]) == expArray[i].GetString());
            break;
        }
        case JsValue::RealType: {
            indent << "checking real conversion" << std::endl;
            TF_AXIOM(IsHolding<double>(array[i]));
            TF_AXIOM(Get<double>(array[i]) == expArray[i].GetReal());
            break;
        }
        case JsValue::IntType: {
            indent << "checking int conversion" << std::endl;
            TF_AXIOM((IsHolding<int64_t>(array[i])&& 
                    Get<int64_t>(array[i]) == expArray[i].GetInt()) || 
                   (IsHolding<uint64_t>(array[i])&& 
                    Get<uint64_t>(array[i]) == static_cast<uint64_t>(expArray[i].GetInt())));
            break;
        }
        case JsValue::NullType: {
            indent << "checking null conversion" << std::endl;
            TF_AXIOM(IsEmpty(array[i]));
            break;
        }
        default: TF_FATAL_ERROR("Unknown type in test array");
        }
    }
}

int main(int argc, char const *argv[])
{
    std::cout << "opening values.json" << std::endl;
    std::ifstream ifs("values.json");
    if (!ifs) {
        TF_CODING_ERROR("Failed to open 'values.json' for reading");
        return 1;
    }

    // Set up expected values.
    std::cout << "parsing input stream" << std::endl;
    const JsValue value = JsParseStream(ifs);
    TF_AXIOM(value);
    TF_AXIOM(value.IsObject());

    std::cout << "unwrapping envelope" << std::endl;
    JsObject envelope = value.GetJsObject();
    TF_AXIOM(envelope["Object"].IsObject());
    JsObject object = envelope["Object"].GetJsObject();
    TF_AXIOM(!object.empty());

    // Convert the top-level value to another container type.
    std::cout << "converting container" << std::endl;
    const _Any result = JsConvertToContainerType<_Any, _Dictionary>(value);
    TF_AXIOM(!IsEmpty(result));
    TF_AXIOM(IsHolding<_Dictionary>(result));

    std::cout << "checking converted top-level object" << std::endl;
    const _Dictionary& dict = Get<_Dictionary>(result);
    _Dictionary::const_iterator i = dict.find("Object");
    TF_AXIOM(i != dict.end());
    TF_AXIOM(IsHolding<_Dictionary>(i->second));
    const _Dictionary& aObject = Get<_Dictionary>(i->second);

    std::cout << "checking converted values" << std::endl;

    for (const auto& p : aObject) {
        const std::type_info* ti = GetType(p.second);
        indent << "key " << p.first << " typeid is " <<
            (ti ? ArchGetDemangled(*ti) : "nil") << std::endl;

        IndenterScope scope(*indenter);

        if (p.first == "Array") {
            indent << "checking array conversion" << std::endl;
            TF_AXIOM(object[p.first].IsArray());
            TF_AXIOM(IsHolding<_AnyVector>(p.second));
            _CheckArray(Get<_AnyVector>(p.second), object[p.first].GetJsArray());
            _CheckArray(Get<_AnyVector>(p.second), 
                        object[p.first].Get<JsArray>());

            // This array has heterogeneous values, so IsArrayOf<T> should
            // always return false.
            TF_AXIOM(!object[p.first].IsArrayOf<JsObject>());
            TF_AXIOM(!object[p.first].IsArrayOf<JsArray>());
            TF_AXIOM(!object[p.first].IsArrayOf<string>());
            TF_AXIOM(!object[p.first].IsArrayOf<double>());
            TF_AXIOM(!object[p.first].IsArrayOf<int>());
            TF_AXIOM(!object[p.first].IsArrayOf<int64_t>());
            TF_AXIOM(!object[p.first].IsArrayOf<uint64_t>());

        } else if (p.first == "ArrayString") {
            indent << "checking string array conversion" << std::endl;
            TF_AXIOM(object[p.first].IsArray());
            TF_AXIOM(object[p.first].Is<JsArray>());
            TF_AXIOM(IsHolding<_AnyVector>(p.second));
            _CheckArray(Get<_AnyVector>(p.second), object[p.first].GetJsArray());
            _CheckArray(Get<_AnyVector>(p.second), 
                        object[p.first].Get<JsArray>());
            _CheckArrayOf<string>(object[p.first]);
        } else if (p.first == "ArrayInt64") {
            indent << "checking int64 array conversion" << std::endl;
            TF_AXIOM(object[p.first].IsArray());
            TF_AXIOM(object[p.first].Is<JsArray>());
            TF_AXIOM(IsHolding<_AnyVector>(p.second));
            _CheckArray(Get<_AnyVector>(p.second), object[p.first].GetJsArray());
            _CheckArray(Get<_AnyVector>(p.second), 
                        object[p.first].Get<JsArray>());
            _CheckArrayOf<int64_t>(object[p.first]);
        } else if (p.first == "ArrayUInt64") {
            indent << "checking uint array conversion" << std::endl;
            TF_AXIOM(object[p.first].IsArray());
            TF_AXIOM(object[p.first].Is<JsArray>());
            TF_AXIOM(IsHolding<_AnyVector>(p.second));
            _CheckArray(Get<_AnyVector>(p.second), object[p.first].GetJsArray());
            _CheckArray(Get<_AnyVector>(p.second), 
                        object[p.first].Get<JsArray>());
            _CheckArrayOf<uint64_t>(object[p.first]);
        } else if (p.first == "ArrayReal") {
            indent << "checking real array conversion" << std::endl;
            TF_AXIOM(object[p.first].IsArray());
            TF_AXIOM(object[p.first].Is<JsArray>());
            TF_AXIOM(IsHolding<_AnyVector>(p.second));
            _CheckArray(Get<_AnyVector>(p.second), object[p.first].GetJsArray());
            _CheckArray(Get<_AnyVector>(p.second), 
                        object[p.first].Get<JsArray>());
            _CheckArrayOf<double>(object[p.first]);
        } else if (p.first == "ArrayBool") {
            indent << "checking bool array conversion" << std::endl;
            TF_AXIOM(object[p.first].IsArray());
            TF_AXIOM(object[p.first].Is<JsArray>());
            TF_AXIOM(IsHolding<_AnyVector>(p.second));
            _CheckArray(Get<_AnyVector>(p.second), object[p.first].GetJsArray());
            _CheckArray(Get<_AnyVector>(p.second), 
                        object[p.first].Get<JsArray>());
            _CheckArrayOf<bool>(object[p.first]);
        } else if (p.first == "String") {
            indent << "checking string conversion" << std::endl;
            TF_AXIOM(object[p.first].IsString());
            TF_AXIOM(object[p.first].Is<string>());
            TF_AXIOM(IsHolding<string>(p.second));
            TF_AXIOM(Get<string>(p.second) == object[p.first].GetString());
            TF_AXIOM(Get<string>(p.second) == object[p.first].Get<string>());
        } else if (p.first == "Int64") {
            indent << "checking int conversion" << std::endl;
            TF_AXIOM(object[p.first].IsInt());
            TF_AXIOM(object[p.first].Is<int64_t>());
            TF_AXIOM(IsHolding<int64_t>(p.second));
            TF_AXIOM(Get<int64_t>(p.second) == object[p.first].GetInt());
            TF_AXIOM(Get<int64_t>(p.second) == object[p.first].Get<int64_t>());
        } else if (p.first == "UInt64") {
            indent << "checking uint conversion" << std::endl;
            TF_AXIOM(object[p.first].IsInt());
            TF_AXIOM(object[p.first].Is<uint64_t>());
            TF_AXIOM(IsHolding<uint64_t>(p.second));
            TF_AXIOM(Get<uint64_t>(p.second) == static_cast<uint64_t>(object[p.first].GetInt()));
            TF_AXIOM(Get<uint64_t>(p.second) == object[p.first].Get<uint64_t>());
        } else if (p.first == "Real") {
            indent << "checking real conversion" << std::endl;
            TF_AXIOM(object[p.first].IsReal());
            TF_AXIOM(object[p.first].Is<double>());
            TF_AXIOM(IsHolding<double>(p.second));
            TF_AXIOM(Get<double>(p.second) == object[p.first].GetReal());
            TF_AXIOM(Get<double>(p.second) == object[p.first].Get<double>());
        } else if (p.first == "BoolTrue") {
            indent << "checking bool(true) conversion" << std::endl;
            TF_AXIOM(object[p.first].IsBool());
            TF_AXIOM(object[p.first].Is<bool>());
            TF_AXIOM(IsHolding<bool>(p.second));
            TF_AXIOM(Get<bool>(p.second));
            TF_AXIOM(object[p.first].Get<bool>());
        } else if (p.first == "BoolFalse") {
            indent << "checking bool(false) conversion" << std::endl;
            TF_AXIOM(object[p.first].IsBool());
            TF_AXIOM(object[p.first].Is<bool>());
            TF_AXIOM(IsHolding<bool>(p.second));
            TF_AXIOM(!Get<bool>(p.second));
            TF_AXIOM(!object[p.first].Get<bool>());
        } else if (p.first == "Null") {
            indent << "checking null conversion" << std::endl;
            TF_AXIOM(object[p.first].IsNull());
            TF_AXIOM(IsEmpty(p.second));
        }
    }

    std::cout << "PASSED" << std::endl;
    return 0;
}
