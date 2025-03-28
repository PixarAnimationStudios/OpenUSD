//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/colorSpaceAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateColorSpaceNameAttr(UsdColorSpaceAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorSpaceNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdColorSpaceAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "Usd.ColorSpaceAPI(%s)",
        primRepr.c_str());
}

struct UsdColorSpaceAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdColorSpaceAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdColorSpaceAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdColorSpaceAPI::CanApply(prim, &whyNot);
    return UsdColorSpaceAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdColorSpaceAPI()
{
    typedef UsdColorSpaceAPI This;

    UsdColorSpaceAPI_CanApplyResult::Wrap<UsdColorSpaceAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("ColorSpaceAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetColorSpaceNameAttr",
             &This::GetColorSpaceNameAttr)
        .def("CreateColorSpaceNameAttr",
             &_CreateColorSpaceNameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/pyContainerConversions.h"

namespace {

// Wrapper class to handle the abstract ColorSpaceCache
struct ColorSpaceCacheWrapper : UsdColorSpaceAPI::ColorSpaceCache, 
                                    wrapper<UsdColorSpaceAPI::ColorSpaceCache> {
    TfToken Find(const SdfPath& prim) override {
        if (override find = this->get_override("Find")) {
            return find(prim);
        }
        return {};
    }

    void Insert(const SdfPath& prim, const TfToken& colorSpace) override {
        if (override insert = this->get_override("Insert")) {
            insert(prim, colorSpace);
        }
    }
};

void wrapColorSpaceCache() {
    typedef UsdColorSpaceAPI::ColorSpaceCache This;

    class_<ColorSpaceCacheWrapper, noncopyable>("ColorSpaceCache")
        .def("Find", &This::Find)
        .def("Insert", &This::Insert)
    ;
}

// Wrapper class for ColorSpaceHashCache which inherits ColorSpaceCache
struct ColorSpaceHashCacheWrapper : UsdColorSpaceAPI::ColorSpaceHashCache, 
                                wrapper<UsdColorSpaceAPI::ColorSpaceHashCache> {
    TfToken Find(const SdfPath& prim) override {
        if (override find = this->get_override("Find"))
            return find(prim);
        return ColorSpaceHashCache::Find(prim); // Optional fallback
    }

    void Insert(const SdfPath& prim, const TfToken& colorSpace) override {
        if (override insert = this->get_override("Insert"))
            insert(prim, colorSpace);
        else
            ColorSpaceHashCache::Insert(prim, colorSpace); // Optional fallback
    }
};

void wrapColorSpaceHashCache() {
    typedef UsdColorSpaceAPI::ColorSpaceHashCache This;

    class_<ColorSpaceHashCacheWrapper, bases<UsdColorSpaceAPI::ColorSpaceCache>, noncopyable>("ColorSpaceHashCache")
        .def("Find", &This::Find)
        .def("Insert", &This::Insert)
    ;
}

WRAP_CUSTOM {

    wrapColorSpaceCache();
    wrapColorSpaceHashCache();

    _class
        .def("ComputeColorSpaceName", 
                (TfToken (*)(UsdPrim, 
                             UsdColorSpaceAPI::ColorSpaceCache*)) 
                &UsdColorSpaceAPI::ComputeColorSpaceName)
        .def("ComputeColorSpaceName",
                (TfToken (*)(const UsdAttribute&, 
                             UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::ComputeColorSpaceName)
        .def("ComputeColorSpace",
                (GfColorSpace (*)(UsdPrim, 
                                  UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::ComputeColorSpace)
        .def("ComputeColorSpace",
                (GfColorSpace (*)(const UsdAttribute&,
                                  UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::ComputeColorSpace)
        .def("IsValidColorSpaceName",
                (bool (*)(UsdPrim, const TfToken&, 
                          UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::IsValidColorSpaceName)
    ;
}


}
