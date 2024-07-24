//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primCompositionQuery.h"
#include "pxr/usd/usd/resolveTarget.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/scope.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

template <typename ProxyType>
tuple
_GetIntroducingListEditor(const UsdPrimCompositionQueryArc &arc)
{
    ProxyType editor;
    typename ProxyType::value_type value;
    if (arc.GetIntroducingListEditor(&editor, &value)) {
        return make_tuple(object(editor), object(value));
    }
    TF_CODING_ERROR("Failed to get list editor value for the given type of "
                    "of the composition arc");
    return make_tuple(object(),object());
}

static tuple
_WrapGetIntroducingListEditor(const UsdPrimCompositionQueryArc &arc)
{
    switch (arc.GetArcType()) {
    case PcpArcTypeReference:
        return _GetIntroducingListEditor<SdfReferenceEditorProxy>(arc);
    case PcpArcTypePayload:
        return _GetIntroducingListEditor<SdfPayloadEditorProxy>(arc);
    case PcpArcTypeInherit:
    case PcpArcTypeSpecialize:
        return _GetIntroducingListEditor<SdfPathEditorProxy>(arc);
    case PcpArcTypeVariant:
        return _GetIntroducingListEditor<SdfNameEditorProxy>(arc);
    default:
        return make_tuple(object(),object());
    }
}

void wrapUsdPrimCompositionQueryArc()
{
    class_<UsdPrimCompositionQueryArc>("CompositionArc", no_init)
        .def("GetTargetNode", 
             &UsdPrimCompositionQueryArc::GetTargetNode)
        .def("GetIntroducingNode", 
             &UsdPrimCompositionQueryArc::GetIntroducingNode)
        .def("GetTargetLayer", &UsdPrimCompositionQueryArc::GetTargetLayer)
        .def("GetTargetPrimPath", 
             &UsdPrimCompositionQueryArc::GetTargetPrimPath)
        .def("GetIntroducingLayer", 
             &UsdPrimCompositionQueryArc::GetIntroducingLayer)
        .def("GetIntroducingPrimPath", 
             &UsdPrimCompositionQueryArc::GetIntroducingPrimPath)
        .def("GetIntroducingListEditor", &_WrapGetIntroducingListEditor)
        .def("GetArcType", &UsdPrimCompositionQueryArc::GetArcType)
        .def("IsImplicit", &UsdPrimCompositionQueryArc::IsImplicit)
        .def("IsAncestral", &UsdPrimCompositionQueryArc::IsAncestral)
        .def("HasSpecs", &UsdPrimCompositionQueryArc::HasSpecs)
        .def("IsIntroducedInRootLayerStack", 
             &UsdPrimCompositionQueryArc::IsIntroducedInRootLayerStack)
        .def("IsIntroducedInRootLayerPrimSpec", 
             &UsdPrimCompositionQueryArc::IsIntroducedInRootLayerPrimSpec)        
        .def("MakeResolveTargetUpTo", 
             &UsdPrimCompositionQueryArc::MakeResolveTargetUpTo,
             arg("subLayer")=object())
        .def("MakeResolveTargetStrongerThan", 
             &UsdPrimCompositionQueryArc::MakeResolveTargetStrongerThan,
             arg("subLayer")=object())    ;
}

void wrapUsdPrimCompositionQuery()
{
    using This = UsdPrimCompositionQuery;
        
    scope s = class_<This>
        ("PrimCompositionQuery", no_init)
        .def(init<const UsdPrim &>(arg("prim")))
        .def(init<const UsdPrim &, const This::Filter &>((arg("prim"), arg("filter"))))
        .def("GetDirectReferences", &This::GetDirectReferences,
             return_value_policy<return_by_value>())
            .staticmethod("GetDirectReferences")
        .def("GetDirectInherits", &This::GetDirectInherits)
            .staticmethod("GetDirectInherits")
        .def("GetDirectRootLayerArcs", &This::GetDirectRootLayerArcs)
            .staticmethod("GetDirectRootLayerArcs")
        .add_property("filter", &This::GetFilter, &This::SetFilter)
        .def("GetCompositionArcs", &This::GetCompositionArcs,
             return_value_policy<TfPySequenceToList>())
    ;

    enum_<This::ArcIntroducedFilter>("ArcIntroducedFilter")
        .value("All", This::ArcIntroducedFilter::All)
        .value("IntroducedInRootLayerStack", This::ArcIntroducedFilter::IntroducedInRootLayerStack)
        .value("IntroducedInRootLayerPrimSpec", This::ArcIntroducedFilter::IntroducedInRootLayerPrimSpec)
    ;
    enum_<This::ArcTypeFilter>("ArcTypeFilter")
        .value("All", This::ArcTypeFilter::All)
        .value("Reference", This::ArcTypeFilter::Reference)
        .value("Payload", This::ArcTypeFilter::Payload)
        .value("Inherit", This::ArcTypeFilter::Inherit)
        .value("Specialize", This::ArcTypeFilter::Specialize)
        .value("Variant", This::ArcTypeFilter::Variant)
        .value("ReferenceOrPayload", This::ArcTypeFilter::ReferenceOrPayload)
        .value("InheritOrSpecialize", This::ArcTypeFilter::InheritOrSpecialize)
        .value("NotReferenceOrPayload", This::ArcTypeFilter::NotReferenceOrPayload)
        .value("NotInheritOrSpecialize", This::ArcTypeFilter::NotInheritOrSpecialize)
        .value("NotVariant", This::ArcTypeFilter::NotVariant)
    ;
    enum_<This::DependencyTypeFilter>("DependencyTypeFilter")
        .value("All", This::DependencyTypeFilter::All)
        .value("Direct", This::DependencyTypeFilter::Direct)
        .value("Ancestral", This::DependencyTypeFilter::Ancestral)
    ;
    enum_<This::HasSpecsFilter>("HasSpecsFilter")
        .value("All", This::HasSpecsFilter::All)
        .value("HasSpecs", This::HasSpecsFilter::HasSpecs)
        .value("HasNoSpecs", This::HasSpecsFilter::HasNoSpecs)
    ;

    class_<This::Filter>("Filter")
        .def(init<>())
        .def_readwrite("arcIntroducedFilter", &This::Filter::arcIntroducedFilter)
        .def_readwrite("arcTypeFilter", &This::Filter::arcTypeFilter)
        .def_readwrite("dependencyTypeFilter", &This::Filter::dependencyTypeFilter)
        .def_readwrite("hasSpecsFilter", &This::Filter::hasSpecsFilter)
        .def(self == self)
        .def(self != self)
    ;

}
