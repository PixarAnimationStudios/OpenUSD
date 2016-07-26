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
#include "pxr/usd/sdf/types.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

using std::map;
using std::string;
using std::vector;

TF_DEFINE_ENV_SETTING(SDF_WRITE_OLD_TYPENAMES, false,
                      "Write values using old type-name alias");

TF_DEFINE_ENV_SETTING(SDF_CONVERT_TO_NEW_TYPENAMES, false,
                      "Force all serialized type-names to the new style");

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
static map<string, map<int, double> *> *_UnitsMap;
static map<string, TfEnum> *_DefaultUnitsMap;
static map<string, TfEnum> *_UnitCategoryToDefaultUnitMap;
static map<string, string> *_UnitTypeNameToUnitCategoryMap;
static TfEnum (*_UnitIndicesTable)[_SDF_UNIT_MAX_UNITS];
static string (*_UnitNameTable)[_SDF_UNIT_MAX_UNITS];
static map<string, TfEnum> *_UnitNameToUnitMap;
static map<string, uint32_t> *_UnitTypeIndicesTable;

static void _AddToUnitsMaps(const TfEnum &unit, 
                            const string &unitName,
                            double scale,
                            const string &category)
{
    if (not _UnitsMap) {
        _UnitsMap = new map<string, map<int, double> *>;
        _DefaultUnitsMap = new map<string, TfEnum>;
        _UnitCategoryToDefaultUnitMap = new map<string, TfEnum>;
        _UnitTypeNameToUnitCategoryMap = new map<string, string>;
        _UnitIndicesTable = 
            (TfEnum (*)[_SDF_UNIT_MAX_UNITS])new TfEnum[_SDF_UNIT_NUM_TYPES *
                                                        _SDF_UNIT_MAX_UNITS];
        _UnitNameTable = 
            (std::string (*)[_SDF_UNIT_MAX_UNITS])new std::string[_SDF_UNIT_NUM_TYPES *
                                                                  _SDF_UNIT_MAX_UNITS];
        _UnitNameToUnitMap = new map<string, TfEnum>;
        _UnitTypeIndicesTable = new map<string, uint32_t>;
    }
    const char *enumTypeName = unit.GetType().name();
    map<int, double> *scalesMap = (*_UnitsMap)[enumTypeName];

    if (not scalesMap) {
        scalesMap = (*_UnitsMap)[enumTypeName] = new map<int, double>;
    }
    (*scalesMap)[unit.GetValueAsInt()] = scale;
    if (scale == 1.0) {
        (*_DefaultUnitsMap)[enumTypeName] = unit;
        (*_UnitCategoryToDefaultUnitMap)[category] = unit;
        (*_UnitTypeNameToUnitCategoryMap)[unit.GetType().name()] = category;
    }

    uint32_t typeIndex;
    map<string, uint32_t>::iterator i =
        _UnitTypeIndicesTable->find(unit.GetType().name());
    if (i == _UnitTypeIndicesTable->end()) {
        typeIndex = (uint32_t)_UnitTypeIndicesTable->size();
        (*_UnitTypeIndicesTable)[unit.GetType().name()] = typeIndex;
    }
    else {
        typeIndex = i->second;
    }
    _UnitIndicesTable[typeIndex][unit.GetValueAsInt()] = unit;
    _UnitNameTable[typeIndex][unit.GetValueAsInt()] = unitName;
    (*_UnitNameToUnitMap)[unitName] = unit;
}

#define _PROCESS_ENUMERANT(r, category, elem) {                         \
    TF_ADD_ENUM_NAME(                                                   \
        BOOST_PP_CAT(Sdf ## category ## Unit, _SDF_UNIT_TAG(elem)),     \
        _SDF_UNIT_NAME(elem));                                          \
    _AddToUnitsMaps(                                                    \
        BOOST_PP_CAT(Sdf ## category ## Unit, _SDF_UNIT_TAG(elem)),     \
        _SDF_UNIT_NAME(elem),                                           \
        _SDF_UNIT_SCALE(elem), #category);                              \
    }

#define _REGISTRY_FUNCTION(r, unused, elem)                          \
TF_REGISTRY_FUNCTION_WITH_TAG(TfEnum, _SDF_UNITSLIST_CATEGORY(elem)) \
{                                                                    \
    BOOST_PP_SEQ_FOR_EACH(_PROCESS_ENUMERANT,                        \
                          _SDF_UNITSLIST_CATEGORY(elem),             \
                          _SDF_UNITSLIST_TUPLES(elem))               \
}
BOOST_PP_LIST_FOR_EACH(_REGISTRY_FUNCTION, ~, _SDF_UNITS)
#undef _REGISTRY_FUNCTION
#undef _PROCESS_ENUMERANT

#define _REGISTRY_FUNCTION(r, unused, elem)                          \
TF_REGISTRY_FUNCTION_WITH_TAG(TfType, _SDF_UNITSLIST_CATEGORY(elem)) \
{                                                                    \
    TfType::Define<_SDF_UNITSLIST_ENUM(elem)>();                     \
}                                                                    \
TF_REGISTRY_FUNCTION_WITH_TAG(VtValue, _SDF_UNITSLIST_CATEGORY(elem))   \
{                                                                       \
    _RegisterEnumWithVtValue<_SDF_UNITSLIST_ENUM(elem)>();              \
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

    if (not _DefaultUnitsMap) {
        // CODE_COVERAGE_OFF
        // This can only happen if someone calls this function from a
        // registry function, and that does not happen.
        TF_CODING_ERROR("_DefaultUnitsMap not initialized.");
        return empty;
        // CODE_COVERAGE_ON
    }
    map<string, TfEnum>::const_iterator it =
        _DefaultUnitsMap->find(unit.GetType().name());

    if ( it == _DefaultUnitsMap->end() ) {
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

    if (not _UnitTypeNameToUnitCategoryMap) {
        // CODE_COVERAGE_OFF
        // This can only happen if someone calls this function from a
        // registry function, and that does not happen.
        TF_CODING_ERROR("_UnitTypeNameToUnitCategoryMap not initialized.");
        return empty;
        // CODE_COVERAGE_ON
    }
    map<string, string>::const_iterator it =
        _UnitTypeNameToUnitCategoryMap->find(unit.GetType().name());

    if ( it == _UnitTypeNameToUnitCategoryMap->end() ) {
        TF_WARN("Unsupported unit '%s'.",
                ArchGetDemangled(unit.GetType()).c_str());
        return empty;
    }
    return it->second;
}

std::pair<uint32_t, uint32_t>
Sdf_GetUnitIndices( const TfEnum &unit )
{
    return std::make_pair((*_UnitTypeIndicesTable)[unit.GetType().name()],
                          unit.GetValueAsInt());
}

double SdfConvertUnit( const TfEnum &fromUnit, const TfEnum &toUnit )
{
    if (not _UnitsMap) {
        // CODE_COVERAGE_OFF
        // This can only happen if someone calls this function from a
        // registry function, and that does not happen.
        TF_CODING_ERROR("_UnitsMap not initialized.");
        return 0.0;
        // CODE_COVERAGE_ON
    }
    if ( not toUnit.IsA(fromUnit.GetType()) ) {
        TF_WARN("Can not convert from '%s' to '%s'.",
                TfEnum::GetFullName(fromUnit).c_str(),
                TfEnum::GetFullName(toUnit).c_str());
        return 0.0;
    }
    map<string, map<int, double> *>::const_iterator it =
        _UnitsMap->find(fromUnit.GetType().name());

    if ( it == _UnitsMap->end() ) {
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

    if (not _UnitTypeIndicesTable or
        not _UnitNameTable) {
        // CODE_COVERAGE_OFF
        // This can only happen if someone calls this function from a
        // registry function, and that does not happen.
        TF_CODING_ERROR("_UnitTypeIndicesTable or _UnitNameTable not initialized.");
        return empty;
        // CODE_COVERAGE_ON
    }

    // first check if this is a known type
    map<string, uint32_t>::const_iterator it =
        _UnitTypeIndicesTable->find(unit.GetType().name());
    if (it == _UnitTypeIndicesTable->end()) {
        TF_WARN("Unsupported unit '%s'.",
                ArchGetDemangled(unit.GetType()).c_str());
        return empty;
    }

    // get indices
    std::pair<uint32_t, uint32_t> indices = Sdf_GetUnitIndices(unit);
    // look up menva name in our table
    return _UnitNameTable[indices.first][indices.second];
}

const TfEnum &
SdfGetUnitFromName( const std::string &name )
{
    static TfEnum empty;

    if (not _UnitNameToUnitMap) {
        // CODE_COVERAGE_OFF
        // This can only happen if someone calls this function from a
        // registry function, and that does not happen.
        TF_CODING_ERROR("_UnitNameToUnitMap not initialized.");
        return empty;
        // CODE_COVERAGE_ON
    }
    map<string, TfEnum>::const_iterator it =
        _UnitNameToUnitMap->find(name);

    if ( it == _UnitNameToUnitMap->end() ) {
        TF_WARN("Unknown unit name '%s'.", name.c_str());
        return empty;
    }

    return it->second;
}

bool SdfBoolFromString( const std::string &str, bool *parseOk )
{
    if (parseOk)
        *parseOk = true;

    const char* s = str.c_str();
    if (strcasecmp(s, "false") == 0)
        return false;
    if (strcasecmp(s, "true") == 0)
        return true;
    if (strcasecmp(s, "no") == 0)
        return false;
    if (strcasecmp(s, "yes") == 0)
        return true;
    if (strcmp(s, "0") == 0)
        return false;
    if (strcmp(s, "1") == 0)
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
    if (TfGetEnvSetting(SDF_WRITE_OLD_TYPENAMES)) {
        // Return the last registered alias, which is the old type name.
        const TfToken name = typeName.GetAliasesAsTokens().back();
        if (not name.IsEmpty()) {
            return name;
        }
    }
    if (TfGetEnvSetting(SDF_CONVERT_TO_NEW_TYPENAMES)) {
        // Return the first registered alias, which is the new type name.
        const TfToken name = typeName.GetAliasesAsTokens().front();
        if (not name.IsEmpty()) {
            return name;
        }
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
