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
#include "pxr/base/tf/enum.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/arch/demangle.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include "pxr/base/tf/hashmap.h"

#include <tbb/spin_mutex.h>

#include <iostream>
#include <set>

using std::string;
using std::vector;
using std::type_info;

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TfEnum>();
}

// Convenience typedefs for value/name tables.
typedef TfHashMap<TfEnum, string, TfHash> _EnumToNameTableType;
typedef TfHashMap<string, TfEnum, TfHash> _NameToEnumTableType;
typedef TfHashMap<string, vector<string>, TfHash> _TypeNameToNameVectorTableType;
typedef TfHashMap<string, const type_info *, TfHash> _TypeNameToTypeTableType;

class Tf_EnumRegistry : boost::noncopyable {
private:
    static Tf_EnumRegistry& _GetInstance() {
        return TfSingleton<Tf_EnumRegistry>::GetInstance();
    }

    Tf_EnumRegistry() {
        TfSingleton<Tf_EnumRegistry>::SetInstanceConstructed(*this);
        TfRegistryManager::GetInstance().SubscribeTo<TfEnum>();
    }
    
    ~Tf_EnumRegistry() {
        TfRegistryManager::GetInstance().UnsubscribeFrom<TfEnum>();
    }

    void _Remove(TfEnum val) {
        tbb::spin_mutex::scoped_lock lock(_tableLock);

        _typeNameToType.erase(ArchGetDemangled(val.GetType()));

        vector<string>& v = _typeNameToNameVector[val.GetType().name()];
        vector<string> original = v;
        string name = _enumToName[val];

        v.clear();
        for (size_t i = 0; i < original.size(); i++)
            if (original[i] != name)
                v.push_back(original[i]);
        
        _fullNameToEnum.erase(_enumToFullName[val]);
        _enumToFullName.erase(val);
        _enumToName.erase(val);
        _enumToDisplayName.erase(val);
    }   

    tbb::spin_mutex            _tableLock;
    _EnumToNameTableType       _enumToName;
    _EnumToNameTableType       _enumToFullName;
    _EnumToNameTableType       _enumToDisplayName;
    _NameToEnumTableType       _fullNameToEnum;
    _TypeNameToNameVectorTableType _typeNameToNameVector;
    _TypeNameToTypeTableType  _typeNameToType;
    
    friend class TfEnum;
    friend class TfSingleton<Tf_EnumRegistry>;
}; 

TF_INSTANTIATE_SINGLETON(Tf_EnumRegistry);

void
TfEnum::_AddName(TfEnum val, const string &valName, const string &displayName)
{
    TfAutoMallocTag2 tag("Tf", "TfEnum::_AddName");
    string typeName = ArchGetDemangled(val.GetType());

    /*
     * In case valName looks like "stuff::VALUE", strip off the leading
     * prefix.
     */
    size_t i = valName.rfind(':');
    string shortName = (i == string::npos) ? valName : valName.substr(i+1);

    if (shortName.empty())
        return;

    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    string fullName = typeName + "::" + shortName;

    r._enumToName[val] = shortName;
    r._enumToFullName[val] = fullName;
    r._enumToDisplayName[val] = displayName.empty() ? shortName : displayName;
    r._fullNameToEnum[fullName] = val;
    r._typeNameToNameVector[val.GetType().name()].push_back(shortName);
    r._typeNameToType[typeName] = &val.GetType();

    boost::function<void ()> fn =
        boost::bind(&Tf_EnumRegistry::_Remove, &r, val);
    TfRegistryManager::GetInstance().AddFunctionForUnload(fn);
}

string
TfEnum::GetName(TfEnum val)
{
    if (TfSafeTypeCompare(val.GetType(), typeid(int)))
        return TfIntToString(val.GetValueAsInt());

    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    _EnumToNameTableType::iterator i = r._enumToName.find(val);
    return (i != r._enumToName.end() ? i->second : "");
}

string
TfEnum::GetFullName(TfEnum val)
{
    if (TfSafeTypeCompare(val.GetType(), typeid(int)))
        return TfStringPrintf("int::%d", val.GetValueAsInt());
    
    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    _EnumToNameTableType::iterator i = r._enumToFullName.find(val);
    return (i != r._enumToFullName.end() ? i->second : "");
}

string
TfEnum::GetDisplayName(TfEnum val)
{
    if (TfSafeTypeCompare(val.GetType(), typeid(int)))
        return TfIntToString(val.GetValueAsInt());

    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    _EnumToNameTableType::iterator i = r._enumToDisplayName.find(val);
    return (i != r._enumToDisplayName.end() ? i->second : "");
}

vector<string>
TfEnum::GetAllNames(const type_info &ti)
{
    if (TfSafeTypeCompare(ti, typeid(int)))
        return vector<string>();
    
    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    _TypeNameToNameVectorTableType::iterator i = r._typeNameToNameVector.find(ti.name());
    return (i != r._typeNameToNameVector.end() ? i->second : vector<string>());
}

const type_info *
TfEnum::GetTypeFromName(const string& typeName)
{
    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    _TypeNameToTypeTableType::iterator i = r._typeNameToType.find(typeName);
    if (i == r._typeNameToType.end()) return NULL;
    return i->second;
}

TfEnum
TfEnum::GetValueFromName(const type_info& ti, const string &name, bool *foundIt)
{
    bool found = false;
    TfEnum value = GetValueFromFullName(
        ArchGetDemangled(ti) + "::" + name, &found);

    // Make sure that the found enum is the correct type.
    found = found && TfSafeTypeCompare(*(value._typeInfo), ti);
    if (foundIt)
        *foundIt = found;
    return found ? value : TfEnum(-1);
}

TfEnum
TfEnum::GetValueFromFullName(const string &fullname, bool *foundIt)
{
    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    _NameToEnumTableType::iterator i = r._fullNameToEnum.find(fullname);
    if (i != r._fullNameToEnum.end()) {
        if (foundIt)
            *foundIt = true;
        return TfEnum(i->second);
    }
    else if (fullname.find("int::") == 0) {
        if (foundIt)
            *foundIt = true;
        return TfEnum(atoi(fullname.c_str() + 5));
    }
    else {
        if (foundIt)
            *foundIt = false;
        return TfEnum(-1);
    }
}

void
TfEnum::_FatalGetValueError(std::type_info const& typeInfo) const
{
    TF_FATAL_ERROR("Attempted to get a '%s' from a TfEnum holding "
                   "a '%s'.",
                   ArchGetDemangled(typeInfo).c_str(),
                   _typeInfo->name());
}

bool
TfEnum::IsKnownEnumType(const std::string& typeName)
{
    Tf_EnumRegistry& r = Tf_EnumRegistry::_GetInstance();
    tbb::spin_mutex::scoped_lock lock(r._tableLock);

    _TypeNameToTypeTableType::iterator i = r._typeNameToType.find(typeName);
    if (i == r._typeNameToType.end()) return false;
    return true;
}

std::ostream &
operator<<(std::ostream& out, const TfEnum& e)
{
    return out << TfEnum::GetFullName(e);
}
