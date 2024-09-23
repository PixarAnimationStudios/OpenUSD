//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/external/boost/python.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static string
_Repr(const PcpMapFunction &f)
{
    if (f.IsIdentity()) {
        return "Pcp.MapFunction.Identity()";
    }
    string s = "Pcp.MapFunction(";
    if (!f.IsNull()) {
        const pxr_boost::python::dict sourceToTargetMap = 
            TfPyCopyMapToDictionary(f.GetSourceToTargetMap());

        s += TfPyObjectRepr(sourceToTargetMap);
        if (f.GetTimeOffset() != SdfLayerOffset()) {
            s += ", ";
            s += TfPyRepr(f.GetTimeOffset());
        }
    }
    s += ")";
    return s;
}

static string
_Str(const PcpMapFunction& f)
{
    return f.GetString();
}

static PcpMapFunction*
_Create(const pxr_boost::python::dict & sourceToTargetMap, 
        SdfLayerOffset offset)
{
    PcpMapFunction::PathMap pathMap;
    pxr_boost::python::list keys = sourceToTargetMap.keys();
    for (int i=0; i < pxr_boost::python::len(keys); ++i) {
        // Just blindly try to extract SdfPaths.
        // If the dict is not holding the right types,
        // we'll raise a boost exception, which boost
        // will turn into a suitable TypeError.
        SdfPath source =
            pxr_boost::python::extract<SdfPath>(keys[i]);
        SdfPath target =
            pxr_boost::python::extract<SdfPath>(sourceToTargetMap[keys[i]]);
        pathMap[source] = target;
    }

    PcpMapFunction mapFunction = PcpMapFunction::Create(pathMap, offset);

    // Return a newly allocated instance.  pxr_boost::python will free this
    // object when the holding python object expires.
    return new PcpMapFunction(mapFunction);
}

} // anonymous namespace 

void wrapMapFunction()
{    
    typedef PcpMapFunction This;

    TfPyContainerConversions::from_python_sequence<
        std::vector< PcpMapFunction >,
        TfPyContainerConversions::variable_capacity_policy >();

    class_<This>("MapFunction")
        .def(init<const This &>())
        .def("__init__",
             pxr_boost::python::make_constructor(
                &_Create,
                default_call_policies(),
                (arg("sourceToTargetMap"),
                 arg("timeOffset") = SdfLayerOffset())))

        .def("__repr__", _Repr)
        .def("__str__", _Str)

        .def("Identity", &This::Identity,
             return_value_policy<return_by_value>())
        .staticmethod("Identity")
        .def("IdentityPathMap", &This::IdentityPathMap,
             return_value_policy<TfPyMapToDictionary>())
        .staticmethod("IdentityPathMap")
        .add_property("isIdentity", &This::IsIdentity)
        .add_property("isIdentityPathMapping", &This::IsIdentityPathMapping)
        .add_property("isNull", &This::IsNull)

        .def("MapSourceToTarget",
             +[](This const &self, SdfPathExpression const &pathExpr) {
                 return self.MapSourceToTarget(pathExpr);
             }, arg("pathExpr"))
        .def("MapTargetToSource",
             +[](This const &self, SdfPathExpression const &pathExpr) {
                 return self.MapTargetToSource(pathExpr);
             }, arg("pathExpr"))

        .def("MapSourceToTarget", (SdfPath (This::*)(const SdfPath &) const)
             &This::MapSourceToTarget, (arg("path")))
        .def("MapTargetToSource", (SdfPath (This::*)(const SdfPath &) const)
             &This::MapTargetToSource, (arg("path")))

        .def("Compose", &This::Compose)
        .def("ComposeOffset", &This::ComposeOffset, arg("offset"))
        .def("GetInverse", &This::GetInverse)

        .add_property("sourceToTargetMap",
            make_function( &This::GetSourceToTargetMap,
                           return_value_policy<TfPyMapToDictionary>()) )
        .add_property("timeOffset",
            make_function( &This::GetTimeOffset,
                           return_value_policy<return_by_value>()) )

        .def(self == self)
        .def(self != self)
        ;
}
