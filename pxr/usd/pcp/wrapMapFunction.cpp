//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

class Pcp_MapFunctionPyAccess
{
public:
    static size_t GetNumMappingSets(const PcpMapFunction& f)
    {
        return f._GetNumMappingSets();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_Repr(const PcpMapFunction &f)
{
    if (f.IsIdentity()) {
        return "Pcp.MapFunction.Identity()";
    }
    string s = "Pcp.MapFunction(";
    if (!f.IsNull()) {
        const nanobind::dict sourceToTargetMap =
            TfPyCopyMapToDictionary(f.GetSourceToTargetMap());

        s += TfPyObjectRepr(sourceToTargetMap);
        if (f.GetTimeOffset() != SdfLayerOffset()) {
            s += ", ";
            s += TfPyRepr(f.GetTimeOffset());
        }
    }
    s += ")";

    if (f.IsDeferredComposition()) {
        std::string sourceRepr;

        // XXX:
        // We may need to store more information to generate a repr that
        // exactly replicates a map function with more than 1 set of
        // mappings, so for now we just generate a non-eval'able repr
        // for simplicity.
        const size_t numSubMappings =
            Pcp_MapFunctionPyAccess::GetNumMappingSets(f);
        if (numSubMappings > 1) {
            sourceRepr = TfStringPrintf("<%zu mapping sets, composed: %s>",
                numSubMappings, s.c_str());
        }

        s = "Pcp.MapFunction.DeferredComposition(" + 
            (sourceRepr.empty() ? s : sourceRepr) + ")";
    }
    return s;
}

static string
_Str(const PcpMapFunction& f)
{
    return f.GetString();
}

static PcpMapFunction
_Create(const nanobind::dict & sourceToTargetMap, 
        SdfLayerOffset offset)
{
    PcpMapFunction::PathMap pathMap;
    for (auto item : sourceToTargetMap) {
        SdfPath source = nanobind::cast<SdfPath>(item.first);
        SdfPath target = nanobind::cast<SdfPath>(item.second);
        pathMap[source] = target;
    }

    return PcpMapFunction::Create(pathMap, offset);
}

} // anonymous namespace 

void wrapMapFunction(nanobind::module_ &m)
{    
    namespace nb = nanobind;
    typedef PcpMapFunction This;

    TfPyContainerConversions::from_python_sequence<
        std::vector< PcpMapFunction >,
        TfPyContainerConversions::variable_capacity_policy >();

    nb::class_<This>(m, "MapFunction")
        .def(nb::init<>())
        .def(nb::init<const This &>())
        .def("__init__",
             [](This *self, const nb::dict &sourceToTargetMap, SdfLayerOffset offset) {
                 new (self) This(_Create(sourceToTargetMap, offset));
             },
             nb::arg("sourceToTargetMap"),
             nb::arg("timeOffset") = SdfLayerOffset())

        .def("__repr__", _Repr)
        .def("__str__", _Str)

        .def_static("Identity", &This::Identity,
                    nb::rv_policy::copy)
        .def_static("IdentityPathMap", &This::IdentityPathMap,
                    nb::rv_policy::copy)

        .def_static("DeferredComposition", &This::DeferredComposition)
        .def_static("ImpliedClass", &This::ImpliedClass)

        .def_prop_ro("isIdentity", &This::IsIdentity)
        .def_prop_ro("isIdentityPathMapping", &This::IsIdentityPathMapping)
        .def_prop_ro("isNull", &This::IsNull)
        .def_prop_ro("isDeferredComposition", &This::IsDeferredComposition)

        .def("MapSourceToTarget",
             [](This const &self, const SdfPathExpression &pathExpr) {
                 return self.MapSourceToTarget(pathExpr);
             },
             nb::arg("pathExpr"))
        .def("MapTargetToSource",
             [](This const &self, const SdfPathExpression &pathExpr) {
                 return self.MapTargetToSource(pathExpr);
             },
             nb::arg("pathExpr"))

        .def("MapSourceToTarget", (SdfPath (This::*)(const SdfPath &) const)
             &This::MapSourceToTarget, nb::arg("path"))
        .def("MapTargetToSource", (SdfPath (This::*)(const SdfPath &) const)
             &This::MapTargetToSource, nb::arg("path"))

        .def("Compose", &This::Compose)
        .def("ComposeOffset", &This::ComposeOffset, nb::arg("offset"))
        .def("GetInverse", &This::GetInverse)

        .def_prop_ro("sourceToTargetMap", &This::GetSourceToTargetMap,
                     nb::rv_policy::copy)
        .def_prop_ro("timeOffset", &This::GetTimeOffset,
                     nb::rv_policy::copy)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;

}
