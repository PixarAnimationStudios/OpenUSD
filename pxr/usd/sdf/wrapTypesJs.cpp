#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/variantSpec.h"

#include <emscripten/bind.h>
using namespace emscripten;

pxr::SdfValueTypeName
_FindType(const std::string& typeName)
{
    return pxr::SdfSchema::GetInstance().FindType(typeName);
}

template<class T>
class SdfJsWrapListEditorBase {
    typedef T Type;
    typedef typename Type::value_type value_type;
    typedef typename Type::value_vector_type value_vector_type;
    typedef typename Type::ApplyCallback ApplyCallback;
    typedef typename Type::ModifyCallback ModifyCallback;

    public:
    template<typename This>
    static void createClass(const char *name)
    {
        class_<T>(name)
            .constructor<>()
            .property("explicitItems",
                &This::_GetExplicitItems,
                &This::_SetExplicitItems)
            .property("addedItems",
                &This::_GetAddedItems,
                &This::_SetAddedItems)
            .property("prependedItems",
                &This::_GetPrependedItems,
                &This::_SetPrependedItems)
            .property("appendedItems",
                &This::_GetAppendedItems,
                &This::_SetAppendedItems)
            .property("deletedItems",
                &This::_GetDeletedItems,
                &This::_SetDeletedItems)
            .property("orderedItems",
                &This::_GetOrderedItems,
                &This::_SetOrderedItems)
        ;
    }

    template<typename I>
    static val _Get(const I &items) {
        val result = val::array();
        for (size_t i = 0; i < items.size(); i++) {
            result.set(i, static_cast<value_type>(items[i]));
        }

        return result;
    }

    static void _Set(value_vector_type &itemVec, const val& v)
    {
        size_t length = v["length"].as<size_t>();
        itemVec.reserve(length);
        for (size_t i = 0; i < length; i++) {
            itemVec.push_back(v[i].as<value_type>());
        }
    }
    static val _GetExplicitItems(const Type& x) {
        return _Get(x.GetExplicitItems());
    }
    static val _GetAddedItems(const Type& x) {
        return _Get(x.GetAddedItems());
    }
    static val _GetPrependedItems(const Type& x) {
        return _Get(x.GetPrependedItems());
    }
    static val _GetAppendedItems(const Type& x) {
        return _Get(x.GetAppendedItems());
    }
    static val _GetDeletedItems(const Type& x) {
        return _Get(x.GetDeletedItems());
    }
    static val _GetOrderedItems(const Type& x) {
        return _Get(x.GetOrderedItems());
    }
};

template<typename T>
class SdfJsWrapListEditor : public SdfJsWrapListEditorBase<T> {
    typedef T Type;
    typedef typename Type::value_type value_type;
    typedef typename Type::value_vector_type value_vector_type;
    typedef typename Type::ApplyCallback ApplyCallback;
    typedef typename Type::ModifyCallback ModifyCallback;
    typedef SdfJsWrapListEditorBase<T> Base;

    public:
    SdfJsWrapListEditor(const char *name) {
        SdfJsWrapListEditorBase<T>::template createClass<SdfJsWrapListEditor<T>>(name);
    }

    static void _SetExplicitItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.SetExplicitItems(items);
    }
    static void _SetAddedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.SetAddedItems(items);
    }
    static void _SetPrependedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.SetPrependedItems(items);
    }
    static void _SetAppendedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.SetAppendedItems(items);
    }
    static void _SetDeletedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.SetDeletedItems(items);
    }
    static void _SetOrderedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.SetOrderedItems(items);
    }
};

template<typename T>
class SdfJsWrapListEditorProxy : public SdfJsWrapListEditorBase<T> {
    typedef T Type;
    typedef typename Type::value_type value_type;
    typedef typename Type::value_vector_type value_vector_type;
    typedef typename Type::ApplyCallback ApplyCallback;
    typedef typename Type::ModifyCallback ModifyCallback;
    typedef SdfJsWrapListEditorBase<T> Base;

    public:
    SdfJsWrapListEditorProxy(const char *name) {
        SdfJsWrapListEditorBase<T>::template createClass<SdfJsWrapListEditorProxy<T>>(name);
    }

    static void _SetExplicitItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.GetExplicitItems() = items;
    }
    static void _SetAddedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.GetAddedItems() = items;
    }
    static void _SetPrependedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.GetPrependedItems() = items;
    }
    static void _SetAppendedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.GetAppendedItems() = items;
    }
    static void _SetDeletedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.GetDeletedItems() = items;
    }
    static void _SetOrderedItems(Type& x, const val& v) {
        value_vector_type items;
        Base::_Set(items, v);
        x.GetOrderedItems() = items;
    }
};

EMSCRIPTEN_BINDINGS(ValueTypeNames) {
    class_<pxr::Sdf_ValueTypeNamesType>("ValueTypeNames")
        .class_function("Find", &_FindType)
        .class_property("Asset", &pxr::SdfValueTypeNames->Asset)
        .class_property("Color3f", &pxr::SdfValueTypeNames->Color3f)
        .class_property("Float", &pxr::SdfValueTypeNames->Float)
        .class_property("Float2", &pxr::SdfValueTypeNames->Float2)
        .class_property("Float3", &pxr::SdfValueTypeNames->Float3)
        .class_property("Token", &pxr::SdfValueTypeNames->Token)
    ;

    enum_<pxr::SdfSpecifier>("SdfSpecifier")
        .value("SdfSpecifierDef", pxr::SdfSpecifier::SdfSpecifierDef)
        .value("SdfSpecifierOver", pxr::SdfSpecifier::SdfSpecifierOver)
        .value("SdfSpecifierClass", pxr::SdfSpecifier::SdfSpecifierClass)
        ;

    SdfJsWrapListEditorProxy<pxr::SdfPathEditorProxy>("SdfPathEditorProxy");
    SdfJsWrapListEditorProxy<pxr::SdfPayloadsProxy>("SdfPayloadsProxy");
    SdfJsWrapListEditorProxy<pxr::SdfReferencesProxy>("SdfReferencesProxy");
    SdfJsWrapListEditorProxy<pxr::SdfVariantSetNamesProxy>("SdfVariantSetNamesProxy");

    SdfJsWrapListEditor<pxr::SdfPathListOp>("SdfPathListOp");
    SdfJsWrapListEditor<pxr::SdfPayloadListOp>("SdfPayloadListOp");
    SdfJsWrapListEditor<pxr::SdfReferenceListOp>("SdfReferenceListOp");
    SdfJsWrapListEditor<pxr::SdfStringListOp>("SdfStringListOp");
    SdfJsWrapListEditor<pxr::SdfTokenListOp>("SdfTokenListOp");
    SdfJsWrapListEditor<pxr::SdfIntListOp>("SdfIntListOp");
    SdfJsWrapListEditor<pxr::SdfInt64ListOp>("SdfInt64ListOp");
    SdfJsWrapListEditor<pxr::SdfUIntListOp>("SdfUIntListOp");
    SdfJsWrapListEditor<pxr::SdfUInt64ListOp>("SdfUInt64ListOp");
    SdfJsWrapListEditor<pxr::SdfUnregisteredValueListOp>("UnregisteredValueListOp");

}
