//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/pyChildrenView.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/pyListOp.h"
#include "pxr/usd/sdf/pyListEditorProxy.h"
#include "pxr/usd/sdf/pyListProxy.h"
#include "pxr/usd/sdf/pyMapEditProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyStaticTokens.h"

#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

class Sdf_VariantSelectionProxyWrap {
public:
    typedef SdfVariantSelectionProxy Type;
    typedef Type::key_type key_type;
    typedef Type::mapped_type mapped_type;
    typedef Type::value_type value_type;
    typedef Type::iterator iterator;
    typedef Type::const_iterator const_iterator;
    typedef std::pair<key_type, mapped_type> pair_type;

    static void SetItem(Type& x,
                        const key_type& key, const mapped_type& value)
    {
        if (value.empty()) {
            x.erase(key);
        }
        else {
            std::pair<iterator, bool> i = x.insert(value_type(key, value));
            if (!i.second && i.first != iterator()) {
                i.first->second = value;
            }
        }
    }

    static mapped_type SetDefault(Type& x, const key_type& key,
                                  const mapped_type& def)
    {
        const_iterator i = x.find(key);
        if (i != x.end()) {
            return i->second;
        }
        else if (!def.empty()) {
            SdfChangeBlock block;
            return x[key] = def;
        }
        else {
            return def;
        }
    }

    static void Update(Type& x, const std::vector<pair_type>& values)
    {
        SdfChangeBlock block;
        TF_FOR_ALL(i, values) {
            if (i->second.empty()) {
                x.erase(i->first);
            }
            else {
                x[i->first] = i->second;
            }
        }
    }

    static void UpdateDict(Type& x, const nb::dict& d)
    {
        std::vector<pair_type> values;
        for (auto item : d) {
            values.push_back(pair_type(
                nb::cast<key_type>(item.first),
                nb::cast<mapped_type>(item.second)));
        }
        Update(x, values);
    }

    static void UpdateList(Type& x, const nb::list& pairs)
    {
        std::vector<pair_type> values;
        for (size_t i = 0, n = pairs.size(); i != n; ++i) {
            values.push_back(pair_type(
                nb::cast<key_type>(pairs[i][0]),
                nb::cast<mapped_type>(pairs[i][1])));
        }
        Update(x, values);
    }
};

static
void
_ModifyVariantSelectionProxy()
{
    // The map edit proxy for SdfVariantSelectionProxy has to have a
    // special behavior for assignment:  assigning the empty string
    // means delete.  Rather than mess with SdfPyMapEditProxy we just
    // edit the python class, replacing the original methods with
    // customized methods.  We need to fix __setitem__, setdefault,
    // and update.
    typedef Sdf_VariantSelectionProxyWrap Wrap;
    nb::class_<SdfVariantSelectionProxy> cls =
        nb::borrow<nb::class_<SdfVariantSelectionProxy>>(
            nb::type<SdfVariantSelectionProxy>());

    // Erase old methods.
    PyTypeObject* typeObj = reinterpret_cast<PyTypeObject*>(cls.ptr());
    PyDict_DelItemString(typeObj->tp_dict, "__setitem__");
    PyDict_DelItemString(typeObj->tp_dict, "setdefault");
    PyDict_DelItemString(typeObj->tp_dict, "update");
    PyType_Modified(typeObj);

    // Insert new methods.
    cls.def("__setitem__", &Wrap::SetItem);
    cls.def("setdefault", &Wrap::SetDefault);
    cls.def("update", &Wrap::UpdateDict);
    cls.def("update", &Wrap::UpdateList);
}

static TfEnum
_DefaultUnitWrapper1(const TfEnum & unit)
{
    return SdfDefaultUnit(unit);
}

static TfEnum
_DefaultUnitWrapper2(const TfToken &typeName)
{
    return SdfDefaultUnit(typeName);
}

static string
_UnitCategoryWrapper(const TfEnum & unit)
{
    return SdfUnitCategory(unit);
}

static string
_UnregisteredValueRepr(const SdfUnregisteredValue &self)
{
    string value = TfPyRepr(self.GetValue());
    return TF_PY_REPR_PREFIX + "UnregisteredValue(" + value + ")";
}

static int
_UnregisteredValueHash(const SdfUnregisteredValue &self)
{
    const VtValue &value = self.GetValue();
    if (value.IsHolding<VtDictionary>()) {
        return VtDictionaryHash()(value.Get<VtDictionary>());
    } else if (value.IsHolding<std::string>()) {
        return TfHash()(value.Get<std::string>());
    } else {
        return 0;
    }
}

static string
_SdfValueBlockRepr(const SdfValueBlock &self) 
{
    return TF_PY_REPR_PREFIX + "ValueBlock";
}  

static int
_SdfValueBlockHash(const SdfValueBlock &self)
{
    return TfHash{}(self);
}

static string
_SdfAnimationBlockRepr(const SdfAnimationBlock &self) 
{
    return TF_PY_REPR_PREFIX + "AnimationBlock";
}

static int
_SdfAnimationBlockHash(const SdfAnimationBlock &self)
{
    return TfHash{}(self);
}

SdfValueTypeName
_FindType(const std::string& typeName)
{
    return SdfSchema::GetInstance().FindType(typeName);
}

nb::tuple
_ConvertToValidMetadataDictionary(VtDictionary dict)
{
    std::string errMsg;
    bool success = SdfConvertToValidMetadataDictionary(&dict, &errMsg);
    return nb::make_tuple(success, dict, errMsg);
}

} // anonymous namespace 

void wrapTypes(nanobind::module_& m)
{
    TF_NB_WRAP_PUBLIC_TOKENS(m, "ValueRoleNames",
                             SdfValueRoleNames, SDF_VALUE_ROLE_NAME_TOKENS);

    m.def("DefaultUnit", _DefaultUnitWrapper1,
        "For a given unit of measurement get the default compatible unit.");

    m.def("DefaultUnit", _DefaultUnitWrapper2,
        "For a given typeName ('Vector', 'Point' etc.) get the "
        "default unit of measurement.");

    m.def("UnitCategory", _UnitCategoryWrapper,
        "For a given unit of measurement get the unit category.");

    m.def("ConvertUnit", &SdfConvertUnit,
        "Convert a unit of measurement to a compatible unit.");

    m.def("ValueHasValidType", &SdfValueHasValidType);
    m.def("GetTypeForValueTypeName", &SdfGetTypeForValueTypeName);
    m.def("GetValueTypeNameForValue", &SdfGetValueTypeNameForValue);

    m.def("ConvertToValidMetadataDictionary",
         &_ConvertToValidMetadataDictionary);

    m.def("GetUnitFromName", &SdfGetUnitFromName);
    m.def("GetNameForUnit", &SdfGetNameForUnit);

    TfNbWrapEnum<SdfListOpType>(m);
    TfNbWrapEnum<SdfPermission>(m);
    TfNbWrapEnum<SdfSpecifier>(m);
    TfNbWrapEnum<SdfVariability>(m);
    TfNbWrapEnum<SdfSpecType>(m);
    TfNbWrapEnum<SdfAuthoringError>(m);

    VtValueFromPython<SdfListOpType>();
    VtValueFromPython<SdfPermission>();
    VtValueFromPython<SdfSpecifier>();
    VtValueFromPython<SdfVariability>();
    VtValueFromPython<SdfSpecType>();
    VtValueFromPython<SdfAuthoringError>();

    // Wrap all units enums.
    #define _WRAP_ENUM(elem)                                 \
        TfNbWrapEnum<_SDF_UNITSLIST_ENUM(elem)>(m);          \
        VtValueFromPython<_SDF_UNITSLIST_ENUM(elem)>();
    _SDF_FOR_EACH_UNITS(_WRAP_ENUM, _SDF_UNITS)
    #undef _WRAP_ENUM

    SdfPyWrapListProxy<SdfNameOrderProxy>();
    SdfPyWrapListProxy<SdfSubLayerProxy>();
    SdfPyWrapListEditorProxy<SdfConnectionsProxy>();
    SdfPyWrapListEditorProxy<SdfInheritsProxy>();
    SdfPyWrapListEditorProxy<SdfPayloadsProxy>();
    SdfPyWrapListEditorProxy<SdfReferencesProxy>();
    SdfPyWrapListEditorProxy<SdfVariantSetNamesProxy>();

    SdfPyWrapChildrenView<SdfAttributeSpecView>();
    SdfPyWrapChildrenView<SdfPrimSpecView>();
    SdfPyWrapChildrenView<SdfPropertySpecView>();
    SdfPyWrapChildrenView<SdfRelationalAttributeSpecView>();
    SdfPyWrapChildrenView<SdfRelationshipSpecView>();
    SdfPyWrapChildrenView<SdfVariantView>();
    SdfPyWrapChildrenView<SdfVariantSetView>();

    SdfPyWrapMapEditProxy<SdfDictionaryProxy>();
    SdfPyWrapMapEditProxy<SdfVariantSelectionProxy>();

    SdfPyWrapListOp<SdfPathListOp>(m, "PathListOp");
    SdfPyWrapListOp<SdfPayloadListOp>(m, "PayloadListOp");
    SdfPyWrapListOp<SdfReferenceListOp>(m, "ReferenceListOp");
    SdfPyWrapListOp<SdfStringListOp>(m, "StringListOp");
    SdfPyWrapListOp<SdfTokenListOp>(m, "TokenListOp");
    SdfPyWrapListOp<SdfIntListOp>(m, "IntListOp");
    SdfPyWrapListOp<SdfInt64ListOp>(m, "Int64ListOp");
    SdfPyWrapListOp<SdfUIntListOp>(m, "UIntListOp");
    SdfPyWrapListOp<SdfUInt64ListOp>(m, "UInt64ListOp");
    SdfPyWrapListOp<SdfUnregisteredValueListOp>(m, "UnregisteredValueListOp");

    VtValueFromPython<SdfPathListOp>();
    VtValueFromPython<SdfPayloadListOp>();
    VtValueFromPython<SdfReferenceListOp>();
    VtValueFromPython<SdfStringListOp>();
    VtValueFromPython<SdfTokenListOp>();
    VtValueFromPython<SdfIntListOp>();
    VtValueFromPython<SdfInt64ListOp>();
    VtValueFromPython<SdfUIntListOp>();
    VtValueFromPython<SdfUInt64ListOp>();
    VtValueFromPython<SdfUnregisteredValueListOp>();

    // Modify class wrappers for special behaviors (see function comments).
    _ModifyVariantSelectionProxy();

    nb::class_<SdfUnregisteredValue>(m, "UnregisteredValue")
        .def(nb::init<>())
        .def(nb::init<const std::string &>())
        .def(nb::init<const VtDictionary &>())
        .def(nb::init<const SdfUnregisteredValue &>())
        .def(nb::init<const SdfUnregisteredValueListOp &>())

        .def_prop_ro("value", &SdfUnregisteredValue::GetValue)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("__repr__", _UnregisteredValueRepr)
        .def("__hash__", _UnregisteredValueHash)
        ;

    VtValueFromPython<SdfUnregisteredValue>();

    #define _DEF_VTN(name) \
        .def_prop_ro_static(#name, [](nb::handle) { return SdfValueTypeNames->name; })

    nb::class_<Sdf_ValueTypeNamesType>(m, "ValueTypeNames")
        .def_static("Find", &_FindType)
        _DEF_VTN(Bool)
        _DEF_VTN(UChar)
        _DEF_VTN(Int)
        _DEF_VTN(UInt)
        _DEF_VTN(Int64)
        _DEF_VTN(UInt64)
        _DEF_VTN(Half)
        _DEF_VTN(Float)
        _DEF_VTN(Double)
        _DEF_VTN(TimeCode)
        _DEF_VTN(Duration)
        _DEF_VTN(String)
        _DEF_VTN(Token)
        _DEF_VTN(Asset)
        _DEF_VTN(Opaque)
        _DEF_VTN(Group)
        _DEF_VTN(PathExpression)
        _DEF_VTN(Int2)
        _DEF_VTN(Int3)
        _DEF_VTN(Int4)
        _DEF_VTN(Half2)
        _DEF_VTN(Half3)
        _DEF_VTN(Half4)
        _DEF_VTN(Float2)
        _DEF_VTN(Float3)
        _DEF_VTN(Float4)
        _DEF_VTN(Double2)
        _DEF_VTN(Double3)
        _DEF_VTN(Double4)
        _DEF_VTN(Point3h)
        _DEF_VTN(Point3f)
        _DEF_VTN(Point3d)
        _DEF_VTN(Vector3h)
        _DEF_VTN(Vector3f)
        _DEF_VTN(Vector3d)
        _DEF_VTN(Normal3h)
        _DEF_VTN(Normal3f)
        _DEF_VTN(Normal3d)
        _DEF_VTN(Color3h)
        _DEF_VTN(Color3f)
        _DEF_VTN(Color3d)
        _DEF_VTN(Color4h)
        _DEF_VTN(Color4f)
        _DEF_VTN(Color4d)
        _DEF_VTN(Quath)
        _DEF_VTN(Quatf)
        _DEF_VTN(Quatd)
        _DEF_VTN(Matrix2d)
        _DEF_VTN(Matrix3d)
        _DEF_VTN(Matrix4d)
        _DEF_VTN(Frame4d)
        _DEF_VTN(TexCoord2h)
        _DEF_VTN(TexCoord2f)
        _DEF_VTN(TexCoord2d)
        _DEF_VTN(TexCoord3h)
        _DEF_VTN(TexCoord3f)
        _DEF_VTN(TexCoord3d)

        _DEF_VTN(BoolArray)
        _DEF_VTN(UCharArray)
        _DEF_VTN(IntArray)
        _DEF_VTN(UIntArray)
        _DEF_VTN(Int64Array)
        _DEF_VTN(UInt64Array)
        _DEF_VTN(HalfArray)
        _DEF_VTN(FloatArray)
        _DEF_VTN(DoubleArray)
        _DEF_VTN(TimeCodeArray)
        _DEF_VTN(DurationArray)
        _DEF_VTN(StringArray)
        _DEF_VTN(TokenArray)
        _DEF_VTN(AssetArray)
        _DEF_VTN(PathExpressionArray)
        _DEF_VTN(Int2Array)
        _DEF_VTN(Int3Array)
        _DEF_VTN(Int4Array)
        _DEF_VTN(Half2Array)
        _DEF_VTN(Half3Array)
        _DEF_VTN(Half4Array)
        _DEF_VTN(Float2Array)
        _DEF_VTN(Float3Array)
        _DEF_VTN(Float4Array)
        _DEF_VTN(Double2Array)
        _DEF_VTN(Double3Array)
        _DEF_VTN(Double4Array)
        _DEF_VTN(Point3hArray)
        _DEF_VTN(Point3fArray)
        _DEF_VTN(Point3dArray)
        _DEF_VTN(Vector3hArray)
        _DEF_VTN(Vector3fArray)
        _DEF_VTN(Vector3dArray)
        _DEF_VTN(Normal3hArray)
        _DEF_VTN(Normal3fArray)
        _DEF_VTN(Normal3dArray)
        _DEF_VTN(Color3hArray)
        _DEF_VTN(Color3fArray)
        _DEF_VTN(Color3dArray)
        _DEF_VTN(Color4hArray)
        _DEF_VTN(Color4fArray)
        _DEF_VTN(Color4dArray)
        _DEF_VTN(QuathArray)
        _DEF_VTN(QuatfArray)
        _DEF_VTN(QuatdArray)
        _DEF_VTN(Matrix2dArray)
        _DEF_VTN(Matrix3dArray)
        _DEF_VTN(Matrix4dArray)
        _DEF_VTN(Frame4dArray)
        _DEF_VTN(TexCoord2hArray)
        _DEF_VTN(TexCoord2fArray)
        _DEF_VTN(TexCoord2dArray)
        _DEF_VTN(TexCoord3hArray)
        _DEF_VTN(TexCoord3fArray)
        _DEF_VTN(TexCoord3dArray)
        ;
    #undef _DEF_VTN

    nb::class_<SdfValueBlock>(m, "ValueBlock")
        .def(nb::init<>())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__repr__", _SdfValueBlockRepr)
        .def("__hash__", _SdfValueBlockHash);
    VtValueFromPython<SdfValueBlock>();

    nb::class_<SdfAnimationBlock>(m, "AnimationBlock")
        .def(nb::init<>())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__repr__", _SdfAnimationBlockRepr)
        .def("__hash__", _SdfAnimationBlockHash);
    VtValueFromPython<SdfAnimationBlock>();
}
