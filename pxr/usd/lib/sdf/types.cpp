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
// Types.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include <boost/lexical_cast.hpp>

using std::map;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdfValueRoleNames, SDF_VALUE_ROLE_NAME_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    // Enums.
    TfType::Define<SdfPermission>();
    TfType::Define<SdfSpecifier>();
    TfType::Define<SdfVariability>();
    TfType::Define<SdfSpecType>();

    // Other.
    TfType::Define<SdfTimeSampleMap>()
        .Alias(TfType::GetRoot(), "SdfTimeSampleMap")
        ;
    TfType::Define<SdfVariantSelectionMap>();
    TfType::Define<SdfRelocatesMap>().
        Alias(TfType::GetRoot(), "SdfRelocatesMap").
        Alias(TfType::GetRoot(), "map<SdfPath, SdfPath>");
        ;
    TfType::Define<SdfUnregisteredValue>();
    TfType::Define<SdfValueBlock>(); 
}

template <typename T>
static VtValue
_GetTfEnumForEnumValue(const VtValue &value)
{
    return VtValue(TfEnum(value.Get<T>()));
}

template <class T>
static void _RegisterEnumWithVtValue()
{
    VtValue::RegisterCast<T, TfEnum>(_GetTfEnumForEnumValue<T>);
    VtValue::RegisterSimpleBidirectionalCast<int, T>();
}

TF_REGISTRY_FUNCTION(VtValue)
{
    _RegisterEnumWithVtValue<SdfPermission>();
    _RegisterEnumWithVtValue<SdfSpecifier>();
    _RegisterEnumWithVtValue<SdfVariability>();
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    // SdfSpecType
    TF_ADD_ENUM_NAME(SdfSpecTypeUnknown);
    TF_ADD_ENUM_NAME(SdfSpecTypeAttribute);
    TF_ADD_ENUM_NAME(SdfSpecTypeConnection);
    TF_ADD_ENUM_NAME(SdfSpecTypeExpression);
    TF_ADD_ENUM_NAME(SdfSpecTypeMapper);
    TF_ADD_ENUM_NAME(SdfSpecTypeMapperArg);
    TF_ADD_ENUM_NAME(SdfSpecTypePrim);
    TF_ADD_ENUM_NAME(SdfSpecTypePseudoRoot);
    TF_ADD_ENUM_NAME(SdfSpecTypeRelationship);
    TF_ADD_ENUM_NAME(SdfSpecTypeRelationshipTarget);
    TF_ADD_ENUM_NAME(SdfSpecTypeVariant);
    TF_ADD_ENUM_NAME(SdfSpecTypeVariantSet);

    // SdfSpecifier
    TF_ADD_ENUM_NAME(SdfSpecifierDef, "Def");
    TF_ADD_ENUM_NAME(SdfSpecifierOver, "Over");
    TF_ADD_ENUM_NAME(SdfSpecifierClass, "Class");

    // SdfPermission
    TF_ADD_ENUM_NAME(SdfPermissionPublic, "Public");
    TF_ADD_ENUM_NAME(SdfPermissionPrivate, "Private");

    // SdfVariability
    TF_ADD_ENUM_NAME(SdfVariabilityVarying, "Varying");
    TF_ADD_ENUM_NAME(SdfVariabilityUniform, "Uniform");
    TF_ADD_ENUM_NAME(SdfVariabilityConfig, "Config");
}

// Register all units with the TfEnum registry.
struct _UnitsInfo {
    map<string, map<int, double> *> _UnitsMap;
    map<string, TfEnum> _DefaultUnitsMap;
    map<string, TfEnum> _UnitCategoryToDefaultUnitMap;
    map<string, string> _UnitTypeNameToUnitCategoryMap;
    TfEnum _UnitIndicesTable[_SDF_UNIT_NUM_TYPES][_SDF_UNIT_MAX_UNITS];
    string _UnitNameTable[_SDF_UNIT_NUM_TYPES][_SDF_UNIT_MAX_UNITS];
    map<string, TfEnum> _UnitNameToUnitMap;
    map<string, uint32_t> _UnitTypeIndicesTable;
};

static void _AddToUnitsMaps(_UnitsInfo &info,
                            const TfEnum &unit, 
                            const string &unitName,
                            double scale,
                            const string &category)
{
    const char *enumTypeName = unit.GetType().name();
    map<int, double> *scalesMap = info._UnitsMap[enumTypeName];

    if (!scalesMap) {
        scalesMap = info._UnitsMap[enumTypeName] = new map<int, double>;
    }
    (*scalesMap)[unit.GetValueAsInt()] = scale;
    if (scale == 1.0) {
        info._DefaultUnitsMap[enumTypeName] = unit;
        info._UnitCategoryToDefaultUnitMap[category] = unit;
        info._UnitTypeNameToUnitCategoryMap[unit.GetType().name()] = category;
    }

    uint32_t typeIndex;
    map<string, uint32_t>::iterator i =
        info._UnitTypeIndicesTable.find(unit.GetType().name());
    if (i == info._UnitTypeIndicesTable.end()) {
        typeIndex = (uint32_t)info._UnitTypeIndicesTable.size();
        info._UnitTypeIndicesTable[unit.GetType().name()] = typeIndex;
    }
    else {
        typeIndex = i->second;
    }
    info._UnitIndicesTable[typeIndex][unit.GetValueAsInt()] = unit;
    info._UnitNameTable[typeIndex][unit.GetValueAsInt()] = unitName;
    info._UnitNameToUnitMap[unitName] = unit;
}

#define _ADD_UNIT_ENUM(r, category, elem)                               \
    TF_ADD_ENUM_NAME(                                                   \
        BOOST_PP_CAT(Sdf ## category ## Unit, _SDF_UNIT_TAG(elem)),     \
        _SDF_UNIT_NAME(elem));

#define _REGISTRY_FUNCTION(r, unused, elem)                          \
TF_REGISTRY_FUNCTION_WITH_TAG(TfEnum, _SDF_UNITSLIST_CATEGORY(elem)) \
{                                                                    \
    BOOST_PP_SEQ_FOR_EACH(_ADD_UNIT_ENUM,                            \
                          _SDF_UNITSLIST_CATEGORY(elem),             \
                          _SDF_UNITSLIST_TUPLES(elem));              \
}

BOOST_PP_LIST_FOR_EACH(_REGISTRY_FUNCTION, ~, _SDF_UNITS)

#define _ADD_UNIT_TO_MAPS(r, category, elem)                            \
    _AddToUnitsMaps(                                                    \
        *info,                                                          \
        BOOST_PP_CAT(Sdf ## category ## Unit, _SDF_UNIT_TAG(elem)),     \
        _SDF_UNIT_NAME(elem),                                           \
        _SDF_UNIT_SCALE(elem), #category);

#define _POPULATE_UNIT_MAPS(r, unused, elem)                          \
    BOOST_PP_SEQ_FOR_EACH(_ADD_UNIT_TO_MAPS,                          \
                          _SDF_UNITSLIST_CATEGORY(elem),              \
                          _SDF_UNITSLIST_TUPLES(elem))                \

static _UnitsInfo *_MakeUnitsMaps() {
    _UnitsInfo *info = new _UnitsInfo;
    BOOST_PP_LIST_FOR_EACH(_POPULATE_UNIT_MAPS, ~, _SDF_UNITS);
    return info;
}

static _UnitsInfo &_GetUnitsInfo() {
    static _UnitsInfo *unitsInfo = _MakeUnitsMaps();
    return *unitsInfo;
}

#undef _REGISTRY_FUNCTION
#undef _PROCESS_ENUMERANT

#define _REGISTRY_FUNCTION(r, unused, elem)                          \
TF_REGISTRY_FUNCTION_WITH_TAG(TfType, BOOST_PP_CAT(Type, _SDF_UNITSLIST_CATEGORY(elem))) \
{                                                                    \
    TfType::Define<_SDF_UNITSLIST_ENUM(elem)>();                     \
}                                                                    \
TF_REGISTRY_FUNCTION_WITH_TAG(VtValue, BOOST_PP_CAT(Value, _SDF_UNITSLIST_CATEGORY(elem))) \
{                                                                    \
    _RegisterEnumWithVtValue<_SDF_UNITSLIST_ENUM(elem)>();           \
}
BOOST_PP_LIST_FOR_EACH(_REGISTRY_FUNCTION, ~, _SDF_UNITS)
#undef _REGISTRY_FUNCTION

TfEnum
SdfDefaultUnit( const TfToken &typeName )
{
    return SdfSchema::GetInstance().FindType(typeName).GetDefaultUnit();
}

const TfEnum &
SdfDefaultUnit( const TfEnum &unit )
{
    static TfEnum empty;
    _UnitsInfo &info = _GetUnitsInfo();
    map<string, TfEnum>::const_iterator it =
        info._DefaultUnitsMap.find(unit.GetType().name());

    if ( it == info._DefaultUnitsMap.end() ) {
        TF_WARN("Unsupported unit '%s'.",
                ArchGetDemangled(unit.GetType()).c_str());
        return empty;
    }
    return it->second;
}

const string &
SdfUnitCategory( const TfEnum &unit )
{
    static string empty;
    _UnitsInfo &info = _GetUnitsInfo();
    map<string, string>::const_iterator it =
        info._UnitTypeNameToUnitCategoryMap.find(unit.GetType().name());

    if (it == info._UnitTypeNameToUnitCategoryMap.end()) {
        TF_WARN("Unsupported unit '%s'.",
                ArchGetDemangled(unit.GetType()).c_str());
        return empty;
    }
    return it->second;
}

std::pair<uint32_t, uint32_t>
Sdf_GetUnitIndices( const TfEnum &unit )
{
    _UnitsInfo &info = _GetUnitsInfo();
    return std::make_pair(info._UnitTypeIndicesTable[unit.GetType().name()],
                          unit.GetValueAsInt());
}

double SdfConvertUnit( const TfEnum &fromUnit, const TfEnum &toUnit )
{
    _UnitsInfo &info = _GetUnitsInfo();
    if (!toUnit.IsA(fromUnit.GetType()) ) {
        TF_WARN("Can not convert from '%s' to '%s'.",
                TfEnum::GetFullName(fromUnit).c_str(),
                TfEnum::GetFullName(toUnit).c_str());
        return 0.0;
    }
    map<string, map<int, double> *>::const_iterator it =
        info._UnitsMap.find(fromUnit.GetType().name());

    if ( it == info._UnitsMap.end() ) {
        TF_WARN("Unsupported unit '%s'.",
                ArchGetDemangled(fromUnit.GetType()).c_str());
        return 0.0;
    }
    return (*it->second)[fromUnit.GetValueAsInt()] /
        (*it->second)[toUnit.GetValueAsInt()];
}

const string &
SdfGetNameForUnit( const TfEnum &unit )
{
    static std::string empty;
    _UnitsInfo &info = _GetUnitsInfo();

    // first check if this is a known type
    map<string, uint32_t>::const_iterator it =
        info._UnitTypeIndicesTable.find(unit.GetType().name());
    if (it == info._UnitTypeIndicesTable.end()) {
        TF_WARN("Unsupported unit '%s'.",
                ArchGetDemangled(unit.GetType()).c_str());
        return empty;
    }

    // get indices
    std::pair<uint32_t, uint32_t> indices = Sdf_GetUnitIndices(unit);
    // look up menva name in our table
    return info._UnitNameTable[indices.first][indices.second];
}

const TfEnum &
SdfGetUnitFromName( const std::string &name )
{
    static TfEnum empty;
    _UnitsInfo &info = _GetUnitsInfo();
    map<string, TfEnum>::const_iterator it = info._UnitNameToUnitMap.find(name);

    if ( it == info._UnitNameToUnitMap.end() ) {
        TF_WARN("Unknown unit name '%s'.", name.c_str());
        return empty;
    }

    return it->second;
}

bool SdfBoolFromString( const std::string &str, bool *parseOk )
{
    if (parseOk)
        *parseOk = true;

    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (strcmp(s.c_str(), "false") == 0)
        return false;
    if (strcmp(s.c_str(), "true") == 0)
        return true;
    if (strcmp(s.c_str(), "no") == 0)
        return false;
    if (strcmp(s.c_str(), "yes") == 0)
        return true;

    if (strcmp(s.c_str(), "0") == 0)
        return false;
    if (strcmp(s.c_str(), "1") == 0)
        return true;

    if (parseOk)
        *parseOk = false;
    return true;
}

bool
SdfValueHasValidType(const VtValue& value)
{
    return static_cast<bool>(SdfSchema::GetInstance().FindType(value));
}

// Given sdf valueType name, produce TfType.
TfType
SdfGetTypeForValueTypeName(TfToken const &name)
{
    return SdfSchema::GetInstance().FindType(name).GetType();
}

// Given VtValue, produce corresponding valueType name
SdfValueTypeName
SdfGetValueTypeNameForValue(const VtValue& val)
{
    return SdfSchema::GetInstance().FindType(val);
}

TfToken
SdfGetRoleNameForValueTypeName(const TfToken &name)
{
    return SdfSchema::GetInstance().FindType(name).GetRole();
}

std::ostream& operator<<(std::ostream& out, const SdfSpecifier& spec)
{
    return out << TfEnum::GetDisplayName(TfEnum(spec)) << std::endl;
}

std::ostream & operator<<( std::ostream &out,
                           const SdfRelocatesMap &reloMap )
{
    TF_FOR_ALL(it, reloMap) {
        out << it->first << ": " << it->second << std::endl;
    }
    return out;
}

std::ostream & operator<<( std::ostream &out,
                           const SdfTimeSampleMap &sampleMap )
{
    TF_FOR_ALL(it, sampleMap) {
        out << it->first << ": " << it->second << std::endl;
    }
    return out;
}

std::ostream &
VtStreamOut(const SdfVariantSelectionMap &varSelMap, std::ostream &stream)
{
    return stream << varSelMap;
}

SdfUnregisteredValue::SdfUnregisteredValue()
{
}

SdfUnregisteredValue::SdfUnregisteredValue(const std::string &value) :
    _value(value)
{
}

SdfUnregisteredValue::SdfUnregisteredValue(const VtDictionary &value) :
    _value(value)
{
}

SdfUnregisteredValue::SdfUnregisteredValue(
    const SdfUnregisteredValueListOp& value) :
    _value(value)
{
}

bool SdfUnregisteredValue::operator==(const SdfUnregisteredValue &other) const
{
    return _value == other._value;
}

std::ostream &operator << (std::ostream &out, const SdfUnregisteredValue &value)
{
    return out << value.GetValue();
}

Sdf_ValueTypeNamesType::Sdf_ValueTypeNamesType()
{
    // Do nothing
}

Sdf_ValueTypeNamesType::~Sdf_ValueTypeNamesType()
{
    // Do nothing
}

const Sdf_ValueTypeNamesType*
Sdf_ValueTypeNamesType::_Init::New()
{
    return SdfSchema::GetInstance()._NewValueTypeNames();
}

TfToken
Sdf_ValueTypeNamesType::GetSerializationName(
    const SdfValueTypeName& typeName) const
{
    // Return the first registered alias, which is the new type name.
    const TfToken name = typeName.GetAliasesAsTokens().front();
    if (!name.IsEmpty()) {
        return name;
    }
        
    return typeName.GetAsToken();
}

TfToken
Sdf_ValueTypeNamesType::GetSerializationName(const VtValue& value) const
{
    return GetSerializationName(SdfSchema::GetInstance().FindType(value));
}

TfToken
Sdf_ValueTypeNamesType::GetSerializationName(const TfToken& name) const
{
    const SdfValueTypeName typeName = SdfSchema::GetInstance().FindType(name);
    return typeName ? GetSerializationName(typeName) : name;
}

TfStaticData<const Sdf_ValueTypeNamesType,
             Sdf_ValueTypeNamesType::_Init> SdfValueTypeNames;

std::ostream&
operator<<(std::ostream& ostr, SdfValueBlock const& block)
{ 
    return ostr << "None"; 
}

std::ostream &
operator<<(std::ostream &out, const SdfHumanReadableValue &hrval)
{
    return out << "<< " << hrval.GetText() << " >>";
}

size_t
hash_value(const SdfHumanReadableValue &hrval)
{
    return TfHash()(hrval.GetText());
}

PXR_NAMESPACE_CLOSE_SCOPE
