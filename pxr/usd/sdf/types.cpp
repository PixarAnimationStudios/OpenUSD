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

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"

#include <unordered_map>

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

    // SdfAuthoringError
    TF_ADD_ENUM_NAME(SdfAuthoringErrorUnrecognizedFields);
    TF_ADD_ENUM_NAME(SdfAuthoringErrorUnrecognizedSpecType);
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

// Gets the type/unit pair for a unit enum.
static std::pair<uint32_t, uint32_t>
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

// Return a human-readable description of the passed value for diagnostic
// messages.
static std::string
_GetDiagnosticStringForValue(VtValue const &value)
{
    std::string valueStr = TfStringify(value);
    // Truncate the value after 32 chars so we don't spam huge diagnostic
    // strings.
    if (valueStr.size() > 32) {
        valueStr.erase(valueStr.begin() + 32, valueStr.end());
        valueStr += "...";
    }
    return TfStringPrintf("<%s> '%s'", value.GetTypeName().c_str(),
                          valueStr.c_str());
}

// Return either empty string (if keyPath is empty) or a string indicating the
// path of keys in a nested dictionary, padded by one space on the left.
static std::string
_GetKeyPathText(std::vector<std::string> const &keyPath)
{
    return keyPath.empty() ? std::string() :
        TfStringPrintf(" under key '%s'", TfStringJoin(keyPath, ":").c_str());
}

// Add a human-readable error to errMsgs indicating that the passed value does
// not have a valid scene description datatype.
static void
_AddInvalidTypeError(
    char const *prefix,
    VtValue const &value,
    std::vector<std::string> *errMsgs,
    std::vector<std::string> const *keyPath)
{
    errMsgs->push_back(
        TfStringPrintf(
            "%s%s%s is not a valid scene description "
            "datatype", prefix,
            _GetDiagnosticStringForValue(value).c_str(),
            _GetKeyPathText(*keyPath).c_str()));
}

// Function pointer type for converting VtValue holding std::vector<VtValue> to
// VtArray of a specific type T.
using _ValueVectorToVtArrayFn =
    bool (*)(VtValue *, std::vector<std::string> *,
             std::vector<std::string> const *);

// This function template converts value (holding vector<VtValue>) to
// VtArray<T>.  We instantiate this for every type SDF_VALUE_TYPES and dispatch
// to the correct one based on the type held by the first element in the
// vector<VtValue>.
template <class T>
bool _ValueVectorToVtArray(VtValue *value,
                           std::vector<std::string> *errMsgs,
                           std::vector<std::string> const *keyPath)
{
    // Guarantees: value holds std::vector<VtValue>, and that vector is not
    // empty.  Also, the first element is a T.  The task here is to attempt to
    // populate a VtArray<T> with all the elements of the vector cast to T.  If
    // any fail, add a message to errMsgs indicating the failed element.
    auto const &valVec = value->UncheckedGet<std::vector<VtValue>>();
    auto begin = valVec.begin(), end = valVec.end();
    VtArray<T> result(distance(begin, end));
    
    bool allValid = true;
    for (T *e = result.data(); begin != end; ++begin) {
        VtValue cast = VtValue::Cast<T>(*begin);
        if (cast.IsEmpty()) {
            errMsgs->push_back(
                TfStringPrintf("failed to cast array element "
                               "%zu: %s%s to <%s>",
                               std::distance(valVec.begin(), begin),
                               _GetDiagnosticStringForValue(*begin).c_str(),
                               _GetKeyPathText(*keyPath).c_str(),
                               ArchGetDemangled<T>().c_str()));
            allValid = false;
        }
        else {
            cast.Swap(*e++);
        }
    }
    if (!allValid) {
        *value = VtValue();
        return false;
    }
    value->Swap(result);
    return true;
}

// Look up the function to convert vector<VtValue> to VtArray<T> for type and
// return it.  The caller guarantees that type is one of SDF_VALUE_TYPES.
static _ValueVectorToVtArrayFn
_GetTypedValueVectorToVtArrayFn(TfType const &type)
{
    using FnMap = std::unordered_map<
        TfType, _ValueVectorToVtArrayFn, TfHash>;
    static FnMap *valueVectorToVtArrayFnMap = []() {
        FnMap *ret = new FnMap(BOOST_PP_SEQ_SIZE(SDF_VALUE_TYPES));

// Add conversion functions for all SDF_VALUE_TYPES.
#define _ADD_FN(r, unused, elem)                                        \
        ret->emplace(TfType::Find<SDF_VALUE_CPP_TYPE(elem)>(),          \
                     _ValueVectorToVtArray<SDF_VALUE_CPP_TYPE(elem)>);

        BOOST_PP_SEQ_FOR_EACH(_ADD_FN, ~, SDF_VALUE_TYPES)
#undef _ADD_FN
        return ret;
    }();

    auto iter = valueVectorToVtArrayFnMap->find(type);
    if (TF_VERIFY(iter != valueVectorToVtArrayFnMap->end(),
                  "Value type '%s' returns true from "
                  "SdfValueHasValidType but does not appear in "
                  "SDF_VALUE_TYPES.", type.GetTypeName().c_str())) {
        return iter->second;
    }
    return nullptr;
}

// Try to convert 'value' holding vector<VtValue> to a VtArray, using the type
// of the first element in the vector.
static bool
_ValueVectorToAnyVtArray(VtValue *value, std::vector<std::string> *errMsgs,
                         std::vector<std::string> const *keyPath)
{
    std::vector<VtValue> const &valVec =
        value->UncheckedGet<std::vector<VtValue>>();
    // If this is an empty vector, we cannot sensibly choose any type for the
    // VtArray.  Error.
    if (valVec.empty()) {
        errMsgs->push_back(
            TfStringPrintf(
                "cannot infer type from empty vector/list%s -- use "
                "an empty typed array like VtIntArray/VtStringArray instead",
                _GetKeyPathText(*keyPath).c_str()));
        *value = VtValue();
        return false;
    }
    // Pull the type from the first element, and try to invoke the conversion
    // function to convert all elements.
    if (SdfValueHasValidType(valVec.front())) {
        return _GetTypedValueVectorToVtArrayFn(
            valVec.front().GetType())(value, errMsgs, keyPath);
    }
    else {
        _AddInvalidTypeError("first vector/list element ",
                             valVec.front(), errMsgs, keyPath);
        *value = VtValue();
        return false;
    }
    return true;
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED

using _PySeqToVtArrayFn =
    bool (*)(VtValue *, std::vector<std::string> *,
             std::vector<std::string> const *);

template <class T>
bool _PySeqToVtArray(VtValue *value,
                     std::vector<std::string> *errMsgs,
                     std::vector<std::string> const *keyPath)
{
    using ElemType = T;
    bool allValid = true;
    TfPyLock lock;
    TfPyObjWrapper obj = value->UncheckedGet<TfPyObjWrapper>();
    Py_ssize_t len = PySequence_Length(obj.ptr());
    VtArray<T> result(len);
    ElemType *elem = result.data();
    for (Py_ssize_t i = 0; i != len; ++i) {
        boost::python::handle<> h(PySequence_ITEM(obj.ptr(), i));
        if (!h) {
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }
            errMsgs->push_back(
                TfStringPrintf("failed to obtain element %s from sequence%s",
                               TfStringify(i).c_str(),
                               _GetKeyPathText(*keyPath).c_str()));
            allValid = false;
        }
        boost::python::extract<ElemType> e(h.get());
        if (!e.check()) {
            errMsgs->push_back(
                TfStringPrintf("failed to cast sequence element "
                               "%s: %s%s to <%s>",
                               TfStringify(i).c_str(),
                               _GetDiagnosticStringForValue(
                                   boost::python::extract<VtValue>(
                                       h.get())).c_str(),
                               _GetKeyPathText(*keyPath).c_str(),
                               ArchGetDemangled<ElemType>().c_str()));
            allValid = false;
        }
        else {
            *elem++ = e();
        }
    }
    if (!allValid) {
        *value = VtValue();
        return false;
    }
    value->Swap(result);
    return true;
}

static _PySeqToVtArrayFn
_GetTypedPySeqToVtArrayFn(TfType const &type)
{
    using FnMap = std::unordered_map<TfType, _PySeqToVtArrayFn, TfHash>;
    static FnMap *pySeqToVtArrayFnMap = []() {
        FnMap *ret = new FnMap(BOOST_PP_SEQ_SIZE(SDF_VALUE_TYPES));

// Add conversion functions for all SDF_VALUE_TYPES.
#define _ADD_FN(r, unused, elem)                                        \
        ret->emplace(TfType::Find<SDF_VALUE_CPP_TYPE(elem)>(),          \
                     _PySeqToVtArray<SDF_VALUE_CPP_TYPE(elem)>);

        BOOST_PP_SEQ_FOR_EACH(_ADD_FN, ~, SDF_VALUE_TYPES)
#undef _ADD_FN
        return ret;
    }();

    auto iter = pySeqToVtArrayFnMap->find(type);
    if (TF_VERIFY(iter != pySeqToVtArrayFnMap->end(),
                  "Value type '%s' returns true from "
                  "SdfValueHasValidType but does not appear in "
                  "SDF_VALUE_TYPES.", type.GetTypeName().c_str())) {
        return iter->second;
    }
    return nullptr;
}

static bool
_PyObjToAnyVtArray(VtValue *value, std::vector<std::string> *errMsgs,
                   std::vector<std::string> *keyPath)
{
    TfPyLock pyLock;
    TfPyObjWrapper obj = value->UncheckedGet<TfPyObjWrapper>();

    if (!PySequence_Check(obj.ptr())) {
        errMsgs->push_back(
            TfStringPrintf(
                "cannot convert python object as sequence%s",
                _GetKeyPathText(*keyPath).c_str()));
        *value = VtValue();
        return false;
    }

    Py_ssize_t len = PySequence_Length(obj.ptr());
    // If this is an empty sequence, we cannot sensibly choose any type for the
    // VtArray.  Error.
    if (len == 0) {
        errMsgs->push_back(
            TfStringPrintf(
                "cannot infer type from empty sequence%s -- use"
                "an empty typed array like VtIntArray/VtStringArray instead",
                _GetKeyPathText(*keyPath).c_str()));
        *value = VtValue();
        return false;
    }
    // Pull the type from the first element, and try to invoke the conversion
    // function to convert all elements.
    boost::python::handle<> h(PySequence_ITEM(obj.ptr(), 0));
    if (!h) {
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
        errMsgs->push_back(
            TfStringPrintf("failed to obtain first element from sequence%s",
                           _GetKeyPathText(*keyPath).c_str()));
        *value = VtValue();
        return false;
    }
    boost::python::extract<VtValue> e(h.get());
    if (!e.check()) {
        errMsgs->push_back(
            TfStringPrintf("failed to obtain first element from sequence%s",
                           _GetKeyPathText(*keyPath).c_str()));
        *value = VtValue();
        return false;
    }
    VtValue firstVal = e();
    if (SdfValueHasValidType(firstVal)) {
        return _GetTypedPySeqToVtArrayFn(
            firstVal.GetType())(value, errMsgs, keyPath);
    }
    else {
        _AddInvalidTypeError("first sequence element ",
                             firstVal, errMsgs, keyPath);
        *value = VtValue();
        return false;
    }
    return true;
}

#endif // PXR_PYTHON_SUPPORT_ENABLED

static bool
_ConvertToValidMetadataDictValueInternal(
    VtValue *value, std::vector<std::string> *errMsgs,
    std::vector<std::string> *keyPath) {

    bool allValid = true;

    if (value->IsHolding<VtDictionary>()) {
        VtDictionary d;
        value->UncheckedSwap(d);
        for (auto &kv: d) {
            keyPath->push_back(kv.first);
            allValid &= _ConvertToValidMetadataDictValueInternal(
                &kv.second, errMsgs, keyPath);
            keyPath->pop_back();
        }
        value->UncheckedSwap(d);
    }
    else if (value->IsHolding<std::vector<VtValue>>()) {
        allValid &= _ValueVectorToAnyVtArray(value, errMsgs, keyPath);
    }
#ifdef PXR_PYTHON_SUPPORT_ENABLED
    else if (value->IsHolding<TfPyObjWrapper>()) {
        allValid &= _PyObjToAnyVtArray(value, errMsgs, keyPath);
    }
#endif // PXR_PYTHON_SUPPORT_ENABLED
    else if (!SdfValueHasValidType(*value)) {
        allValid = false;
        *value = VtValue();
    }
    return allValid;
}

bool
SdfConvertToValidMetadataDictionary(VtDictionary *dict, std::string *errMsg)
{
    std::vector<std::string> keyPath;
    std::vector<std::string> errMsgs;
    bool allValid = true;
    for (auto &kv: *dict) {
        keyPath.push_back(kv.first);
        allValid &= _ConvertToValidMetadataDictValueInternal(
            &kv.second, &errMsgs, &keyPath);
        keyPath.pop_back();
    }
    *errMsg = TfStringJoin(errMsgs, "; ");
    return allValid;
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

// Defined in schema.cpp
const Sdf_ValueTypeNamesType* Sdf_InitializeValueTypeNames();

const Sdf_ValueTypeNamesType*
Sdf_ValueTypeNamesType::_Init::New()
{
    return Sdf_InitializeValueTypeNames();
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
