//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primCompositionQuery.h"
#include "pxr/usd/usd/resolveTarget.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

template <typename ProxyType>
nb::tuple
_GetIntroducingListEditor(const UsdPrimCompositionQueryArc &arc)
{
    ProxyType editor;
    typename ProxyType::value_type value;
    if (arc.GetIntroducingListEditor(&editor, &value)) {
        return nb::make_tuple(nb::cast(editor), nb::cast(value));
    }
    TF_CODING_ERROR("Failed to get list editor value for the given type of "
                    "of the composition arc");
    return nb::make_tuple(nb::none(), nb::none());
}

static nb::tuple
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
        return nb::make_tuple(nb::none(), nb::none());
    }
}

void wrapUsdPrimCompositionQueryArc(nanobind::module_ &m)
{
    nb::class_<UsdPrimCompositionQueryArc>(m, "CompositionArc")
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
             nb::arg("subLayer")=SdfLayerHandle())
        .def("MakeResolveTargetStrongerThan", 
             &UsdPrimCompositionQueryArc::MakeResolveTargetStrongerThan,
             nb::arg("subLayer")=SdfLayerHandle())
    ;
}

void wrapUsdPrimCompositionQuery(nanobind::module_ &m)
{
    using This = UsdPrimCompositionQuery;
        
    nb::class_<This> cls(m, "PrimCompositionQuery");

    nb::enum_<This::ArcIntroducedFilter>(cls, "ArcIntroducedFilter")
        .value("All", This::ArcIntroducedFilter::All)
        .value("IntroducedInRootLayerStack", This::ArcIntroducedFilter::IntroducedInRootLayerStack)
        .value("IntroducedInRootLayerPrimSpec", This::ArcIntroducedFilter::IntroducedInRootLayerPrimSpec)
    ;
    nb::enum_<This::ArcTypeFilter>(cls, "ArcTypeFilter")
        .value("All", This::ArcTypeFilter::All)
        .value("Reference", This::ArcTypeFilter::Reference)
        .value("Payload", This::ArcTypeFilter::Payload)
        .value("Inherit", This::ArcTypeFilter::Inherit)
        .value("Specialize", This::ArcTypeFilter::Specialize)
        .value("Variant", This::ArcTypeFilter::Variant)
        .value("Relocate", This::ArcTypeFilter::Relocate)
        .value("ReferenceOrPayload", This::ArcTypeFilter::ReferenceOrPayload)
        .value("InheritOrSpecialize", This::ArcTypeFilter::InheritOrSpecialize)
        .value("NotReferenceOrPayload", This::ArcTypeFilter::NotReferenceOrPayload)
        .value("NotInheritOrSpecialize", This::ArcTypeFilter::NotInheritOrSpecialize)
        .value("NotVariant", This::ArcTypeFilter::NotVariant)
        .value("NotRelocate", This::ArcTypeFilter::NotRelocate)
    ;
    nb::enum_<This::DependencyTypeFilter>(cls, "DependencyTypeFilter")
        .value("All", This::DependencyTypeFilter::All)
        .value("Direct", This::DependencyTypeFilter::Direct)
        .value("Ancestral", This::DependencyTypeFilter::Ancestral)
    ;
    nb::enum_<This::HasSpecsFilter>(cls, "HasSpecsFilter")
        .value("All", This::HasSpecsFilter::All)
        .value("HasSpecs", This::HasSpecsFilter::HasSpecs)
        .value("HasNoSpecs", This::HasSpecsFilter::HasNoSpecs)
    ;

    nb::class_<This::Filter>(cls, "Filter")
        .def(nb::init<>())
        .def_rw("arcIntroducedFilter", &This::Filter::arcIntroducedFilter)
        .def_rw("arcTypeFilter", &This::Filter::arcTypeFilter)
        .def_rw("dependencyTypeFilter", &This::Filter::dependencyTypeFilter)
        .def_rw("hasSpecsFilter", &This::Filter::hasSpecsFilter)
        .def("__eq__", [](This::Filter &self, const This::Filter &rhs) { return self == rhs; })
        .def("__ne__", [](This::Filter &self, const This::Filter &rhs) { return self != rhs; })
    ;

    cls
        .def(nb::init<const UsdPrim &>(), nb::arg("prim"))
        .def(nb::init<const UsdPrim &, const This::Filter &>(),
             nb::arg("prim"), nb::arg("filter"))
        .def_static("GetDirectReferences", &This::GetDirectReferences)
        .def_static("GetDirectInherits", &This::GetDirectInherits)
        .def_static("GetDirectRootLayerArcs", &This::GetDirectRootLayerArcs)
        .def_prop_rw("filter", &This::GetFilter, &This::SetFilter)
        .def("GetCompositionArcs", &This::GetCompositionArcs)
    ;
}

